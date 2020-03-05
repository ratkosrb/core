/*
 * Copyright (C) 2005-2011 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "MoveMap.h"
#include "GridMap.h"
#include "Creature.h"
#include "PathGenerator.h"
#include "Log.h"
#include "Map.h"
#include "Transport.h"

#include "Detour/Include/DetourCommon.h"

// Distance to target
#define SMOOTH_PATH_SLOP 0.4f
// Distance between path steps
#define SMOOTH_PATH_STEP_SIZE 2.0f

////////////////// PathGenerator //////////////////
PathGenerator::PathGenerator(WorldObject const* owner) :
    m_polyLength(0), m_type(PATHFIND_BLANK), m_useStraightPath(false), m_forceDestination(false),
    m_pointPathLimit(MAX_POINT_PATH_LENGTH), m_straightLine(false), m_endPosition(G3D::Vector3::zero()),
    m_transport(nullptr), m_source(owner), m_navMesh(nullptr), m_navMeshQuery(nullptr)
{
    memset(m_pathPolyRefs, 0, sizeof(m_pathPolyRefs));
    //DEBUG_FILTER_LOG(LOG_FILTER_PATHFINDING, "++ PathGenerator::PathGenerator for %u \n", m_source->GetGUIDLow());
    CreateFilter();
}


PathGenerator::~PathGenerator()
{
    //DEBUG_FILTER_LOG(LOG_FILTER_PATHFINDING, "++ PathGenerator::~PathGenerator() for %u \n", m_source->GetGUID());
}

void PathGenerator::SetPathLengthLimit(float dist)
{
    m_pointPathLimit = std::min<uint32>(MAX_POINT_PATH_LENGTH, uint32(dist / SMOOTH_PATH_STEP_SIZE));
}

bool PathGenerator::CalculatePath(float destX, float destY, float destZ, bool forceDest, bool straightLine, bool offsets)
{
    // A m_navMeshQuery object is not thread safe, but a same PathGenerator can be shared between threads.
    // So need to get a new one.
    MMAP::MMapManager* mmap = MMAP::MMapFactory::createOrGetMMapManager();
    if (m_transport)
    {
        if (!offsets)
            m_transport->CalculatePassengerOffset(destX, destY, destZ);
        m_navMeshQuery = mmap->GetModelNavMeshQuery(m_transport->GetDisplayId());
    }
    else
        m_navMeshQuery = mmap->GetNavMeshQuery(m_source->GetMapId());

    if (m_navMeshQuery)
        m_navMesh = m_navMeshQuery->getAttachedNavMesh();

    m_pathPoints.clear();

    G3D::Vector3 oldDest = GetEndPosition();
    G3D::Vector3 dest(destX, destY, destZ);
    SetEndPosition(dest);

    float x, y, z;
    m_source->GetSafePosition(x, y, z, m_transport);
    G3D::Vector3 start(x, y, z);
    SetStartPosition(start);

    m_forceDestination = forceDest;
    m_straightLine = straightLine;
    m_type = PATHFIND_BLANK;

    //DEBUG_FILTER_LOG(LOG_FILTER_PATHFINDING, "++ PathGenerator::calculate() for %u \n", m_source->GetGUIDLow());

    // make sure navMesh works - we can run on map w/o mmap
    // check if the start and end point have a .mmtile loaded (can we pass via not loaded tile on the way?)
    if (!m_navMesh || !m_navMeshQuery || (m_source->IsUnit() && ((Unit*)m_source)->HasUnitState(UNIT_STAT_IGNORE_PATHFINDING)) ||
            !HaveTiles(start) || !HaveTiles(dest))
    {
        BuildShortcut();
        m_type = PathType(PATHFIND_NORMAL | PATHFIND_NOT_USING_PATH);
        return true;
    }

    UpdateFilter();

    // check if destination moved - if not we can optimize something here
    // we are following old, precalculated path?
    float dist = m_source->GetObjectBoundingRadius();
    if (InRange(oldDest, dest, dist, dist) && m_pathPoints.size() > 2)
    {
        // our target is not moving - we just coming closer
        // we are moving on precalculated path - enjoy the ride
        //DEBUG_FILTER_LOG(LOG_FILTER_PATHFINDING, "++ PathGenerator::calculate:: precalculated path\n");
        m_pathPoints.erase(m_pathPoints.begin());
        return false;
    }
    else
    {
        // target moved, so we need to update the poly path
        BuildPolyPath(start, dest);
        return true;
    }
}

dtPolyRef PathGenerator::FindWalkPoly(dtNavMeshQuery const* query, float const* pointYZX, dtQueryFilter const& filter, float* closestPointYZX, float zSearchDist)
{
    ASSERT(query);

    // WARNING : Nav mesh coords are Y, Z, X (and not X, Y, Z)
    float extents[3] = { 5.0f, zSearchDist, 5.0f };
    dtPolyRef polyRef;

    // Default recastnavigation method
    if (dtStatusFailed(query->findNearestPoly(pointYZX, extents, &filter, &polyRef, closestPointYZX)))
        return 0;
    // Do not select points over player pos
    if (closestPointYZX[1] > pointYZX[1] + 3.0f)
        return 0;
    return polyRef;
}

dtPolyRef PathGenerator::GetPathPolyByPosition(dtPolyRef const* polyPath, uint32 polyPathSize, float const* point, float* distance) const
{
    if (!polyPath || !polyPathSize)
        return INVALID_POLYREF;

    dtPolyRef nearestPoly = INVALID_POLYREF;
    float minDist2d = FLT_MAX;
    float minDist3d = 0.0f;

    for (uint32 i = 0; i < polyPathSize; ++i)
    {
        float closestPoint[VERTEX_SIZE];
        if (dtStatusFailed(m_navMeshQuery->closestPointOnPoly(polyPath[i], point, closestPoint, nullptr)))
            continue;

        float d = dtVdist2DSqr(point, closestPoint);
        if (d < minDist2d)
        {
            minDist2d = d;
            nearestPoly = polyPath[i];
            minDist3d = dtVdistSqr(point, closestPoint);
        }

        if (minDist2d < 1.0f) // shortcut out - close enough for us
            break;
    }

    if (distance)
        *distance = dtMathSqrtf(minDist3d);

    return (minDist2d < 3.0f) ? nearestPoly : INVALID_POLYREF;
}

dtPolyRef PathGenerator::GetPolyByLocation(float const* point, float *distance, uint32 allowedFlags)
{
    dtQueryFilter filter;
    filter.setIncludeFlags(m_filter.getIncludeFlags() | allowedFlags);

    // first we check the current path
    // if the current path doesn't contain the current poly,
    // we need to use the expensive navMesh.findNearestPoly
    dtPolyRef polyRef = GetPathPolyByPosition(m_pathPolyRefs, m_polyLength, point, distance);
    if (polyRef != INVALID_POLYREF)
        return polyRef;

    // we don't have it in our old path
    // try to get it by findNearestPoly()
    // first try with low search box
    float extents[VERTEX_SIZE] = { 3.0f, 5.0f, 3.0f };    // bounds of poly search area
    float closestPoint[VERTEX_SIZE] = { 0.0f, 0.0f, 0.0f };
    if (dtStatusSucceed(m_navMeshQuery->findNearestPoly(point, extents, &filter, &polyRef, closestPoint)) && polyRef != INVALID_POLYREF)
    {
        *distance = dtVdist(closestPoint, point);
        return polyRef;
    }

    // still nothing ..
    // try with bigger search box
    // Note that the extent should not overlap more than 128 polygons in the navmesh (see dtNavMeshQuery::findNearestPoly)
    extents[1] = 50.0f;

    if (dtStatusSucceed(m_navMeshQuery->findNearestPoly(point, extents, &filter, &polyRef, closestPoint)) && polyRef != INVALID_POLYREF)
    {
        *distance = dtVdist(closestPoint, point);
        return polyRef;
    }

    return INVALID_POLYREF;
}

void PathGenerator::BuildPolyPath(G3D::Vector3 const& startPos, G3D::Vector3 const& endPos)
{
    // *** getting start/end poly logic ***

    float distToStartPoly, distToEndPoly;
    float startPoint[VERTEX_SIZE] = {startPos.y, startPos.z, startPos.x};
    float endPoint[VERTEX_SIZE] = {endPos.y, endPos.z, endPos.x};

    // First case : easy flying / swimming
    if (m_source->IsUnit() && ((((Unit*)m_source)->CanSwim() && m_source->GetTerrain()->IsSwimmable(endPos.x, endPos.y, endPos.z)) || ((Unit*)m_source)->CanFly()))
    {
        if (!m_source->GetMap()->FindCollisionModel(startPos.x, startPos.y, startPos.z, endPos.x, endPos.y, endPos.z))
        {
            if (((Unit*)m_source)->CanSwim())
                BuildUnderwaterPath();
            else
            {
                BuildShortcut();
                m_type = PathType(PATHFIND_NORMAL | PATHFIND_NOT_USING_PATH);
                if (((Unit*)m_source)->CanFly())
                    m_type |= PATHFIND_FLYPATH;
            }
            return;
        }
        else if (((Unit*)m_source)->CanFly())
            m_forceDestination = true;
    }

    dtPolyRef startPoly = GetPolyByLocation(startPoint, &distToStartPoly);
    dtPolyRef endPoly = GetPolyByLocation(endPoint, &distToEndPoly);

    // we have a hole in our mesh
    // make shortcut path and mark it as NOPATH ( with flying and swimming exception )
    // its up to caller how he will use this info
    if (startPoly == INVALID_POLYREF || endPoly == INVALID_POLYREF)
    {
        //TC_LOG_DEBUG("maps.mmaps", "++ BuildPolyPath :: (startPoly == 0 || endPoly == 0)");
        BuildShortcut();
        bool path = m_source->GetTypeId() == TYPEID_UNIT && m_source->ToCreature()->CanFly();

        bool waterPath = m_source->GetTypeId() == TYPEID_UNIT && m_source->ToCreature()->CanSwim();
        if (waterPath)
        {
            // Check both start and end points, if they're both in water, then we can *safely* let the creature move
            for (uint32 i = 0; i < m_pathPoints.size(); ++i)
            {
                GridMapLiquidStatus status = m_source->GetMap()->GetTerrain()->getLiquidStatus(m_pathPoints[i].x, m_pathPoints[i].y, m_pathPoints[i].z, MAP_ALL_LIQUIDS, nullptr);
                // One of the points is not in the water, cancel movement.
                if (status == LIQUID_MAP_NO_WATER)
                {
                    waterPath = false;
                    break;
                }
            }
        }

        m_type = (path || waterPath) ? PathType(PATHFIND_NORMAL | PATHFIND_NOT_USING_PATH) : PATHFIND_NOPATH;
        if (path)
            m_type |= PATHFIND_FLYPATH;
        return;
    }

    // we may need a better number here
    bool farFromPoly = (distToStartPoly > 7.0f || distToEndPoly > 7.0f);
    if (farFromPoly)
    {
        //TC_LOG_DEBUG("maps.mmaps", "++ BuildPolyPath :: farFromPoly distToStartPoly=%.3f distToEndPoly=%.3f", distToStartPoly, distToEndPoly);

        bool buildShotrcut = false;

        G3D::Vector3 const& p = (distToStartPoly > 7.0f) ? startPos : endPos;
        if (m_source->GetMap()->GetTerrain()->IsUnderWater(p.x, p.y, p.z))
        {
            //TC_LOG_DEBUG("maps.mmaps", "++ BuildPolyPath :: underWater case");
            if (Unit const* sourceUnit = m_source->ToUnit())
                if (sourceUnit->CanSwim())
                    buildShotrcut = true;
        }
        else
        {
            //TC_LOG_DEBUG("maps.mmaps", "++ BuildPolyPath :: flying case");
            if (Unit const* sourceUnit = m_source->ToUnit())
            {
                if (sourceUnit->CanFly())
                {
                    m_type |= PATHFIND_FLYPATH;
                    buildShotrcut = true;
                }
                // Allow to build a shortcut if the unit is falling and it's trying to move downwards towards a target (i.e. charging)
                else if (sourceUnit->IsFalling() && endPos.z < startPos.z)
                    buildShotrcut = true;
            }
        }

        if (buildShotrcut)
        {
            BuildShortcut();
            m_type = PathType(PATHFIND_NORMAL | PATHFIND_NOT_USING_PATH | PATHFIND_FARFROMPOLY);
            return;
        }
        else
        {
            float closestPoint[VERTEX_SIZE];
            // we may want to use closestPointOnPolyBoundary instead
            if (dtStatusSucceed(m_navMeshQuery->closestPointOnPoly(endPoly, endPoint, closestPoint, nullptr)))
            {
                dtVcopy(endPoint, closestPoint);
                SetActualEndPosition(G3D::Vector3(endPoint[2], endPoint[0], endPoint[1]));
            }

            m_type = PathType(PATHFIND_INCOMPLETE | PATHFIND_FARFROMPOLY);
        }
    }

    // *** poly path generating logic ***

    // start and end are on same polygon
    // handle this case as if they were 2 different polygons, building a line path split in some few points
    if (startPoly == endPoly)
    {
        //TC_LOG_DEBUG("maps.mmaps", "++ BuildPolyPath :: (startPoly == endPoly)");
        m_pathPolyRefs[0] = startPoly;
        m_polyLength = 1;

        m_type = farFromPoly ? PathType(PATHFIND_INCOMPLETE | PATHFIND_FARFROMPOLY) : PATHFIND_NORMAL;

        BuildPointPath(startPoint, endPoint);
        return;
    }

    // look for startPoly/endPoly in current path
    /// @todo we can merge it with getPathPolyByPosition() loop
    bool startPolyFound = false;
    bool endPolyFound = false;
    uint32 pathStartIndex = 0;
    uint32 pathEndIndex = 0;

    if (m_polyLength)
    {
        for (; pathStartIndex < m_polyLength; ++pathStartIndex)
        {
            // here to catch few bugs
            if (m_pathPolyRefs[pathStartIndex] == INVALID_POLYREF)
            {
                /*
                TC_LOG_ERROR("maps.mmaps", "Invalid poly ref in BuildPolyPath. m_polyLength: %u, pathStartIndex: %u,"
                                     " startPos: %s, endPos: %s, mapid: %u",
                                     m_polyLength, pathStartIndex, startPos.toString().c_str(), endPos.toString().c_str(),
                                     m_source->GetMapId());
                */
                break;
            }

            if (m_pathPolyRefs[pathStartIndex] == startPoly)
            {
                startPolyFound = true;
                break;
            }
        }

        for (pathEndIndex = m_polyLength-1; pathEndIndex > pathStartIndex; --pathEndIndex)
            if (m_pathPolyRefs[pathEndIndex] == endPoly)
            {
                endPolyFound = true;
                break;
            }
    }

    if (startPolyFound && endPolyFound)
    {
        //TC_LOG_DEBUG("maps.mmaps", "++ BuildPolyPath :: (startPolyFound && endPolyFound)");

        // we moved along the path and the target did not move out of our old poly-path
        // our path is a simple subpath case, we have all the data we need
        // just "cut" it out

        m_polyLength = pathEndIndex - pathStartIndex + 1;
        memmove(m_pathPolyRefs, m_pathPolyRefs + pathStartIndex, m_polyLength * sizeof(dtPolyRef));
    }
    else if (startPolyFound && !endPolyFound)
    {
        //TC_LOG_DEBUG("maps.mmaps", "++ BuildPolyPath :: (startPolyFound && !endPolyFound)");

        // we are moving on the old path but target moved out
        // so we have atleast part of poly-path ready

        m_polyLength -= pathStartIndex;

        // try to adjust the suffix of the path instead of recalculating entire length
        // at given interval the target cannot get too far from its last location
        // thus we have less poly to cover
        // sub-path of optimal path is optimal

        // take ~80% of the original length
        /// @todo play with the values here
        uint32 prefixPolyLength = uint32(m_polyLength * 0.8f + 0.5f);
        memmove(m_pathPolyRefs, m_pathPolyRefs+pathStartIndex, prefixPolyLength * sizeof(dtPolyRef));

        dtPolyRef suffixStartPoly = m_pathPolyRefs[prefixPolyLength-1];

        // we need any point on our suffix start poly to generate poly-path, so we need last poly in prefix data
        float suffixEndPoint[VERTEX_SIZE];
        if (dtStatusFailed(m_navMeshQuery->closestPointOnPoly(suffixStartPoly, endPoint, suffixEndPoint, nullptr)))
        {
            // we can hit offmesh connection as last poly - closestPointOnPoly() don't like that
            // try to recover by using prev polyref
            --prefixPolyLength;
            suffixStartPoly = m_pathPolyRefs[prefixPolyLength-1];
            if (dtStatusFailed(m_navMeshQuery->closestPointOnPoly(suffixStartPoly, endPoint, suffixEndPoint, nullptr)))
            {
                // suffixStartPoly is still invalid, error state
                BuildShortcut();
                m_type = PATHFIND_NOPATH;
                return;
            }
        }

        // generate suffix
        uint32 suffixPolyLength = 0;

        dtStatus dtResult;
        if (m_straightLine)
        {
            float hit = 0;
            float hitNormal[3];
            memset(hitNormal, 0, sizeof(hitNormal));

            dtResult = m_navMeshQuery->raycast(
                            suffixStartPoly,
                            suffixEndPoint,
                            endPoint,
                            &m_filter,
                            &hit,
                            hitNormal,
                            m_pathPolyRefs + prefixPolyLength - 1,
                            (int*)&suffixPolyLength,
                            MAX_PATH_LENGTH - prefixPolyLength);

            // raycast() sets hit to FLT_MAX if there is a ray between start and end
            if (hit != FLT_MAX)
            {
                // the ray hit something, return no path instead of the incomplete one
                Clear();
                m_polyLength = 2;
                m_pathPoints.resize(2);
                m_pathPoints[0] = GetStartPosition();
                float hitPos[3];
                dtVlerp(hitPos, startPoint, endPoint, hit);
                m_pathPoints[1] = G3D::Vector3(hitPos[2], hitPos[0], hitPos[1]);

                m_type = PATHFIND_INCOMPLETE;
                return;
            }
        }
        else
        {
            dtResult = m_navMeshQuery->findPath(
                            suffixStartPoly,    // start polygon
                            endPoly,            // end polygon
                            suffixEndPoint,     // start position
                            endPoint,           // end position
                            &m_filter,            // polygon search filter
                            m_pathPolyRefs + prefixPolyLength - 1,    // [out] path
                            (int*)&suffixPolyLength,
                            MAX_PATH_LENGTH - prefixPolyLength);   // max number of polygons in output path
        }

        if (!suffixPolyLength || dtStatusFailed(dtResult))
        {
            // this is probably an error state, but we'll leave it
            // and hopefully recover on the next Update
            // we still need to copy our preffix
            //TC_LOG_ERROR("maps.mmaps", "Path Build failed\n%s", m_source->GetDebugInfo().c_str());
        }

        //TC_LOG_DEBUG("maps.mmaps", "++  m_polyLength=%u prefixPolyLength=%u suffixPolyLength=%u", m_polyLength, prefixPolyLength, suffixPolyLength);

        // new path = prefix + suffix - overlap
        m_polyLength = prefixPolyLength + suffixPolyLength - 1;
    }
    else
    {
        //TC_LOG_DEBUG("maps.mmaps", "++ BuildPolyPath :: (!startPolyFound && !endPolyFound)");

        // either we have no path at all -> first run
        // or something went really wrong -> we aren't moving along the path to the target
        // just generate new path

        // free and invalidate old path data
        Clear();

        dtStatus dtResult;
        if (m_straightLine)
        {
            float hit = 0;
            float hitNormal[3];
            memset(hitNormal, 0, sizeof(hitNormal));

            dtResult = m_navMeshQuery->raycast(
                            startPoly,
                            startPoint,
                            endPoint,
                            &m_filter,
                            &hit,
                            hitNormal,
                            m_pathPolyRefs,
                            (int*)&m_polyLength,
                            MAX_PATH_LENGTH);

            // raycast() sets hit to FLT_MAX if there is a ray between start and end
            if (hit != FLT_MAX)
            {
                // the ray hit something, return no path instead of the incomplete one
                Clear();
                m_polyLength = 2;
                m_pathPoints.resize(2);
                m_pathPoints[0] = GetStartPosition();
                float hitPos[3];
                dtVlerp(hitPos, startPoint, endPoint, hit);
                m_pathPoints[1] = G3D::Vector3(hitPos[2], hitPos[0], hitPos[1]);

                m_type = PATHFIND_INCOMPLETE;
                return;
            }
            else
                m_navMeshQuery->getPolyHeight(m_pathPolyRefs[m_polyLength - 1], endPoint, &endPoint[1]);
        }
        else
        {
            dtResult = m_navMeshQuery->findPath(
                            startPoly,          // start polygon
                            endPoly,            // end polygon
                            startPoint,         // start position
                            endPoint,           // end position
                            &m_filter,           // polygon search filter
                            m_pathPolyRefs,     // [out] path
                            (int*)&m_polyLength,
                            MAX_PATH_LENGTH);   // max number of polygons in output path
        }

        if (!m_polyLength || dtStatusFailed(dtResult))
        {
            // only happens if we passed bad data to findPath(), or navmesh is messed up
            //TC_LOG_ERROR("maps.mmaps", "%u's Path Build failed: 0 length path", m_source->GetGUID().GetCounter());
            BuildShortcut();
            m_type = PATHFIND_NOPATH;
            return;
        }
    }

    // by now we know what type of path we can get
    if (m_pathPolyRefs[m_polyLength - 1] == endPoly && !(m_type & PATHFIND_INCOMPLETE))
        m_type = PATHFIND_NORMAL;
    else
        m_type = PATHFIND_INCOMPLETE;

    if (farFromPoly)
        m_type = PathType(m_type | PATHFIND_FARFROMPOLY);

    // generate the point-path out of our up-to-date poly-path
    BuildPointPath(startPoint, endPoint);
}

