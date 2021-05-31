/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Common.h"
#include "Transport.h"
#include "MapManager.h"
#include "ObjectMgr.h"
#include "Path.h"
#include "WorldPacket.h"
#include "World.h"
#include "GameObjectAI.h"
#include "MapReference.h"
#include "Player.h"
#include "Cell.h"
#include "CellImpl.h"
#include "GameObjectModel.h"
#include "ObjectAccessor.h"

GenericTransport::GenericTransport() : GameObject(), m_passengerTeleportItr(m_passengers.begin())
{

}

Transport::Transport() : GenericTransport(),
    m_transportInfo(nullptr), m_isMoving(true), m_pendingStop(false)
{
    // the path progress is the only value that seem to matter
    m_updateFlag = UPDATEFLAG_TRANSPORT;
}

Transport::~Transport()
{
    sObjectAccessor.RemoveObject(this);
    ASSERT(m_passengers.empty());
}

bool Transport::Create(uint32 guidlow, uint32 entry, uint32 mapid, float x, float y, float z, float ang, uint32 animprogress)
{
    Relocate(x, y, z, ang);

    if (!IsPositionValid())
    {
        sLog.outError("Transport (GUID: %u) not created. Suggested coordinates isn't valid (X: %f Y: %f)",
                      guidlow, x, y);
        return false;
    }

    Object::_Create(guidlow, 0, HIGHGUID_MO_TRANSPORT);

    GameObjectInfo const* goinfo = ObjectMgr::GetGameObjectInfo(entry);

    if (!goinfo)
    {
        sLog.outErrorDb("Transport not created: entry in `gameobject_template` not found, guidlow: %u map: %u  (X: %f Y: %f Z: %f) ang: %f", guidlow, mapid, x, y, z, ang);
        return false;
    }

    m_goInfo = goinfo;

    TransportTemplate const* tInfo = sTransportMgr->GetTransportTemplate(entry);
    if (!tInfo)
    {
        sLog.outErrorDb("Transport %u (name: %s) will not be created, missing `transport_template` entry.", entry, goinfo->name);
        return false;
    }

    m_transportInfo = tInfo;

    // initialize waypoints
    m_nextFrame = tInfo->keyFrames.begin();
    m_currentFrame = m_nextFrame++;

    m_pathProgress = time(nullptr) % (tInfo->pathTime / 1000);
    m_pathProgress *= 1000;
    SetObjectScale(goinfo->size);
    SetUInt32Value(GAMEOBJECT_FACTION, goinfo->faction);
    SetUInt32Value(GAMEOBJECT_FLAGS, goinfo->flags);
    SetPeriod(tInfo->pathTime);
    SetEntry(goinfo->id);
    SetDisplayId(goinfo->displayId);
    SetGoState(GO_STATE_READY);
    SetGoType(GAMEOBJECT_TYPE_MO_TRANSPORT);
    SetGoAnimProgress(animprogress);
    SetName(goinfo->name);
    UpdateRotationFields(0.0f, 1.0f);

    sObjectAccessor.AddObject(this);
    return true;
}

void GenericTransport::CleanupsBeforeDelete()
{
    while (!m_passengers.empty())
    {
        WorldObject* obj = *m_passengers.begin();
        RemovePassenger(obj);
    }

    GameObject::CleanupsBeforeDelete();
}

void Transport::Update(uint32 update_diff, uint32 /*time_diff*/)
{
    uint32 const positionUpdateDelay = 50;

    if (AI())
        AI()->UpdateAI(update_diff);
    else
        AIM_Initialize();

    if (GetKeyFrames().size() <= 1)
        return;

    if (IsMoving() || !m_pendingStop)
        m_pathProgress = m_pathProgress + update_diff;

    // Set current waypoint
    // Desired outcome: m_currentFrame->DepartureTime < m_pathProgress < m_nextFrame->ArriveTime
    // ... arrive | ... delay ... | departure
    //      event /         event /
    uint32 pathProgress = m_pathProgress % GetPeriod();
    for (;;)
    {
        if (pathProgress >= m_currentFrame->ArriveTime && pathProgress < m_currentFrame->DepartureTime)
        {
            SetMoving(false);
            break;  // its a stop frame and we are waiting
        }

        // not waiting anymore
        SetMoving(true);

        if (pathProgress >= m_currentFrame->DepartureTime && pathProgress < m_currentFrame->NextArriveTime)
            break;  // found current waypoint

        MoveToNextWaypoint();

        DEBUG_LOG("Transport %u (%s) moved to node %u %u %f %f %f", GetEntry(), GetName(), m_currentFrame->Node->index, m_currentFrame->Node->mapid, m_currentFrame->Node->x, m_currentFrame->Node->y, m_currentFrame->Node->z);

        // Departure event
        if (m_currentFrame->IsTeleportFrame())
        {
            if (TeleportTransport(m_nextFrame->Node->mapid, m_nextFrame->Node->x, m_nextFrame->Node->y, m_nextFrame->Node->z, m_nextFrame->InitialOrientation))
                return; // Update more in new map thread
        }
        else if (m_currentFrame->IsUpdateFrame())
        {
            SendOutOfRangeUpdateToMap();
            SendCreateUpdateToMap();
        }
    }

    // Set position
    m_positionChangeTimer.Update(update_diff);
    if (m_positionChangeTimer.Passed())
    {
        m_positionChangeTimer.Reset(positionUpdateDelay);
        if (IsMoving() && pathProgress)
        {
            float t = CalculateSegmentPos(float(pathProgress) * 0.001f);
            G3D::Vector3 pos, dir;
            m_currentFrame->Spline->evaluate_percent(m_currentFrame->Index, t, pos);
            m_currentFrame->Spline->evaluate_derivative(m_currentFrame->Index, t, dir);
            UpdatePosition(pos.x, pos.y, pos.z, atan2(dir.y, dir.x) + M_PI);
        }
    }
}

