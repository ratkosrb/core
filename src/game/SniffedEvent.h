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
#include <string>

inline uint32 GetKnownObjectTypeId(std::string const& typeName)
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
    bool IsEmpty()
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

struct SniffedEvent_CreatureText : SniffedEvent
{
    SniffedEvent_CreatureText(uint32 guid, uint32 entry, std::string text, uint32 chatType, std::string comment) : 
        m_guid(guid), m_entry(entry), m_chatType(chatType), m_text(text), m_comment(comment) {};
    uint32 m_guid = 0;
    uint32 m_entry = 0;
    uint32 m_chatType = 0;
    std::string m_text;
    std::string m_comment;
    void Execute() const final;
    SniffedEventType GetType() const final
    {
        return SE_CREATURE_TEXT;
    }
    KnownObject GetSourceObject() const final
    {
        return KnownObject(m_guid, m_entry, TYPEID_UNIT);
    }
};

struct SniffedEvent_CreatureEmote : SniffedEvent
{
    SniffedEvent_CreatureEmote(uint32 guid, uint32 entry, uint32 emoteId) : m_guid(guid), m_entry(entry), m_emoteId(emoteId) {};
    uint32 m_guid = 0;
    uint32 m_entry = 0;
    uint32 m_emoteId = 0;
    void Execute() const final;
    SniffedEventType GetType() const final
    {
        return SE_CREATURE_EMOTE;
    }
    KnownObject GetSourceObject() const final
    {
        return KnownObject(m_guid, m_entry, TYPEID_UNIT);
    }
};

struct SniffedEvent_CreatureAttackStart : SniffedEvent
{
    SniffedEvent_CreatureAttackStart(uint32 guid, uint32 entry, uint32 victimGuid, uint32 victimId, uint32 victimType) :
        m_guid(guid), m_entry(entry), m_victimGuid(victimGuid), m_victimId(victimId), m_victimType(victimType) {};
    uint32 m_guid = 0;
    uint32 m_entry = 0;
    uint32 m_victimGuid = 0;
    uint32 m_victimId = 0;
    uint32 m_victimType;
    void Execute() const final;
    SniffedEventType GetType() const final
    {
        return SE_CREATURE_ATTACK_START;
    }
    KnownObject GetSourceObject() const final
    {
        return KnownObject(m_guid, m_entry, TYPEID_UNIT);
    }
    KnownObject GetTargetObject() const final
    {
        return KnownObject(m_victimGuid, m_victimId, TypeID(m_victimType));
    }
};

struct SniffedEvent_CreatureAttackStop : SniffedEvent
{
    SniffedEvent_CreatureAttackStop(uint32 guid, uint32 entry, uint32 victimGuid, uint32 victimId, uint32 victimType) :
        m_guid(guid), m_entry(entry), m_victimGuid(victimGuid), m_victimId(victimId), m_victimType(victimType) {};
    uint32 m_guid = 0;
    uint32 m_entry = 0;
    uint32 m_victimGuid = 0;
    uint32 m_victimId = 0;
    uint32 m_victimType;
    void Execute() const final;
    SniffedEventType GetType() const final
    {
        return SE_CREATURE_ATTACK_STOP;
    }
    KnownObject GetSourceObject() const final
    {
        return KnownObject(m_guid, m_entry, TYPEID_UNIT);
    }
    KnownObject GetTargetObject() const final
    {
        return KnownObject(m_victimGuid, m_victimId, TypeID(m_victimType));
    }
};

struct SniffedEvent_CreatureUpdate_entry : SniffedEvent
{
    SniffedEvent_CreatureUpdate_entry(uint32 guid, uint32 entry, uint32 value) :
        m_guid(guid), m_entry(entry), m_value(value) {};
    uint32 m_guid = 0;
    uint32 m_entry = 0;
    uint32 m_value = 0;
    void Execute() const final;
    SniffedEventType GetType() const final
    {
        return SE_CREATURE_UPDATE_ENTRY;
    }
    KnownObject GetSourceObject() const final
    {
        return KnownObject(m_guid, m_entry, TYPEID_UNIT);
    }
};

struct SniffedEvent_CreatureUpdate_display_id : SniffedEvent
{
    SniffedEvent_CreatureUpdate_display_id(uint32 guid, uint32 entry, uint32 value) :
        m_guid(guid), m_entry(entry), m_value(value) {};
    uint32 m_guid = 0;
    uint32 m_entry = 0;
    uint32 m_value = 0;
    void Execute() const final;
    SniffedEventType GetType() const final
    {
        return SE_CREATURE_UPDATE_DISPLAY_ID;
    }
    KnownObject GetSourceObject() const final
    {
        return KnownObject(m_guid, m_entry, TYPEID_UNIT);
    }
};

struct SniffedEvent_CreatureUpdate_faction : SniffedEvent
{
    SniffedEvent_CreatureUpdate_faction(uint32 guid, uint32 entry, uint32 value) :
        m_guid(guid), m_entry(entry), m_value(value) {};
    uint32 m_guid = 0;
    uint32 m_entry = 0;
    uint32 m_value = 0;
    void Execute() const final;
    SniffedEventType GetType() const final
    {
        return SE_CREATURE_UPDATE_FACTION;
    }
    KnownObject GetSourceObject() const final
    {
        return KnownObject(m_guid, m_entry, TYPEID_UNIT);
    }
};