void PathGenerator::BuildPointPath(const float *startPoint, const float *endPoint)
{
    float pathPoints[MAX_POINT_PATH_LENGTH*VERTEX_SIZE];
    uint32 pointCount = 0;
    dtStatus dtResult = DT_FAILURE;
    if (m_straightLine)
    {
        dtResult = DT_SUCCESS;
        pointCount = 1;
        memcpy(&pathPoints[VERTEX_SIZE * 0], startPoint, sizeof(float)* 3); // first point

        // path has to be split into polygons with dist SMOOTH_PATH_STEP_SIZE between them
        G3D::Vector3 startVec = G3D::Vector3(startPoint[0], startPoint[1], startPoint[2]);
        G3D::Vector3 endVec = G3D::Vector3(endPoint[0], endPoint[1], endPoint[2]);
        G3D::Vector3 diffVec = (endVec - startVec);
        G3D::Vector3 prevVec = startVec;
        float len = diffVec.length();
        diffVec *= SMOOTH_PATH_STEP_SIZE / len;
        while (len > SMOOTH_PATH_STEP_SIZE)
        {
            len -= SMOOTH_PATH_STEP_SIZE;
            prevVec += diffVec;
            pathPoints[VERTEX_SIZE * pointCount + 0] = prevVec.x;
            pathPoints[VERTEX_SIZE * pointCount + 1] = prevVec.y;
            pathPoints[VERTEX_SIZE * pointCount + 2] = prevVec.z;
            ++pointCount;
        }

        memcpy(&pathPoints[VERTEX_SIZE * pointCount], endPoint, sizeof(float)* 3); // last point
        ++pointCount;
    }
    else if (m_useStraightPath)
    {
        dtResult = m_navMeshQuery->findStraightPath(
                startPoint,         // start position
                endPoint,           // end position
                m_pathPolyRefs,     // current path
                m_polyLength,       // lenth of current path
                pathPoints,         // [out] path corner points
                nullptr,               // [out] flags
                nullptr,               // [out] shortened path
                (int*)&pointCount,
                m_pointPathLimit);   // maximum number of points/polygons to use
    }
    else
    {
        dtResult = FindSmoothPath(
                startPoint,         // start position
                endPoint,           // end position
                m_pathPolyRefs,     // current path
                m_polyLength,       // length of current path
                pathPoints,         // [out] path corner points
                (int*)&pointCount,
                m_pointPathLimit);    // maximum number of points
    }

    // Special case with start and end positions very close to each other
    if (m_polyLength == 1 && pointCount == 1)
    {
        // First point is start position, append end position
        dtVcopy(&pathPoints[1 * VERTEX_SIZE], endPoint);
        pointCount++;
    }
    else if ( pointCount < 2 || dtStatusFailed(dtResult))
    {
        // only happens if pass bad data to findStraightPath or navmesh is broken
        // single point paths can be generated here
        /// @todo check the exact cases
        //TC_LOG_DEBUG("maps.mmaps", "++ PathGenerator::BuildPointPath FAILED! path sized %d returned\n", pointCount);
        BuildShortcut();
        m_type = PathType(m_type | PATHFIND_NOPATH);
        return;
    }
    else if (pointCount == m_pointPathLimit)
    {
        //TC_LOG_DEBUG("maps.mmaps", "++ PathGenerator::BuildPointPath FAILED! path sized %d returned, lower than limit set to %d", pointCount, _pointPathLimit);
        BuildShortcut();
        m_type = PathType(m_type | PATHFIND_SHORT);
        return;
    }

    m_pathPoints.resize(pointCount);
    for (uint32 i = 0; i < pointCount; ++i)
        m_pathPoints[i] = G3D::Vector3(pathPoints[i*VERTEX_SIZE+2], pathPoints[i*VERTEX_SIZE], pathPoints[i*VERTEX_SIZE+1]);

    NormalizePath();

    // first point is always our current location - we need the next one
    SetActualEndPosition(m_pathPoints[pointCount-1]);

    // force the given destination, if needed
    if (m_forceDestination &&
        (!(m_type & PATHFIND_NORMAL) || !InRange(GetEndPosition(), GetActualEndPosition(), 1.0f, 1.0f)))
    {
        // we may want to keep partial subpath
        if (Dist3DSqr(GetActualEndPosition(), GetEndPosition()) < 0.3f * Dist3DSqr(GetStartPosition(), GetEndPosition()))
        {
            SetActualEndPosition(GetEndPosition());
            m_pathPoints[m_pathPoints.size()-1] = GetEndPosition();
        }
        else
        {
            SetActualEndPosition(GetEndPosition());
            BuildShortcut();
        }

        m_type = PathType(PATHFIND_NORMAL | PATHFIND_NOT_USING_PATH);
        if (m_source->IsUnit() && ((Unit*)m_source)->CanFly())
            m_type |= PATHFIND_FLYPATH;
    }

    //TC_LOG_DEBUG("maps.mmaps", "++ PathGenerator::BuildPointPath path type %d size %d poly-size %d", _type, pointCount, _polyLength);
}