void GenericTransport::AddPassenger(WorldObject* passenger)
{
    if (!IsInWorld())
        return;

    if (m_passengers.insert(passenger).second)
    {
        DEBUG_LOG("Object %s added to transport %s.", passenger->GetName(), GetName());
        passenger->SetTransport(this);
        passenger->m_movementInfo.AddMovementFlag(MOVEFLAG_ONTRANSPORT);
        passenger->m_movementInfo.t_guid = GetObjectGuid();
        passenger->m_movementInfo.t_time = GetPathProgress();
        if (!passenger->m_movementInfo.t_pos.x)
        {
            passenger->m_movementInfo.t_pos.x = passenger->GetPositionX();
            passenger->m_movementInfo.t_pos.y = passenger->GetPositionY();
            passenger->m_movementInfo.t_pos.z = passenger->GetPositionZ();
            passenger->m_movementInfo.t_pos.o = passenger->GetOrientation();
            CalculatePassengerOffset(passenger->m_movementInfo.t_pos.x, passenger->m_movementInfo.t_pos.y, passenger->m_movementInfo.t_pos.z, &passenger->m_movementInfo.t_pos.o);
        }
    }
}

void GenericTransport::RemovePassenger(WorldObject* passenger)
{
    bool erased = false;
    if (m_passengerTeleportItr != m_passengers.end())
    {
        PassengerSet::iterator itr = m_passengers.find(passenger);
        if (itr != m_passengers.end())
        {
            if (itr == m_passengerTeleportItr)
                ++m_passengerTeleportItr;

            m_passengers.erase(itr);
            erased = true;
        }
    }
    else
        erased = m_passengers.erase(passenger) > 0;

    if (erased)
    {
        passenger->SetTransport(nullptr);
        passenger->m_movementInfo.ClearTransportData();
        DEBUG_LOG("Object %s removed from transport %s.", passenger->GetName(), GetName());
    }
}

bool ElevatorTransport::Create(uint32 guidlow, uint32 name_id, Map* map, float x, float y, float z, float ang, float rotation0, float rotation1, float rotation2, float rotation3, uint32 animprogress, GOState go_state)
{
    if (GenericTransport::Create(guidlow, name_id, map, x, y, z, ang, rotation0, rotation1, rotation2, rotation3, animprogress, go_state))
    {
        m_pathProgress = 0;
        m_animationInfo = sTransportMgr->GetTransportAnimInfo(GetGOInfo()->id);
        m_currentSeg = 0;
        return true;
    }
    return false;
}

