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

#include "ClassicDefines.h"
#include "ReplayMgr.h"
#include "WorldSession.h"
#include "Player.h"
#include "Map.h"
#include "World.h"
#include "WorldPacket.h"
#include "Log.h"
#include "Util.h"
#include "ReplayBotAI.h"
#include "PlayerBotMgr.h"
#include "Chat.h"
#include "Spell.h"
#include "SpellAuras.h"
#include "WaypointManager.h"
#include "MovementPacketSender.h"
#include "MoveSplineInit.h"
#include "MoveSpline.h"
#include "DynamicObject.h"

void ReplayMgr::LoadSniffedEvents()
{
    LoadWeatherUpdates();
    LoadWorldText();
    LoadWorldStateUpdates();
    LoadUnitCreate1("creature_create1_time", TYPEID_UNIT);
    LoadUnitCreate2("creature_create2_time", TYPEID_UNIT);
    LoadUnitDestroy("creature_destroy_time", TYPEID_UNIT);
    LoadUnitCreate1("player_create1_time", TYPEID_PLAYER);
    LoadUnitCreate2("player_create2_time", TYPEID_PLAYER);
    LoadUnitDestroy("player_destroy_time", TYPEID_PLAYER);
    LoadCreatureClientSideMovement();
    LoadServerSideMovementSplines("player_movement_server_spline", m_characterMovementSplines);
    LoadServerSideMovementSplines("creature_movement_server_spline", m_creatureMovementSplines);
    LoadServerSideMovementSplines("creature_movement_server_combat_spline", m_creatureMovementCombatSplines);
    LoadServerSideMovement("player_movement_server", TYPEID_PLAYER, m_characterMovementSplines);
    LoadServerSideMovement("creature_movement_server", TYPEID_UNIT, m_creatureMovementSplines);
    LoadServerSideMovement("creature_movement_server_combat", TYPEID_UNIT, m_creatureMovementCombatSplines);
    LoadCreatureTextTemplate();
    LoadCreatureText();
    LoadCreatureEquipmentUpdate();
    LoadUnitEmote("creature_emote", TYPEID_UNIT);
    LoadUnitEmote("player_emote", TYPEID_PLAYER);
    LoadUnitGuidValuesUpdate("creature_guid_values_update", TYPEID_UNIT);
    LoadUnitGuidValuesUpdate("player_guid_values_update", TYPEID_PLAYER);
    LoadUnitAttackLog("creature_attack_log", TYPEID_UNIT);
    LoadUnitAttackLog("player_attack_log", TYPEID_PLAYER);
    LoadUnitAttackToggle<SniffedEvent_UnitAttackStart>("creature_attack_start", TYPEID_UNIT);
    LoadUnitAttackToggle<SniffedEvent_UnitAttackStop>("creature_attack_stop", TYPEID_UNIT);
    LoadUnitAttackToggle<SniffedEvent_UnitAttackStart>("player_attack_start", TYPEID_PLAYER);
    LoadUnitAttackToggle<SniffedEvent_UnitAttackStop>("player_attack_stop", TYPEID_PLAYER);
    LoadCreatureValuesUpdate<SniffedEvent_UnitUpdate_entry>("entry");
    LoadCreatureValuesUpdate_float<SniffedEvent_UnitUpdate_scale>("scale");
    LoadCreatureValuesUpdate<SniffedEvent_UnitUpdate_display_id>("display_id");
    LoadCreatureValuesUpdate<SniffedEvent_UnitUpdate_mount>("mount_display_id");
    LoadCreatureValuesUpdate<SniffedEvent_UnitUpdate_faction>("faction");
    LoadCreatureValuesUpdate<SniffedEvent_UnitUpdate_level>("level");
    LoadCreatureValuesUpdate<SniffedEvent_UnitUpdate_aura_state>("aura_state");
    LoadCreatureValuesUpdate<SniffedEvent_UnitUpdate_emote_state>("emote_state");
    LoadCreatureValuesUpdate<SniffedEvent_UnitUpdate_stand_state>("stand_state");
    LoadCreatureValuesUpdate<SniffedEvent_UnitUpdate_vis_flags>("vis_flags");
    LoadCreatureValuesUpdate<SniffedEvent_UnitUpdate_sheath_state>("sheath_state");
    LoadCreatureValuesUpdate<SniffedEvent_UnitUpdate_shapeshift_form>("shapeshift_form");
    LoadCreatureValuesUpdate<SniffedEvent_UnitUpdate_npc_flags>("npc_flags");
    LoadCreatureValuesUpdate<SniffedEvent_UnitUpdate_unit_flags>("unit_flags");
    LoadCreatureValuesUpdate<SniffedEvent_UnitUpdate_max_health>("max_health");
    LoadCreatureValuesUpdate<SniffedEvent_UnitUpdate_current_health>("current_health");
    LoadCreatureValuesUpdate<SniffedEvent_UnitUpdate_max_mana>("max_mana");
    LoadCreatureValuesUpdate<SniffedEvent_UnitUpdate_current_mana>("current_mana");
    LoadCreatureValuesUpdate_float<SniffedEvent_UnitUpdate_bounding_radius>("bounding_radius");
    LoadCreatureValuesUpdate_float<SniffedEvent_UnitUpdate_combat_reach>("combat_reach");
    LoadCreatureValuesUpdate<SniffedEvent_UnitUpdate_base_attack_time>("base_attack_time");
    LoadCreatureValuesUpdate<SniffedEvent_UnitUpdate_ranged_attack_time>("ranged_attack_time");
    LoadPlayerValuesUpdate<SniffedEvent_UnitUpdate_entry>("entry");
    LoadPlayerValuesUpdate_float<SniffedEvent_UnitUpdate_scale>("scale");
    LoadPlayerValuesUpdate<SniffedEvent_UnitUpdate_display_id>("display_id");
    LoadPlayerValuesUpdate<SniffedEvent_UnitUpdate_mount>("mount_display_id");
    LoadPlayerValuesUpdate<SniffedEvent_UnitUpdate_faction>("faction");
    LoadPlayerValuesUpdate<SniffedEvent_UnitUpdate_level>("level");
    LoadPlayerValuesUpdate<SniffedEvent_UnitUpdate_aura_state>("aura_state");
    LoadPlayerValuesUpdate<SniffedEvent_UnitUpdate_emote_state>("emote_state");
    LoadPlayerValuesUpdate<SniffedEvent_UnitUpdate_stand_state>("stand_state");
    LoadPlayerValuesUpdate<SniffedEvent_UnitUpdate_vis_flags>("vis_flags");
    LoadPlayerValuesUpdate<SniffedEvent_UnitUpdate_sheath_state>("sheath_state");
    LoadPlayerValuesUpdate<SniffedEvent_UnitUpdate_shapeshift_form>("shapeshift_form");
    LoadPlayerValuesUpdate<SniffedEvent_UnitUpdate_npc_flags>("npc_flags");
    LoadPlayerValuesUpdate<SniffedEvent_UnitUpdate_unit_flags>("unit_flags");
    LoadPlayerValuesUpdate<SniffedEvent_UnitUpdate_max_health>("max_health");
    LoadPlayerValuesUpdate<SniffedEvent_UnitUpdate_current_health>("current_health");
    LoadPlayerValuesUpdate<SniffedEvent_UnitUpdate_max_mana>("max_mana");
    LoadPlayerValuesUpdate<SniffedEvent_UnitUpdate_current_mana>("current_mana");
    LoadPlayerValuesUpdate_float<SniffedEvent_UnitUpdate_bounding_radius>("bounding_radius");
    LoadPlayerValuesUpdate_float<SniffedEvent_UnitUpdate_combat_reach>("combat_reach");
    LoadPlayerValuesUpdate<SniffedEvent_UnitUpdate_base_attack_time>("base_attack_time");
    LoadPlayerValuesUpdate<SniffedEvent_UnitUpdate_ranged_attack_time>("ranged_attack_time");
    LoadCreatureSpeedUpdate(MOVE_WALK);
    LoadCreatureSpeedUpdate(MOVE_RUN);
    LoadCreatureSpeedUpdate(MOVE_SWIM);
    LoadPlayerSpeedUpdate(MOVE_WALK);
    LoadPlayerSpeedUpdate(MOVE_RUN);
    LoadPlayerSpeedUpdate(MOVE_SWIM);
    LoadUnitAurasUpdate("creature_auras_update", TYPEID_UNIT);
    LoadUnitAurasUpdate("player_auras_update", TYPEID_PLAYER);
    LoadDynamicObjectCreate();
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
    LoadPlayerChat();
    LoadPlayerEquipmentUpdate();
    LoadPlayMusic();
    LoadPlaySound();
    LoadPlaySpellVisualKit();
    LoadQuestAcceptTimes();
    LoadQuestCompleteTimes();
    LoadCreatureInteractTimes();
    LoadGameObjectUseTimes();
    LoadItemUseTimes();
    LoadReclaimCorpseTimes();
    LoadReleaseSpiritTimes();
    LoadQuestUpdateComplete();
    LoadQuestUpdateFailed();

    sLog.outString(">> Loaded %u sniffed events", (uint32)m_eventsMap.size());
    sLog.outString();
}