void PathGenerator::NormalizePath()
{
    for (uint32 i = 0; i < m_pathPoints.size(); ++i)
        m_source->UpdateAllowedPositionZ(m_pathPoints[i].x, m_pathPoints[i].y, m_pathPoints[i].z);
}

void PathGenerator::BuildShortcut()
{
    //TC_LOG_DEBUG("maps.mmaps", "++ BuildShortcut :: making shortcut");

    Clear();

    // make two point path, our curr pos is the start, and dest is the end
    m_pathPoints.resize(2);

    // set start and a default next position
    m_pathPoints[0] = GetStartPosition();
    m_pathPoints[1] = GetActualEndPosition();

    NormalizePath();

    m_type = PATHFIND_SHORTCUT;
    if (m_source->IsUnit() && ((Unit*)m_source)->CanFly())
        m_type |= PATHFIND_FLYPATH | PATHFIND_NORMAL;
}

void PathGenerator::BuildUnderwaterPath()
{
    Clear();

    // make two point path, our curr pos is the start, and dest is the end
    m_pathPoints.resize(2);

    // set start and a default next position
    m_pathPoints[0] = GetStartPosition();
    m_pathPoints[1] = GetActualEndPosition();

    GridMapLiquidData liquidData;
    uint32 liquidStatus = m_source->GetTerrain()->getLiquidStatus(GetActualEndPosition().x, GetActualEndPosition().y, GetActualEndPosition().z, MAP_ALL_LIQUIDS, &liquidData);
    // No water here ...
    if (liquidStatus == LIQUID_MAP_NO_WATER)
    {
        m_type = PATHFIND_SHORTCUT;
        if (m_source->IsUnit() && ((Unit*)m_source)->CanWalk())
        {
            // Find real height
            m_type |= PATHFIND_NORMAL;
            m_source->UpdateGroundPositionZ(m_pathPoints[1].x, m_pathPoints[1].y, m_pathPoints[1].z);
        }
        else
        {
            m_type |= PATHFIND_INCOMPLETE;
            m_pathPoints[1] = GetStartPosition();
        }
        return;
    }
    m_type = PATHFIND_BLANK;
    if (m_pathPoints[1].z > liquidData.level)
    {
        if (!(m_source->IsUnit() && ((Unit*)m_source)->CanFly()))
        {
            m_pathPoints[1].z = liquidData.level;
            if (m_pathPoints[1].z > (liquidData.level + 2))
                m_type |= PATHFIND_INCOMPLETE;
        }
    }
    if (!(m_type & PATHFIND_INCOMPLETE))
        m_type |= PATHFIND_NORMAL;
}