void ElevatorTransport::Update(uint32 update_diff, uint32 /*time_diff*/)
{
    if (!m_animationInfo)
        return;

    if (GetGoState() == GO_STATE_READY)
    {
        m_pathProgress += update_diff;
        // TODO: Fix movement in unloaded grid - currently GO will just disappear
        uint32 timer = sWorld.GetCurrentMSTime() % m_animationInfo->TotalTime;
        TransportAnimationEntry const* nodeNext = m_animationInfo->GetNextAnimNode(timer);
        TransportAnimationEntry const* nodePrev = m_animationInfo->GetPrevAnimNode(timer);
        if (nodeNext && nodePrev)
        {
            m_currentSeg = nodePrev->TimeSeg;

            G3D::Vector3 posPrev = G3D::Vector3(nodePrev->X, -nodePrev->Y, nodePrev->Z);
            G3D::Vector3 posNext = G3D::Vector3(nodeNext->X, -nodeNext->Y, nodeNext->Z);
            G3D::Vector3 currentPos;
            if (posPrev == posNext)
                currentPos = posPrev;
            else
            {
                uint32 timeElapsed = timer - nodePrev->TimeSeg;
                uint32 timeDiff = nodeNext->TimeSeg - nodePrev->TimeSeg;
                G3D::Vector3 segmentDiff = posNext - posPrev;
                float velocityX = float(segmentDiff.x) / timeDiff, velocityY = float(segmentDiff.y) / timeDiff, velocityZ = float(segmentDiff.z) / timeDiff;

                currentPos = G3D::Vector3(timeElapsed * velocityX, timeElapsed * velocityY, timeElapsed * velocityZ);
                currentPos += posPrev;
            }

            currentPos += G3D::Vector3(m_stationaryPosition.x, m_stationaryPosition.y, m_stationaryPosition.z);

            GetMap()->GameObjectRelocation(this, currentPos.x, currentPos.y, currentPos.z, GetOrientation());
            // SummonCreature(1, currentPos.x, currentPos.y, currentPos.z, GetOrientation(), TEMPSUMMON_TIMED_DESPAWN, 5000);
            UpdateModelPosition();

            UpdatePassengerPositions(m_passengers);
        }

    }
}

uint32 ElevatorTransport::GetPathProgress() const
{
    return sWorld.GetCurrentMSTime() % m_animationInfo->TotalTime;
}

void GenericTransport::UpdatePosition(float x, float y, float z, float o)
{
    Relocate(x, y, z, o);
    UpdateModelPosition();

    UpdatePassengerPositions(m_passengers);
}

void Transport::MoveToNextWaypoint()
{
    // Set frames
    m_currentFrame = m_nextFrame++;
    if (m_nextFrame == GetKeyFrames().end())
        m_nextFrame = GetKeyFrames().begin();
}

float Transport::CalculateSegmentPos(float now)
{
    KeyFrame const& frame = *m_currentFrame;
    float const speed = float(m_goInfo->moTransport.moveSpeed);
    float const accel = float(m_goInfo->moTransport.accelRate);
    float timeSinceStop = frame.TimeFrom + (now - (1.0f / IN_MILLISECONDS) * frame.DepartureTime);
    float timeUntilStop = frame.TimeTo - (now - (1.0f / IN_MILLISECONDS) * frame.DepartureTime);
    float segmentPos, dist;
    float accelTime = m_transportInfo->accelTime;
    float accelDist = m_transportInfo->accelDist;
    // calculate from nearest stop, less confusing calculation...
    if (timeSinceStop < timeUntilStop)
    {
        if (timeSinceStop < accelTime)
            dist = 0.5f * accel * timeSinceStop * timeSinceStop;
        else
            dist = accelDist + (timeSinceStop - accelTime) * speed;
        segmentPos = dist - frame.DistSinceStop;
    }
    else
    {
        if (timeUntilStop < m_transportInfo->accelTime)
            dist = 0.5f * accel * timeUntilStop * timeUntilStop;
        else
            dist = accelDist + (timeUntilStop - accelTime) * speed;
        segmentPos = frame.DistUntilStop - dist;
    }

    return segmentPos / frame.NextDistFromPrev;
}

bool Transport::TeleportTransport(uint32 newMapid, float x, float y, float z, float o)
{
    Map const* oldMap = GetMap();

    uint32 newInstanceId = sMapMgr.GetContinentInstanceId(newMapid, x, y);
    SetLocationInstanceId(newInstanceId);
    Map* newMap = sMapMgr.CreateMap(newMapid, this);
    GetMap()->Remove<Transport>(this, false);
    SetMap(newMap);

    for (m_passengerTeleportItr = m_passengers.begin(); m_passengerTeleportItr != m_passengers.end();)
    {
        WorldObject* obj = (*m_passengerTeleportItr++);

        float destX, destY, destZ, destO;
        destX = obj->GetTransOffsetX();
        destY = obj->GetTransOffsetY();
        destZ = obj->GetTransOffsetZ();
        destO = obj->GetTransOffsetO();
        CalculatePassengerPosition(destX, destY, destZ, &destO, x, y, z, o);

        switch (obj->GetTypeId())
        {
            case TYPEID_UNIT:
                // Units teleport on transport not implemented.
                RemovePassenger(obj);
                break;
            case TYPEID_GAMEOBJECT:
            {
                GameObject* go = obj->ToGameObject();
                go->GetMap()->Remove(go, false);
                go->Relocate(destX, destY, destZ, destO);
                go->SetMap(newMap);
                newMap->Add(go);
                break;
            }
            case TYPEID_PLAYER:
            {
                // Remove some auras to prevent undermap
                Player* player = obj->ToPlayer();
                if (!player->IsInWorld())
                {
                    RemovePassenger(player);
                    break;
                }

                if (!player->IsAlive())
                    player->ResurrectPlayer(1.0f);

                player->RemoveSpellsCausingAura(SPELL_AURA_MOD_CONFUSE);
                player->RemoveSpellsCausingAura(SPELL_AURA_MOD_FEAR);
                player->CombatStopWithPets(true);

                // No need for teleport packet if no map change
                // The client still shows the correct loading screen when one is needed (Grom'Gol-Undercity)
                if (newMapid == player->GetMapId())
                {
                    player->TeleportPositionRelocation(destX, destY, destZ, destO);
                    if (newInstanceId != player->GetInstanceId())
                        sMapMgr.ScheduleInstanceSwitch(player, newInstanceId);
                }
                else
                    player->TeleportTo(newMapid, destX, destY, destZ, destO,
                        TELE_TO_NOT_LEAVE_TRANSPORT);

                break;
            }
            case TYPEID_DYNAMICOBJECT:
                obj->AddObjectToRemoveList();
                break;
            default:
                break;
        }
    }

    Relocate(x, y, z, o);
    GetMap()->Add<Transport>(this);

    return newMap != oldMap;
}

