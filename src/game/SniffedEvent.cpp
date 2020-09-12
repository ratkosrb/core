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

#include "Policies/SingletonImp.h"
#include "ReplayMgr.h"
#include "WorldSession.h"
#include "Player.h"
#include "Map.h"
#include "World.h"
#include "WorldPacket.h"
#include "Log.h"
#include "Util.h"
#include "ProgressBar.h"
#include "ReplayBotAI.h"
#include "PlayerBotMgr.h"
#include "Chat.h"
#include "Spell.h"

void ReplayMgr::LoadCreatureCreate1()
{
    if (auto result = SniffDatabase.Query("SELECT `guid`, `unixtime`, `position_x`, `position_y`, `position_z`, `orientation` FROM `creature_create1_time` ORDER BY `unixtime`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 guid = fields[0].GetUInt32();
            uint32 creatureId = GetCreatureEntryFromGuid(guid);
            uint32 unixtime = fields[1].GetUInt32();
            float position_x = fields[2].GetFloat();
            float position_y = fields[3].GetFloat();
            float position_z = fields[4].GetFloat();
            float orientation = fields[5].GetFloat();

            std::shared_ptr<SniffedEvent_CreatureCreate1> newEvent = std::make_shared<SniffedEvent_CreatureCreate1>(guid, creatureId, position_x, position_y, position_z, orientation);
            m_eventsMap.insert(std::make_pair(unixtime, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

void SniffedEvent_CreatureCreate1::Execute() const
{
    Creature* pCreature = sReplayMgr.GetCreature(m_guid);
    if (!pCreature)
    {
        sLog.outError("SniffedEvent_CreatureCreate1: Cannot find source creature!");
        return;
    }
    pCreature->NearTeleportTo(m_x, m_y, m_z, m_o);
    pCreature->SetVisibility(VISIBILITY_ON);
}

void ReplayMgr::LoadCreatureCreate2()
{
    if (auto result = SniffDatabase.Query("SELECT `guid`, `unixtime`, `position_x`, `position_y`, `position_z`, `orientation` FROM `creature_create2_time` ORDER BY `unixtime`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 guid = fields[0].GetUInt32();
            uint32 creatureId = GetCreatureEntryFromGuid(guid);
            uint32 unixtime = fields[1].GetUInt32();
            float position_x = fields[2].GetFloat();
            float position_y = fields[3].GetFloat();
            float position_z = fields[4].GetFloat();
            float orientation = fields[5].GetFloat();

            std::shared_ptr<SniffedEvent_CreatureCreate2> newEvent = std::make_shared<SniffedEvent_CreatureCreate2>(guid, creatureId, position_x, position_y, position_z, orientation);
            m_eventsMap.insert(std::make_pair(unixtime, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

void SniffedEvent_CreatureCreate2::Execute() const
{
    Creature* pCreature = sReplayMgr.GetCreature(m_guid);
    if (!pCreature)
    {
        sLog.outError("SniffedEvent_CreatureCreate2: Cannot find source creature!");
        return;
    }
    pCreature->NearTeleportTo(m_x, m_y, m_z, m_o);
    pCreature->SetVisibility(VISIBILITY_ON);
}

void ReplayMgr::LoadCreatureDestroy()
{
    if (auto result = SniffDatabase.Query("SELECT `guid`, `unixtime` FROM `creature_destroy_time` ORDER BY `unixtime`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 guid = fields[0].GetUInt32();
            uint32 creatureId = GetCreatureEntryFromGuid(guid);
            uint32 unixtime = fields[1].GetUInt32();

            std::shared_ptr<SniffedEvent_CreatureDestroy> newEvent = std::make_shared<SniffedEvent_CreatureDestroy>(guid, creatureId);
            m_eventsMap.insert(std::make_pair(unixtime, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

void SniffedEvent_CreatureDestroy::Execute() const
{
    Creature* pCreature = sReplayMgr.GetCreature(m_guid);
    if (!pCreature)
    {
        sLog.outError("SniffedEvent_CreatureDestroy: Cannot find source creature!");
        return;
    }
    pCreature->SetVisibility(VISIBILITY_OFF);
}

void ReplayMgr::LoadCreatureMovement(char const* tableName)
{
    if (auto result = SniffDatabase.PQuery("SELECT `id`, `spline_count`, `move_time`, `start_position_x`, `start_position_y`, `start_position_z`, `end_position_x`, `end_position_y`, `end_position_z`, `orientation`, `unixtime` FROM `%s` ORDER BY `unixtime`", tableName))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 guid = fields[0].GetUInt32();
            uint32 creatureId = GetCreatureEntryFromGuid(guid);
            uint32 spline_count = fields[1].GetUInt32();
            uint32 moveTime = fields[2].GetUInt32();
            float startX = fields[3].GetFloat();
            float startY = fields[4].GetFloat();
            float startZ = fields[5].GetFloat();
            float endX = fields[6].GetFloat();
            float endY = fields[7].GetFloat();
            float endZ = fields[8].GetFloat();
            float orientation = fields[9].GetFloat();
            uint32 unixtime = fields[10].GetUInt32();

            if (spline_count == 0 && orientation != 100)
            {
                std::shared_ptr<SniffedEvent_CreatureFacing> newEvent = std::make_shared<SniffedEvent_CreatureFacing>(guid, creatureId, orientation);
                m_eventsMap.insert(std::make_pair(unixtime, newEvent));
            }
            else
            {
                float x = spline_count ? endX : startX;
                float y = spline_count ? endY : startY;
                float z = spline_count ? endZ : startZ;
                std::shared_ptr<SniffedEvent_CreatureMovement> newEvent = std::make_shared<SniffedEvent_CreatureMovement>(guid, creatureId, moveTime, x, y, z, orientation);
                m_eventsMap.insert(std::make_pair(unixtime, newEvent));
            }
        } while (result->NextRow());
        delete result;
    }
}

void SniffedEvent_CreatureMovement::Execute() const
{
    Creature* pCreature = sReplayMgr.GetCreature(m_guid);
    if (!pCreature)
    {
        sLog.outError("SniffedEvent_CreatureMovement: Cannot find source creature!");
        return;
    }
    float speed = m_moveTime != 0 ? pCreature->GetDistance(m_x, m_y, m_z) / ((float)m_moveTime * 0.001f) : 0.0f;
    float orientation = m_o != 100 ? m_o : -10;
    pCreature->MonsterMoveWithSpeed(m_x, m_y, m_z, orientation, speed, MOVE_FORCE_DESTINATION);
}

void SniffedEvent_CreatureFacing::Execute() const
{
    Creature* pCreature = sReplayMgr.GetCreature(m_guid);
    if (!pCreature)
    {
        sLog.outError("SniffedEvent_CreatureFacing: Cannot find source creature!");
        return;
    }
    pCreature->SetOrientation(m_o);
}

enum ChatMessageType
{
    SNIFF_CHAT_TYPE_MONSTERSAY           = 12,
    SNIFF_CHAT_TYPE_MONSTERPARTY         = 13,
    SNIFF_CHAT_TYPE_MONSTERYELL          = 14,
    SNIFF_CHAT_TYPE_MONSTERWHISPER       = 15,
    SNIFF_CHAT_TYPE_MONSTEREMOTE         = 16,
};

void ReplayMgr::LoadCreatureTextTemplate()
{
    if (auto result = SniffDatabase.Query("SELECT `CreatureID`, `GroupID`, `Text`, `Type`, `Comment` FROM `creature_text_template`"))
    {
        do
        {
            Field* fields = result->Fetch();

            CreatureText textEntry;
            textEntry.creatureId = fields[0].GetUInt32();
            textEntry.groupId = fields[1].GetUInt32();
            textEntry.text = fields[2].GetCppString();
            textEntry.chatType = fields[3].GetUInt32();
            textEntry.comment = fields[4].GetCppString();

            m_creatureTextTemplates.emplace_back(std::move(textEntry));
        } while (result->NextRow());
        delete result;
    }
}

void ReplayMgr::LoadCreatureText()
{
    if (auto result = SniffDatabase.Query("SELECT `UnixTime`, `CreatureGuid`, `CreatureID`, `GroupID` FROM `creature_text` ORDER BY `UnixTime`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 unixtime = fields[0].GetUInt32();
            uint32 creatureGuid = fields[1].GetUInt32();
            uint32 creatureId = fields[2].GetUInt32();
            uint32 groupId = fields[3].GetUInt32();

            CreatureText const* textEntry = GetCreatureTextTemplate(creatureId, groupId);
            std::string text = textEntry ? textEntry->text : "<error>";
            uint32 chatType = textEntry ? textEntry->chatType : 0;
            std::string comment = textEntry ? textEntry->comment : "<error>";

            std::shared_ptr<SniffedEvent_CreatureText> newEvent = std::make_shared<SniffedEvent_CreatureText>(creatureGuid, creatureId, text, chatType, comment);
            m_eventsMap.insert(std::make_pair(unixtime, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

void SniffedEvent_CreatureText::Execute() const
{
    Creature* pCreature = sReplayMgr.GetCreature(m_guid);
    if (!pCreature)
    {
        sLog.outError("SniffedEvent_CreatureText: Cannot find source creature!");
        return;
    }

    switch (m_chatType)
    {
        case SNIFF_CHAT_TYPE_MONSTERYELL:
            pCreature->MonsterYell(m_text.c_str());
            break;
        case SNIFF_CHAT_TYPE_MONSTERWHISPER:
            for (const auto& itr : pCreature->GetMap()->GetPlayers())
                pCreature->MonsterWhisper(m_text.c_str(), itr.getSource());
            break;
        case SNIFF_CHAT_TYPE_MONSTEREMOTE:
            pCreature->MonsterTextEmote(m_text.c_str());
            break;
        case SNIFF_CHAT_TYPE_MONSTERSAY:
        case SNIFF_CHAT_TYPE_MONSTERPARTY:
        default:
            pCreature->MonsterSay(m_text.c_str());
            break;
    }
}

void ReplayMgr::LoadCreatureEmote()
{
    if (auto result = SniffDatabase.Query("SELECT `unixtime`, `emote_id`, `guid` FROM `creature_emote` ORDER BY `unixtime`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 unixtime = fields[0].GetUInt32();
            uint32 emoteId = fields[1].GetUInt32();
            uint32 guid = fields[2].GetUInt32();
            uint32 creatureId = GetCreatureEntryFromGuid(guid);

            std::shared_ptr<SniffedEvent_CreatureEmote> newEvent = std::make_shared<SniffedEvent_CreatureEmote>(guid, creatureId, emoteId);
            m_eventsMap.insert(std::make_pair(unixtime, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

void SniffedEvent_CreatureEmote::Execute() const
{
    Creature* pCreature = sReplayMgr.GetCreature(m_guid);
    if (!pCreature)
    {
        sLog.outError("SniffedEvent_CreatureEmote: Cannot find source creature!");
        return;
    }

    pCreature->HandleEmote(m_emoteId);
}

template void ReplayMgr::LoadCreatureTargetChange<SniffedEvent_CreatureTargetChange>(char const* tableName);
template void ReplayMgr::LoadCreatureTargetChange<SniffedEvent_CreatureAttackStart>(char const* tableName);
template void ReplayMgr::LoadCreatureTargetChange<SniffedEvent_CreatureAttackStop>(char const* tableName);

template <class T>
void ReplayMgr::LoadCreatureTargetChange(char const* tableName)
{
    if (auto result = SniffDatabase.PQuery("SELECT `unixtime`, `victim_guid`, `victim_id`, `victim_type`, `guid` FROM `%s` ORDER BY `unixtime`", tableName))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 unixtime = fields[0].GetUInt32();
            uint32 victimGuid = fields[1].GetUInt32();
            uint32 victimId = fields[2].GetUInt32();
            std::string victimType = fields[3].GetCppString();
            uint32 guid = fields[4].GetUInt32();
            uint32 creatureId = GetCreatureEntryFromGuid(guid);

            std::shared_ptr<T> newEvent = std::make_shared<T>(guid, creatureId, victimGuid, victimId, GetKnownObjectTypeId(victimType));
            m_eventsMap.insert(std::make_pair(unixtime, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

void SniffedEvent_CreatureTargetChange::Execute() const
{
    Creature* pCreature = sReplayMgr.GetCreature(m_guid);
    if (!pCreature)
    {
        sLog.outError("SniffedEvent_CreatureTargetChange: Cannot find source creature!");
        return;
    }

    if (GetTargetObject().IsEmpty())
        pCreature->ClearTarget();
    else if (Unit* pVictim = ToUnit(sReplayMgr.GetStoredObject(GetTargetObject())))
        pCreature->SetTargetGuid(pVictim->GetObjectGuid());
}

void SniffedEvent_CreatureAttackStart::Execute() const
{
    Creature* pCreature = sReplayMgr.GetCreature(m_guid);
    if (!pCreature)
    {
        sLog.outError("SniffedEvent_CreatureAttackStart: Cannot find source creature!");
        return;
    }

    if (Unit* pVictim = ToUnit(sReplayMgr.GetStoredObject(GetTargetObject())))
        pCreature->SendMeleeAttackStart(pVictim);
}

void SniffedEvent_CreatureAttackStop::Execute() const
{
    Creature* pCreature = sReplayMgr.GetCreature(m_guid);
    if (!pCreature)
    {
        sLog.outError("SniffedEvent_CreatureAttackStop: Cannot find source creature!");
        return;
    }

    if (Unit* pVictim = ToUnit(sReplayMgr.GetStoredObject(GetTargetObject())))
        pCreature->SendMeleeAttackStop(pVictim);
}

template void ReplayMgr::LoadCreatureUpdate<SniffedEvent_CreatureUpdate_max_health>(char const* fieldName);
template void ReplayMgr::LoadCreatureUpdate<SniffedEvent_CreatureUpdate_current_health>(char const* fieldName);
template void ReplayMgr::LoadCreatureUpdate<SniffedEvent_CreatureUpdate_unit_flags>(char const* fieldName);
template void ReplayMgr::LoadCreatureUpdate<SniffedEvent_CreatureUpdate_npc_flags>(char const* fieldName);
template void ReplayMgr::LoadCreatureUpdate<SniffedEvent_CreatureUpdate_stand_state>(char const* fieldName);
template void ReplayMgr::LoadCreatureUpdate<SniffedEvent_CreatureUpdate_emote_state>(char const* fieldName);
template void ReplayMgr::LoadCreatureUpdate<SniffedEvent_CreatureUpdate_faction>(char const* fieldName);
template void ReplayMgr::LoadCreatureUpdate<SniffedEvent_CreatureUpdate_mount>(char const* fieldName);
template void ReplayMgr::LoadCreatureUpdate<SniffedEvent_CreatureUpdate_display_id>(char const* fieldName);
template void ReplayMgr::LoadCreatureUpdate<SniffedEvent_CreatureUpdate_entry>(char const* fieldName);

template <class T>
void ReplayMgr::LoadCreatureUpdate(char const* fieldName)
{
    if (auto result = SniffDatabase.PQuery("SELECT `guid`, `unixtime`, `%s` FROM `creature_update` WHERE (`%s` IS NOT NULL) ORDER BY `unixtime`", fieldName, fieldName))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 guid = fields[0].GetUInt32();;
            uint32 creatureId = GetCreatureEntryFromGuid(guid);
            uint32 unixtime = fields[1].GetUInt32();
            uint32 value = fields[2].GetUInt32();

            std::shared_ptr<T> newEvent = std::make_shared<T>(guid, creatureId, value);
            m_eventsMap.insert(std::make_pair(unixtime, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

void SniffedEvent_CreatureUpdate_entry::Execute() const
{
    Creature* pCreature = sReplayMgr.GetCreature(m_guid);
    if (!pCreature)
    {
        sLog.outError("SniffedEvent_CreatureUpdate_entry: Cannot find source creature!");
        return;
    }

    pCreature->SetUInt32Value(OBJECT_FIELD_ENTRY, m_value);
}

void SniffedEvent_CreatureUpdate_display_id::Execute() const
{
    Creature* pCreature = sReplayMgr.GetCreature(m_guid);
    if (!pCreature)
    {
        sLog.outError("SniffedEvent_CreatureUpdate_display_id: Cannot find source creature!");
        return;
    }

    pCreature->SetUInt32Value(UNIT_FIELD_DISPLAYID, m_value);
}

void SniffedEvent_CreatureUpdate_mount::Execute() const
{
    Creature* pCreature = sReplayMgr.GetCreature(m_guid);
    if (!pCreature)
    {
        sLog.outError("SniffedEvent_CreatureUpdate_mount: Cannot find source creature!");
        return;
    }

    pCreature->SetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID, m_value);
}

void SniffedEvent_CreatureUpdate_faction::Execute() const
{
    Creature* pCreature = sReplayMgr.GetCreature(m_guid);
    if (!pCreature)
    {
        sLog.outError("SniffedEvent_CreatureUpdate_faction: Cannot find source creature!");
        return;
    }

    pCreature->SetUInt32Value(UNIT_FIELD_FACTIONTEMPLATE, m_value);
}

void SniffedEvent_CreatureUpdate_emote_state::Execute() const
{
    Creature* pCreature = sReplayMgr.GetCreature(m_guid);
    if (!pCreature)
    {
        sLog.outError("SniffedEvent_CreatureUpdate_emote_state: Cannot find source creature!");
        return;
    }

    pCreature->SetUInt32Value(UNIT_NPC_EMOTESTATE, m_value);
}

void SniffedEvent_CreatureUpdate_stand_state::Execute() const
{
    Creature* pCreature = sReplayMgr.GetCreature(m_guid);
    if (!pCreature)
    {
        sLog.outError("SniffedEvent_CreatureUpdate_stand_state: Cannot find source creature!");
        return;
    }

    pCreature->SetStandState(m_value);
}

void SniffedEvent_CreatureUpdate_npc_flags::Execute() const
{
    Creature* pCreature = sReplayMgr.GetCreature(m_guid);
    if (!pCreature)
    {
        sLog.outError("SniffedEvent_CreatureUpdate_npc_flags: Cannot find source creature!");
        return;
    }

    pCreature->SetUInt32Value(UNIT_NPC_FLAGS, ConvertClassicNpcFlagsToVanilla(m_value));
}

void SniffedEvent_CreatureUpdate_unit_flags::Execute() const
{
    Creature* pCreature = sReplayMgr.GetCreature(m_guid);
    if (!pCreature)
    {
        sLog.outError("SniffedEvent_CreatureUpdate_unit_flags: Cannot find source creature!");
        return;
    }

    pCreature->SetUInt32Value(UNIT_FIELD_FLAGS, m_value);
}

void SniffedEvent_CreatureUpdate_current_health::Execute() const
{
    Creature* pCreature = sReplayMgr.GetCreature(m_guid);
    if (!pCreature)
    {
        sLog.outError("SniffedEvent_CreatureUpdate_current_health: Cannot find source creature!");
        return;
    }

    if (m_value == 0)
        pCreature->DoKillUnit();
    else
        pCreature->SetHealth(m_value);
}

void SniffedEvent_CreatureUpdate_max_health::Execute() const
{
    Creature* pCreature = sReplayMgr.GetCreature(m_guid);
    if (!pCreature)
    {
        sLog.outError("SniffedEvent_CreatureUpdate_max_health: Cannot find source creature!");
        return;
    }

    pCreature->SetMaxHealth(m_value);
}

void ReplayMgr::LoadGameObjectCreate1()
{
    if (auto result = SniffDatabase.Query("SELECT `guid`, `unixtime`, `position_x`, `position_y`, `position_z`, `orientation` FROM `gameobject_create1_time` ORDER BY `unixtime`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 guid = fields[0].GetUInt32();
            uint32 entry = GetGameObjectEntryFromGuid(guid);
            uint32 unixtime = fields[1].GetUInt32();
            float position_x = fields[2].GetFloat();
            float position_y = fields[3].GetFloat();
            float position_z = fields[4].GetFloat();
            float orientation = fields[5].GetFloat();

            std::shared_ptr<SniffedEvent_GameObjectCreate1> newEvent = std::make_shared<SniffedEvent_GameObjectCreate1>(guid, entry, position_x, position_y, position_z, orientation);
            m_eventsMap.insert(std::make_pair(unixtime, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

void SniffedEvent_GameObjectCreate1::Execute() const
{
    GameObject* pGo = sReplayMgr.GetGameObject(m_guid);
    if (!pGo)
    {
        sLog.outError("SniffedEvent_GameObjectCreate1: Cannot find source gameobject!");
        return;
    }
    pGo->Relocate(m_x, m_y, m_z, m_o);
    pGo->SetVisible(true);
}

void ReplayMgr::LoadGameObjectCreate2()
{
    if (auto result = SniffDatabase.Query("SELECT `guid`, `unixtime`, `position_x`, `position_y`, `position_z`, `orientation` FROM `gameobject_create2_time` ORDER BY `unixtime`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 guid = fields[0].GetUInt32();
            uint32 entry = GetGameObjectEntryFromGuid(guid);
            uint32 unixtime = fields[1].GetUInt32();
            float position_x = fields[2].GetFloat();
            float position_y = fields[3].GetFloat();
            float position_z = fields[4].GetFloat();
            float orientation = fields[5].GetFloat();

            std::shared_ptr<SniffedEvent_GameObjectCreate2> newEvent = std::make_shared<SniffedEvent_GameObjectCreate2>(guid, entry, position_x, position_y, position_z, orientation);
            m_eventsMap.insert(std::make_pair(unixtime, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

void SniffedEvent_GameObjectCreate2::Execute() const
{
    GameObject* pGo = sReplayMgr.GetGameObject(m_guid);
    if (!pGo)
    {
        sLog.outError("SniffedEvent_GameObjectCreate2: Cannot find source gameobject!");
        return;
    }
    pGo->Relocate(m_x, m_y, m_z, m_o);
    pGo->SetVisible(true);
}

void ReplayMgr::LoadGameObjectDestroy()
{
    if (auto result = SniffDatabase.Query("SELECT `guid`, `unixtime` FROM `gameobject_destroy_time` ORDER BY `unixtime`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 guid = fields[0].GetUInt32();
            uint32 entry = GetGameObjectEntryFromGuid(guid);
            uint32 unixtime = fields[1].GetUInt32();

            std::shared_ptr<SniffedEvent_GameObjectDestroy> newEvent = std::make_shared<SniffedEvent_GameObjectDestroy>(guid, entry);
            m_eventsMap.insert(std::make_pair(unixtime, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

void SniffedEvent_GameObjectDestroy::Execute() const
{
    GameObject* pGo = sReplayMgr.GetGameObject(m_guid);
    if (!pGo)
    {
        sLog.outError("SniffedEvent_GameObjectDestroy: Cannot find source gameobject!");
        return;
    }
    pGo->SetVisible(false);
}


template void ReplayMgr::LoadGameObjectUpdate<SniffedEvent_GameObjectUpdate_flags>(char const* fieldName);
template void ReplayMgr::LoadGameObjectUpdate<SniffedEvent_GameObjectUpdate_state>(char const* fieldName);

template <class T>
void ReplayMgr::LoadGameObjectUpdate(char const* fieldName)
{
    if (auto result = SniffDatabase.PQuery("SELECT `guid`, `unixtime`, `%s` FROM `gameobject_update` WHERE (`%s` IS NOT NULL) ORDER BY `unixtime`", fieldName, fieldName))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 guid = fields[0].GetUInt32();;
            uint32 entry = GetGameObjectEntryFromGuid(guid);
            uint32 unixtime = fields[1].GetUInt32();
            uint32 value = fields[2].GetUInt32();

            std::shared_ptr<T> newEvent = std::make_shared<T>(guid, entry, value);
            m_eventsMap.insert(std::make_pair(unixtime, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

void SniffedEvent_GameObjectUpdate_flags::Execute() const
{
    GameObject* pGo = sReplayMgr.GetGameObject(m_guid);
    if (!pGo)
    {
        sLog.outError("SniffedEvent_GameObjectUpdate_flags: Cannot find source gameobject!");
        return;
    }
    pGo->SetUInt32Value(GAMEOBJECT_FLAGS, m_value);
}

void SniffedEvent_GameObjectUpdate_state::Execute() const
{
    GameObject* pGo = sReplayMgr.GetGameObject(m_guid);
    if (!pGo)
    {
        sLog.outError("SniffedEvent_GameObjectUpdate_state: Cannot find source gameobject!");
        return;
    }
    pGo->SetGoState(GOState(m_value));
}

void ReplayMgr::LoadSpellCastStart()
{
    if (auto result = SniffDatabase.Query("SELECT `UnixTime`, `CasterGuid`, `CasterId`, `CasterType`, `SpellId`, `CastFlags`, `TargetGuid`, `TargetId`, `TargetType` FROM `spell_cast_start` ORDER BY `UnixTime`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 unixtime = fields[0].GetUInt32();
            uint32 casterGuid = fields[1].GetUInt32();
            uint32 casterId = fields[2].GetUInt32();
            std::string casterType = fields[3].GetCppString();
            uint32 spellId = fields[4].GetUInt32();
            uint32 castFlags = fields[5].GetUInt32();
            uint32 targetGuid = fields[6].GetUInt32();
            uint32 targetId = fields[7].GetUInt32();
            std::string targetType = fields[8].GetCppString();

            if (casterType == "Pet")
                continue;

            std::shared_ptr<SniffedEvent_SpellCastStart> newEvent = std::make_shared<SniffedEvent_SpellCastStart>(spellId, castFlags, casterGuid, casterId, GetKnownObjectTypeId(casterType), targetGuid, targetId, GetKnownObjectTypeId(targetType));
            m_eventsMap.insert(std::make_pair(unixtime, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

void SniffedEvent_SpellCastStart::Execute() const
{
    WorldObject* pCaster = sReplayMgr.GetStoredObject(GetSourceObject());
    if (!pCaster)
    {
        sLog.outError("SniffedEvent_SpellCastStart: Cannot find caster %s!", FormatObjectName(GetSourceObject()).c_str());
        return;
    }
    WorldObject* pTarget = pCaster;
    if (!GetTargetObject().IsEmpty())
    {
        pTarget = sReplayMgr.GetStoredObject(GetTargetObject());
        if (!pTarget)
        {
            sLog.outError("SniffedEvent_SpellCastStart: Cannot find target %s!", FormatObjectName(GetTargetObject()).c_str());
            return;
        }
    }

    WorldPacket data(SMSG_SPELL_START, (8 + 8 + 4 + 2 + 4));
#if SUPPORTED_CLIENT_BUILD > CLIENT_BUILD_1_8_4
    data << pCaster->GetPackGUID();
    data << pCaster->GetPackGUID();
#else
    data << pCaster->GetGUID();
    data << pCaster->GetGUID();
#endif
    data << uint32(m_spellId);                              // spellId
    data << uint16(m_castFlags);                            // cast flags
    data << uint32(1000);                                   // delay?

    SpellCastTargets targets;
    if (Unit* pUnitTarget = pTarget->ToUnit())
        targets.setUnitTarget(pUnitTarget);
    else if (GameObject* pGoTarget = pTarget->ToGameObject())
        targets.setGOTarget(pGoTarget);
    data << targets;

    if (m_castFlags & CAST_FLAG_AMMO)                         // projectile info
        Spell::WriteAmmoToPacket(&data, pCaster, pCaster->ToUnit());

    pCaster->SendObjectMessageToSet(&data, false);
}

void ReplayMgr::LoadSpellCastGo()
{
    if (auto result = SniffDatabase.Query("SELECT `UnixTime`, `CasterGuid`, `CasterId`, `CasterType`, `SpellId`, `MainTargetGuid`, `MainTargetId`, `MainTargetType`, `HitTargetsCount` FROM `spell_cast_go` ORDER BY `UnixTime`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 unixtime = fields[0].GetUInt32();
            uint32 casterGuid = fields[1].GetUInt32();
            uint32 casterId = fields[2].GetUInt32();
            std::string casterType = fields[3].GetCppString();
            uint32 spellId = fields[4].GetUInt32();
            uint32 targetGuid = fields[5].GetUInt32();
            uint32 targetId = fields[6].GetUInt32();
            std::string targetType = fields[7].GetCppString();
            uint32 hitTargetsCount = fields[8].GetUInt32();

            std::shared_ptr<SniffedEvent_SpellCastGo> newEvent = std::make_shared<SniffedEvent_SpellCastGo>(spellId, casterGuid, casterId, GetKnownObjectTypeId(casterType), targetGuid, targetId, GetKnownObjectTypeId(targetType), hitTargetsCount);
            m_eventsMap.insert(std::make_pair(unixtime, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

void SniffedEvent_SpellCastGo::Execute() const
{
    WorldObject* pCaster = sReplayMgr.GetStoredObject(GetSourceObject());
    if (!pCaster)
    {
        sLog.outError("SniffedEvent_SpellCastGo: Cannot find caster %s!", FormatObjectName(GetSourceObject()).c_str());
        return;
    }
    WorldObject* pTarget = pCaster;
    if (!GetTargetObject().IsEmpty())
    {
        pTarget = sReplayMgr.GetStoredObject(GetTargetObject());
        if (!pTarget)
        {
            sLog.outError("SniffedEvent_SpellCastGo: Cannot find target %s!", FormatObjectName(GetTargetObject()).c_str());
            return;
        }
    }

    uint32 castFlags = CAST_FLAG_UNKNOWN9;

    WorldPacket data(SMSG_SPELL_GO, 53);
#if SUPPORTED_CLIENT_BUILD > CLIENT_BUILD_1_8_4
    data << pCaster->GetPackGUID();
    data << pCaster->GetPackGUID();
#else
    data << pCaster->GetGUID();
    data << pCaster->GetGUID();
#endif

    data << uint32(m_spellId);                              // spellId
    data << uint16(castFlags);                              // cast flags

    //WriteSpellGoTargets(&data);

    if (m_hitTargetsCount != 0 && pTarget && !GetTargetObject().IsEmpty())
    {
        data << (uint8)1;
        data << pTarget->GetObjectGuid();
    }
    else
        data << (uint8)0; // hit targets count

    data << (uint8)0; // miss  targets count

    SpellCastTargets targets;
    if (Unit* pUnitTarget = pTarget->ToUnit())
        targets.setUnitTarget(pUnitTarget);
    else if (GameObject* pGoTarget = pTarget->ToGameObject())
        targets.setGOTarget(pGoTarget);
    data << targets;

    if (castFlags & CAST_FLAG_AMMO)                         // projectile info
        Spell::WriteAmmoToPacket(&data, pCaster, pCaster->ToUnit());

    pCaster->SendObjectMessageToSet(&data, false);
}

void ReplayMgr::LoadPlayMusic()
{
    if (auto result = SniffDatabase.Query("SELECT `unixtime`, `music` FROM `play_music` ORDER BY `unixtime`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 unixtime = fields[0].GetUInt32();
            uint32 musicId = fields[0].GetUInt32();

            std::shared_ptr<SniffedEvent_PlayMusic> newEvent = std::make_shared<SniffedEvent_PlayMusic>(musicId);
            m_eventsMap.insert(std::make_pair(unixtime, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

void SniffedEvent_PlayMusic::Execute() const
{
    Player* pPlayer = sReplayMgr.GetActivePlayer();
    if (!pPlayer)
    {
        sLog.outError("SniffedEvent_PlayMusic: Cannot find active player!");
        return;
    }

    pPlayer->PlayDirectMusic(m_music);
}

void ReplayMgr::LoadPlaySound()
{
    if (auto result = SniffDatabase.Query("SELECT `unixtime`, `sound`, `source_guid`, `source_id`, `source_type` FROM `play_sound` ORDER BY `unixtime`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 unixtime = fields[0].GetUInt32();
            uint32 soundId = fields[1].GetUInt32();
            uint32 sourceGuid = fields[2].GetUInt32();
            uint32 sourceId = fields[3].GetUInt32();
            std::string sourceType = fields[4].GetCppString();

            std::shared_ptr<SniffedEvent_PlaySound> newEvent = std::make_shared<SniffedEvent_PlaySound>(soundId, sourceGuid, sourceId, GetKnownObjectTypeId(sourceType));
            m_eventsMap.insert(std::make_pair(unixtime, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

void SniffedEvent_PlaySound::Execute() const
{
    if (!GetSourceObject().IsEmpty())
    {
        WorldObject* pSource = sReplayMgr.GetStoredObject(GetSourceObject());
        if (!pSource)
        {
            sLog.outError("SniffedEvent_PlaySound: Cannot find source object!");
            return;
        }
        pSource->PlayDistanceSound(m_sound);
    }
    else
    {
        Player* pPlayer = sReplayMgr.GetActivePlayer();
        if (!pPlayer)
        {
            sLog.outError("SniffedEvent_PlayMusic: Cannot find active player!");
            return;
        }
        pPlayer->PlayDirectSound(m_sound);
    }
}

void ReplayMgr::LoadQuestAcceptTimes()
{
    if (auto result = SniffDatabase.Query("SELECT `unixtime`, `object_guid`, `object_id`, `object_type`, `quest_id` FROM `quest_client_accept` ORDER BY `unixtime`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 unixtime = fields[0].GetUInt32();
            uint32 objectGuid = fields[1].GetUInt32();
            uint32 objectId = fields[2].GetUInt32();
            std::string objectType = fields[3].GetCppString();
            uint32 questId = fields[4].GetUInt32();

            std::shared_ptr<SniffedEvent_QuestAccept> newEvent = std::make_shared<SniffedEvent_QuestAccept>(questId, objectGuid, objectId, objectType);
            m_eventsMap.insert(std::make_pair(unixtime, newEvent));

        } while (result->NextRow());
    }
}

void SniffedEvent_QuestAccept::Execute() const
{
    WorldObject* pPlayer = sReplayMgr.GetActivePlayer();
    if (!pPlayer)
    {
        sLog.outError("SniffedEvent_QuestAccept: Cannot find active player!");
        return;
    }
    std::string txt = "Client accepts Quest " + sReplayMgr.GetQuestName(m_questId) + " (Entry: " + std::to_string(m_questId) + ") from " + FormatObjectName(KnownObject(m_objectGuid, m_objectId, TypeID(GetKnownObjectTypeId(m_objectType)))) + ".";
    pPlayer->MonsterSay(txt.c_str());
}

void ReplayMgr::LoadQuestCompleteTimes()
{
    if (auto result = SniffDatabase.Query("SELECT `unixtime`, `object_guid`, `object_id`, `object_type`, `quest_id` FROM `quest_client_complete` ORDER BY `unixtime`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 unixtime = fields[0].GetUInt32();
            uint32 objectGuid = fields[1].GetUInt32();
            uint32 objectId = fields[2].GetUInt32();
            std::string objectType = fields[3].GetCppString();
            uint32 questId = fields[4].GetUInt32();

            std::shared_ptr<SniffedEvent_QuestComplete> newEvent = std::make_shared<SniffedEvent_QuestComplete>(questId, objectGuid, objectId, objectType);
            m_eventsMap.insert(std::make_pair(unixtime, newEvent));

        } while (result->NextRow());
    }
}

void SniffedEvent_QuestComplete::Execute() const
{
    WorldObject* pPlayer = sReplayMgr.GetActivePlayer();
    if (!pPlayer)
    {
        sLog.outError("SniffedEvent_QuestComplete: Cannot find active player!");
        return;
    }
    std::string txt = "Client turns in Quest " + sReplayMgr.GetQuestName(m_questId) + " (Entry: " + std::to_string(m_questId) + ") to " + FormatObjectName(KnownObject(m_objectGuid, m_objectId, TypeID(GetKnownObjectTypeId(m_objectType)))) + ".";
    pPlayer->MonsterSay(txt.c_str());
}

void ReplayMgr::LoadCreatureInteractTimes()
{
    if (auto result = SniffDatabase.Query("SELECT `unixtime`, `guid` FROM `creature_client_interact` ORDER BY `unixtime`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 unixtime = fields[0].GetUInt32();
            uint32 guid = fields[1].GetUInt32();
            uint32 entry = GetCreatureEntryFromGuid(guid);

            std::shared_ptr<SniffedEvent_CreatureInteract> newEvent = std::make_shared<SniffedEvent_CreatureInteract>(guid, entry);
            m_eventsMap.insert(std::make_pair(unixtime, newEvent));

        } while (result->NextRow());
    }
}

void SniffedEvent_CreatureInteract::Execute() const
{
    WorldObject* pPlayer = sReplayMgr.GetActivePlayer();
    if (!pPlayer)
    {
        sLog.outError("SniffedEvent_CreatureInteract: Cannot find active player!");
        return;
    }
    std::string txt = "Client interacts with Creature " + std::string(sReplayMgr.GetCreatureName(m_entry)) + " (Guid: " + std::to_string(m_guid) + " Entry: " + std::to_string(m_entry) + ").";
    pPlayer->MonsterSay(txt.c_str());
}

void ReplayMgr::LoadGameObjectUseTimes()
{
    if (auto result = SniffDatabase.Query("SELECT `unixtime`, `guid` FROM `gameobject_client_use` ORDER BY `unixtime`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 unixtime = fields[0].GetUInt32();
            uint32 guid = fields[1].GetUInt32();
            uint32 entry = GetGameObjectEntryFromGuid(guid);

            std::shared_ptr<SniffedEvent_GameObjectUse> newEvent = std::make_shared<SniffedEvent_GameObjectUse>(guid, entry);
            m_eventsMap.insert(std::make_pair(unixtime, newEvent));

        } while (result->NextRow());
    }
}

void SniffedEvent_GameObjectUse::Execute() const
{
    WorldObject* pPlayer = sReplayMgr.GetActivePlayer();
    if (!pPlayer)
    {
        sLog.outError("SniffedEvent_GameObjectUse: Cannot find active player!");
        return;
    }
    std::string txt = "Client uses GameObject " + std::string(sReplayMgr.GetGameObjectName(m_entry)) + " (Guid: " + std::to_string(m_guid) + " Entry: " + std::to_string(m_entry) + ").";
    pPlayer->MonsterSay(txt.c_str());
}

void ReplayMgr::LoadItemUseTimes()
{
    if (auto result = SniffDatabase.Query("SELECT `unixtime`, `entry` FROM `item_client_use` ORDER BY `unixtime`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 unixtime = fields[0].GetUInt32();
            uint32 entry = fields[1].GetUInt32();

            std::shared_ptr<SniffedEvent_ItemUse> newEvent = std::make_shared<SniffedEvent_ItemUse>(entry);
            m_eventsMap.insert(std::make_pair(unixtime, newEvent));

        } while (result->NextRow());
    }
}

void SniffedEvent_ItemUse::Execute() const
{
    WorldObject* pPlayer = sReplayMgr.GetActivePlayer();
    if (!pPlayer)
    {
        sLog.outError("SniffedEvent_ItemUse: Cannot find active player!");
        return;
    }
    std::string txt = "Client uses Item " + std::string(sReplayMgr.GetItemName(m_itemId)) + " (Entry: " + std::to_string(m_itemId) + ").";
    pPlayer->MonsterSay(txt.c_str());

}