void PathGenerator::CreateFilter()
{
    uint16 includeFlags = 0;
    uint16 excludeFlags = 0;

    if (m_source->GetTypeId() == TYPEID_UNIT)
    {
        Creature* creature = (Creature*)m_source;
        if (creature->CanWalk())
            includeFlags |= NAV_GROUND; // walk

        // creatures don't take environmental damage
        if (creature->CanSwim())
            includeFlags |= (NAV_WATER | NAV_MAGMA | NAV_SLIME); // swim
    }
    else // assume Player
    {
        // perfect support not possible, just stay 'safe'
        includeFlags |= (NAV_GROUND | NAV_WATER | NAV_MAGMA | NAV_SLIME);
    }

    m_filter.setIncludeFlags(includeFlags);
    m_filter.setExcludeFlags(excludeFlags);

    UpdateFilter();
}

void PathGenerator::UpdateFilter()
{
    // allow creatures to cheat and use different movement types if they are moved
    // forcefully into terrain they can't normally move in
    if (Unit const* _sourceUnit = m_source->ToUnit())
        if (_sourceUnit->IsInWater() || _sourceUnit->IsUnderWater())
        {
            uint16 includedFlags = m_filter.getIncludeFlags();
            includedFlags |= GetNavTerrain(m_source->GetPositionX(),
                m_source->GetPositionY(),
                m_source->GetPositionZ());

            m_filter.setIncludeFlags(includedFlags);
        }
}

