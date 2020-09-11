/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2018  MaNGOS project <https://getmangos.eu>
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
 *
 * World of Warcraft, and all World of Warcraft or Warcraft art, images,
 * and lore are copyrighted by Blizzard Entertainment, Inc.
 */

#ifndef MANGOS_H_SNIFFEDEVENT
#define MANGOS_H_SNIFFEDEVENT

#include "Common.h"
#include "SharedDefines.h"
#include "Timer.h"

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
    bool IsEmpty()
    {
        return !m_type;
    }
};

enum SniffedEventType : uint8
{
    SE_CREATURE_CREATE1,
    SE_CREATURE_CREATE2,
    SE_CREATURE_DESTROY,
    SE_CREATURE_TEXT,
    SE_CREATURE_EMOTE,
    SE_CREATURE_ATTACK_START,
    SE_CREATURE_ATTACK_STOP,
    SE_CREATURE_MOVEMENT,
    SE_CREATURE_FACING,
    SE_CREATURE_UPDATE_ENTRY,
    SE_CREATURE_UPDATE_DISPLAY_ID,
    SE_CREATURE_UPDATE_FACTION,
    SE_CREATURE_UPDATE_EMOTE_STATE,
    SE_CREATURE_UPDATE_STAND_STATE,
    SE_CREATURE_UPDATE_NPC_FLAGS,
    SE_CREATURE_UPDATE_UNIT_FLAGS,
    SE_GAMEOBJECT_CREATE1,
    SE_GAMEOBJECT_CREATE2,
    SE_GAMEOBJECT_DESTROY,
    SE_GAMEOBJECT_UPDATE_FLAGS,
    SE_GAMEOBJECT_UPDATE_STATE,
    SE_PLAY_MUSIC,
    SE_PLAY_SOUND,
    SE_SPELL_CAST_START,
    SE_SPELL_CAST_GO,
    SE_CLIENT_QUEST_ACCEPT,
    SE_CLIENT_QUEST_COMPLETE,
    SE_CLIENT_CREATURE_INTERACT,
    SE_CLIENT_GAMEOBJECT_USE,
    SE_CLIENT_ITEM_USE
};

struct SniffedEvent
{
    virtual void Execute() const = 0;
    virtual SniffedEventType GetType() const = 0;
    virtual KnownObject GetSourceObject() const { return KnownObject(); };
    virtual KnownObject GetTargetObject() const { return KnownObject(); };
};

struct SniffedEvent_CreatureCreate1 : SniffedEvent
{
    SniffedEvent_CreatureCreate1(uint32 guid, uint32 entry, float x, float y, float z, float o) :
        m_guid(guid), m_entry(entry), m_x(x), m_y(y), m_z(z), m_o(o) {};
    uint32 m_guid = 0;
    uint32 m_entry = 0;
    float m_x = 0.0f;
    float m_y = 0.0f;
    float m_z = 0.0f;
    float m_o = 0.0f;
    void Execute() const final;
    SniffedEventType GetType() const final
    {
        return SE_CREATURE_CREATE1;
    }
    KnownObject GetSourceObject() const final
    {
        return KnownObject(m_guid, m_entry, TYPEID_UNIT);
    }
};

struct SniffedEvent_CreatureCreate2 : SniffedEvent
{
    SniffedEvent_CreatureCreate2(uint32 guid, uint32 entry, float x, float y, float z, float o) :
        m_guid(guid), m_entry(entry), m_x(x), m_y(y), m_z(z), m_o(o) {};
    uint32 m_guid = 0;
    uint32 m_entry = 0;
    float m_x = 0.0f;
    float m_y = 0.0f;
    float m_z = 0.0f;
    float m_o = 0.0f;
    void Execute() const final;
    SniffedEventType GetType() const final
    {
        return SE_CREATURE_CREATE2;
    }
    KnownObject GetSourceObject() const final
    {
        return KnownObject(m_guid, m_entry, TYPEID_UNIT);
    }
};

struct SniffedEvent_CreatureDestroy : SniffedEvent
{
    SniffedEvent_CreatureDestroy(uint32 guid, uint32 entry) : m_guid(guid), m_entry(entry) {};
    uint32 m_guid = 0;
    uint32 m_entry = 0;
    void Execute() const final;
    SniffedEventType GetType() const final
    {
        return SE_CREATURE_DESTROY;
    }
    KnownObject GetSourceObject() const final
    {
        return KnownObject(m_guid, m_entry, TYPEID_UNIT);
    }
};

struct SniffedEvent_CreatureMovement : SniffedEvent
{
    SniffedEvent_CreatureMovement(uint32 guid, uint32 entry, uint32 moveTime, float x, float y, float z, float o) :
        m_guid(guid), m_entry(entry), m_moveTime(moveTime), m_x(x), m_y(y), m_z(z), m_o(o) {};
    uint32 m_guid = 0;
    uint32 m_entry = 0;
    uint32 m_moveTime = 0;
    float m_x = 0.0f;
    float m_y = 0.0f;
    float m_z = 0.0f;
    float m_o = 0.0f;
    void Execute() const final;
    SniffedEventType GetType() const final
    {
        return SE_CREATURE_MOVEMENT;
    }
    KnownObject GetSourceObject() const final
    {
        return KnownObject(m_guid, m_entry, TYPEID_UNIT);
    }
};

struct SniffedEvent_CreatureFacing : SniffedEvent
{
    SniffedEvent_CreatureFacing(uint32 guid, uint32 entry, float o) :
        m_guid(guid), m_entry(entry), m_o(o) {};
    uint32 m_guid = 0;
    uint32 m_entry = 0;
    float m_o = 0.0f;
    void Execute() const final;
    SniffedEventType GetType() const final
    {
        return SE_CREATURE_FACING;
    }
    KnownObject GetSourceObject() const final
    {
        return KnownObject(m_guid, m_entry, TYPEID_UNIT);
    }
};

#endif