void ReplayMgr::LoadWeatherUpdates()
{
    if (auto result = SniffDatabase.Query("SELECT `map_id`, `zone_id`, `weather_state`, `grade`, `unixtimems` FROM `weather_update` ORDER BY `unixtimems`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 mapId = fields[0].GetUInt32();
            uint32 zoneId = fields[1].GetUInt32();
            uint32 weatherState = fields[2].GetUInt32();
            float grade = fields[3].GetFloat();
            uint64 unixtimems = fields[4].GetUInt64();

            if (grade > 1.0f)
                grade = 1.0f;

            switch (weatherState)
            {
                case WEATHER_STATE_FINE:
                    weatherState = WEATHER_TYPE_FINE;
                    grade = 0.0f;
                    break;
                case WEATHER_STATE_FOG:
                    weatherState = WEATHER_TYPE_FINE;
                    break;
                case WEATHER_STATE_DRIZZLE:
                    weatherState = WEATHER_TYPE_RAIN;
                    grade = 0.25f;
                    break;
                case WEATHER_STATE_LIGHT_RAIN:
                    weatherState = WEATHER_TYPE_RAIN;
                    grade = 0.3f;
                    break;
                case WEATHER_STATE_MEDIUM_RAIN:
                    weatherState = WEATHER_TYPE_RAIN;
                    grade = 0.6f;
                    break;
                case WEATHER_STATE_HEAVY_RAIN:
                    weatherState = WEATHER_TYPE_RAIN;
                    grade = 1.0f;
                    break;
                case WEATHER_STATE_LIGHT_SNOW:
                    weatherState = WEATHER_TYPE_SNOW;
                    grade = 0.3f;
                    break;
                case WEATHER_STATE_MEDIUM_SNOW:
                    weatherState = WEATHER_TYPE_SNOW;
                    grade = 0.6f;
                    break;
                case WEATHER_STATE_HEAVY_SNOW:
                    weatherState = WEATHER_TYPE_SNOW;
                    grade = 1.0f;
                    break;
                case WEATHER_STATE_LIGHT_SANDSTORM:
                    weatherState = WEATHER_TYPE_STORM;
                    grade = 0.3f;
                    break;
                case WEATHER_STATE_MEDIUM_SANDSTORM:
                    weatherState = WEATHER_TYPE_STORM;
                    grade = 0.6f;
                    break;
                case WEATHER_STATE_HEAVY_SANDSTORM:
                    weatherState = WEATHER_TYPE_STORM;
                    grade = 1.0f;
                    break;
                case WEATHER_STATE_THUNDERS:
                    weatherState = WEATHER_TYPE_STORM;
                    grade = 0.25f;
                    break;
                default:
                    continue;
            }

            std::shared_ptr<SniffedEvent_WeatherUpdate> newEvent = std::make_shared<SniffedEvent_WeatherUpdate>(mapId, zoneId, weatherState, grade);
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

void SniffedEvent_WeatherUpdate::Execute() const
{
    Map* pMap = sMapMgr.FindMap(m_mapId, 0);
    if (!pMap)
    {
        sLog.outError("SniffedEvent_WeatherUpdate: Cannot find map %u!", m_mapId);
        return;
    }

    pMap->SetWeather(m_zoneId, WeatherType(m_weatherType), m_grade, true);
}

void ReplayMgr::LoadWorldText()
{
    if (auto result = SniffDatabase.Query("SELECT `unixtimems`, `text` FROM `world_text` ORDER BY `unixtimems`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint64 unixtimems = fields[0].GetUInt64();
            std::string text = fields[1].GetCppString();

            std::shared_ptr<SniffedEvent_WorldText> newEvent = std::make_shared<SniffedEvent_WorldText>(text);
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

void SniffedEvent_WorldText::Execute() const
{
    sWorld.SendGlobalText(m_text.c_str(), nullptr);
}

void ReplayMgr::LoadWorldStateUpdates()
{
    if (auto result = SniffDatabase.Query("SELECT `unixtimems`, `variable`, `value` FROM `world_state_update` ORDER BY `unixtimems`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint64 unixtimems = fields[0].GetUInt64();
            uint32 variable = fields[1].GetUInt32();
            uint32 value = fields[2].GetUInt32();

            std::shared_ptr<SniffedEvent_WorldStateUpdate> newEvent = std::make_shared<SniffedEvent_WorldStateUpdate>(variable, value, false);
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

        } while (result->NextRow());
    }
}

extern std::map<uint32, uint32> g_defaultWorldStates;

void SniffedEvent_WorldStateUpdate::Execute() const
{
    g_defaultWorldStates[m_variable] = m_value;

    if (m_isInit)
        return;

    Player* pActivePlayer = sReplayMgr.GetActivePlayer();
    if (!pActivePlayer)
    {
        sLog.outError("SniffedEvent_WorldStateUpdate: Cannot find active player!");
        return;
    }

    if (!pActivePlayer->IsInWorld())
    {
        sLog.outError("SniffedEvent_WorldStateUpdate: Active player is not in world!");
        return;
    }

    for (const auto& itr : pActivePlayer->GetMap()->GetPlayers())
    {
        if (Player* pPlayer = itr.getSource())
        {
            if (!pPlayer->GetSession()->GetBot())
            {
                pPlayer->SendUpdateWorldState(m_variable, m_value);
            }
        }
    };
}

void ReplayMgr::LoadUnitCreate1(char const* tableName, TypeID typeId)
{
    if (auto result = SniffDatabase.PQuery("SELECT `guid`, `unixtimems`, `position_x`, `position_y`, `position_z`, `orientation` FROM `%s` ORDER BY `unixtimems`", tableName))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 guid = fields[0].GetUInt32();
            uint32 creatureId = typeId == TYPEID_UNIT ? GetCreatureEntryFromGuid(guid) : 0;
            uint64 unixtimems = fields[1].GetUInt64();
            float position_x = fields[2].GetFloat();
            float position_y = fields[3].GetFloat();
            float position_z = fields[4].GetFloat();
            float orientation = fields[5].GetFloat();

            std::shared_ptr<SniffedEvent_UnitCreate1> newEvent = std::make_shared<SniffedEvent_UnitCreate1>(guid, creatureId, typeId, position_x, position_y, position_z, orientation);
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

void SniffedEvent_UnitCreate1::Execute() const
{
    Unit* pUnit = sReplayMgr.GetUnit(GetSourceObject());
    if (!pUnit)
    {
        sLog.outError("SniffedEvent_UnitCreate1: Cannot find source unit!");
        return;
    }
    if (pUnit->IsInWorld() && (pUnit->GetDistance(m_x, m_y, m_z) > 1.0f))
        pUnit->NearTeleportTo(m_x, m_y, m_z, m_o);
    pUnit->SetVisibility(VISIBILITY_ON);
}

void ReplayMgr::LoadUnitCreate2(char const* tableName, TypeID typeId)
{
    if (auto result = SniffDatabase.PQuery("SELECT `guid`, `unixtimems`, `position_x`, `position_y`, `position_z`, `orientation` FROM `%s` ORDER BY `unixtimems`", tableName))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 guid = fields[0].GetUInt32();
            uint32 creatureId = typeId == TYPEID_UNIT ? GetCreatureEntryFromGuid(guid) : 0;
            uint64 unixtimems = fields[1].GetUInt64();
            float position_x = fields[2].GetFloat();
            float position_y = fields[3].GetFloat();
            float position_z = fields[4].GetFloat();
            float orientation = fields[5].GetFloat();

            std::shared_ptr<SniffedEvent_UnitCreate2> newEvent = std::make_shared<SniffedEvent_UnitCreate2>(guid, creatureId, typeId, position_x, position_y, position_z, orientation);
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

void SniffedEvent_UnitCreate2::Execute() const
{
    Unit* pUnit = sReplayMgr.GetUnit(GetSourceObject());
    if (!pUnit)
    {
        sLog.outError("SniffedEvent_UnitCreate2: Cannot find source unit!");
        return;
    }
    if (pUnit->IsInWorld() && (pUnit->GetDistance(m_x, m_y, m_z) > 1.0f))
        pUnit->NearTeleportTo(m_x, m_y, m_z, m_o);
    pUnit->SetVisibility(VISIBILITY_ON);
}

void ReplayMgr::LoadUnitDestroy(char const* tableName, TypeID typeId)
{
    if (auto result = SniffDatabase.PQuery("SELECT `guid`, `unixtimems` FROM `%s` ORDER BY `unixtimems`", tableName))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 guid = fields[0].GetUInt32();
            uint32 creatureId = typeId == TYPEID_UNIT ? GetCreatureEntryFromGuid(guid) : 0;
            uint64 unixtimems = fields[1].GetUInt64();

            std::shared_ptr<SniffedEvent_UnitDestroy> newEvent = std::make_shared<SniffedEvent_UnitDestroy>(guid, creatureId, typeId);
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

void SniffedEvent_UnitDestroy::Execute() const
{
    Unit* pUnit = sReplayMgr.GetUnit(GetSourceObject());
    if (!pUnit)
    {
        sLog.outError("SniffedEvent_UnitDestroy: Cannot find source unit!");
        return;
    }
    pUnit->SetVisibility(VISIBILITY_OFF);
}

void ReplayMgr::LoadServerSideMovement(char const* tableName, TypeID typeId, SplinesMap const& splinesMap)
{
    if (auto result = SniffDatabase.PQuery("SELECT `guid`, `point`, `move_time`, `spline_flags`, `spline_count`, `start_position_x`, `start_position_y`, `start_position_z`, `end_position_x`, `end_position_y`, `end_position_z`, `orientation`, `unixtime` FROM `%s` ORDER BY `unixtime`", tableName))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 guid = fields[0].GetUInt32();
            uint32 point = fields[1].GetUInt32();
            uint32 creatureId = typeId == TYPEID_UNIT ? GetCreatureEntryFromGuid(guid) : 0;
            uint32 moveTime = fields[2].GetUInt32();
            uint32 splineFlags = fields[3].GetUInt32();
            uint32 spline_count = fields[4].GetUInt32();
            float startX = fields[5].GetFloat();
            float startY = fields[6].GetFloat();
            float startZ = fields[7].GetFloat();
            float endX = fields[8].GetFloat();
            float endY = fields[9].GetFloat();
            float endZ = fields[10].GetFloat();
            float orientation = fields[11].GetFloat();
            uint64 unixtime = fields[12].GetUInt32();

            if (spline_count == 0 && orientation != 100)
            {
                std::shared_ptr<SniffedEvent_UnitUpdate_orientation> newEvent = std::make_shared<SniffedEvent_UnitUpdate_orientation>(guid, creatureId, typeId, orientation);
                m_eventsMap.insert(std::make_pair(unixtime * IN_MILLISECONDS, newEvent));
            }
            else
            {
                std::vector<G3D::Vector3> const* pSplines = nullptr;
                if (spline_count > 1)
                {
                    auto itr1 = splinesMap.find(guid);
                    if (itr1 != splinesMap.end())
                    {
                        auto itr2 = itr1->second.find(point);
                        if (itr2 != itr1->second.end())
                            pSplines = &itr2->second;
                    }
                }

                float x = spline_count ? endX : startX;
                float y = spline_count ? endY : startY;
                float z = spline_count ? endZ : startZ;
                std::shared_ptr<SniffedEvent_ServerSideMovement> newEvent = std::make_shared<SniffedEvent_ServerSideMovement>(guid, creatureId, typeId, moveTime, splineFlags, x, y, z, orientation, pSplines);
                m_eventsMap.insert(std::make_pair(unixtime * IN_MILLISECONDS, newEvent));
            }
        } while (result->NextRow());
        delete result;
    }
}

void ReplayMgr::LoadServerSideMovementSplines(char const* tableName, SplinesMap& splinesMap)
{
    if (auto result = SniffDatabase.PQuery("SELECT `guid`, `parent_point`, `spline_point`, `position_x`, `position_y`, `position_z` FROM `%s` ORDER BY `guid`, `parent_point`, `spline_point`", tableName))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 guid = fields[0].GetUInt32();
            uint32 parent_point = fields[1].GetUInt32();
            //uint32 spline_point = fields[2].GetUInt32();
            G3D::Vector3 position;
            position.x = fields[3].GetFloat();
            position.y = fields[4].GetFloat();
            position.z = fields[5].GetFloat();

            splinesMap[guid][parent_point].push_back(position);
        } while (result->NextRow());
        delete result;
    }
}

void SniffedEvent_ServerSideMovement::Execute() const
{
    Unit* pUnit = sReplayMgr.GetUnit(GetSourceObject());
    if (!pUnit)
    {
        sLog.outError("SniffedEvent_ServerSideMovement: Cannot find source unit!");
        return;
    }

    if (Player* pPlayer = pUnit->ToPlayer())
        if (pPlayer->IsBeingTeleported())
            return;

    if (m_moveTime == 0)
    {
        if (pUnit->GetDistance(m_x, m_y, m_z) > 1.0f)
        {
            if (Creature* pCreature = pUnit->ToCreature())
                pCreature->GetMap()->CreatureRelocation(pCreature, m_x, m_y, m_z, pCreature->GetOrientation());
            else if (Player* pPlayer = pUnit->ToPlayer())
                pPlayer->GetMap()->PlayerRelocation(pPlayer, m_x, m_y, m_z, pPlayer->GetOrientation());
        }
        else
            return;
    }

    if (m_splineFlags & uint32(ClassicSplineFlag::Flying))
        pUnit->AddUnitMovementFlag(MOVEFLAG_FLYING);
    else if (m_splineFlags != 0)
        pUnit->RemoveUnitMovementFlag(MOVEFLAG_FLYING);

    Movement::MoveSplineInit init(*pUnit, "MovementReplay");
    if (m_splines)
        init.MovebyPath(*m_splines);
    else
        init.MoveTo(m_x, m_y, m_z, MOVE_FORCE_DESTINATION);
    
    float speed = m_moveTime != 0 ? pUnit->GetDistance(m_x, m_y, m_z) / ((float)m_moveTime * 0.001f) : 0.0f;
    if (speed > 0.0f)
        init.SetVelocity(speed);
    if (m_o != 100)
        init.SetFacing(m_o);
    init.Launch();
}

void SniffedEvent_UnitUpdate_orientation::Execute() const
{
    Unit* pUnit = sReplayMgr.GetUnit(GetSourceObject());
    if (!pUnit)
    {
        sLog.outError("SniffedEvent_UnitUpdate_orientation: Cannot find source unit!");
        return;
    }
    pUnit->SetOrientation(m_o);
}

void ReplayMgr::LoadCreatureTextTemplate()
{
    if (auto result = SniffDatabase.Query("SELECT `creature_id`, `group_id`, `text`, `chat_type` FROM `creature_text_template`"))
    {
        do
        {
            Field* fields = result->Fetch();

            CreatureText textEntry;
            textEntry.creatureId = fields[0].GetUInt32();
            textEntry.groupId = fields[1].GetUInt32();
            textEntry.text = fields[2].GetCppString();
            textEntry.chatType = fields[3].GetUInt32();

            m_creatureTextTemplates.emplace_back(std::move(textEntry));
        } while (result->NextRow());
        delete result;
    }
}

void ReplayMgr::LoadCreatureText()
{
    if (auto result = SniffDatabase.Query("SELECT `unixtimems`, `guid`, `creature_id`, `group_id` FROM `creature_text` ORDER BY `unixtimems`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint64 unixtimems = fields[0].GetUInt64();
            uint32 creatureGuid = fields[1].GetUInt32();
            uint32 creatureId = fields[2].GetUInt32();
            uint32 groupId = fields[3].GetUInt32();

            CreatureText const* textEntry = GetCreatureTextTemplate(creatureId, groupId);
            std::string text = textEntry ? textEntry->text : "<error>";
            uint32 chatType = textEntry ? textEntry->chatType : 0;

            std::shared_ptr<SniffedEvent_CreatureText> newEvent = std::make_shared<SniffedEvent_CreatureText>(creatureGuid, creatureId, text, chatType);
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

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

    switch (ClassicChatMessageType(m_chatType))
    {
        case ClassicChatMessageType::MonsterYell:
            pCreature->MonsterYell(m_text.c_str());
            break;
        case ClassicChatMessageType::MonsterWhisper:
            for (const auto& itr : pCreature->GetMap()->GetPlayers())
                pCreature->MonsterWhisper(m_text.c_str(), itr.getSource());
            break;
        case ClassicChatMessageType::MonsterEmote:
            pCreature->MonsterTextEmote(m_text.c_str());
            break;
        case ClassicChatMessageType::MonsterSay:
        default:
            pCreature->MonsterSay(m_text.c_str());
            break;
    }
}

void ReplayMgr::LoadCreatureEquipmentUpdate()
{
    if (auto result = SniffDatabase.PQuery("SELECT `guid`, `unixtimems`, `slot`, `item_id` FROM `creature_equipment_values_update` ORDER BY `unixtimems`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 guid = fields[0].GetUInt32();
            uint64 unixtimems = fields[1].GetUInt64();
            uint32 slot = fields[2].GetUInt32();
            uint32 itemId = fields[3].GetUInt32();
            uint32 creatureId = GetCreatureEntryFromGuid(guid);

            if (slot > VIRTUAL_ITEM_SLOT_2)
                continue;

            if (itemId && !sObjectMgr.GetItemPrototype(itemId))
                continue;

            std::shared_ptr<SniffedEvent_CreatureEquipmentUpdate> newEvent = std::make_shared<SniffedEvent_CreatureEquipmentUpdate>(guid, creatureId, slot, itemId);
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

void SniffedEvent_CreatureEquipmentUpdate::Execute() const
{
    Creature* pCreature = sReplayMgr.GetCreature(m_guid);
    if (!pCreature)
    {
        sLog.outError("SniffedEvent_CreatureEquipmentUpdate: Cannot find source creature!");
        return;
    }

    pCreature->SetVirtualItem(VirtualItemSlot(m_slot), m_itemId);
}

void ReplayMgr::LoadPlayerEquipmentUpdate()
{
    if (auto result = SniffDatabase.PQuery("SELECT `guid`, `unixtimems`, `slot`, `item_id` FROM `player_equipment_values_update` ORDER BY `unixtimems`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 guid = fields[0].GetUInt32();
            uint64 unixtimems = fields[1].GetUInt64();
            uint32 slot = fields[2].GetUInt32();
            uint32 itemId = fields[3].GetUInt32();

            if (slot >= EQUIPMENT_SLOT_END)
                continue;

            if (itemId && !sObjectMgr.GetItemPrototype(itemId))
                continue;

            std::shared_ptr<SniffedEvent_PlayerEquipmentUpdate> newEvent = std::make_shared<SniffedEvent_PlayerEquipmentUpdate>(guid, slot, itemId);
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

void SniffedEvent_PlayerEquipmentUpdate::Execute() const
{
    Player* pPlayer = sReplayMgr.GetPlayer(m_guid);
    if (!pPlayer)
    {
        sLog.outError("SniffedEvent_PlayerEquipmentUpdate: Cannot find source player!");
        return;
    }

    uint32 currentItemId = 0;
    if (Item* pItem = pPlayer->GetItemByPos(INVENTORY_SLOT_BAG_0, m_slot))
        currentItemId = pItem->GetEntry();

    if (currentItemId == m_itemId)
        return;

    if (currentItemId)
        pPlayer->DestroyItem(INVENTORY_SLOT_BAG_0, m_slot, true);

    if (!m_itemId)
        return;

    if (ItemPrototype const* pItem = sObjectMgr.GetItemPrototype(m_itemId))
    {
        pPlayer->SatisfyItemRequirements(pItem);
        uint16 eDest = ((INVENTORY_SLOT_BAG_0 << 8) | m_slot);
        pPlayer->EquipNewItem(eDest, m_itemId, true);
    }
}

void ReplayMgr::LoadUnitEmote(char const* tableName, TypeID typeId)
{
    if (auto result = SniffDatabase.PQuery("SELECT `unixtimems`, `emote_id`, `guid` FROM `%s` ORDER BY `unixtimems`", tableName))
    {
        do
        {
            Field* fields = result->Fetch();

            uint64 unixtimems = fields[0].GetUInt64();
            uint32 emoteId = fields[1].GetUInt32();
            uint32 guid = fields[2].GetUInt32();
            uint32 creatureId = typeId == TYPEID_UNIT ? GetCreatureEntryFromGuid(guid) : 0;

            if (!sEmotesStore.LookupEntry(emoteId))
                continue;

            std::shared_ptr<SniffedEvent_UnitEmote> newEvent = std::make_shared<SniffedEvent_UnitEmote>(guid, creatureId, typeId, emoteId);
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

void SniffedEvent_UnitEmote::Execute() const
{
    Unit* pUnit = sReplayMgr.GetUnit(GetSourceObject());
    if (!pUnit)
    {
        sLog.outError("SniffedEvent_UnitEmote: Cannot find source unit!");
        return;
    }

    pUnit->HandleEmote(m_emoteId);
}

void ReplayMgr::LoadUnitAttackLog(char const* tableName, uint32 typeId)
{
    if (auto result = SniffDatabase.PQuery("SELECT `unixtimems`, `victim_guid`, `victim_id`, `victim_type`, `guid`, `hit_info`, `damage`, `blocked_damage`, `victim_state`, `attacker_state`, `spell_id` FROM `%s` ORDER BY `unixtimems`", tableName))
    {
        do
        {
            Field* fields = result->Fetch();

            uint64 unixtimems = fields[0].GetUInt64();
            uint32 victimGuid = fields[1].GetUInt32();
            uint32 victimId = fields[2].GetUInt32();
            std::string victimType = fields[3].GetCppString();
            uint32 guid = fields[4].GetUInt32();
            uint32 creatureId = typeId == TYPEID_UNIT ? GetCreatureEntryFromGuid(guid) : 0;
            uint32 hitInfo = fields[5].GetUInt32();
            uint32 damage = fields[6].GetUInt32();
            int32 blockedDamage = fields[7].GetInt32();
            uint32 victimState = fields[8].GetUInt32();
            int32 attackerState = fields[9].GetInt32();
            uint32 spellId = fields[10].GetUInt32();

            std::shared_ptr<SniffedEvent_UnitAttackLog> newEvent = std::make_shared<SniffedEvent_UnitAttackLog>(guid, creatureId, typeId, victimGuid, victimId, GetKnownObjectTypeId(victimType), ConvertClassicHitInfoFlagsToVanilla(hitInfo), damage, blockedDamage, victimState, attackerState, spellId);
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

void SniffedEvent_UnitAttackLog::Execute() const
{
    Unit* pAttacker = sReplayMgr.GetUnit(GetSourceObject());
    if (!pAttacker)
    {
        sLog.outError("SniffedEvent_UnitAttackLog: Cannot find attacker unit!");
        return;
    }

    Unit* pVictim = ToUnit(sReplayMgr.GetStoredObject(GetTargetObject()));
    if (!pVictim)
    {
        sLog.outError("SniffedEvent_UnitAttackLog: Cannot find victim unit!");
        return;
    }
    
    WorldPacket data(SMSG_ATTACKERSTATEUPDATE, (16 + 45));  // we guess size

    data << uint32(m_hitInfo);
#if SUPPORTED_CLIENT_BUILD > CLIENT_BUILD_1_8_4
    data << pAttacker->GetPackGUID();
    data << pVictim->GetPackGUID();
#else
    data << pAttacker->GetGUID();
    data << pVictim->GetGUID();
#endif
    data << uint32(m_damage); // Total damage

    data << uint8(1); // Sub damage count
    
    // Sub damage description
    for (uint8 i = 0; i < 1; i++)
    {
        data << uint32(SPELL_SCHOOL_NORMAL);
        data << float(1);        // Float coefficient of sub damage
        data << uint32(m_damage);
        data << uint32(0); // absorb
        data << int32(0); // resist
    }
    data << uint32(m_victimState);
    data << uint32(m_attackerState);
    data << uint32(m_spellId);
    data << uint32(m_blockedDamage);
    pAttacker->SendMessageToSet(&data, true);
}

template <class T>
void ReplayMgr::LoadUnitAttackToggle(char const* tableName, uint32 typeId)
{
    if (auto result = SniffDatabase.PQuery("SELECT `unixtimems`, `victim_guid`, `victim_id`, `victim_type`, `guid` FROM `%s` ORDER BY `unixtimems`", tableName))
    {
        do
        {
            Field* fields = result->Fetch();

            uint64 unixtimems = fields[0].GetUInt64();
            uint32 victimGuid = fields[1].GetUInt32();
            uint32 victimId = fields[2].GetUInt32();
            std::string victimType = fields[3].GetCppString();
            uint32 guid = fields[4].GetUInt32();
            uint32 creatureId = typeId == TYPEID_UNIT ? GetCreatureEntryFromGuid(guid) : 0;

            std::shared_ptr<T> newEvent = std::make_shared<T>(guid, creatureId, typeId, victimGuid, victimId, GetKnownObjectTypeId(victimType));
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

        } while (result->NextRow());
        delete result;
    }
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

template <class T>
void ReplayMgr::LoadCreatureValuesUpdate(char const* fieldName)
{
    if (auto result = SniffDatabase.PQuery("SELECT `guid`, `unixtimems`, `%s` FROM `creature_values_update` WHERE (`%s` IS NOT NULL) ORDER BY `unixtimems`", fieldName, fieldName))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 guid = fields[0].GetUInt32();;
            uint32 creatureId = GetCreatureEntryFromGuid(guid);
            uint64 unixtimems = fields[1].GetUInt64();
            uint32 value = fields[2].GetUInt32();

            std::shared_ptr<T> newEvent = std::make_shared<T>(guid, creatureId, TYPEID_UNIT, value);
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

template <class T>
void ReplayMgr::LoadCreatureValuesUpdate_float(char const* fieldName)
{
    if (auto result = SniffDatabase.PQuery("SELECT `guid`, `unixtimems`, `%s` FROM `creature_values_update` WHERE (`%s` IS NOT NULL) ORDER BY `unixtimems`", fieldName, fieldName))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 guid = fields[0].GetUInt32();;
            uint32 creatureId = GetCreatureEntryFromGuid(guid);
            uint64 unixtimems = fields[1].GetUInt64();
            float value = fields[2].GetFloat();

            std::shared_ptr<T> newEvent = std::make_shared<T>(guid, creatureId, TYPEID_UNIT, value);
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

template <class T>
void ReplayMgr::LoadPlayerValuesUpdate(char const* fieldName)
{
    if (auto result = SniffDatabase.PQuery("SELECT `guid`, `unixtimems`, `%s` FROM `player_values_update` WHERE (`%s` IS NOT NULL) ORDER BY `unixtimems`", fieldName, fieldName))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 guid = fields[0].GetUInt32();;
            uint64 unixtimems = fields[1].GetUInt64();
            uint32 value = fields[2].GetUInt32();

            std::shared_ptr<T> newEvent = std::make_shared<T>(guid, 0, TYPEID_PLAYER, value);
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

template <class T>
void ReplayMgr::LoadPlayerValuesUpdate_float(char const* fieldName)
{
    if (auto result = SniffDatabase.PQuery("SELECT `guid`, `unixtimems`, `%s` FROM `player_values_update` WHERE (`%s` IS NOT NULL) ORDER BY `unixtimems`", fieldName, fieldName))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 guid = fields[0].GetUInt32();;
            uint64 unixtimems = fields[1].GetUInt64();
            float value = fields[2].GetFloat();

            std::shared_ptr<T> newEvent = std::make_shared<T>(guid, 0, TYPEID_PLAYER, value);
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

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

    float scale = m_value;
    if (pUnit->IsCreature())
        scale *= Creature::GetScaleForDisplayId(pUnit->GetDisplayId());

    pUnit->SetObjectScale(scale);
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

    // flight path ending
    if (!m_value && pUnit->IsPlayer() && pUnit->HasUnitMovementFlag(MOVEFLAG_FLYING))
    {
        pUnit->RemoveUnitMovementFlag(MOVEFLAG_FLYING);
        if (!pUnit->movespline->Finalized())
        {
            pUnit->StopMoving();
            sLog.outInfo("Forcibly stopping flight path for %s", pUnit->GetGuidStr().c_str());
        }
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

void SniffedEvent_UnitUpdate_level::Execute() const
{
    Unit* pUnit = sReplayMgr.GetUnit(GetSourceObject());
    if (!pUnit)
    {
        sLog.outError("SniffedEvent_UnitUpdate_level: Cannot find source unit!");
        return;
    }

    pUnit->SetUInt32Value(UNIT_FIELD_LEVEL, m_value);
}

void SniffedEvent_UnitUpdate_aura_state::Execute() const
{
    Unit* pUnit = sReplayMgr.GetUnit(GetSourceObject());
    if (!pUnit)
    {
        sLog.outError("SniffedEvent_UnitUpdate_aura_state: Cannot find source unit!");
        return;
    }

    pUnit->SetUInt32Value(UNIT_FIELD_AURASTATE, m_value);
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

void SniffedEvent_UnitUpdate_vis_flags::Execute() const
{
    Unit* pUnit = sReplayMgr.GetUnit(GetSourceObject());
    if (!pUnit)
    {
        sLog.outError("SniffedEvent_UnitUpdate_vis_flags: Cannot find source unit!");
        return;
    }

    pUnit->SetByteValue(UNIT_FIELD_BYTES_1, UNIT_BYTES_1_OFFSET_VIS_FLAG, m_value);
}

void SniffedEvent_UnitUpdate_sheath_state::Execute() const
{
    Unit* pUnit = sReplayMgr.GetUnit(GetSourceObject());
    if (!pUnit)
    {
        sLog.outError("SniffedEvent_UnitUpdate_sheath_state: Cannot find source unit!");
        return;
    }

    pUnit->SetSheath(SheathState(m_value));
}

void SniffedEvent_UnitUpdate_shapeshift_form::Execute() const
{
    Unit* pUnit = sReplayMgr.GetUnit(GetSourceObject());
    if (!pUnit)
    {
        sLog.outError("SniffedEvent_UnitUpdate_shapeshift_form: Cannot find source unit!");
        return;
    }

    pUnit->SetByteValue(UNIT_FIELD_BYTES_1, UNIT_BYTES_1_OFFSET_SHAPESHIFT_FORM, m_value);
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

void SniffedEvent_UnitUpdate_bounding_radius::Execute() const
{
    Unit* pUnit = sReplayMgr.GetUnit(GetSourceObject());
    if (!pUnit)
    {
        sLog.outError("SniffedEvent_UnitUpdate_bounding_radius: Cannot find source unit!");
        return;
    }

    pUnit->SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS, m_value);
}

void SniffedEvent_UnitUpdate_combat_reach::Execute() const
{
    Unit* pUnit = sReplayMgr.GetUnit(GetSourceObject());
    if (!pUnit)
    {
        sLog.outError("SniffedEvent_UnitUpdate_combat_reach: Cannot find source unit!");
        return;
    }

    pUnit->SetFloatValue(UNIT_FIELD_COMBATREACH, m_value);
}

void SniffedEvent_UnitUpdate_base_attack_time::Execute() const
{
    Unit* pUnit = sReplayMgr.GetUnit(GetSourceObject());
    if (!pUnit)
    {
        sLog.outError("SniffedEvent_UnitUpdate_base_attack_time: Cannot find source unit!");
        return;
    }

    pUnit->SetFloatValue(UNIT_FIELD_BASEATTACKTIME, m_value);
}

void SniffedEvent_UnitUpdate_ranged_attack_time::Execute() const
{
    Unit* pUnit = sReplayMgr.GetUnit(GetSourceObject());
    if (!pUnit)
    {
        sLog.outError("SniffedEvent_UnitUpdate_ranged_attack_time: Cannot find source unit!");
        return;
    }

    pUnit->SetUInt32Value(UNIT_FIELD_RANGEDATTACKTIME, m_value);
}

void ReplayMgr::LoadUnitGuidValuesUpdate(char const* tableName, uint32 typeId)
{
    if (auto result = SniffDatabase.PQuery("SELECT `guid`, `unixtimems`, `field_name`, `object_guid`, `object_id`, `object_type` FROM `%s` ORDER BY `unixtimems`", tableName))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 guid = fields[0].GetUInt32();
            uint32 creatureId = typeId == TYPEID_UNIT ? GetCreatureEntryFromGuid(guid) : 0;
            uint64 unixtimems = fields[1].GetUInt64();
            std::string fieldName = fields[2].GetCppString();
            uint32 objectGuid = fields[3].GetUInt32();
            uint32 objectId = fields[4].GetUInt32();
            std::string objectType = fields[5].GetCppString();
            
            uint32 fieldIndex = 0;
            if (fieldName == "Charm")
                fieldIndex = UNIT_FIELD_CHARM;
            else if (fieldName == "Summon")
                fieldIndex = UNIT_FIELD_SUMMON;
            else if (fieldName == "CharmedBy")
                fieldIndex = UNIT_FIELD_CHARMEDBY;
            else if (fieldName == "SummonedBy")
                fieldIndex = UNIT_FIELD_SUMMONEDBY;
            else if (fieldName == "CreatedBy")
                fieldIndex = UNIT_FIELD_CREATEDBY;
            else if (fieldName == "Target")
                fieldIndex = UNIT_FIELD_TARGET;
            else
                continue;

            std::shared_ptr<SniffedEvent_UnitUpdate_guid_value> newEvent = std::make_shared<SniffedEvent_UnitUpdate_guid_value>(guid, creatureId, typeId, objectGuid, objectId, GetKnownObjectTypeId(objectType), fieldIndex);
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

void SniffedEvent_UnitUpdate_guid_value::Execute() const
{
    Unit* pUnit = sReplayMgr.GetUnit(GetSourceObject());
    if (!pUnit)
    {
        sLog.outError("SniffedEvent_UnitUpdate_guid_value: Cannot find source unit!");
        return;
    }

    ObjectGuid guidValue;

    if (!GetTargetObject().IsEmpty())
    {
        WorldObject* pObject = sReplayMgr.GetStoredObject(GetTargetObject());
        if (!pObject)
        {
            sLog.outError("SniffedEvent_UnitUpdate_guid_value: Cannot find target object!");
            return;
        }
        guidValue = pObject->GetObjectGuid();
    }
    

    pUnit->SetGuidValue(m_updateField, guidValue);
}

void ReplayMgr::LoadCreatureSpeedUpdate(uint32 speedType)
{
    if (auto result = SniffDatabase.PQuery("SELECT `guid`, `unixtimems`, `speed_rate` FROM `creature_speed_update` WHERE `speed_type`=%u ORDER BY `unixtimems`", speedType))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 guid = fields[0].GetUInt32();;
            uint32 creatureId = GetCreatureEntryFromGuid(guid);
            uint64 unixtimems = fields[1].GetUInt64();
            float speedRate = fields[2].GetFloat();

            std::shared_ptr<SniffedEvent_UnitUpdate_speed> newEvent = std::make_shared<SniffedEvent_UnitUpdate_speed>(guid, creatureId, TYPEID_UNIT, speedType, speedRate);
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

void ReplayMgr::LoadPlayerSpeedUpdate(uint32 speedType)
{
    if (auto result = SniffDatabase.PQuery("SELECT `guid`, `unixtimems`, `speed_rate` FROM `player_speed_update` WHERE `speed_type`=%u ORDER BY `unixtimems`", speedType))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 guid = fields[0].GetUInt32();;
            uint64 unixtimems = fields[1].GetUInt64();
            float speedRate = fields[2].GetFloat();

            std::shared_ptr<SniffedEvent_UnitUpdate_speed> newEvent = std::make_shared<SniffedEvent_UnitUpdate_speed>(guid, 0, TYPEID_PLAYER, speedType, speedRate);
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

void SniffedEvent_UnitUpdate_speed::Execute() const
{
    Unit* pUnit = sReplayMgr.GetUnit(GetSourceObject());
    if (!pUnit)
    {
        sLog.outError("SniffedEvent_UnitUpdate_speed: Cannot find source unit!");
        return;
    }
    pUnit->SetSpeedRateDirect(UnitMoveType(m_speedType), m_speedRate);
    if (pUnit->IsInWorld() && pUnit->GetVisibility() != VISIBILITY_OFF)
        MovementPacketSender::SendSpeedChangeToAll(pUnit, UnitMoveType(m_speedType), m_speedRate);
}

void ReplayMgr::LoadUnitAurasUpdate(char const* tableName, uint32 typeId)
{
    if (auto result = SniffDatabase.PQuery("SELECT `guid`, `unixtimems`, `update_id`, `slot`, `spell_id`, `level`, `charges` FROM `%s` ORDER BY `unixtimems`", tableName))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 guid = fields[0].GetUInt32();
            uint32 creatureId = typeId == TYPEID_UNIT ? GetCreatureEntryFromGuid(guid) : 0;
            uint64 unixtimems = fields[1].GetUInt64();
            uint32 updateId = fields[2].GetUInt32();
            uint32 slot = fields[3].GetUInt32();
            uint32 spellId = fields[4].GetUInt32();
            uint32 level = fields[5].GetUInt32();
            uint32 charges = fields[6].GetUInt32();

            if (slot >= MAX_AURAS)
                continue;

            if (spellId && !sSpellMgr.GetSpellEntry(spellId))
                continue;

            std::shared_ptr<SniffedEvent_UnitUpdate_auras> newEvent = std::make_shared<SniffedEvent_UnitUpdate_auras>(guid, creatureId, typeId, updateId, slot, spellId, level, charges);
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

void SetAuraFlag(Unit* pUnit, SpellEntry const* pSpellEntry, uint32 slot, bool add)
{
    uint32 index = slot >> 3;
    uint32 byte = (slot & 7) << 2;
    uint32 val = pUnit->GetUInt32Value(UNIT_FIELD_AURAFLAGS + index);
    val &= ~(uint32(AFLAG_MASK_ALL) << byte);
    if (add)
    {
        uint32 flags = AFLAG_NONE;

        if (pSpellEntry->IsPositiveSpell())
        {
            if (!pSpellEntry->HasAttribute(SPELL_ATTR_CANT_CANCEL))
                flags |= AFLAG_CANCELABLE;
            flags |= AFLAG_UNK3;
        }
        else
            flags |= AFLAG_UNK4;

        val |= (flags << byte);
    }
    pUnit->SetUInt32Value(UNIT_FIELD_AURAFLAGS + index, val);
}

void SetAuraLevel(Unit* pUnit, uint32 slot, uint32 level)
{
    uint32 index = slot / 4;
    uint32 byte = (slot % 4) * 8;
    uint32 val = pUnit->GetUInt32Value(UNIT_FIELD_AURALEVELS + index);
    val &= ~(0xFF << byte);
    val |= (level << byte);
    pUnit->SetUInt32Value(UNIT_FIELD_AURALEVELS + index, val);
}

void SetAuraCharges(Unit* pUnit, uint32 slot, uint32 charges)
{
    uint32 index = slot / 4;
    uint32 byte = (slot % 4) * 8;
    uint32 val = pUnit->GetUInt32Value(UNIT_FIELD_AURAAPPLICATIONS + index);
    val &= ~(0xFF << byte);
    // field expect count-1 for proper amount show, also prevent overflow at client side
    val |= ((uint8(charges <= 255 ? charges - 1 : 255 - 1)) << byte);
    pUnit->SetUInt32Value(UNIT_FIELD_AURAAPPLICATIONS + index, val);
}

void SniffedEvent_UnitUpdate_auras::Execute() const
{
    Unit* pUnit = sReplayMgr.GetUnit(GetSourceObject());
    if (!pUnit)
    {
        sLog.outError("SniffedEvent_UnitUpdate_auras: Cannot find source unit!");
        return;
    }

    SpellEntry const* pSpellEntry = nullptr;
    if (m_spellId)
    {
        if (!(pSpellEntry = sSpellMgr.GetSpellEntry(m_spellId)))
            return;
    }
    
    pUnit->SetUInt32Value(UNIT_FIELD_AURA + m_slot, m_spellId);
    SetAuraFlag(pUnit, pSpellEntry, m_slot, m_spellId != 0);
    SetAuraLevel(pUnit, m_slot, m_level);
    SetAuraCharges(pUnit, m_slot, m_charges);
}

void ReplayMgr::LoadDynamicObjectCreate()
{
    std::map<uint32, DynamicObjectSpawn> spawns;

    if (auto result = SniffDatabase.Query("SELECT `guid`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `caster_guid`, `caster_id`, `caster_type`, `spell_id`, `radius`, `type` FROM `dynamicobject` ORDER BY `guid`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 guid = fields[0].GetUInt32();
            DynamicObjectSpawn& spawn = spawns[guid];
            spawn.m_map = fields[1].GetUInt32();
            spawn.m_x = fields[2].GetFloat();
            spawn.m_y = fields[3].GetFloat();
            spawn.m_z = fields[4].GetFloat();
            spawn.m_o = fields[5].GetFloat();
            spawn.m_casterGuid = fields[6].GetUInt32();
            spawn.m_casterId = fields[7].GetUInt32();
            spawn.m_casterType = GetKnownObjectTypeId(fields[8].GetCppString());
            spawn.m_spellId = fields[9].GetUInt32();
            spawn.m_radius = fields[10].GetFloat();
            spawn.m_type = fields[11].GetUInt32();
        } while (result->NextRow());
        delete result;
    }

    std::map<uint32, uint64> destroyTimes;

    if (auto result = SniffDatabase.Query("SELECT `guid`, `unixtimems` FROM `dynamicobject_destroy_time` ORDER BY `unixtimems`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 guid = fields[0].GetUInt32();
            uint64 unixtimems = fields[1].GetUInt64();
            destroyTimes[guid] = unixtimems;
        } while (result->NextRow());
        delete result;
    }

    if (auto result = SniffDatabase.PQuery("SELECT `guid`, `unixtimems` FROM `dynamicobject_create2_time` ORDER BY `unixtimems`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 guid = fields[0].GetUInt32();
            uint64 unixtimems = fields[1].GetUInt64();

            auto objectData = spawns.find(guid);
            if (objectData == spawns.end())
                continue;

            uint32 duration = 60000;
            auto destroyTime = destroyTimes.find(guid);
            if (destroyTime != destroyTimes.end())
                duration = destroyTime->second - unixtimems;

            std::shared_ptr<SniffedEvent_DynamicObjectCreate> newEvent = std::make_shared<SniffedEvent_DynamicObjectCreate>(objectData->second, duration);
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

        } while (result->NextRow());
        delete result;
    }

    if (auto result = SniffDatabase.PQuery("SELECT `guid`, `unixtimems` FROM `dynamicobject_create1_time` WHERE `guid` NOT IN (SELECT `guid` FROM `dynamicobject_create2_time`) ORDER BY `unixtimems`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 guid = fields[0].GetUInt32();
            uint64 unixtimems = fields[1].GetUInt64();

            auto objectData = spawns.find(guid);
            if (objectData == spawns.end())
                continue;

            uint32 duration = 60000;
            auto destroyTime = destroyTimes.find(guid);
            if (destroyTime != destroyTimes.end())
                duration = destroyTime->second - unixtimems;

            std::shared_ptr<SniffedEvent_DynamicObjectCreate> newEvent = std::make_shared<SniffedEvent_DynamicObjectCreate>(objectData->second, duration);
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

void SniffedEvent_DynamicObjectCreate::Execute() const
{
    Unit* pCaster = sReplayMgr.GetUnit(GetSourceObject());
    if (!pCaster)
    {
        sLog.outError("SniffedEvent_DynamicObjectCreate: Cannot find caster unit!");
        return;
    }

    if (!pCaster->IsInWorld())
    {
        sLog.outError("SniffedEvent_DynamicObjectCreate: Caster is not in world!");
        return;
    }

    DynamicObject* dynObj = new DynamicObject;
    if (!dynObj->Create(pCaster->GetMap()->GenerateLocalLowGuid(HIGHGUID_DYNAMICOBJECT), pCaster, m_objectData.m_spellId,
        EFFECT_INDEX_0, m_objectData.m_x, m_objectData.m_y, m_objectData.m_z, m_duration, m_objectData.m_radius, DYNAMIC_OBJECT_AREA_SPELL))
    {
        delete dynObj;
        return;
    }

    pCaster->AddDynObject(dynObj);
    pCaster->GetMap()->Add(dynObj);
}

void ReplayMgr::LoadGameObjectCreate1()
{
    if (auto result = SniffDatabase.Query("SELECT `guid`, `unixtimems`, `position_x`, `position_y`, `position_z`, `orientation` FROM `gameobject_create1_time` ORDER BY `unixtimems`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 guid = fields[0].GetUInt32();
            uint32 entry = GetGameObjectEntryFromGuid(guid);
            uint64 unixtimems = fields[1].GetUInt64();
            float position_x = fields[2].GetFloat();
            float position_y = fields[3].GetFloat();
            float position_z = fields[4].GetFloat();
            float orientation = fields[5].GetFloat();

            std::shared_ptr<SniffedEvent_GameObjectCreate1> newEvent = std::make_shared<SniffedEvent_GameObjectCreate1>(guid, entry, position_x, position_y, position_z, orientation);
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

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
    if (auto result = SniffDatabase.Query("SELECT `guid`, `unixtimems`, `position_x`, `position_y`, `position_z`, `orientation` FROM `gameobject_create2_time` ORDER BY `unixtimems`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 guid = fields[0].GetUInt32();
            uint32 entry = GetGameObjectEntryFromGuid(guid);
            uint64 unixtimems = fields[1].GetUInt64();
            float position_x = fields[2].GetFloat();
            float position_y = fields[3].GetFloat();
            float position_z = fields[4].GetFloat();
            float orientation = fields[5].GetFloat();

            std::shared_ptr<SniffedEvent_GameObjectCreate2> newEvent = std::make_shared<SniffedEvent_GameObjectCreate2>(guid, entry, position_x, position_y, position_z, orientation);
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

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
    if (auto result = SniffDatabase.Query("SELECT `guid`, `anim_id`, `unixtimems` FROM `gameobject_custom_anim` ORDER BY `unixtimems`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 guid = fields[0].GetUInt32();
            uint32 entry = GetGameObjectEntryFromGuid(guid);
            uint32 animId = fields[1].GetUInt32();
            uint64 unixtimems = fields[2].GetUInt64();

            std::shared_ptr<SniffedEvent_GameObjectCustomAnim> newEvent = std::make_shared<SniffedEvent_GameObjectCustomAnim>(guid, entry, animId);
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

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
    if (auto result = SniffDatabase.Query("SELECT `guid`, `unixtimems` FROM `gameobject_despawn_anim` ORDER BY `unixtimems`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 guid = fields[0].GetUInt32();
            uint32 entry = GetGameObjectEntryFromGuid(guid);
            uint64 unixtimems = fields[1].GetUInt64();

            std::shared_ptr<SniffedEvent_GameObjectDespawnAnim> newEvent = std::make_shared<SniffedEvent_GameObjectDespawnAnim>(guid, entry);
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

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
    if (auto result = SniffDatabase.Query("SELECT `guid`, `unixtimems` FROM `gameobject_destroy_time` ORDER BY `unixtimems`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 guid = fields[0].GetUInt32();
            uint32 entry = GetGameObjectEntryFromGuid(guid);
            uint64 unixtimems = fields[1].GetUInt64();

            std::shared_ptr<SniffedEvent_GameObjectDestroy> newEvent = std::make_shared<SniffedEvent_GameObjectDestroy>(guid, entry);
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

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

template <class T>
void ReplayMgr::LoadGameObjectUpdate(char const* fieldName)
{
    if (auto result = SniffDatabase.PQuery("SELECT `guid`, `unixtimems`, `%s` FROM `gameobject_values_update` WHERE (`%s` IS NOT NULL) ORDER BY `unixtimems`", fieldName, fieldName))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 guid = fields[0].GetUInt32();;
            uint32 entry = GetGameObjectEntryFromGuid(guid);
            uint64 unixtimems = fields[1].GetUInt64();
            uint32 value = fields[2].GetUInt32();

            std::shared_ptr<T> newEvent = std::make_shared<T>(guid, entry, value);
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

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
    if (auto result = SniffDatabase.Query("SELECT `unixtimems`, `caster_guid`, `caster_id`, `caster_type`, `spell_id` FROM `spell_cast_failed` ORDER BY `unixtimems`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint64 unixtimems = fields[0].GetUInt64();
            uint32 casterGuid = fields[1].GetUInt32();
            uint32 casterId = fields[2].GetUInt32();
            std::string casterType = fields[3].GetCppString();
            uint32 spellId = fields[4].GetUInt32();

            if (!sSpellMgr.GetSpellEntry(spellId))
                continue;

            std::shared_ptr<SniffedEvent_SpellCastFailed> newEvent = std::make_shared<SniffedEvent_SpellCastFailed>(spellId, casterGuid, casterId, GetKnownObjectTypeId(casterType));
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

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
    if (auto result = SniffDatabase.Query("SELECT `unixtimems`, `caster_guid`, `caster_id`, `caster_type`, `caster_unit_guid`, `caster_unit_id`, `caster_unit_type`, `spell_id`, `cast_time`, `cast_flags`, `ammo_display_id`, `ammo_inventory_type`, `target_guid`, `target_id`, `target_type` FROM `spell_cast_start` ORDER BY `unixtimems`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint64 unixtimems = fields[0].GetUInt64();
            uint32 casterGuid = fields[1].GetUInt32();
            uint32 casterId = fields[2].GetUInt32();
            std::string casterType = fields[3].GetCppString();
            uint32 casterUnitGuid = fields[4].GetUInt32();
            uint32 casterUnitId = fields[5].GetUInt32();
            std::string casterUnitType = fields[6].GetCppString();
            if (casterType == "Item" && casterUnitType.length() > 1)
            {
                casterGuid = casterUnitGuid;
                casterId = casterUnitId;
                casterType = casterUnitType;
            }
            uint32 spellId = fields[7].GetUInt32();
            uint32 castTime = fields[8].GetUInt32();
            uint32 castFlags = fields[9].GetUInt32();
            uint32 ammoDisplayId = fields[10].GetUInt32();
            uint32 ammoInventoryType = fields[11].GetUInt32();
            uint32 targetGuid = fields[12].GetUInt32();
            uint32 targetId = fields[13].GetUInt32();
            std::string targetType = fields[14].GetCppString();

            if (!sSpellMgr.GetSpellEntry(spellId))
                continue;

            std::shared_ptr<SniffedEvent_SpellCastStart> newEvent = std::make_shared<SniffedEvent_SpellCastStart>(spellId, castTime, castFlags, ammoDisplayId, ammoInventoryType, casterGuid, casterId, GetKnownObjectTypeId(casterType), targetGuid, targetId, GetKnownObjectTypeId(targetType));
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

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
    data << uint32(m_spellId);
    data << uint16(m_castFlags);
    data << uint32(m_castTime);

    SpellCastTargets targets;
    if (Unit* pUnitTarget = pTarget->ToUnit())
        targets.setUnitTarget(pUnitTarget);
    else if (GameObject* pGoTarget = pTarget->ToGameObject())
        targets.setGOTarget(pGoTarget);
    data << targets;

    if (m_castFlags & CAST_FLAG_AMMO)                         // projectile info
    {
        data << uint32(m_ammoDisplayId);
        data << uint32(m_ammoInventoryType);
    }

    pCaster->SendObjectMessageToSet(&data, false);
}

void ReplayMgr::LoadSpellCastGo()
{
    //                                             0             1              2            3              4                   5                 6                   7           8             9                  10                     11                  12                13                  14                   15                     16                    17                      18                 19
    if (auto result = SniffDatabase.Query("SELECT `unixtimems`, `caster_guid`, `caster_id`, `caster_type`, `caster_unit_guid`, `caster_unit_id`, `caster_unit_type`, `spell_id`, `cast_flags`, `ammo_display_id`, `ammo_inventory_type`, `main_target_guid`, `main_target_id`, `main_target_type`, `hit_targets_count`, `hit_targets_list_id`, `miss_targets_count`, `miss_targets_list_id`, `src_position_id`, `dst_position_id` FROM `spell_cast_go` ORDER BY `unixtimems`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint64 unixtimems = fields[0].GetUInt64();
            uint32 casterGuid = fields[1].GetUInt32();
            uint32 casterId = fields[2].GetUInt32();
            std::string casterType = fields[3].GetCppString();
            uint32 casterUnitGuid = fields[4].GetUInt32();
            uint32 casterUnitId = fields[5].GetUInt32();
            std::string casterUnitType = fields[6].GetCppString();
            if (casterType == "Item" && casterUnitType.length() > 1)
            {
                casterGuid = casterUnitGuid;
                casterId = casterUnitId;
                casterType = casterUnitType;
            }
            uint32 spellId = fields[7].GetUInt32();
            uint32 castFlags = fields[8].GetUInt32();
            uint32 ammoDisplayId = fields[9].GetUInt32();
            uint32 ammoInventoryType = fields[10].GetUInt32();
            uint32 targetGuid = fields[11].GetUInt32();
            uint32 targetId = fields[12].GetUInt32();
            std::string targetType = fields[13].GetCppString();
            uint32 hitTargetsCount = fields[14].GetUInt32();
            uint32 hitTargetsListId = fields[15].GetUInt32();
            uint32 missTargetsCount = fields[16].GetUInt32();
            uint32 missTargetsListId = fields[17].GetUInt32();
            uint32 srcPositionId = fields[18].GetUInt32();
            uint32 dstPositionId = fields[19].GetUInt32();

            if (!sSpellMgr.GetSpellEntry(spellId))
                continue;

            std::shared_ptr<SniffedEvent_SpellCastGo> newEvent = std::make_shared<SniffedEvent_SpellCastGo>(spellId, castFlags, ammoDisplayId, ammoInventoryType, casterGuid, casterId, GetKnownObjectTypeId(casterType), targetGuid, targetId, GetKnownObjectTypeId(targetType), hitTargetsCount, hitTargetsListId, missTargetsCount, missTargetsListId, srcPositionId, dstPositionId);
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

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

            uint32 id = fields[0].GetUInt32();
            float x = fields[1].GetFloat();
            float y = fields[2].GetFloat();
            float z = fields[3].GetFloat();
            m_spellCastGoPositions.insert({ id , G3D::Vector3(x, y, z) });

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

    WorldPacket data(SMSG_SPELL_GO, 53);
#if SUPPORTED_CLIENT_BUILD > CLIENT_BUILD_1_8_4
    data << pCaster->GetPackGUID();
    data << pCaster->GetPackGUID();
#else
    data << pCaster->GetGUID();
    data << pCaster->GetGUID();
#endif

    data << uint32(m_spellId);                              // spellId
    data << uint16(m_castFlags);                            // cast flags

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

    if (m_castFlags & CAST_FLAG_AMMO)                         // projectile info
    {
        data << uint32(m_ammoDisplayId);
        data << uint32(m_ammoInventoryType);
    }

    pCaster->SendObjectMessageToSet(&data, false);
}

void ReplayMgr::LoadSpellChannelStart()
{
    if (auto result = SniffDatabase.Query("SELECT `unixtimems`, `caster_guid`, `caster_id`, `caster_type`, `spell_id`, `duration` FROM `spell_channel_start` ORDER BY `unixtimems`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint64 unixtimems = fields[0].GetUInt64();
            uint32 casterGuid = fields[1].GetUInt32();
            uint32 casterId = fields[2].GetUInt32();
            std::string casterType = fields[3].GetCppString();
            uint32 spellId = fields[4].GetUInt32();
            int32 duration = fields[5].GetInt32();

            if (!sSpellMgr.GetSpellEntry(spellId))
                continue;

            std::shared_ptr<SniffedEvent_SpellChannelStart> newEvent = std::make_shared<SniffedEvent_SpellChannelStart>(spellId, duration, casterGuid, casterId, GetKnownObjectTypeId(casterType));
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

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
    if (auto result = SniffDatabase.Query("SELECT `unixtimems`, `caster_guid`, `caster_id`, `caster_type`, `duration` FROM `spell_channel_update` ORDER BY `unixtimems`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint64 unixtimems = fields[0].GetUInt64();
            uint32 casterGuid = fields[1].GetUInt32();
            uint32 casterId = fields[2].GetUInt32();
            std::string casterType = fields[3].GetCppString();
            int32 duration = fields[4].GetInt32();

            std::shared_ptr<SniffedEvent_SpellChannelUpdate> newEvent = std::make_shared<SniffedEvent_SpellChannelUpdate>(duration, casterGuid, casterId, GetKnownObjectTypeId(casterType));
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

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

void ReplayMgr::LoadPlayerChat()
{
    if (auto result = SniffDatabase.Query("SELECT `guid`, `sender_name`, `text`, `chat_type`, `channel_name`, `unixtimems` FROM `player_chat` ORDER BY `unixtimems`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 guid = fields[0].GetUInt32();
            std::string senderName = fields[1].GetCppString();
            std::string text = fields[2].GetCppString();
            uint8 chatType = fields[3].GetUInt8();
            std::string channelName = fields[4].GetCppString();
            uint64 unixtimems = fields[5].GetUInt64();

            std::shared_ptr<SniffedEvent_PlayerChat> newEvent = std::make_shared<SniffedEvent_PlayerChat>(guid, senderName, text, chatType, channelName);
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

        } while (result->NextRow());
        delete result;
    }
}

void SniffedEvent_PlayerChat::Execute() const
{
    ChatMsg chatType = ConvertClassicChatTypeToVanilla(m_chatType);
    ObjectGuid guid;
    if (m_guid)
    {
        if (Player* pSender = sReplayMgr.GetPlayer(m_guid))
            guid = pSender->GetObjectGuid();
    }
    else if (!m_senderName.empty())
    {
        guid = sObjectMgr.GetPlayerGuidByName(m_senderName);
        if (guid.IsEmpty())
        {
            sLog.outError("SniffedEvent_PlayerChat: Cannot find bot!");
        }
    }

    WorldPacket data;
    ChatHandler::BuildChatPacket(data, chatType, m_text.c_str(), LANG_UNIVERSAL, 0, guid, m_senderName.c_str(), ObjectGuid(), "", m_channelName.c_str(), 0);
    sWorld.SendGlobalMessage(&data);
}

void ReplayMgr::LoadPlayMusic()
{
    if (auto result = SniffDatabase.Query("SELECT `unixtimems`, `music` FROM `play_music` ORDER BY `unixtimems`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint64 unixtimems = fields[0].GetUInt64();
            uint32 musicId = fields[1].GetUInt32();

            std::shared_ptr<SniffedEvent_PlayMusic> newEvent = std::make_shared<SniffedEvent_PlayMusic>(musicId);
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

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
    if (auto result = SniffDatabase.Query("SELECT `unixtimems`, `sound`, `source_guid`, `source_id`, `source_type` FROM `play_sound` ORDER BY `unixtimems`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint64 unixtimems = fields[0].GetUInt64();
            uint32 soundId = fields[1].GetUInt32();
            uint32 sourceGuid = fields[2].GetUInt32();
            uint32 sourceId = fields[3].GetUInt32();
            std::string sourceType = fields[4].GetCppString();

            std::shared_ptr<SniffedEvent_PlaySound> newEvent = std::make_shared<SniffedEvent_PlaySound>(soundId, sourceGuid, sourceId, GetKnownObjectTypeId(sourceType));
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

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
    if (auto result = SniffDatabase.Query("SELECT `unixtimems`, `kit_id`, `caster_guid`, `caster_id`, `caster_type` FROM `play_spell_visual_kit` ORDER BY `unixtimems`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint64 unixtimems = fields[0].GetUInt64();
            uint32 kitId = fields[1].GetUInt32();
            uint32 sourceGuid = fields[2].GetUInt32();
            uint32 sourceId = fields[3].GetUInt32();
            std::string sourceType = fields[4].GetCppString();

            std::shared_ptr<SniffedEvent_PlaySpellVisualKit> newEvent = std::make_shared<SniffedEvent_PlaySpellVisualKit>(kitId, sourceGuid, sourceId, GetKnownObjectTypeId(sourceType));
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

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

void ReplayMgr::LoadQuestAcceptTimes()
{
    if (auto result = SniffDatabase.Query("SELECT `unixtimems`, `object_guid`, `object_id`, `object_type`, `quest_id` FROM `client_quest_accept` ORDER BY `unixtimems`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint64 unixtimems = fields[0].GetUInt64();
            uint32 objectGuid = fields[1].GetUInt32();
            uint32 objectId = fields[2].GetUInt32();
            std::string objectType = fields[3].GetCppString();
            uint32 questId = fields[4].GetUInt32();

            std::shared_ptr<SniffedEvent_QuestAccept> newEvent = std::make_shared<SniffedEvent_QuestAccept>(questId, objectGuid, objectId, objectType);
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

        } while (result->NextRow());
    }
}

void SniffedEvent_QuestAccept::Execute() const
{
    Player* pPlayer = sReplayMgr.GetActivePlayer();
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
    if (auto result = SniffDatabase.Query("SELECT `unixtimems`, `object_guid`, `object_id`, `object_type`, `quest_id` FROM `client_quest_complete` ORDER BY `unixtimems`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint64 unixtimems = fields[0].GetUInt64();
            uint32 objectGuid = fields[1].GetUInt32();
            uint32 objectId = fields[2].GetUInt32();
            std::string objectType = fields[3].GetCppString();
            uint32 questId = fields[4].GetUInt32();

            std::shared_ptr<SniffedEvent_QuestComplete> newEvent = std::make_shared<SniffedEvent_QuestComplete>(questId, objectGuid, objectId, objectType);
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

        } while (result->NextRow());
    }
}

void SniffedEvent_QuestComplete::Execute() const
{
    Player* pPlayer = sReplayMgr.GetActivePlayer();
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
    if (auto result = SniffDatabase.Query("SELECT `unixtimems`, `guid` FROM `client_creature_interact` ORDER BY `unixtimems`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint64 unixtimems = fields[0].GetUInt64();
            uint32 guid = fields[1].GetUInt32();
            uint32 entry = GetCreatureEntryFromGuid(guid);

            std::shared_ptr<SniffedEvent_CreatureInteract> newEvent = std::make_shared<SniffedEvent_CreatureInteract>(guid, entry);
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

        } while (result->NextRow());
    }
}

void SniffedEvent_CreatureInteract::Execute() const
{
    Player* pPlayer = sReplayMgr.GetActivePlayer();
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
    if (auto result = SniffDatabase.Query("SELECT `unixtimems`, `guid` FROM `client_gameobject_use` ORDER BY `unixtimems`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint64 unixtimems = fields[0].GetUInt64();
            uint32 guid = fields[1].GetUInt32();
            uint32 entry = GetGameObjectEntryFromGuid(guid);

            std::shared_ptr<SniffedEvent_GameObjectUse> newEvent = std::make_shared<SniffedEvent_GameObjectUse>(guid, entry);
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

        } while (result->NextRow());
    }
}

void SniffedEvent_GameObjectUse::Execute() const
{
    Player* pPlayer = sReplayMgr.GetActivePlayer();
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
    if (auto result = SniffDatabase.Query("SELECT `unixtimems`, `entry` FROM `client_item_use` ORDER BY `unixtimems`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint64 unixtimems = fields[0].GetUInt64();
            uint32 entry = fields[1].GetUInt32();

            std::shared_ptr<SniffedEvent_ItemUse> newEvent = std::make_shared<SniffedEvent_ItemUse>(entry);
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

        } while (result->NextRow());
    }
}

void SniffedEvent_ItemUse::Execute() const
{
    Player* pPlayer = sReplayMgr.GetActivePlayer();
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
    if (auto result = SniffDatabase.Query("SELECT `unixtimems` FROM `client_reclaim_corpse` ORDER BY `unixtimems`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint64 unixtimems = fields[0].GetUInt64();

            std::shared_ptr<SniffedEvent_ReclaimCorpse> newEvent = std::make_shared<SniffedEvent_ReclaimCorpse>();
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

        } while (result->NextRow());
    }
}

void SniffedEvent_ReclaimCorpse::Execute() const
{
    Player* pPlayer = sReplayMgr.GetActivePlayer();
    if (!pPlayer)
    {
        sLog.outError("SniffedEvent_ReclaimCorpse: Cannot find active player!");
        return;
    }
    pPlayer->MonsterSay("Client reclaims corpse.");
}

void ReplayMgr::LoadReleaseSpiritTimes()
{
    if (auto result = SniffDatabase.Query("SELECT `unixtimems` FROM `client_release_spirit` ORDER BY `unixtimems`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint64 unixtimems = fields[0].GetUInt64();

            std::shared_ptr<SniffedEvent_ReleaseSpirit> newEvent = std::make_shared<SniffedEvent_ReleaseSpirit>();
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

        } while (result->NextRow());
    }
}

void SniffedEvent_ReleaseSpirit::Execute() const
{
    Player* pPlayer = sReplayMgr.GetActivePlayer();
    if (!pPlayer)
    {
        sLog.outError("SniffedEvent_ReleaseSpirit: Cannot find active player!");
        return;
    }
    pPlayer->MonsterSay("Client releases spirit.");
}

void ReplayMgr::LoadQuestUpdateComplete()
{
    if (auto result = SniffDatabase.Query("SELECT `quest_id`, `unixtimems` FROM `quest_update_complete` ORDER BY `unixtimems`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 questId = fields[0].GetUInt32();
            uint64 unixtimems = fields[1].GetUInt64();

            std::shared_ptr<SniffedEvent_QuestUpdateComplete> newEvent = std::make_shared<SniffedEvent_QuestUpdateComplete>(questId);
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

        } while (result->NextRow());
    }
}

void SniffedEvent_QuestUpdateComplete::Execute() const
{
    Player* pPlayer = sReplayMgr.GetActivePlayer();
    if (!pPlayer)
    {
        sLog.outError("SniffedEvent_QuestUpdateComplete: Cannot find active player!");
        return;
    }
    std::string txt = "Quest " + sReplayMgr.GetQuestName(m_questId) + " (Entry: " + std::to_string(m_questId) + ") marked as completed.";
    pPlayer->MonsterSay(txt.c_str());
}

void ReplayMgr::LoadQuestUpdateFailed()
{
    if (auto result = SniffDatabase.Query("SELECT `quest_id`, `unixtimems` FROM `quest_update_failed` ORDER BY `unixtimems`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 questId = fields[0].GetUInt32();
            uint64 unixtimems = fields[1].GetUInt64();

            std::shared_ptr<SniffedEvent_QuestUpdateFailed> newEvent = std::make_shared<SniffedEvent_QuestUpdateFailed>(questId);
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

        } while (result->NextRow());
    }
}

void SniffedEvent_QuestUpdateFailed::Execute() const
{
    Player* pPlayer = sReplayMgr.GetActivePlayer();
    if (!pPlayer)
    {
        sLog.outError("SniffedEvent_QuestUpdateFailed: Cannot find active player!");
        return;
    }
    std::string txt = "Quest " + sReplayMgr.GetQuestName(m_questId) + " (Entry: " + std::to_string(m_questId) + ") marked as failed.";
    pPlayer->MonsterSay(txt.c_str());
}

std::shared_ptr<WaypointPath> ReplayMgr::GetOrCreateWaypoints(uint32 guid, bool useStartPosition)
{
    // Only generate path if we haven't done it before.
    auto existingPathsItr = m_creatureWaypoints.find(guid);
    if (existingPathsItr != m_creatureWaypoints.end())
        return existingPathsItr->second;

    uint32 firstMoveTime = 0;
    std::shared_ptr<WaypointPath> path = std::make_shared<WaypointPath>();
    //                                              0       1        2            3               4               5                   6                   7                   8                 9                 10                11             12
    if (auto result = SniffDatabase.PQuery("SELECT `guid`, `point`, `move_time`, `spline_flags`, `spline_count`, `start_position_x`, `start_position_y`, `start_position_z`, `end_position_x`, `end_position_y`, `end_position_z`, `orientation`, `unixtime` FROM `creature_movement_server` WHERE `guid`=%u", guid))
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
    if (auto result = SniffDatabase.PQuery("SELECT COUNT(`point`) FROM `creature_movement_server` WHERE `guid`=%u", guid))
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