NavTerrain PathGenerator::GetNavTerrain(float x, float y, float z)
{
    GridMapLiquidData data;
    GridMapLiquidStatus liquidStatus = m_source->GetMap()->GetTerrain()->getLiquidStatus(x, y, z, MAP_ALL_LIQUIDS, &data);
    if (liquidStatus == LIQUID_MAP_NO_WATER)
        return NAV_GROUND;

    switch (data.type_flags)
    {
        case MAP_LIQUID_TYPE_WATER:
        case MAP_LIQUID_TYPE_OCEAN:
            return NAV_WATER;
        case MAP_LIQUID_TYPE_MAGMA:
        case MAP_LIQUID_TYPE_SLIME:
            return NAV_MAGMA;
        default:
            return NAV_GROUND;
    }
}

bool PathGenerator::HaveTiles(G3D::Vector3 const& p) const
{
    if (m_transport)
        return true;
    int tx, ty;
    float point[VERTEX_SIZE] = {p.y, p.z, p.x};

    // check if the start and end point have a .mmtile loaded
    m_navMesh->calcTileLoc(point, &tx, &ty);
    return (m_navMesh->getTileAt(tx, ty, 0) != nullptr);
}

uint32 PathGenerator::FixupCorridor(dtPolyRef* path, uint32 npath, uint32 maxPath, dtPolyRef const* visited, uint32 nvisited)
{
    int32 furthestPath = -1;
    int32 furthestVisited = -1;

    // Find furthest common polygon.
    for (int32 i = npath-1; i >= 0; --i)
    {
        bool found = false;
        for (int32 j = nvisited-1; j >= 0; --j)
        {
            if (path[i] == visited[j])
            {
                furthestPath = i;
                furthestVisited = j;
                found = true;
            }
        }
        if (found)
            break;
    }

    // If no intersection found just return current path.
    if (furthestPath == -1 || furthestVisited == -1)
        return npath;

    // Concatenate paths.

    // Adjust beginning of the buffer to include the visited.
    uint32 req = nvisited - furthestVisited;
    uint32 orig = uint32(furthestPath + 1) < npath ? furthestPath + 1 : npath;
    uint32 size = npath > orig ? npath - orig : 0;
    if (req + size > maxPath)
        size = maxPath-req;

    if (size)
        memmove(path + req, path + orig, size * sizeof(dtPolyRef));

    // Store visited
    for (uint32 i = 0; i < req; ++i)
        path[i] = visited[(nvisited - 1) - i];

    return req+size;
}

