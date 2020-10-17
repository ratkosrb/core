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
#include "SpellAuras.h"
#include "WaypointManager.h"

void ReplayMgr::LoadSniffedEvents()
{
    LoadCreatureCreate1();
    LoadCreatureCreate2();
    LoadCreatureDestroy();
    LoadCreatureMovement("creature_movement");
    LoadCreatureMovement("creature_movement_combat");
    LoadCreatureTextTemplate();
    LoadCreatureText();
    LoadCreatureEmote();
    LoadUnitTargetChange<SniffedEvent_UnitTargetChange>("creature_target_change", TYPEID_UNIT);
    LoadUnitTargetChange<SniffedEvent_UnitAttackStart>("creature_attack_start", TYPEID_UNIT);
    LoadUnitTargetChange<SniffedEvent_UnitAttackStop>("creature_attack_stop", TYPEID_UNIT);
    LoadUnitTargetChange<SniffedEvent_UnitTargetChange>("character_target_change", TYPEID_PLAYER);
    LoadUnitTargetChange<SniffedEvent_UnitAttackStart>("character_attack_start", TYPEID_PLAYER);
    LoadUnitTargetChange<SniffedEvent_UnitAttackStop>("character_attack_stop", TYPEID_PLAYER);
    LoadCreatureUpdate<SniffedEvent_UnitUpdate_entry>("entry");
    LoadCreatureUpdate_float<SniffedEvent_UnitUpdate_scale>("scale");
    LoadCreatureUpdate<SniffedEvent_UnitUpdate_display_id>("display_id");
    LoadCreatureUpdate<SniffedEvent_UnitUpdate_mount>("mount");
    LoadCreatureUpdate<SniffedEvent_UnitUpdate_faction>("faction");
    LoadCreatureUpdate<SniffedEvent_UnitUpdate_emote_state>("emote_state");
    LoadCreatureUpdate<SniffedEvent_UnitUpdate_stand_state>("stand_state");
    LoadCreatureUpdate<SniffedEvent_UnitUpdate_npc_flags>("npc_flags");
    LoadCreatureUpdate<SniffedEvent_UnitUpdate_unit_flags>("unit_flags");
    LoadCreatureUpdate<SniffedEvent_UnitUpdate_max_health>("max_health");
    LoadCreatureUpdate<SniffedEvent_UnitUpdate_current_health>("current_health");
    LoadCreatureUpdate<SniffedEvent_UnitUpdate_max_mana>("max_mana");
    LoadCreatureUpdate<SniffedEvent_UnitUpdate_current_mana>("current_mana");
    LoadPlayerUpdate<SniffedEvent_UnitUpdate_entry>("entry");
    LoadPlayerUpdate_float<SniffedEvent_UnitUpdate_scale>("scale");
    LoadPlayerUpdate<SniffedEvent_UnitUpdate_display_id>("display_id");
    LoadPlayerUpdate<SniffedEvent_UnitUpdate_mount>("mount");
    LoadPlayerUpdate<SniffedEvent_UnitUpdate_faction>("faction");
    LoadPlayerUpdate<SniffedEvent_UnitUpdate_emote_state>("emote_state");
    LoadPlayerUpdate<SniffedEvent_UnitUpdate_stand_state>("stand_state");
    LoadPlayerUpdate<SniffedEvent_UnitUpdate_npc_flags>("npc_flags");
    LoadPlayerUpdate<SniffedEvent_UnitUpdate_unit_flags>("unit_flags");
    LoadPlayerUpdate<SniffedEvent_UnitUpdate_max_health>("max_health");
    LoadPlayerUpdate<SniffedEvent_UnitUpdate_current_health>("current_health");
    LoadPlayerUpdate<SniffedEvent_UnitUpdate_max_mana>("max_mana");
    LoadPlayerUpdate<SniffedEvent_UnitUpdate_current_mana>("current_mana");
    LoadGameObjectCreate1();
    LoadGameObjectCreate2();
    LoadGameObjectCustomAnim();
    LoadGameObjectDespawnAnim();
    LoadGameObjectDestroy();
    LoadGameObjectUpdate<SniffedEvent_GameObjectUpdate_flags>("flags");
    LoadGameObjectUpdate<SniffedEvent_GameObjectUpdate_state>("state");
    LoadSpellCastFailed();
    LoadSpellCastStart();
    LoadSpellCastGo();
    LoadSpellCastGoTargets();
    LoadSpellCastGoPositions();
    LoadSpellChannelStart();
    LoadSpellChannelUpdate();
    LoadPlayMusic();
    LoadPlaySound();
    LoadPlaySpellVisualKit();
    LoadWorldText();
    LoadQuestAcceptTimes();
    LoadQuestCompleteTimes();
    LoadCreatureInteractTimes();
    LoadGameObjectUseTimes();
    LoadItemUseTimes();
    LoadReclaimCorpseTimes();
    LoadReleaseSpiritTimes();
    
    sLog.outString(">> Loaded %u sniffed events", (uint32)m_eventsMap.size());
    sLog.outString();
}

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

