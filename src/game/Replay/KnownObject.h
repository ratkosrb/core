/*
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

#ifndef MANGOS_H_KNOWNOBJECT
#define MANGOS_H_KNOWNOBJECT

#include "Common.h"
#include "SharedDefines.h"
#include <string>

inline TypeID GetKnownObjectTypeId(std::string const& typeName)
{
    if (typeName == "Creature" || typeName == "Pet")
        return TYPEID_UNIT;
    else if (typeName == "Player")
        return TYPEID_PLAYER;
    else if (typeName == "GameObject")
        return TYPEID_GAMEOBJECT;
    return TYPEID_OBJECT;
}

struct KnownObject
{
    KnownObject() {};
    KnownObject(uint32 guid, uint32 entry, TypeID type) :
        m_guid(guid), m_entry(entry), m_type(type) {};
    uint32 m_guid = 0;
    uint32 m_entry = 0;
    TypeID m_type = TYPEID_OBJECT;
    bool operator==(KnownObject const& other) const
    {
        return m_guid == other.m_guid &&
            m_entry == other.m_entry &&
            m_type == other.m_type;
    }
    bool operator!=(KnownObject const& other) const
    {
        return !(m_guid == other.m_guid &&
            m_entry == other.m_entry &&
            m_type == other.m_type);
    }
    bool operator<(KnownObject const& other) const
    {
        return m_guid < other.m_guid;
    }
    bool IsEmpty() const
    {
        return !m_type;
    }
};

inline std::string FormatObjectName(KnownObject object)
{
    std::string name;
    if (object.m_type == TYPEID_PLAYER)
        name = "Player (Guid: " + std::to_string(object.m_guid) + ")";
    else if (object.m_type == TYPEID_UNIT)
        name = "Creature (Guid: " + std::to_string(object.m_guid) + " Entry: " + std::to_string(object.m_entry) + ")";
    else if (object.m_type == TYPEID_GAMEOBJECT)
        name = "GameObject (Guid: " + std::to_string(object.m_guid) + " Entry: " + std::to_string(object.m_entry) + ")";
    else
        name = "Entry: " + std::to_string(object.m_entry);
    return name;
}

#endif