int fixupShortcuts(dtPolyRef* path, int npath, dtNavMeshQuery const* navQuery)
{
    if (npath < 3)
        return npath;

    // Get connected polygons
    static int const maxNeis = 16;
    dtPolyRef neis[maxNeis];
    int nneis = 0;

    dtMeshTile const* tile = 0;
    dtPoly const* poly = 0;
    if (dtStatusFailed(navQuery->getAttachedNavMesh()->getTileAndPolyByRef(path[0], &tile, &poly)))
        return npath;

    for (unsigned int k = poly->firstLink; k != DT_NULL_LINK; k = tile->links[k].next)
    {
        dtLink const* link = &tile->links[k];
        if (link->ref != 0)
        {
            if (nneis < maxNeis)
                neis[nneis++] = link->ref;
        }
    }

    // If any of the neighbour polygons is within the next few polygons
    // in the path, short cut to that polygon directly.
    static int const maxLookAhead = 6;
    int cut = 0;
    for (int i = dtMin(maxLookAhead, npath) - 1; i > 1 && cut == 0; i--)
    {
        for (int j = 0; j < nneis; j++)
        {
            if (path[i] == neis[j])
            {
                cut = i;
                break;
            }
        }
    }
    if (cut > 1)
    {
        int offset = cut - 1;
        npath -= offset;
        for (int i = 1; i < npath; i++)
            path[i] = path[i + offset];
    }

    return npath;
}

bool PathGenerator::GetSteerTarget(float const* startPos, float const* endPos,
                              float minTargetDist, dtPolyRef const* path, uint32 pathSize,
                              float* steerPos, unsigned char& steerPosFlag, dtPolyRef& steerPosRef)
{
    // Find steer target.
    static const uint32 MAX_STEER_POINTS = 3;
    float steerPath[MAX_STEER_POINTS*VERTEX_SIZE];
    unsigned char steerPathFlags[MAX_STEER_POINTS];
    dtPolyRef steerPathPolys[MAX_STEER_POINTS];
    uint32 nsteerPath = 0;
    dtStatus dtResult = m_navMeshQuery->findStraightPath(startPos, endPos, path, pathSize,
                                                steerPath, steerPathFlags, steerPathPolys, (int*)&nsteerPath, MAX_STEER_POINTS);
    if (!nsteerPath || dtStatusFailed(dtResult))
        return false;

    // Find vertex far enough to steer to.
    uint32 ns = 0;
    while (ns < nsteerPath)
    {
        // Stop at Off-Mesh link or when point is further than slop away.
        if ((steerPathFlags[ns] & DT_STRAIGHTPATH_OFFMESH_CONNECTION) ||
            !InRangeYZX(&steerPath[ns*VERTEX_SIZE], startPos, minTargetDist, 1000.0f))
            break;
        ns++;
    }
    // Failed to find good point to steer to.
    if (ns >= nsteerPath)
        return false;

    dtVcopy(steerPos, &steerPath[ns*VERTEX_SIZE]);
    steerPos[1] = startPos[1];  // keep Z value
    steerPosFlag = steerPathFlags[ns];
    steerPosRef = steerPathPolys[ns];

    return true;
}