void GenericTransport::UpdatePassengerPositions(PassengerSet& passengers)
{
    for (const auto passenger : passengers)
        UpdatePassengerPosition(passenger);
}

void GenericTransport::UpdatePassengerPosition(WorldObject* passenger)
{
    // transport teleported but passenger not yet (can happen for players)
    if (passenger->FindMap() != GetMap())
        return;

    // Do not use Unit::UpdatePosition here, we don't want to remove auras
    // as if regular movement occurred
    float x, y, z, o;
    x = passenger->GetTransOffsetX();
    y = passenger->GetTransOffsetY();
    z = passenger->GetTransOffsetZ();
    o = passenger->GetTransOffsetO();
    CalculatePassengerPosition(x, y, z, &o);
    if (!MaNGOS::IsValidMapCoord(x, y, z))
    {
        sLog.outError("[TRANSPORTS] Object %s [guid %u] has invalid position on transport.", passenger->GetName(), passenger->GetGUIDLow());
        return;
    }
    switch (passenger->GetTypeId())
    {
        case TYPEID_UNIT:
        {
            Creature* creature = passenger->ToCreature();
            if (passenger->IsInWorld())
                GetMap()->CreatureRelocation(creature, x, y, z, o);
            else
                passenger->Relocate(x, y, z, o);
            creature->m_movementInfo.t_time = GetPathProgress();
            break;
        }
        case TYPEID_PLAYER:
            //relocate only passengers in world and skip any player that might be still logging in/teleporting
            if (passenger->IsInWorld())
                GetMap()->PlayerRelocation(passenger->ToPlayer(), x, y, z, o);
            else
            {
                passenger->Relocate(x, y, z, o);
                static_cast<Player*>(passenger)->m_movementInfo.t_guid = GetObjectGuid();
            }
            static_cast<Player*>(passenger)->m_movementInfo.t_time = GetPathProgress();
            break;
        case TYPEID_GAMEOBJECT:
            //GetMap()->GameObjectRelocation(passenger->ToGameObject(), x, y, z, o, false);
            break;
        case TYPEID_DYNAMICOBJECT:
            //GetMap()->DynamicObjectRelocation(passenger->ToDynObject(), x, y, z, o);
            break;
        default:
            break;
    }
}

void Transport::DoEventIfAny(KeyFrame const& node, bool departure)
{
}

void Transport::BuildUpdate(UpdateDataMapType& data_map)
{
    Map::PlayerList const& players = GetMap()->GetPlayers();
    if (players.isEmpty())
        return;

    for (const auto& player : players)
        BuildUpdateDataForPlayer(player.getSource(), data_map);

    ClearUpdateMask(true);
}


void Transport::SendOutOfRangeUpdateToMap()
{
    Map::PlayerList const& players = GetMap()->GetPlayers();
    if (!players.isEmpty())
    {
        UpdateData data;
        BuildOutOfRangeUpdateBlock(data);
        WorldPacket packet;
        data.BuildPacket(&packet);
        for (const auto& player : players)
            if (player.getSource()->GetTransport() != this)
                player.getSource()->SendDirectMessage(&packet);
    }
}

void Transport::SendCreateUpdateToMap()
{
    Map::PlayerList const& players = GetMap()->GetPlayers();
    if (!players.isEmpty())
    {
        for (const auto& player : players)
            if (player.getSource()->GetTransport() != this)
            {
                UpdateData data;
                BuildCreateUpdateBlockForPlayer(data, player.getSource());
                data.Send(player.getSource()->GetSession());
            }
    }
}