struct SniffedEvent_CreatureUpdate_emote_state : SniffedEvent
{
    SniffedEvent_CreatureUpdate_emote_state(uint32 guid, uint32 entry, uint32 value) :
        m_guid(guid), m_entry(entry), m_value(value) {};
    uint32 m_guid = 0;
    uint32 m_entry = 0;
    uint32 m_value = 0;
    void Execute() const final;
    SniffedEventType GetType() const final
    {
        return SE_CREATURE_UPDATE_EMOTE_STATE;
    }
    KnownObject GetSourceObject() const final
    {
        return KnownObject(m_guid, m_entry, TYPEID_UNIT);
    }
};

struct SniffedEvent_CreatureUpdate_stand_state : SniffedEvent
{
    SniffedEvent_CreatureUpdate_stand_state(uint32 guid, uint32 entry, uint32 value) :
        m_guid(guid), m_entry(entry), m_value(value) {};
    uint32 m_guid = 0;
    uint32 m_entry = 0;
    uint32 m_value = 0;
    void Execute() const final;
    SniffedEventType GetType() const final
    {
        return SE_CREATURE_UPDATE_STAND_STATE;
    }
    KnownObject GetSourceObject() const final
    {
        return KnownObject(m_guid, m_entry, TYPEID_UNIT);
    }
};

struct SniffedEvent_CreatureUpdate_npc_flags : SniffedEvent
{
    SniffedEvent_CreatureUpdate_npc_flags(uint32 guid, uint32 entry, uint32 value) :
        m_guid(guid), m_entry(entry), m_value(value) {};
    uint32 m_guid = 0;
    uint32 m_entry = 0;
    uint32 m_value = 0;
    void Execute() const final;
    SniffedEventType GetType() const final
    {
        return SE_CREATURE_UPDATE_NPC_FLAGS;
    }
    KnownObject GetSourceObject() const final
    {
        return KnownObject(m_guid, m_entry, TYPEID_UNIT);
    }
};

struct SniffedEvent_CreatureUpdate_unit_flags : SniffedEvent
{
    SniffedEvent_CreatureUpdate_unit_flags(uint32 guid, uint32 entry, uint32 value) :
        m_guid(guid), m_entry(entry), m_value(value) {};
    uint32 m_guid = 0;
    uint32 m_entry = 0;
    uint32 m_value = 0;
    void Execute() const final;
    SniffedEventType GetType() const final
    {
        return SE_CREATURE_UPDATE_UNIT_FLAGS;
    }
    KnownObject GetSourceObject() const final
    {
        return KnownObject(m_guid, m_entry, TYPEID_UNIT);
    }
};

struct SniffedEvent_SpellCastStart : SniffedEvent
{
    SniffedEvent_SpellCastStart(uint32 spellId, uint32 castFlags, uint32 casterGuid, uint32 casterId, uint32 casterType, uint32 targetGuid, uint32 targetId, uint32 targetType) :
        m_spellId(spellId), m_castFlags(castFlags), m_casterGuid(casterGuid), m_casterId(casterId), m_casterType(casterType), m_targetGuid(targetGuid), m_targetId(targetId), m_targetType(targetType) {};
    uint32 m_spellId = 0;
    uint32 m_castFlags = 0;
    uint32 m_casterGuid = 0;
    uint32 m_casterId = 0;
    uint32 m_casterType;
    uint32 m_targetGuid = 0;
    uint32 m_targetId = 0;
    uint32 m_targetType;
    void Execute() const final;
    SniffedEventType GetType() const final
    {
        return SE_SPELL_CAST_START;
    }
    KnownObject GetSourceObject() const final
    {
        return KnownObject(m_casterGuid, m_casterId, TypeID(m_casterType));
    }
    KnownObject GetTargetObject() const final
    {
        return KnownObject(m_targetGuid, m_targetId, TypeID(m_targetType));
    }
};

struct SniffedEvent_SpellCastGo : SniffedEvent
{
    SniffedEvent_SpellCastGo(uint32 spellId, uint32 casterGuid, uint32 casterId, uint32 casterType, uint32 targetGuid, uint32 targetId, uint32 targetType) :
        m_spellId(spellId), m_casterGuid(casterGuid), m_casterId(casterId), m_casterType(casterType), m_targetGuid(targetGuid), m_targetId(targetId), m_targetType(targetType) {};
    uint32 m_spellId = 0;
    uint32 m_casterGuid = 0;
    uint32 m_casterId = 0;
    uint32 m_casterType;
    uint32 m_targetGuid = 0;
    uint32 m_targetId = 0;
    uint32 m_targetType;
    void Execute() const final;
    SniffedEventType GetType() const final
    {
        return SE_SPELL_CAST_GO;
    }
    KnownObject GetSourceObject() const final
    {
        return KnownObject(m_casterGuid, m_casterId, TypeID(m_casterType));
    }
    KnownObject GetTargetObject() const final
    {
        return KnownObject(m_targetGuid, m_targetId, TypeID(m_targetType));
    }
};


#endif