//Compute the cross product AB x AC
float CrossProduct(float* pointA, float* pointB, float* pointC)
{
    float AB[2];
    float AC[2];
    AB[0] = pointB[0] - pointA[0];
    AB[1] = pointB[1] - pointA[1];
    AC[0] = pointC[0] - pointA[0];
    AC[1] = pointC[1] - pointA[1];
    float cross = AB[0] * AC[1] - AB[1] * AC[0];

    return cross;
}

//Compute the distance from A to B
float Distance(float* pointA, float* pointB)
{
    float d1 = pointA[0] - pointB[0];
    float d2 = pointA[2] - pointB[2];

    return sqrt(d1 * d1 + d2 * d2);
}

float Distance2DPointToLineYZX(float* lineA, float* lineB, float* point)
{
    return std::abs(CrossProduct(lineA, lineB, point) / Distance(lineA, lineB));
}

dtStatus PathGenerator::FindSmoothPath(float const* startPos, float const* endPos,
                                     dtPolyRef const* polyPath, uint32 polyPathSize,
                                     float* smoothPath, int* smoothPathSize, uint32 maxSmoothPathSize)
{
    *smoothPathSize = 0;
    uint32 nsmoothPath = 0;

    dtPolyRef polys[MAX_PATH_LENGTH];
    memcpy(polys, polyPath, sizeof(dtPolyRef)*polyPathSize);
    uint32 npolys = polyPathSize;

    float iterPos[VERTEX_SIZE], targetPos[VERTEX_SIZE];

    if (polyPathSize > 1)
    {
        // Pick the closest poitns on poly border
        if (dtStatusFailed(m_navMeshQuery->closestPointOnPolyBoundary(polys[0], startPos, iterPos)))
            return DT_FAILURE;

        if (dtStatusFailed(m_navMeshQuery->closestPointOnPolyBoundary(polys[npolys - 1], endPos, targetPos)))
            return DT_FAILURE;
    }
    else
    {
        // Case where the path is on the same poly
        dtVcopy(iterPos, startPos);
        dtVcopy(targetPos, endPos);
    }

    dtVcopy(&smoothPath[nsmoothPath*VERTEX_SIZE], iterPos);
    nsmoothPath++;

    // Move towards target a small advancement at a time until target reached or
    // when ran out of memory to store the path.
    while (npolys && nsmoothPath < maxSmoothPathSize)
    {
        // Find location to steer towards.
        float steerPos[VERTEX_SIZE];
        unsigned char steerPosFlag;
        dtPolyRef steerPosRef = INVALID_POLYREF;

        if (!GetSteerTarget(iterPos, targetPos, SMOOTH_PATH_SLOP, polys, npolys, steerPos, steerPosFlag, steerPosRef))
            break;

        bool endOfPath = (steerPosFlag & DT_STRAIGHTPATH_END) != 0;
        bool offMeshConnection = (steerPosFlag & DT_STRAIGHTPATH_OFFMESH_CONNECTION) != 0;

        // Find movement delta.
        float delta[VERTEX_SIZE];
        dtVsub(delta, steerPos, iterPos);
        float len = dtMathSqrtf(dtVdot(delta, delta));
        // If the steer target is end of path or off-mesh link, do not move past the location.
        if ((endOfPath || offMeshConnection) && len < SMOOTH_PATH_STEP_SIZE)
            len = 1.0f;
        else
            len = SMOOTH_PATH_STEP_SIZE / len;

        float moveTgt[VERTEX_SIZE];
        dtVmad(moveTgt, iterPos, delta, len);

        // Move
        float result[VERTEX_SIZE];
        const static uint32 MAX_VISIT_POLY = 16;
        dtPolyRef visited[MAX_VISIT_POLY];

        uint32 nvisited = 0;
        if (dtStatusFailed(m_navMeshQuery->moveAlongSurface(polys[0], iterPos, moveTgt, &m_filter, result, visited, (int*)&nvisited, MAX_VISIT_POLY)))
            return DT_FAILURE;
        npolys = FixupCorridor(polys, npolys, MAX_PATH_LENGTH, visited, nvisited);

        if (dtStatusFailed(m_navMeshQuery->getPolyHeight(polys[0], result, &result[1])))
            DEBUG_LOG("Cannot find height at position X: %f Y: %f Z: %f for %s", result[2], result[0], result[1], m_source->GetGuidStr().c_str());
        result[1] += 0.5f;
        dtVcopy(iterPos, result);

        // Handle end of path and off-mesh links when close enough.
        if (endOfPath && InRangeYZX(iterPos, steerPos, SMOOTH_PATH_SLOP, 1.0f))
        {
            // Reached end of path.
            dtVcopy(iterPos, targetPos);
            if (nsmoothPath < maxSmoothPathSize)
            {
                dtVcopy(&smoothPath[nsmoothPath*VERTEX_SIZE], iterPos);
                nsmoothPath++;
            }
            break;
        }
        else if (offMeshConnection && InRangeYZX(iterPos, steerPos, SMOOTH_PATH_SLOP, 1.0f))
        {
            // Advance the path up to and over the off-mesh connection.
            dtPolyRef prevRef = INVALID_POLYREF;
            dtPolyRef polyRef = polys[0];
            uint32 npos = 0;
            while (npos < npolys && polyRef != steerPosRef)
            {
                prevRef = polyRef;
                polyRef = polys[npos];
                npos++;
            }

            for (uint32 i = npos; i < npolys; ++i)
                polys[i-npos] = polys[i];

            npolys -= npos;

            // Handle the connection.
            float connectionStartPos[VERTEX_SIZE], connectionEndPos[VERTEX_SIZE];
            if (dtStatusSucceed(m_navMesh->getOffMeshConnectionPolyEndPoints(prevRef, polyRef, connectionStartPos, connectionEndPos)))
            {
                if (nsmoothPath < maxSmoothPathSize)
                {
                    dtVcopy(&smoothPath[nsmoothPath*VERTEX_SIZE], connectionStartPos);
                    nsmoothPath++;
                }
                // Move position at the other side of the off-mesh link.
                dtVcopy(iterPos, connectionEndPos);
                if (dtStatusFailed(m_navMeshQuery->getPolyHeight(polys[0], iterPos, &iterPos[1])))
                    return DT_FAILURE;
                iterPos[1] += 0.5f;
            }
        }

        // Store results.
        if (nsmoothPath < maxSmoothPathSize)
        {
            dtVcopy(&smoothPath[nsmoothPath*VERTEX_SIZE], iterPos);
            nsmoothPath++;
        }
    }

    *smoothPathSize = nsmoothPath;

    // this is most likely a loop
    return nsmoothPath < MAX_POINT_PATH_LENGTH ? DT_SUCCESS : DT_FAILURE;
}