template void ReplayMgr::LoadUnitTargetChange<SniffedEvent_UnitTargetChange>(char const* tableName, uint32 typeId);
template void ReplayMgr::LoadUnitTargetChange<SniffedEvent_UnitAttackStart>(char const* tableName, uint32 typeId);
template void ReplayMgr::LoadUnitTargetChange<SniffedEvent_UnitAttackStop>(char const* tableName, uint32 typeId);

template <class T>
void ReplayMgr::LoadUnitTargetChange(char const* tableName, uint32 typeId)
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

            std::shared_ptr<T> newEvent = std::make_shared<T>(guid, creatureId, typeId, victimGuid, victimId, GetKnownObjectTypeId(victimType));
            m_eventsMap.insert(std::make_pair(unixtime, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

void SniffedEvent_UnitTargetChange::Execute() const
{
    Unit* pUnit = sReplayMgr.GetUnit(GetSourceObject());
    if (!pUnit)
    {
        sLog.outError("SniffedEvent_UnitTargetChange: Cannot find source unit!");
        return;
    }

    if (GetTargetObject().IsEmpty())
        pUnit->ClearTarget();
    else if (Unit* pVictim = ToUnit(sReplayMgr.GetStoredObject(GetTargetObject())))
        pUnit->SetTargetGuid(pVictim->GetObjectGuid());
}

void SniffedEvent_UnitAttackStart::Execute() const
{
    Unit* pUnit = sReplayMgr.GetUnit(GetSourceObject());
    if (!pUnit)
    {
        sLog.outError("SniffedEvent_UnitAttackStart: Cannot find source unit!");
        return;
    }

    if (Unit* pVictim = ToUnit(sReplayMgr.GetStoredObject(GetTargetObject())))
        pUnit->SendMeleeAttackStart(pVictim);
}

void SniffedEvent_UnitAttackStop::Execute() const
{
    Unit* pUnit = sReplayMgr.GetUnit(GetSourceObject());
    if (!pUnit)
    {
        sLog.outError("SniffedEvent_UnitAttackStop: Cannot find source unit!");
        return;
    }

    if (Unit* pVictim = ToUnit(sReplayMgr.GetStoredObject(GetTargetObject())))
        pUnit->SendMeleeAttackStop(pVictim);
}

template void ReplayMgr::LoadCreatureUpdate<SniffedEvent_UnitUpdate_max_mana>(char const* fieldName);
template void ReplayMgr::LoadCreatureUpdate<SniffedEvent_UnitUpdate_current_mana>(char const* fieldName);
template void ReplayMgr::LoadCreatureUpdate<SniffedEvent_UnitUpdate_max_health>(char const* fieldName);
template void ReplayMgr::LoadCreatureUpdate<SniffedEvent_UnitUpdate_current_health>(char const* fieldName);
template void ReplayMgr::LoadCreatureUpdate<SniffedEvent_UnitUpdate_unit_flags>(char const* fieldName);
template void ReplayMgr::LoadCreatureUpdate<SniffedEvent_UnitUpdate_npc_flags>(char const* fieldName);
template void ReplayMgr::LoadCreatureUpdate<SniffedEvent_UnitUpdate_stand_state>(char const* fieldName);
template void ReplayMgr::LoadCreatureUpdate<SniffedEvent_UnitUpdate_emote_state>(char const* fieldName);
template void ReplayMgr::LoadCreatureUpdate<SniffedEvent_UnitUpdate_faction>(char const* fieldName);
template void ReplayMgr::LoadCreatureUpdate<SniffedEvent_UnitUpdate_mount>(char const* fieldName);
template void ReplayMgr::LoadCreatureUpdate<SniffedEvent_UnitUpdate_display_id>(char const* fieldName);
template void ReplayMgr::LoadCreatureUpdate<SniffedEvent_UnitUpdate_entry>(char const* fieldName);

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

            std::shared_ptr<T> newEvent = std::make_shared<T>(guid, creatureId, TYPEID_UNIT, value);
            m_eventsMap.insert(std::make_pair(unixtime, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

template void ReplayMgr::LoadCreatureUpdate_float<SniffedEvent_UnitUpdate_scale>(char const* fieldName);

template <class T>
void ReplayMgr::LoadCreatureUpdate_float(char const* fieldName)
{
    if (auto result = SniffDatabase.PQuery("SELECT `guid`, `unixtime`, `%s` FROM `creature_update` WHERE (`%s` IS NOT NULL) ORDER BY `unixtime`", fieldName, fieldName))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 guid = fields[0].GetUInt32();;
            uint32 creatureId = GetCreatureEntryFromGuid(guid);
            uint32 unixtime = fields[1].GetUInt32();
            float value = fields[2].GetFloat();

            std::shared_ptr<T> newEvent = std::make_shared<T>(guid, creatureId, TYPEID_UNIT, value);
            m_eventsMap.insert(std::make_pair(unixtime, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

template void ReplayMgr::LoadPlayerUpdate<SniffedEvent_UnitUpdate_max_mana>(char const* fieldName);
template void ReplayMgr::LoadPlayerUpdate<SniffedEvent_UnitUpdate_current_mana>(char const* fieldName);
template void ReplayMgr::LoadPlayerUpdate<SniffedEvent_UnitUpdate_max_health>(char const* fieldName);
template void ReplayMgr::LoadPlayerUpdate<SniffedEvent_UnitUpdate_current_health>(char const* fieldName);
template void ReplayMgr::LoadPlayerUpdate<SniffedEvent_UnitUpdate_unit_flags>(char const* fieldName);
template void ReplayMgr::LoadPlayerUpdate<SniffedEvent_UnitUpdate_npc_flags>(char const* fieldName);
template void ReplayMgr::LoadPlayerUpdate<SniffedEvent_UnitUpdate_stand_state>(char const* fieldName);
template void ReplayMgr::LoadPlayerUpdate<SniffedEvent_UnitUpdate_emote_state>(char const* fieldName);
template void ReplayMgr::LoadPlayerUpdate<SniffedEvent_UnitUpdate_faction>(char const* fieldName);
template void ReplayMgr::LoadPlayerUpdate<SniffedEvent_UnitUpdate_mount>(char const* fieldName);
template void ReplayMgr::LoadPlayerUpdate<SniffedEvent_UnitUpdate_display_id>(char const* fieldName);
template void ReplayMgr::LoadPlayerUpdate<SniffedEvent_UnitUpdate_entry>(char const* fieldName);

template <class T>
void ReplayMgr::LoadPlayerUpdate(char const* fieldName)
{
    if (auto result = SniffDatabase.PQuery("SELECT `guid`, `unixtime`, `%s` FROM `character_update` WHERE (`%s` IS NOT NULL) ORDER BY `unixtime`", fieldName, fieldName))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 guid = fields[0].GetUInt32();;
            uint32 unixtime = fields[1].GetUInt32();
            uint32 value = fields[2].GetUInt32();

            std::shared_ptr<T> newEvent = std::make_shared<T>(guid, 0, TYPEID_PLAYER, value);
            m_eventsMap.insert(std::make_pair(unixtime, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

template void ReplayMgr::LoadPlayerUpdate_float<SniffedEvent_UnitUpdate_scale>(char const* fieldName);

template <class T>
void ReplayMgr::LoadPlayerUpdate_float(char const* fieldName)
{
    if (auto result = SniffDatabase.PQuery("SELECT `guid`, `unixtime`, `%s` FROM `character_update` WHERE (`%s` IS NOT NULL) ORDER BY `unixtime`", fieldName, fieldName))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 guid = fields[0].GetUInt32();;
            uint32 unixtime = fields[1].GetUInt32();
            float value = fields[2].GetFloat();

            std::shared_ptr<T> newEvent = std::make_shared<T>(guid, 0, TYPEID_PLAYER, value);
            m_eventsMap.insert(std::make_pair(unixtime, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

void SniffedEvent_UnitUpdate_entry::Execute() const
{
    Unit* pUnit = sReplayMgr.GetUnit(GetSourceObject());
    if (!pUnit)
    {
        sLog.outError("SniffedEvent_UnitUpdate_entry: Cannot find source unit!");
        return;
    }

    pUnit->SetUInt32Value(OBJECT_FIELD_ENTRY, m_value);
}

void SniffedEvent_UnitUpdate_scale::Execute() const
{
    Unit* pUnit = sReplayMgr.GetUnit(GetSourceObject());
    if (!pUnit)
    {
        sLog.outError("SniffedEvent_UnitUpdate_scale: Cannot find source unit!");
        return;
    }

    pUnit->SetObjectScale(m_value);
}

void SniffedEvent_UnitUpdate_display_id::Execute() const
{
    Unit* pUnit = sReplayMgr.GetUnit(GetSourceObject());
    if (!pUnit)
    {
        sLog.outError("SniffedEvent_UnitUpdate_display_id: Cannot find source unit!");
        return;
    }

    pUnit->SetUInt32Value(UNIT_FIELD_DISPLAYID, m_value);
}

void SniffedEvent_UnitUpdate_mount::Execute() const
{
    Unit* pUnit = sReplayMgr.GetUnit(GetSourceObject());
    if (!pUnit)
    {
        sLog.outError("SniffedEvent_UnitUpdate_mount: Cannot find source unit!");
        return;
    }

    pUnit->SetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID, m_value);
}

void SniffedEvent_UnitUpdate_faction::Execute() const
{
    Unit* pUnit = sReplayMgr.GetUnit(GetSourceObject());
    if (!pUnit)
    {
        sLog.outError("SniffedEvent_UnitUpdate_faction: Cannot find source unit!");
        return;
    }

    pUnit->SetUInt32Value(UNIT_FIELD_FACTIONTEMPLATE, m_value);
}

void SniffedEvent_UnitUpdate_emote_state::Execute() const
{
    Unit* pUnit = sReplayMgr.GetUnit(GetSourceObject());
    if (!pUnit)
    {
        sLog.outError("SniffedEvent_UnitUpdate_emote_state: Cannot find source unit!");
        return;
    }

    pUnit->SetUInt32Value(UNIT_NPC_EMOTESTATE, m_value);
}

void SniffedEvent_UnitUpdate_stand_state::Execute() const
{
    Unit* pUnit = sReplayMgr.GetUnit(GetSourceObject());
    if (!pUnit)
    {
        sLog.outError("SniffedEvent_UnitUpdate_stand_state: Cannot find source unit!");
        return;
    }

    pUnit->SetStandState(m_value);
}

void SniffedEvent_UnitUpdate_npc_flags::Execute() const
{
    Unit* pUnit = sReplayMgr.GetUnit(GetSourceObject());
    if (!pUnit)
    {
        sLog.outError("SniffedEvent_UnitUpdate_npc_flags: Cannot find source unit!");
        return;
    }

    pUnit->SetUInt32Value(UNIT_NPC_FLAGS, ConvertClassicNpcFlagsToVanilla(m_value));
}

void SniffedEvent_UnitUpdate_unit_flags::Execute() const
{
    Unit* pUnit = sReplayMgr.GetUnit(GetSourceObject());
    if (!pUnit)
    {
        sLog.outError("SniffedEvent_UnitUpdate_unit_flags: Cannot find source unit!");
        return;
    }

    pUnit->SetUInt32Value(UNIT_FIELD_FLAGS, m_value);
}

void SniffedEvent_UnitUpdate_current_health::Execute() const
{
    Unit* pUnit = sReplayMgr.GetUnit(GetSourceObject());
    if (!pUnit)
    {
        sLog.outError("SniffedEvent_UnitUpdate_current_health: Cannot find source unit!");
        return;
    }

    if (m_value == 0 && pUnit->IsCreature())
        pUnit->DoKillUnit();
    else
        pUnit->SetHealth(m_value);
}

void SniffedEvent_UnitUpdate_max_health::Execute() const
{
    Unit* pUnit = sReplayMgr.GetUnit(GetSourceObject());
    if (!pUnit)
    {
        sLog.outError("SniffedEvent_UnitUpdate_max_health: Cannot find source unit!");
        return;
    }

    pUnit->SetMaxHealth(m_value);
}

void SniffedEvent_UnitUpdate_current_mana::Execute() const
{
    Unit* pUnit = sReplayMgr.GetUnit(GetSourceObject());
    if (!pUnit)
    {
        sLog.outError("SniffedEvent_UnitUpdate_current_mana: Cannot find source unit!");
        return;
    }

    pUnit->SetPower(POWER_MANA, m_value);
}

void SniffedEvent_UnitUpdate_max_mana::Execute() const
{
    Unit* pUnit = sReplayMgr.GetUnit(GetSourceObject());
    if (!pUnit)
    {
        sLog.outError("SniffedEvent_UnitUpdate_max_mana: Cannot find source unit!");
        return;
    }

    pUnit->SetMaxPower(POWER_MANA, m_value);
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

void ReplayMgr::LoadGameObjectCustomAnim()
{
    if (auto result = SniffDatabase.Query("SELECT `guid`, `anim_id`, `unixtime` FROM `gameobject_custom_anim` ORDER BY `unixtime`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 guid = fields[0].GetUInt32();
            uint32 entry = GetGameObjectEntryFromGuid(guid);
            uint32 animId = fields[1].GetUInt32();

            uint32 unixtime = fields[2].GetUInt32();

            std::shared_ptr<SniffedEvent_GameObjectCustomAnim> newEvent = std::make_shared<SniffedEvent_GameObjectCustomAnim>(guid, entry, animId);
            m_eventsMap.insert(std::make_pair(unixtime, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

void SniffedEvent_GameObjectCustomAnim::Execute() const
{
    GameObject* pGo = sReplayMgr.GetGameObject(m_guid);
    if (!pGo)
    {
        sLog.outError("SniffedEvent_GameObjectCustomAnim: Cannot find source gameobject!");
        return;
    }
    pGo->SendGameObjectCustomAnim(m_animId);
}

void ReplayMgr::LoadGameObjectDespawnAnim()
{
    if (auto result = SniffDatabase.Query("SELECT `guid`, `unixtime` FROM `gameobject_despawn_anim` ORDER BY `unixtime`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 guid = fields[0].GetUInt32();
            uint32 entry = GetGameObjectEntryFromGuid(guid);
            uint32 unixtime = fields[1].GetUInt32();

            std::shared_ptr<SniffedEvent_GameObjectDespawnAnim> newEvent = std::make_shared<SniffedEvent_GameObjectDespawnAnim>(guid, entry);
            m_eventsMap.insert(std::make_pair(unixtime, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

void SniffedEvent_GameObjectDespawnAnim::Execute() const
{
    GameObject* pGo = sReplayMgr.GetGameObject(m_guid);
    if (!pGo)
    {
        sLog.outError("SniffedEvent_GameObjectDespawnAnim: Cannot find source gameobject!");
        return;
    }
    pGo->SendObjectDeSpawnAnim(pGo->GetObjectGuid());
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

void ReplayMgr::LoadSpellCastFailed()
{
    if (auto result = SniffDatabase.Query("SELECT `unixtime`, `caster_guid`, `caster_id`, `caster_type`, `spell_id` FROM `spell_cast_failed` ORDER BY `unixtime`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 unixtime = fields[0].GetUInt32();
            uint32 casterGuid = fields[1].GetUInt32();
            uint32 casterId = fields[2].GetUInt32();
            std::string casterType = fields[3].GetCppString();
            uint32 spellId = fields[4].GetUInt32();

            if (casterType == "Pet")
                continue;

            std::shared_ptr<SniffedEvent_SpellCastFailed> newEvent = std::make_shared<SniffedEvent_SpellCastFailed>(spellId, casterGuid, casterId, GetKnownObjectTypeId(casterType));
            m_eventsMap.insert(std::make_pair(unixtime, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

void SniffedEvent_SpellCastFailed::Execute() const
{
    WorldObject* pCaster = sReplayMgr.GetStoredObject(GetSourceObject());
    if (!pCaster)
    {
        sLog.outError("SniffedEvent_SpellCastFailed: Cannot find caster %s!", FormatObjectName(GetSourceObject()).c_str());
        return;
    }

    WorldPacket data(SMSG_SPELL_FAILED_OTHER, (8 + 4));
    data << pCaster->GetObjectGuid();
    data << m_spellId;
    pCaster->SendObjectMessageToSet(&data, false);
}

void ReplayMgr::LoadSpellCastStart()
{
    if (auto result = SniffDatabase.Query("SELECT `unixtime`, `caster_guid`, `caster_id`, `caster_type`, `spell_id`, `cast_flags`, `target_guid`, `target_id`, `target_type` FROM `spell_cast_start` ORDER BY `unixtime`"))
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
    if (auto result = SniffDatabase.Query("SELECT `unixtime`, `caster_guid`, `caster_id`, `caster_type`, `spell_id`, `main_target_guid`, `main_target_id`, `main_target_type`, `hit_targets_count`, `hit_targets_list_id`, `miss_targets_count`, `miss_targets_list_id`, `src_position_id`, `dst_position_id` FROM `spell_cast_go` ORDER BY `unixtime`"))
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
            uint32 hitTargetsListId = fields[9].GetUInt32();
            uint32 missTargetsCount = fields[10].GetUInt32();
            uint32 missTargetsListId = fields[11].GetUInt32();
            uint32 srcPositionId = fields[12].GetUInt32();
            uint32 dstPositionId = fields[13].GetUInt32();

            std::shared_ptr<SniffedEvent_SpellCastGo> newEvent = std::make_shared<SniffedEvent_SpellCastGo>(spellId, casterGuid, casterId, GetKnownObjectTypeId(casterType), targetGuid, targetId, GetKnownObjectTypeId(targetType), hitTargetsCount, hitTargetsListId, missTargetsCount, missTargetsListId, srcPositionId, dstPositionId);
            m_eventsMap.insert(std::make_pair(unixtime, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

void ReplayMgr::LoadSpellCastGoTargets()
{
    if (auto result = SniffDatabase.Query("SELECT `list_id`, `target_guid`, `target_id`, `target_type` FROM `spell_cast_go_target` ORDER BY `list_id`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 listId = fields[0].GetUInt32();
            uint32 targetGuid = fields[1].GetUInt32();
            uint32 targetId = fields[2].GetUInt32();
            std::string targetType = fields[3].GetCppString();
            
            m_spellCastGoTargets[listId].push_back(KnownObject(targetGuid, targetId, TypeID(GetKnownObjectTypeId(targetType))));

        } while (result->NextRow());
        delete result;
    }
}

void ReplayMgr::LoadSpellCastGoPositions()
{
    if (auto result = SniffDatabase.Query("SELECT `id`, `position_x`, `position_y`, `position_z` FROM `spell_cast_go_position` ORDER BY `id`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 listId = fields[0].GetUInt32();
            float x = fields[1].GetFloat();
            float y = fields[2].GetFloat();
            float z = fields[3].GetFloat();
            m_spellCastGoPositions.insert({ listId , G3D::Vector3(x, y, z) });

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

    if (m_hitTargetsCount != 0 && m_hitTargetsListId != 0)
    {
        uint32 targetsCount = 0;
        size_t hitPos = data.wpos();
        data << (uint8)0; // placeholder
        if (std::vector<KnownObject> const* vTargets = sReplayMgr.GetSpellTargetList(m_hitTargetsListId))
        {
            for (const auto& itr : *vTargets)
            {
                if (WorldObject* pHitTarget = sReplayMgr.GetStoredObject(itr))
                {
                    targetsCount++;
                    data << pHitTarget->GetObjectGuid();

                    /*
                    // uncomment code to fake aura application
                    if (SpellEntry const* pSpellInfo = sSpellMgr.GetSpellEntry(m_spellId))
                    {
                        if (pSpellInfo->HasEffect(SPELL_EFFECT_APPLY_AURA) && !pSpellInfo->IsChanneledSpell())
                        {
                            if (Unit* pUnitTarget = pHitTarget->ToUnit())
                            {
                                if (!pUnitTarget->HasAura(m_spellId))
                                {
                                    Unit* pUnitCaster = pCaster->IsUnit() ? pCaster->ToUnit() : pUnitTarget;
                                    SpellAuraHolder* holder = CreateSpellAuraHolder(pSpellInfo, pUnitTarget, pUnitCaster, pCaster);

                                    for (uint32 i = 0; i < MAX_EFFECT_INDEX; ++i)
                                    {
                                        uint8 eff = pSpellInfo->Effect[i];
                                        if (eff >= TOTAL_SPELL_EFFECTS)
                                            continue;
                                        if (eff == SPELL_EFFECT_APPLY_AURA)
                                        {
                                            Aura* aur = CreateAura(pSpellInfo, SpellEffectIndex(i), nullptr, holder, pUnitTarget, pUnitCaster);
                                            holder->AddAura(aur, SpellEffectIndex(i));
                                        }
                                    }
                                    if (!pUnitTarget->AddSpellAuraHolder(holder))
                                        holder = nullptr;
                                }
                            }
                        }
                    }
                    */
                }
            }
        }
        if (targetsCount)
            data.put<uint8>(hitPos, (uint8)targetsCount);
    }
    else
        data << (uint8)0; // hit targets count

    if (m_missTargetsCount != 0 && m_missTargetsListId != 0)
    {
        uint32 targetsCount = 0;
        size_t missPos = data.wpos();
        data << (uint8)0; // placeholder
        if (std::vector<KnownObject> const* vTargets = sReplayMgr.GetSpellTargetList(m_missTargetsListId))
        {
            for (const auto& itr : *vTargets)
            {
                if (WorldObject* pMissTarget = sReplayMgr.GetStoredObject(itr))
                {
                    targetsCount++;
                    data << pMissTarget->GetObjectGuid();
                    data << (uint8)SPELL_MISS_MISS;
                }
            }
        }
        if (targetsCount)
            data.put<uint8>(missPos, (uint8)targetsCount);
    }
    else
        data << (uint8)0; // miss targets count

    SpellCastTargets targets;

    if (Unit* pUnitTarget = pTarget->ToUnit())
        targets.setUnitTarget(pUnitTarget);
    else if (GameObject* pGoTarget = pTarget->ToGameObject())
        targets.setGOTarget(pGoTarget);

    if (m_srcPositionId)
        if (G3D::Vector3* pPosition = sReplayMgr.GetSpellPosition(m_srcPositionId))
            targets.setSource(pPosition->x, pPosition->y, pPosition->z);

    if (m_dstPositionId)
        if (G3D::Vector3* pPosition = sReplayMgr.GetSpellPosition(m_dstPositionId))
            targets.setDestination(pPosition->x, pPosition->y, pPosition->z);

    data << targets;

    if (castFlags & CAST_FLAG_AMMO)                         // projectile info
        Spell::WriteAmmoToPacket(&data, pCaster, pCaster->ToUnit());

    pCaster->SendObjectMessageToSet(&data, false);
}

void ReplayMgr::LoadSpellChannelStart()
{
    if (auto result = SniffDatabase.Query("SELECT `unixtime`, `caster_guid`, `caster_id`, `caster_type`, `spell_id`, `duration` FROM `spell_channel_start` ORDER BY `unixtime`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 unixtime = fields[0].GetUInt32();
            uint32 casterGuid = fields[1].GetUInt32();
            uint32 casterId = fields[2].GetUInt32();
            std::string casterType = fields[3].GetCppString();
            uint32 spellId = fields[4].GetUInt32();
            int32 duration = fields[5].GetInt32();

            if (casterType == "Pet")
                continue;

            std::shared_ptr<SniffedEvent_SpellChannelStart> newEvent = std::make_shared<SniffedEvent_SpellChannelStart>(spellId, duration, casterGuid, casterId, GetKnownObjectTypeId(casterType));
            m_eventsMap.insert(std::make_pair(unixtime, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

void SniffedEvent_SpellChannelStart::Execute() const
{
    WorldObject* pCaster = sReplayMgr.GetStoredObject(GetSourceObject());
    if (!pCaster)
    {
        sLog.outError("SniffedEvent_SpellChannelStart: Cannot find caster %s!", FormatObjectName(GetSourceObject()).c_str());
        return;
    }

    // In vanilla MSG_CHANNEL_START is only sent to the caster itself.
    if (Unit* pUnitCaster = pCaster->ToUnit())
    {
        pUnitCaster->SetChannelObjectGuid(pUnitCaster->GetTargetGuid());
        pUnitCaster->SetUInt32Value(UNIT_CHANNEL_SPELL, m_spellId);
    }
}

void ReplayMgr::LoadSpellChannelUpdate()
{
    if (auto result = SniffDatabase.Query("SELECT `unixtime`, `caster_guid`, `caster_id`, `caster_type`, `duration` FROM `spell_channel_update` ORDER BY `unixtime`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 unixtime = fields[0].GetUInt32();
            uint32 casterGuid = fields[1].GetUInt32();
            uint32 casterId = fields[2].GetUInt32();
            std::string casterType = fields[3].GetCppString();
            int32 duration = fields[4].GetInt32();

            if (casterType == "Pet")
                continue;

            std::shared_ptr<SniffedEvent_SpellChannelUpdate> newEvent = std::make_shared<SniffedEvent_SpellChannelUpdate>(duration, casterGuid, casterId, GetKnownObjectTypeId(casterType));
            m_eventsMap.insert(std::make_pair(unixtime, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

void SniffedEvent_SpellChannelUpdate::Execute() const
{
    WorldObject* pCaster = sReplayMgr.GetStoredObject(GetSourceObject());
    if (!pCaster)
    {
        sLog.outError("SniffedEvent_SpellChannelUpdate: Cannot find caster %s!", FormatObjectName(GetSourceObject()).c_str());
        return;
    }

    // In vanilla MSG_CHANNEL_UPDATE is only sent to the caster itself.
    if (m_duration == 0)
    {
        if (Unit* pUnitCaster = pCaster->ToUnit())
        {
            pUnitCaster->SetChannelObjectGuid(ObjectGuid());
            pUnitCaster->SetUInt32Value(UNIT_CHANNEL_SPELL, 0);
        }
    }
}

void ReplayMgr::LoadPlayMusic()
{
    if (auto result = SniffDatabase.Query("SELECT `unixtime`, `music` FROM `play_music` ORDER BY `unixtime`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 unixtime = fields[0].GetUInt32();
            uint32 musicId = fields[1].GetUInt32();

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

void ReplayMgr::LoadPlaySpellVisualKit()
{
    if (auto result = SniffDatabase.Query("SELECT `unixtime`, `kit_id`, `caster_guid`, `caster_id`, `caster_type` FROM `play_spell_visual_kit` ORDER BY `unixtime`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 unixtime = fields[0].GetUInt32();
            uint32 kitId = fields[1].GetUInt32();
            uint32 sourceGuid = fields[2].GetUInt32();
            uint32 sourceId = fields[3].GetUInt32();
            std::string sourceType = fields[4].GetCppString();

            std::shared_ptr<SniffedEvent_PlaySpellVisualKit> newEvent = std::make_shared<SniffedEvent_PlaySpellVisualKit>(kitId, sourceGuid, sourceId, GetKnownObjectTypeId(sourceType));
            m_eventsMap.insert(std::make_pair(unixtime, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

void SniffedEvent_PlaySpellVisualKit::Execute() const
{
    Unit* pUnit = ToUnit(sReplayMgr.GetStoredObject(GetSourceObject()));
    if (!pUnit)
    {
        sLog.outError("SniffedEvent_PlaySpellVisualKit: Cannot find source unit!");
        return;
    }

    pUnit->SendPlaySpellVisual(m_kitId);
}

void ReplayMgr::LoadWorldText()
{
    if (auto result = SniffDatabase.Query("SELECT `UnixTime`, `Text` FROM `world_text` ORDER BY `UnixTime`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 unixtime = fields[0].GetUInt32();
            std::string text = fields[1].GetCppString();

            std::shared_ptr<SniffedEvent_WorldText> newEvent = std::make_shared<SniffedEvent_WorldText>(text);
            m_eventsMap.insert(std::make_pair(unixtime, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

void SniffedEvent_WorldText::Execute() const
{
    sWorld.SendGlobalText(m_text.c_str(), nullptr);
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

void ReplayMgr::LoadReclaimCorpseTimes()
{
    if (auto result = SniffDatabase.Query("SELECT `unixtime` FROM `client_reclaim_corpse` ORDER BY `unixtime`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 unixtime = fields[0].GetUInt32();

            std::shared_ptr<SniffedEvent_ReclaimCorpse> newEvent = std::make_shared<SniffedEvent_ReclaimCorpse>();
            m_eventsMap.insert(std::make_pair(unixtime, newEvent));

        } while (result->NextRow());
    }
}

void SniffedEvent_ReclaimCorpse::Execute() const
{
    WorldObject* pPlayer = sReplayMgr.GetActivePlayer();
    if (!pPlayer)
    {
        sLog.outError("SniffedEvent_ReclaimCorpse: Cannot find active player!");
        return;
    }
    pPlayer->MonsterSay("Client reclaims corpse.");
}

void ReplayMgr::LoadReleaseSpiritTimes()
{
    if (auto result = SniffDatabase.Query("SELECT `unixtime` FROM `client_release_spirit` ORDER BY `unixtime`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 unixtime = fields[0].GetUInt32();

            std::shared_ptr<SniffedEvent_ReleaseSpirit> newEvent = std::make_shared<SniffedEvent_ReleaseSpirit>();
            m_eventsMap.insert(std::make_pair(unixtime, newEvent));

        } while (result->NextRow());
    }
}

void SniffedEvent_ReleaseSpirit::Execute() const
{
    WorldObject* pPlayer = sReplayMgr.GetActivePlayer();
    if (!pPlayer)
    {
        sLog.outError("SniffedEvent_ReleaseSpirit: Cannot find active player!");
        return;
    }
    pPlayer->MonsterSay("Client releases spirit.");
}

std::shared_ptr<WaypointPath> ReplayMgr::GetOrCreateWaypoints(uint32 guid, bool useStartPosition)
{
    // Only generate path if we haven't done it before.
    auto existingPathsItr = m_creatureWaypoints.find(guid);
    if (existingPathsItr != m_creatureWaypoints.end())
        return existingPathsItr->second;

    uint32 firstMoveTime = 0;
    std::shared_ptr<WaypointPath> path = std::make_shared<WaypointPath>();
    //                                              0     1        2            3               4               5                   6                   7                   8                 9                 10                11             12
    if (auto result = SniffDatabase.PQuery("SELECT `id`, `point`, `move_time`, `spline_flags`, `spline_count`, `start_position_x`, `start_position_y`, `start_position_z`, `end_position_x`, `end_position_y`, `end_position_z`, `orientation`, `unixtime` FROM `creature_movement` WHERE `id`=%u", guid))
    {
        uint32 pointCounter = 0;
        WaypointNode* lastPoint = nullptr;
        uint32 lastMoveTime = 0;
        uint32 lastUnixTime = 0;
        float lastOrientation = 100.0f;
        do
        {
            Field* fields = result->Fetch();

            uint32 const id = fields[0].GetUInt32();
            uint32 const point = fields[1].GetUInt32();

            uint32 const move_time = fields[2].GetUInt32();
            //uint32 const spline_flags = fields[3].GetUInt32();
            uint32 const spline_count = fields[4].GetUInt32();

            float const start_position_x = fields[5].GetFloat();
            float const start_position_y = fields[6].GetFloat();
            float const start_position_z = fields[7].GetFloat();

            float const end_position_x = fields[8].GetFloat();
            float const end_position_y = fields[9].GetFloat();
            float const end_position_z = fields[10].GetFloat();
            float const final_orientation = fields[11].GetFloat();

            uint32 unixtime = fields[12].GetUInt32();

            if (point == 1)
                firstMoveTime = unixtime;

            uint32 waittime = 0;

            if (lastPoint)
            {
                float fMoveTime = (float)lastMoveTime;
                fMoveTime = fMoveTime / 1000.0f;
                fMoveTime = ceilf(fMoveTime);
                uint32 roundedUpMoveTime = (uint32)fMoveTime;
                uint32 timeDiff = unixtime - lastUnixTime;

                if (timeDiff > (roundedUpMoveTime * 2))
                {
                    if (useStartPosition)
                        waittime = (timeDiff - roundedUpMoveTime) * 1000;
                    else
                        lastPoint->delay = (timeDiff - roundedUpMoveTime) * 1000;
                }
            }

            WaypointNode node;

            if (useStartPosition)
            {
                float orientation = lastOrientation;
                lastOrientation = final_orientation;
                node.x = start_position_x;
                node.y = start_position_y;
                node.z = start_position_z;
                node.orientation = orientation;
                node.delay = waittime;
            }
            else
            {
                float posX = (spline_count == 0) ? start_position_x : end_position_x;
                float posY = (spline_count == 0) ? start_position_y : end_position_y;
                float posZ = (spline_count == 0) ? start_position_z : end_position_z;
                node.x = posX;
                node.y = posY;
                node.z = posZ;
                node.orientation = final_orientation;
                node.delay = waittime;
            }

            auto insertResult = (*path).insert({ pointCounter, node });
            if (insertResult.second)
                lastPoint = &insertResult.first->second;

            pointCounter++;

            lastMoveTime = move_time;
            lastUnixTime = unixtime;

            if (path->size() >= 200)
            {
                sLog.outInfo("[ReplayMgr] Waypoint count for guid %u exceeds 200 points. Cutting it off to prevent client crash.", guid);
                break;
            }

        } while (result->NextRow());
        delete result;
    }

    if (path->empty())
        return nullptr;

    m_creatureWaypoints.insert({ guid, path });
    return path;
}

uint32 ReplayMgr::GetTotalMovementPointsForCreature(uint32 guid)
{
    uint32 count = 0;
    if (auto result = SniffDatabase.PQuery("SELECT COUNT(`point`) FROM `creature_movement` WHERE `id`=%u", guid))
    {
        do
        {
            Field* fields = result->Fetch();
            count = fields[0].GetUInt32();
        } while (result->NextRow());
        delete result;
    }
    return count;
}