// Nostalrius
bool PathGenerator::UpdateForCaster(Unit* pTarget, float castRange)
{
    // If already in range and LOS
    if (pTarget->IsWithinDist3d(m_source->GetPositionX(), m_source->GetPositionY(), m_source->GetPositionZ(), castRange) &&
            pTarget->IsWithinLOS(m_source->GetPositionX(), m_source->GetPositionY(), m_source->GetPositionZ()))
    {
        Clear();
        m_type = PathType(PATHFIND_SHORTCUT | PATHFIND_NORMAL);
        m_pathPoints.resize(2);
        m_pathPoints[0] = GetStartPosition();
        m_pathPoints[1] = GetStartPosition();
        return true;
    }
    uint32 maxIndex = m_pathPoints.size() - 1;
    // We have always keep at least 2 points (else, there is no mvt !)
    for (uint32 i = 1; i <= maxIndex; ++i)
    {
        if (pTarget->IsWithinDist3d(m_pathPoints[i].x, m_pathPoints[i].y, m_pathPoints[i].z, castRange) &&
                pTarget->IsWithinLOS(m_pathPoints[i].x, m_pathPoints[i].y, m_pathPoints[i].z))
        {
            G3D::Vector3 startPoint = m_pathPoints[i - 1];
            G3D::Vector3 endPoint = m_pathPoints[i];
            G3D::Vector3 dirVect = endPoint - startPoint;
            float targetDist1 = pTarget->GetDistance(m_pathPoints[i].x, m_pathPoints[i].y, m_pathPoints[i].z);
            float targetDist2 = pTarget->GetDistance(m_pathPoints[i - 1].x, m_pathPoints[i - 1].y, m_pathPoints[i - 1].z);
            if ((targetDist2 > targetDist1) && (targetDist2 > castRange))
            {
                float nonInRangeDist = (targetDist2 - castRange / targetDist2 - targetDist1);
                float directionLength = sqrt(dirVect.squaredLength());
                // Thales not applicable but still a valid start point due to conditions.
                startPoint += dirVect * nonInRangeDist / directionLength;
            }

            if (pTarget->IsWithinDist3d(startPoint.x, startPoint.y, startPoint.z, castRange) &&
                    pTarget->IsWithinLOS(startPoint.x, startPoint.y, startPoint.z))
                m_pathPoints[i] = startPoint;
            m_pathPoints.resize(i + 1);

            return false;
        }
    }
    return false;
}

bool PathGenerator::UpdateForMelee(Unit* pTarget, float meleeReach)
{
    // Si deja en ligne de vision, et a distance, c'est bon.
    if (pTarget->IsWithinDist3d(m_source->GetPositionX(), m_source->GetPositionY(), m_source->GetPositionZ(), meleeReach))
    {
        Clear();
        m_type = PathType(PATHFIND_SHORTCUT | PATHFIND_NORMAL);
        m_pathPoints.resize(2);
        m_pathPoints[0] = GetStartPosition();
        m_pathPoints[1] = GetStartPosition();
        return true;
    }

    uint32 maxIndex = m_pathPoints.size() - 1;
    // We have always keep at least 2 points (else, there is no mvt !)
    for (uint32 i = 1; i <= maxIndex; ++i)
    {
        if (pTarget->IsWithinDist3d(m_pathPoints[i].x, m_pathPoints[i].y, m_pathPoints[i].z, meleeReach))
        {
            G3D::Vector3 dirVect;
            pTarget->GetPosition(dirVect.x, dirVect.y, dirVect.z);
            dirVect -= m_pathPoints[i - 1];
            float targetDist = pTarget->GetDistance(m_pathPoints[i - 1].x, m_pathPoints[i - 1].y, m_pathPoints[i - 1].z) - meleeReach;
            float directionLength = sqrt(dirVect.squaredLength());
            m_pathPoints[i] = m_pathPoints[i - 1] + dirVect * targetDist / directionLength;
            m_pathPoints.resize(i + 1);
            return false;
        }
    }
    return false;
}

void PathGenerator::CutPathWithDynamicLoS()
{
    uint32 maxIndex = m_pathPoints.size() - 1;
    G3D::Vector3 out;
    // We have always keep at least 2 points (else, there is no mvt !)
    for (uint32 i = 1; i <= maxIndex; ++i)
        if (m_source->GetMap()->GetDynamicObjectHitPos(m_pathPoints[i - 1], m_pathPoints[i], out, -0.1f))
        {
            m_pathPoints[i] = out;
            m_pathPoints.resize(i + 1);
            break;
        }
}

float PathGenerator::Length() const
{
    ASSERT(m_pathPoints.size());
    float length = 0.0f;
    uint32 maxIndex = m_pathPoints.size() - 1;
    for (uint32 i = 1; i <= maxIndex; ++i)
        length += (m_pathPoints[i - 1] - m_pathPoints[i]).length();
    return length;
}

bool PathGenerator::InRangeYZX(float const* v1, float const* v2, float r, float h)
{
    float const dx = v2[0] - v1[0];
    float const dy = v2[1] - v1[1]; // elevation
    float const dz = v2[2] - v1[2];
    return (dx * dx + dz * dz) < r * r && fabsf(dy) < h;
}

bool PathGenerator::InRange(G3D::Vector3 const& p1, G3D::Vector3 const& p2, float r, float h)
{
    G3D::Vector3 d = p1 - p2;
    return (d.x * d.x + d.y * d.y) < r * r && fabsf(d.z) < h;
}

float PathGenerator::Dist3DSqr(G3D::Vector3 const& p1, G3D::Vector3 const& p2)
{
    return (p1 - p2).squaredLength();
}
