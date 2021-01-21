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

#include "Policies/SingletonImp.h"
#include "ClassicDefines.h"
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
#include "Language.h"

INSTANTIATE_SINGLETON_1(ReplayMgr);

void ReplayMgr::LoadPlayers()
{
    uint32 count = 0;

    //                                                               0       1      2             3             4             5              6       7       8        9         10       11    12       13               14               15              16       17            18                   19                  20         21            22                23            24              25          26            27             28             29           30              31           32                 33            34            35           36                37            38                 39                 40              41                       42                      43                     44                45
    std::unique_ptr<QueryResult> result(SniffDatabase.Query("SELECT `guid`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `name`, `race`, `class`, `gender`, `level`, `xp`, `money`, `player_bytes1`, `player_bytes2`, `player_flags`, `scale`, `display_id`, `native_display_id`, `mount_display_id`, `faction`, `unit_flags`, `current_health`, `max_health`, `current_mana`, `max_mana`, `aura_state`, `emote_state`, `stand_state`, `vis_flags`, `sheath_state`, `pvp_flags`, `shapeshift_form`, `move_flags`, `speed_walk`, `speed_run`, `speed_run_back`, `speed_swim`, `speed_swim_back`, `bounding_radius`, `combat_reach`, `main_hand_attack_time`, `off_hand_attack_time`, `ranged_attack_time`, `equipment_cache`, `auras` FROM `player`"));

    if (!result)
    {
        BarGoLink bar(1);
        bar.step();

        sLog.outString();
        sLog.outErrorDb(">> Loaded 0 player definitions. DB table `player` is empty.");
        return;
    }

    BarGoLink bar(result->GetRowCount());

    do
    {
        Field* fields = result->Fetch();
        bar.step();

        uint32 guid = fields[0].GetUInt32();
        CharacterTemplateEntry& character = m_characterTemplates[guid];
        character.guid = guid;
        character.position.mapId = fields[1].GetUInt16();
        character.position.x = fields[2].GetFloat();
        character.position.y = fields[3].GetFloat();
        character.position.z = fields[4].GetFloat();
        character.position.o = fields[5].GetFloat();
        character.name = fields[6].GetCppString();
        character.raceId = fields[7].GetUInt8();
        character.classId = fields[8].GetUInt8();

        if (!sChrRacesStore.LookupEntry(character.raceId) ||
            !sChrClassesStore.LookupEntry(character.classId))
        {
            sLog.outError("ReplayMgr::LoadPlayers - Invalid race or class for character %s (GUID %u)", character.name.c_str(), guid);
            character.raceId = RACE_HUMAN;
            character.classId = CLASS_PALADIN;
        }

        character.gender = fields[9].GetUInt8();
        character.level = fields[10].GetUInt8();
        if (!character.level)
        {
            sLog.outError("ReplayMgr::LoadPlayers - Invalid level for character %s (GUID %u)", character.name.c_str(), guid);
            character.level = 1;
        }
        character.playerBytes = fields[13].GetUInt32();
        character.playerBytes2 = fields[14].GetUInt32();
        character.playerFlags = fields[15].GetUInt32();
        character.scale = fields[16].GetFloat();

        character.display_id = fields[17].GetUInt32();
        if (!sCreatureDisplayInfoStore.LookupEntry(character.display_id))
        {
            sLog.outError("ReplayMgr::LoadPlayers - Invalid display id for character %s (GUID %u)", character.name.c_str(), guid);
            character.display_id = 0;
        }
        character.native_display_id = fields[18].GetUInt32();
        if (!sCreatureDisplayInfoStore.LookupEntry(character.native_display_id))
        {
            sLog.outError("ReplayMgr::LoadPlayers - Invalid native display id for character %s (GUID %u)", character.name.c_str(), guid);
            character.native_display_id = 0;
        }

        character.mount_display_id = fields[19].GetUInt32();
        if (character.mount_display_id && !sCreatureDisplayInfoStore.LookupEntry(character.mount_display_id))
        {
            sLog.outError("ReplayMgr::LoadPlayers - Invalid mount display id for character %s (GUID %u)", character.name.c_str(), guid);
            character.mount_display_id = 0;
        }

        character.faction = fields[20].GetUInt32();
        if (!sObjectMgr.GetFactionTemplateEntry(character.faction))
        {
            sLog.outError("ReplayMgr::LoadPlayers - Invalid faction id for character %s (GUID %u)", character.name.c_str(), guid);
            character.faction = 0;
        }

        character.unit_flags = fields[21].GetUInt32();
        character.current_health = fields[22].GetUInt32();
        character.max_health = fields[23].GetUInt32();
        character.current_mana = fields[24].GetUInt32();
        character.max_mana = fields[25].GetUInt32();
        character.aura_state = fields[26].GetUInt32();
        character.emote_state = fields[27].GetUInt32();
        if (character.emote_state && !sEmotesStore.LookupEntry(character.emote_state))
        {
            sLog.outError("ReplayMgr::LoadPlayers - Invalid emote state for character %s (GUID %u)", character.name.c_str(), guid);
            character.emote_state = 0;
        }

        character.stand_state = fields[28].GetUInt8();
        if (character.stand_state >= MAX_UNIT_STAND_STATE)
        {
            sLog.outError("ReplayMgr::LoadPlayers - Invalid stand state for character %s (GUID %u)", character.name.c_str(), guid);
            character.stand_state = UNIT_STAND_STATE_STAND;
        }

        character.vis_flags = fields[29].GetUInt8();

        character.sheath_state = fields[30].GetUInt8();
        if (character.sheath_state >= MAX_SHEATH_STATE)
        {
            sLog.outError("ReplayMgr::LoadPlayers - Invalid sheath state for character %s (GUID %u)", character.name.c_str(), guid);
            character.sheath_state = SHEATH_STATE_UNARMED;
        }

        character.shapeshift_form = fields[32].GetUInt8();
        character.move_flags = ConvertMovementFlags(fields[33].GetUInt32());
        character.speed_walk = fields[34].GetFloat();
        character.speed_run = fields[35].GetFloat();
        character.speed_run_back = fields[36].GetFloat();
        character.speed_swim = fields[37].GetFloat();
        character.speed_swim_back = fields[38].GetFloat();
        character.bounding_radius = fields[39].GetFloat();
        character.combat_reach = fields[40].GetFloat();
        character.main_hand_attack_time = fields[41].GetUInt32();
        character.off_hand_attack_time = fields[42].GetUInt32();
        character.ranged_attack_time = fields[43].GetUInt32();
        std::string equipmentCache = fields[44].GetCppString();
        std::string temp;
        bool isItemId = true;
        uint32 itemCounter = 0;
        uint32 enchantCounter = 0;
        for (char chr : equipmentCache)
        {
            if (isdigit(chr))
                temp += chr;
            else
            {
                uint32 itemOrEnchantId = atoi(temp.c_str());
                if (isItemId)
                {
                    if (itemOrEnchantId && !sObjectMgr.GetItemPrototype(itemOrEnchantId))
                    {
                        sLog.outError("Non existent item id = %u on sniffed character with guid = %u.", itemOrEnchantId, guid);
                        itemOrEnchantId = 0;
                    }
                    character.equipment[itemCounter].itemId = itemOrEnchantId;
                    itemCounter++;
                }
                else
                {
                    character.equipment[enchantCounter].enchantId = itemOrEnchantId;
                    enchantCounter++;
                }
                isItemId = !isItemId;
                temp.clear();
            }
        }
        ++count;
    }
    while (result->NextRow());

    sLog.outString(">> Loaded %u sniffed character templates", count);
}

void ReplayMgr::LoadInitialPlayerGuidValues()
{
    std::unique_ptr<QueryResult> result(SniffDatabase.Query("SELECT `guid`, `charm_guid`, `charm_id`, `charm_type`, `summon_guid`, `summon_id`, `summon_type`, `charmer_guid`, `charmer_id`, `charmer_type`, `creator_guid`, `creator_id`, `creator_type`, `summoner_guid`, `summoner_id`, `summoner_type`, `target_guid`, `target_id`, `target_type` FROM `player_guid_values`"));

    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();

        uint32 guid = fields[0].GetUInt32();
        auto itr = m_characterTemplates.find(guid);
        if (itr == m_characterTemplates.end())
            continue;

        CharacterTemplateEntry& character = itr->second;

        {
            uint32 charmGuid = fields[1].GetUInt32();
            uint32 charmId = fields[2].GetUInt32();
            std::string charmType = fields[3].GetCppString();
            character.charmGuid = KnownObject(charmGuid, charmId, GetKnownObjectTypeId(charmType));

        }
        
        {
            uint32 summonGuid = fields[4].GetUInt32();
            uint32 summonId = fields[5].GetUInt32();
            std::string summonType = fields[6].GetCppString();
            character.summonGuid = KnownObject(summonGuid, summonId, GetKnownObjectTypeId(summonType));
        }
        
        {
            uint32 charmerGuid = fields[7].GetUInt32();
            uint32 charmerId = fields[8].GetUInt32();
            std::string charmerType = fields[9].GetCppString();
            character.charmerGuid = KnownObject(charmerGuid, charmerId, GetKnownObjectTypeId(charmerType));
        }

        {
            uint32 creatorGuid = fields[10].GetUInt32();
            uint32 creatorId = fields[11].GetUInt32();
            std::string creatorType = fields[12].GetCppString();
            character.creatorGuid = KnownObject(creatorGuid, creatorId, GetKnownObjectTypeId(creatorType));
        }

        {
            uint32 summonerGuid = fields[13].GetUInt32();
            uint32 summonerId = fields[14].GetUInt32();
            std::string summonerType = fields[15].GetCppString();
            character.summonerGuid = KnownObject(summonerGuid, summonerId, GetKnownObjectTypeId(summonerType));
        }

        {
            uint32 targetGuid = fields[16].GetUInt32();
            uint32 targetId = fields[17].GetUInt32();
            std::string targetType = fields[18].GetCppString();
            character.targetGuid = KnownObject(targetGuid, targetId, GetKnownObjectTypeId(targetType));
        }
        
    } while (result->NextRow());
}

uint16 ConvertMovementOpcode(std::string const& opcodeName)
{
    if (opcodeName == "CMSG_MOVE_FALL_LAND")
        return MSG_MOVE_FALL_LAND;
    else if (opcodeName == "CMSG_MOVE_FALL_RESET")
        return MSG_MOVE_FALL_LAND;
    else if (opcodeName == "CMSG_MOVE_HEARTBEAT")
        return MSG_MOVE_HEARTBEAT;
    else if (opcodeName == "CMSG_MOVE_JUMP")
        return MSG_MOVE_JUMP;
    else if (opcodeName == "CMSG_MOVE_SET_FACING")
        return MSG_MOVE_SET_FACING;
    else if (opcodeName == "CMSG_MOVE_SPLINE_DONE")
        return MSG_MOVE_STOP;
    else if (opcodeName == "CMSG_MOVE_START_BACKWARD")
        return MSG_MOVE_START_BACKWARD;
    else if (opcodeName == "CMSG_MOVE_START_FORWARD")
        return MSG_MOVE_START_FORWARD;
    else if (opcodeName == "CMSG_MOVE_START_STRAFE_LEFT")
        return MSG_MOVE_START_STRAFE_LEFT;
    else if (opcodeName == "CMSG_MOVE_START_STRAFE_RIGHT")
        return MSG_MOVE_START_STRAFE_RIGHT;
    else if (opcodeName == "CMSG_MOVE_START_TURN_LEFT")
        return MSG_MOVE_START_TURN_LEFT;
    else if (opcodeName == "CMSG_MOVE_START_TURN_RIGHT")
        return MSG_MOVE_START_TURN_RIGHT;
    else if (opcodeName == "CMSG_MOVE_STOP")
        return MSG_MOVE_STOP;
    else if (opcodeName == "CMSG_MOVE_STOP_STRAFE")
        return MSG_MOVE_STOP_STRAFE;
    else if (opcodeName == "CMSG_MOVE_STOP_TURN")
        return MSG_MOVE_STOP_TURN;

    return MSG_MOVE_HEARTBEAT;
}

void ReplayMgr::LoadCharacterMovements()
{
    uint32 count = 0;

    //                                                               0       1         2            3             4      5             6             7             8              9
    std::unique_ptr<QueryResult> result(SniffDatabase.Query("SELECT `guid`, `opcode`, `move_time`, `move_flags`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `unixtimems` FROM `player_movement_client`"));

    if (!result)
    {
        BarGoLink bar(1);
        bar.step();

        sLog.outString();
        sLog.outErrorDb(">> Loaded 0 character movements. DB table `player_movement_client` is empty.");
        return;
    }

    BarGoLink bar(result->GetRowCount());

    do
    {
        Field* fields = result->Fetch();
        bar.step();

        std::string opcodeName = fields[1].GetCppString();
        uint16 opcode = ConvertMovementOpcode(opcodeName);
        if (!opcode)
            continue;

        uint32 mapId = fields[4].GetUInt32();
        if (!sMapStorage.LookupEntry<MapEntry>(mapId))
            continue;

        uint32 guid = fields[0].GetUInt32();
        uint64 unixtimems = fields[9].GetUInt64();
        CharacterMovementEntry& moveData = m_characterMovements[guid][unixtimems];
        moveData.opcode = opcode;
        moveData.moveTime = fields[2].GetUInt32();
        moveData.moveFlags = ConvertMovementFlags(fields[3].GetUInt32());
        moveData.position.mapId = mapId;
        moveData.position.x = fields[5].GetFloat();
        moveData.position.y = fields[6].GetFloat();
        moveData.position.z = fields[7].GetFloat();
        moveData.position.o = fields[8].GetFloat();

        ++count;
    } while (result->NextRow());

    sLog.outString(">> Loaded %u sniffed character movements", count);
}

void ReplayMgr::LoadCreatureClientSideMovement()
{
    //                                                               0       1         2            3             4      5             6             7             8              9
    std::unique_ptr<QueryResult> result(SniffDatabase.Query("SELECT `guid`, `opcode`, `move_time`, `move_flags`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `unixtimems` FROM `creature_movement_client`"));
    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();

        uint32 guid = fields[0].GetUInt32();
        uint32 creatureId = GetCreatureEntryFromGuid(guid);

        std::string opcodeName = fields[1].GetCppString();
        uint16 opcode = ConvertMovementOpcode(opcodeName);
        if (!opcode)
            continue;

        uint32 mapId = fields[4].GetUInt32();
        if (!sMapStorage.LookupEntry<MapEntry>(mapId))
            continue;

        uint64 unixtimems = fields[9].GetUInt64();
        uint32 moveTime = fields[2].GetUInt32();
        uint32 moveFlags = ConvertMovementFlags(fields[3].GetUInt32());
        float x = fields[5].GetFloat();
        float y = fields[6].GetFloat();
        float z = fields[7].GetFloat();
        float o = fields[8].GetFloat();

        std::shared_ptr<SniffedEvent_ClientSideMovement> newEvent = std::make_shared<SniffedEvent_ClientSideMovement>(guid, creatureId, opcode, moveTime, moveFlags, x, y, z, o);
        m_eventsMap.insert(std::make_pair(unixtimems, newEvent));

    } while (result->NextRow());
}

void SniffedEvent_ClientSideMovement::Execute() const
{
    Creature* pCreature = sReplayMgr.GetCreature(m_guid);
    if (!pCreature)
    {
        sLog.outError("SniffedEvent_ClientSideMovement: Cannot find source creature!");
        return;
    }

    pCreature->GetMap()->CreatureRelocation(pCreature, m_x, m_y, m_z, m_o);
    pCreature->m_movementInfo.SetMovementFlags(MovementFlags(m_moveFlags));
    pCreature->m_movementInfo.UpdateTime(m_moveTime);
    WorldPacket data(m_opcode);
#if SUPPORTED_CLIENT_BUILD > CLIENT_BUILD_1_8_4
    data << pCreature->GetPackGUID();
#else
    data << pCreature->GetGUID();
#endif
    data << pCreature->m_movementInfo;
    pCreature->SendMovementMessageToSet(std::move(data), false);
}

void ReplayMgr::LoadActivePlayer()
{
    uint32 count = 0;

    //                                                               0       1
    std::unique_ptr<QueryResult> result(SniffDatabase.Query("SELECT `guid`, `unixtime` FROM `player_active_player`"));

    if (!result)
    {
        BarGoLink bar(1);
        bar.step();

        sLog.outString();
        sLog.outErrorDb(">> No active player in sniff.");
        return;
    }

    BarGoLink bar(result->GetRowCount());

    std::set<uint32> activePlayers;

    do
    {
        Field* fields = result->Fetch();
        bar.step();

        uint32 guid = fields[0].GetUInt32();
        uint32 unixtime = fields[1].GetUInt32();

        activePlayers.insert(guid);
        m_activePlayers.insert({ unixtime, guid });

    } while (result->NextRow());
    sLog.outString(">> Loaded %u active players", (uint32)activePlayers.size());
}

extern std::map<uint32, uint32> g_defaultWorldStates;

void ReplayMgr::LoadInitialWorldStates()
{
    uint32 count = 0;

    //                                                               0             1           2
    std::unique_ptr<QueryResult> result(SniffDatabase.Query("SELECT `unixtimems`, `variable`, `value` FROM `world_state_init` ORDER BY `unixtimems`"));

    if (!result)
    {
        BarGoLink bar(1);
        bar.step();

        sLog.outString();
        sLog.outErrorDb(">> No initial world state data.");
        return;
    }

    BarGoLink bar(result->GetRowCount());

    do
    {
        Field* fields = result->Fetch();
        bar.step();

        uint64 unixtimems = fields[0].GetUInt64();
        uint32 unixtime = uint32(unixtimems / IN_MILLISECONDS);
        uint32 variable = fields[1].GetUInt32();
        uint32 value = fields[2].GetUInt32();

        if (m_initialWorldStateSendTime == 0)
            m_initialWorldStateSendTime = unixtime;

        if (m_initialWorldStateSendTime == unixtime)
        {
            count++;
            m_initialWorldStates[variable] = value;
            g_defaultWorldStates[variable] = value;
        }
        else
        {
            std::shared_ptr<SniffedEvent_WorldStateUpdate> newEvent = std::make_shared<SniffedEvent_WorldStateUpdate>(variable, value, true);
            m_eventsMap.insert(std::make_pair(unixtimems, newEvent));
        }

    } while (result->NextRow());
    sLog.outString(">> Loaded %u initial world states", count);
}

void ReplayMgr::SpawnCharacters()
{
    uint32 maxGuid = 0;
    for (const auto& itr : m_characterTemplates)
    {
        CharacterMovementMap const* movementMap = nullptr;
        auto movement = m_characterMovements.find(itr.first);
        if (movement != m_characterMovements.end())
            movementMap = &movement->second;
        if (itr.first > maxGuid)
            maxGuid = itr.first;
        ReplayBotAI* ai = new ReplayBotAI(itr.first, &itr.second, movementMap);
        m_playerBots[itr.first] = ai;
        sPlayerBotMgr.AddBot(ai);
    }

    // Spawn chat bots, for characters that were never seen, but said something in chat.
    std::unique_ptr<QueryResult> result(SniffDatabase.Query("SELECT DISTINCT `sender_name` FROM `player_chat` WHERE `guid`=0"));
    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            std::string name = fields[0].GetCppString();
            if (name.empty())
                continue;

            uint32 guid = ++maxGuid;
            CharacterTemplateEntry& character = m_characterTemplates[guid];
            character.name = name;
            character.classId = CLASS_WARRIOR;
            character.raceId = RACE_HUMAN;
            character.level = PLAYER_MAX_LEVEL;
            character.current_health = 100;
            character.max_health = 100;
            ReplayBotAI* ai = new ReplayBotAI(guid, &character, nullptr);
            m_playerBots[guid] = ai;
            sPlayerBotMgr.AddBot(ai);
        } while (result->NextRow());
    }

    sLog.outString("[ReplayMgr] All characters spawned");
}

Player* ReplayMgr::GetActivePlayer()
{
    uint32 currentCharacterGuid = 0;
    for (const auto& itr : m_activePlayers)
    {
        if (itr.first < m_currentSniffTime)
            currentCharacterGuid = itr.second;
        else
            break;
    }

    return GetPlayer(currentCharacterGuid);
}

bool ReplayMgr::GetCurrentClientPosition(WorldLocation& loc)
{
    if (Player* pPlayer = GetActivePlayer())
    {
        loc.mapId = pPlayer->GetMapId();
        loc.x = pPlayer->GetPositionX();
        loc.y = pPlayer->GetPositionY();
        loc.z = pPlayer->GetPositionZ();
        loc.o = pPlayer->GetOrientation();
        return true;
    }
    
    return false;
}

uint32 ReplayMgr::GetCreatureEntryFromGuid(uint32 guid)
{
    if (auto pSpawn = sObjectMgr.GetCreatureData(guid))
        return pSpawn->creature_id[0];
    return 0;
}

uint32 ReplayMgr::GetGameObjectEntryFromGuid(uint32 guid)
{
    if (auto pSpawn = sObjectMgr.GetGOData(guid))
        return pSpawn->id;
    return 0;
}

char const* ReplayMgr::GetCreatureName(uint32 entry)
{
    if (auto data = sObjectMgr.GetCreatureTemplate(entry))
        return data->name;
    return "Unknown Creature";
}

char const* ReplayMgr::GetGameObjectName(uint32 entry)
{
    if (auto data = sObjectMgr.GetGameObjectInfo(entry))
        return data->name;
    return "Unknown GameObject";
}

char const* ReplayMgr::GetItemName(uint32 entry)
{
    if (auto data = sObjectMgr.GetItemPrototype(entry))
        return data->Name1;
    return "Unknown Item";
}

std::string ReplayMgr::GetQuestName(uint32 entry)
{
    if (auto data = sObjectMgr.GetQuestTemplate(entry))
        return data->GetTitle();
    return "Unknown Quest";
}

void ReplayMgr::Update(uint32 const diff)
{
    if (!m_enabled)
        return;

    // Wait for player bots to spawn before starting replay.
    if (!m_playersSpawned && !m_characterTemplates.empty())
        return;

    uint64 oldSniffTimeMs = m_currentSniffTimeMs;
    m_currentSniffTimeMs += diff;
    m_currentSniffTime = m_currentSniffTimeMs / 1000;

    for (const auto& itr : m_eventsMap)
    {
        if (itr.first <= oldSniffTimeMs)
            continue;

        if (itr.first > m_currentSniffTimeMs)
            return;

        itr.second->Execute();
    }

    if (m_currentSniffTimeMs > m_eventsMap.rbegin()->first)
    {
        sLog.outInfo("[ReplayMgr] Sniff replay is over.");
        m_enabled = false;
    }
}

void ReplaySetTimePlayerWorker::run()
{
    sReplayMgr.UpdatePlayersForCurrentTime();
}

void ReplayMgr::ResetPlayerToInitialState(Player* pPlayer, CharacterTemplateEntry const& initialState)
{
    if (pPlayer->GetVisibility() != VISIBILITY_OFF)
        pPlayer->SetVisibility(VISIBILITY_OFF);

    // clear all aura related fields
    for (int i = UNIT_FIELD_AURA; i <= UNIT_FIELD_AURASTATE; ++i)
        pPlayer->SetUInt32Value(i, 0);

    // equip initial gear
    for (uint32 i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
    {
        if (Item* pItem = pPlayer->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            pPlayer->DestroyItem(INVENTORY_SLOT_BAG_0, i, true);

        if (uint32 itemId = initialState.equipment[i].itemId)
        {
            if (ItemPrototype const* pItem = sObjectMgr.GetItemPrototype(itemId))
            {
                pPlayer->SatisfyItemRequirements(pItem);
                uint16 eDest = ((INVENTORY_SLOT_BAG_0 << 8) | i);
                pPlayer->EquipNewItem(eDest, itemId, true);
            }
        }
    }

    pPlayer->SetUInt32Value(UNIT_FIELD_LEVEL, initialState.level);

    if (initialState.native_display_id)
        pPlayer->SetUInt32Value(UNIT_FIELD_NATIVEDISPLAYID, initialState.display_id);

    if (initialState.display_id)
        pPlayer->SetUInt32Value(UNIT_FIELD_DISPLAYID, initialState.display_id);
    else
        pPlayer->SetUInt32Value(UNIT_FIELD_DISPLAYID, pPlayer->GetNativeDisplayId());

    pPlayer->SetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID, initialState.mount_display_id);

    if (initialState.scale)
    {
        float defaultScale = (initialState.scale * Creature::GetScaleForDisplayId(pPlayer->GetDisplayId()));
        pPlayer->SetObjectScale(defaultScale);
    }
    else
        pPlayer->SetObjectScale(pPlayer->GetNativeScale());

    if (initialState.faction)
        pPlayer->SetFactionTemplateId(initialState.faction);
    else
        pPlayer->SetFactionForRace(pPlayer->GetRace());

    pPlayer->SetUInt32Value(UNIT_FIELD_FLAGS, initialState.unit_flags);

    pPlayer->SetMaxHealth(initialState.max_health);
    pPlayer->SetHealth(initialState.current_health);
    pPlayer->SetMaxPower(POWER_MANA, initialState.max_mana);
    pPlayer->SetPower(POWER_MANA, initialState.current_mana);

    pPlayer->SetUInt32Value(UNIT_FIELD_AURASTATE, initialState.aura_state);
    pPlayer->SetUInt32Value(UNIT_NPC_EMOTESTATE, initialState.emote_state);
    pPlayer->SetStandState(initialState.stand_state);
    pPlayer->SetByteValue(UNIT_FIELD_BYTES_1, UNIT_BYTES_1_OFFSET_VIS_FLAG, initialState.vis_flags);
    pPlayer->SetSheath(SheathState(initialState.sheath_state));
    pPlayer->SetByteValue(UNIT_FIELD_BYTES_1, UNIT_BYTES_1_OFFSET_SHAPESHIFT_FORM, initialState.shapeshift_form);
    pPlayer->SetUnitMovementFlags(initialState.move_flags);

    if (pPlayer->GetSpeedRate(MOVE_WALK) != initialState.speed_walk)
        pPlayer->SetSpeedRateDirect(MOVE_WALK, initialState.speed_walk);
    if (pPlayer->GetSpeedRate(MOVE_RUN) != initialState.speed_run)
        pPlayer->SetSpeedRateDirect(MOVE_RUN, initialState.speed_run);
    if (pPlayer->GetSpeedRate(MOVE_RUN_BACK) != initialState.speed_run_back)
        pPlayer->SetSpeedRateDirect(MOVE_RUN_BACK, initialState.speed_run_back);
    if (pPlayer->GetSpeedRate(MOVE_SWIM) != initialState.speed_swim)
        pPlayer->SetSpeedRateDirect(MOVE_SWIM, initialState.speed_swim);
    if (pPlayer->GetSpeedRate(MOVE_SWIM_BACK) != initialState.speed_swim_back)
        pPlayer->SetSpeedRateDirect(MOVE_SWIM_BACK, initialState.speed_swim_back);

    pPlayer->SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS, initialState.bounding_radius);
    pPlayer->SetFloatValue(UNIT_FIELD_COMBATREACH, initialState.combat_reach);
    pPlayer->SetFloatValue(UNIT_FIELD_BASEATTACKTIME, initialState.main_hand_attack_time);
    pPlayer->SetFloatValue(UNIT_FIELD_OFFHANDATTACKTIME, initialState.off_hand_attack_time);
    pPlayer->SetFloatValue(UNIT_FIELD_RANGEDATTACKTIME, initialState.ranged_attack_time);

    ObjectGuid charmGuid;
    if (!initialState.charmGuid.IsEmpty())
        if (WorldObject* pObject = GetStoredObject(initialState.charmGuid))
            charmGuid = pObject->GetObjectGuid();
    pPlayer->SetGuidValue(UNIT_FIELD_CHARM, charmGuid);

    ObjectGuid summonGuid;
    if (!initialState.summonGuid.IsEmpty())
    {
        if (WorldObject* pObject = GetStoredObject(initialState.summonGuid))
        {
            summonGuid = pObject->GetObjectGuid();

            // Assign creator and summoner to pet when updating player,
            // since the players take a second to spawn, and it won't
            // be able to find the owner at the first creature update.
            if (Creature* pCreature = pObject->ToCreature())
            {
                if (CreatureData const* pData = pCreature->GetCreatureData())
                {
                    if (pData->summonerGuid.m_type == TYPEID_PLAYER &&
                        pData->summonerGuid.m_guid == initialState.guid)
                    {
                        pCreature->SetGuidValue(UNIT_FIELD_SUMMONEDBY, pPlayer->GetObjectGuid());
                    }
                    if (pData->creatorGuid.m_type == TYPEID_PLAYER &&
                        pData->creatorGuid.m_guid == initialState.guid)
                    {
                        pCreature->SetGuidValue(UNIT_FIELD_CREATEDBY, pPlayer->GetObjectGuid());
                    }
                }
            }
        }
            
    }
    pPlayer->SetGuidValue(UNIT_FIELD_SUMMON, summonGuid);

    ObjectGuid charmerGuid;
    if (!initialState.charmerGuid.IsEmpty())
        if (WorldObject* pObject = GetStoredObject(initialState.charmerGuid))
            charmerGuid = pObject->GetObjectGuid();
    pPlayer->SetGuidValue(UNIT_FIELD_CHARMEDBY, charmerGuid);

    ObjectGuid creatorGuid;
    if (!initialState.creatorGuid.IsEmpty())
        if (WorldObject* pObject = GetStoredObject(initialState.creatorGuid))
            creatorGuid = pObject->GetObjectGuid();
    pPlayer->SetGuidValue(UNIT_FIELD_CREATEDBY, creatorGuid);

    ObjectGuid summonerGuid;
    if (!initialState.summonerGuid.IsEmpty())
        if (WorldObject* pObject = GetStoredObject(initialState.summonerGuid))
            summonerGuid = pObject->GetObjectGuid();
    pPlayer->SetGuidValue(UNIT_FIELD_SUMMONEDBY, summonerGuid);

    ObjectGuid targetGuid;
    if (!initialState.targetGuid.IsEmpty())
        if (WorldObject* pObject = GetStoredObject(initialState.targetGuid))
            targetGuid = pObject->GetObjectGuid();
    pPlayer->SetGuidValue(UNIT_FIELD_TARGET, targetGuid);

    pPlayer->SetChannelObjectGuid(ObjectGuid());
    pPlayer->SetUInt32Value(UNIT_CHANNEL_SPELL, 0);
}

void ReplayMgr::UpdatePlayerToCurrentState(Player* pPlayer, CharacterTemplateEntry const& initialState)
{
    UnitVisibility visible = pPlayer->GetVisibility();

    for (const auto& itr : m_eventsMap)
    {
        if (itr.first > m_currentSniffTimeMs)
            break;

        if (itr.second->GetSourceObject().m_type != TYPEID_PLAYER ||
            itr.second->GetSourceObject().m_guid != initialState.guid)
            continue;

        switch (itr.second->GetType())
        {
            case SE_UNIT_CREATE1:
            case SE_UNIT_CREATE2:
            {
                visible = VISIBILITY_ON;
                break;
            }
            case SE_UNIT_DESTROY:
            {
                visible = VISIBILITY_OFF;
                break;
            }
            case SE_UNIT_UPDATE_ENTRY:
            case SE_UNIT_UPDATE_SCALE:
            case SE_UNIT_UPDATE_DISPLAY_ID:
            case SE_UNIT_UPDATE_MOUNT:
            case SE_UNIT_UPDATE_FACTION:
            case SE_UNIT_UPDATE_LEVEL:
            case SE_UNIT_UPDATE_AURA_STATE:
            case SE_UNIT_UPDATE_EMOTE_STATE:
            case SE_UNIT_UPDATE_STAND_STATE:
            case SE_UNIT_UPDATE_VIS_FLAGS:
            case SE_UNIT_UPDATE_SHEATH_STATE:
            case SE_UNIT_UPDATE_SHAPESHIFT_FORM:
            case SE_UNIT_UPDATE_NPC_FLAGS:
            case SE_UNIT_UPDATE_UNIT_FLAGS:
            case SE_UNIT_UPDATE_CURRENT_HEALTH:
            case SE_UNIT_UPDATE_MAX_HEALTH:
            case SE_UNIT_UPDATE_CURRENT_MANA:
            case SE_UNIT_UPDATE_MAX_MANA:
            case SE_UNIT_UPDATE_BOUNDING_RADIUS:
            case SE_UNIT_UPDATE_COMBAT_REACH:
            case SE_UNIT_UPDATE_MAIN_HAND_ATTACK_TIME:
            case SE_UNIT_UPDATE_OFF_HAND_ATTACK_TIME:
            case SE_UNIT_UPDATE_SPEED:
            case SE_UNIT_UPDATE_GUID_VALUE:
            case SE_UNIT_UPDATE_AURAS:
            case SE_SPELL_CHANNEL_START:
            case SE_SPELL_CHANNEL_UPDATE:
            {
                itr.second->Execute();
                break;
            }
        }
    }

    auto itr = m_characterMovements.find(initialState.guid);
    if (itr != m_characterMovements.end())
    {
        WorldLocation position = initialState.position;
        for (const auto itr2 : itr->second)
        {
            if (itr2.first > m_currentSniffTimeMs)
                break;

            position = itr2.second.position;
        }

        if ((pPlayer->GetMapId() != position.mapId) || (pPlayer->GetPosition() != position.ToPosition()))
        {
            pPlayer->TeleportTo(position);
        }
    }

    pPlayer->SetVisibility(visible);
}

void ReplayMgr::UpdatePlayersForCurrentTime()
{
    if (m_initialized)
    {
        for (const auto& itr : m_characterTemplates)
        {
            auto const& initialState = itr.second;

            if (Player* pPlayer = GetPlayer(itr.first))
            {
                ResetPlayerToInitialState(pPlayer, initialState);
                UpdatePlayerToCurrentState(pPlayer, initialState);
            }
        }
    }
}

void ReplaySetTimeCreatureWorker::run()
{
    sReplayMgr.UpdateCreaturesForCurrentTime();
}

void ReplayMgr::UpdateCreaturesForCurrentTime()
{
    std::map<uint32, Position> creaturePositions;
    for (const auto itr : m_creatures)
    {
        Creature* pCreature = itr.second;

        if (pCreature->GetVisibility() != VISIBILITY_OFF)
            pCreature->SetVisibility(VISIBILITY_OFF);

        // clear all aura related fields
        for (int i = UNIT_FIELD_AURA; i <= UNIT_FIELD_AURASTATE; ++i)
            pCreature->SetUInt32Value(i, 0);

        if (auto data = pCreature->GetCreatureData())
        {
            creaturePositions[itr.first] = data->position.ToPosition();

            if (!pCreature->IsAlive() && data->current_health > 0)
                pCreature->Respawn();
            else
            {
                if (pCreature->GetMaxHealth() != data->max_health)
                    pCreature->SetMaxHealth(data->max_health);
                if (pCreature->GetHealth() != data->current_health)
                    pCreature->SetHealth(data->current_health);
            } 

            if (pCreature->GetMaxPower(POWER_MANA) != data->max_mana)
                pCreature->SetMaxPower(POWER_MANA, data->max_mana);
            if (pCreature->GetPower(POWER_MANA) != data->current_mana)
                pCreature->SetPower(POWER_MANA, data->current_mana);

            if (pCreature->GetUInt32Value(OBJECT_FIELD_ENTRY) != data->creature_id[0])
                pCreature->SetUInt32Value(OBJECT_FIELD_ENTRY, data->creature_id[0]);

            if (data->display_id && pCreature->GetDisplayId() != data->display_id)
                pCreature->SetDisplayId(data->display_id);

            if (data->native_display_id && pCreature->GetNativeDisplayId() != data->native_display_id)
                pCreature->SetNativeDisplayId(data->native_display_id);

            if (pCreature->GetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID) != data->mount_display_id)
                pCreature->SetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID, data->mount_display_id);

            float defaultScale = (data->scale * Creature::GetScaleForDisplayId(pCreature->GetDisplayId()));
            if (pCreature->GetObjectScale() != defaultScale)
                pCreature->SetObjectScale(defaultScale);

            if (data->faction && pCreature->GetFactionTemplateId() != data->faction)
                pCreature->SetFactionTemplateId(data->faction);
            if (pCreature->GetUInt32Value(UNIT_FIELD_FLAGS) != data->unit_flags)
                pCreature->SetUInt32Value(UNIT_FIELD_FLAGS, data->unit_flags);
            if (pCreature->GetUInt32Value(UNIT_NPC_FLAGS) != data->npc_flags)
                pCreature->SetUInt32Value(UNIT_NPC_FLAGS, data->npc_flags);

            if (pCreature->GetUInt32Value(UNIT_FIELD_AURASTATE) != data->aura_state)
                pCreature->SetUInt32Value(UNIT_FIELD_AURASTATE, data->aura_state);
            if (pCreature->GetUInt32Value(UNIT_NPC_EMOTESTATE) != data->emote_state)
                pCreature->SetUInt32Value(UNIT_NPC_EMOTESTATE, data->emote_state);
            if (pCreature->GetStandState() != data->stand_state)
                pCreature->SetStandState(data->stand_state);
            pCreature->SetByteValue(UNIT_FIELD_BYTES_1, UNIT_BYTES_1_OFFSET_VIS_FLAG, data->vis_flags);
            if (pCreature->GetSheath() != data->sheath_state)
                pCreature->SetSheath(SheathState(data->sheath_state));
            pCreature->SetByteValue(UNIT_FIELD_BYTES_1, UNIT_BYTES_1_OFFSET_SHAPESHIFT_FORM, data->shapeshift_form);

            if (pCreature->GetSpeedRate(MOVE_WALK) != data->speed_walk)
                pCreature->SetSpeedRateDirect(MOVE_WALK, data->speed_walk);
            if (pCreature->GetSpeedRate(MOVE_RUN) != data->speed_run)
                pCreature->SetSpeedRateDirect(MOVE_RUN, data->speed_run);
            if (pCreature->GetSpeedRate(MOVE_RUN_BACK) != data->speed_run_back)
                pCreature->SetSpeedRateDirect(MOVE_RUN_BACK, data->speed_run_back);
            if (pCreature->GetSpeedRate(MOVE_SWIM) != data->speed_swim)
                pCreature->SetSpeedRateDirect(MOVE_SWIM, data->speed_swim);
            if (pCreature->GetSpeedRate(MOVE_SWIM_BACK) != data->speed_swim_back)
                pCreature->SetSpeedRateDirect(MOVE_SWIM_BACK, data->speed_swim_back);

            pCreature->SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS, data->bounding_radius);
            pCreature->SetFloatValue(UNIT_FIELD_COMBATREACH, data->combat_reach);
            pCreature->SetFloatValue(UNIT_FIELD_BASEATTACKTIME, data->main_hand_attack_time);
            pCreature->SetFloatValue(UNIT_FIELD_OFFHANDATTACKTIME, data->off_hand_attack_time);

            pCreature->SetVirtualItem(VIRTUAL_ITEM_SLOT_0, data->main_hand_slot_item);
            pCreature->SetVirtualItem(VIRTUAL_ITEM_SLOT_1, data->off_hand_slot_item);
            pCreature->SetVirtualItem(VIRTUAL_ITEM_SLOT_2, data->ranged_slot_item);

            ObjectGuid charmGuid;
            if (!data->charmGuid.IsEmpty())
                if (WorldObject* pObject = GetStoredObject(data->charmGuid))
                    charmGuid = pObject->GetObjectGuid();
            pCreature->SetGuidValue(UNIT_FIELD_CHARM, charmGuid);

            ObjectGuid summonGuid;
            if (!data->summonGuid.IsEmpty())
                if (WorldObject* pObject = GetStoredObject(data->summonGuid))
                    summonGuid = pObject->GetObjectGuid();
            pCreature->SetGuidValue(UNIT_FIELD_SUMMON, summonGuid);

            ObjectGuid charmerGuid;
            if (!data->charmerGuid.IsEmpty())
                if (WorldObject* pObject = GetStoredObject(data->charmerGuid))
                    charmerGuid = pObject->GetObjectGuid();
            pCreature->SetGuidValue(UNIT_FIELD_CHARMEDBY, charmerGuid);

            ObjectGuid creatorGuid;
            if (!data->creatorGuid.IsEmpty())
                if (WorldObject* pObject = GetStoredObject(data->creatorGuid))
                    creatorGuid = pObject->GetObjectGuid();
            pCreature->SetGuidValue(UNIT_FIELD_CREATEDBY, creatorGuid);

            ObjectGuid summonerGuid;
            if (!data->summonerGuid.IsEmpty())
                if (WorldObject* pObject = GetStoredObject(data->summonerGuid))
                    summonerGuid = pObject->GetObjectGuid();
            pCreature->SetGuidValue(UNIT_FIELD_SUMMONEDBY, summonerGuid);

            ObjectGuid targetGuid;
            if (!data->targetGuid.IsEmpty())
                if (WorldObject* pObject = GetStoredObject(data->targetGuid))
                    targetGuid = pObject->GetObjectGuid();
            pCreature->SetGuidValue(UNIT_FIELD_TARGET, targetGuid);
        }

        pCreature->SetChannelObjectGuid(ObjectGuid());
        pCreature->SetUInt32Value(UNIT_CHANNEL_SPELL, 0);
    }

    std::set<uint32> visibleCreatures;
    for (const auto& itr : m_eventsMap)
    {
        if (itr.first > m_currentSniffTimeMs)
            break;

        if (itr.second->GetSourceObject().m_type != TYPEID_UNIT)
            continue;

        switch (itr.second->GetType())
        {
            case SE_UNIT_CREATE1:
            {
                uint32 const guid = itr.second->GetSourceObject().m_guid;
                visibleCreatures.insert(guid);
                auto createEvent = std::static_pointer_cast<SniffedEvent_UnitCreate1>(itr.second);
                creaturePositions[guid] = Position(createEvent->m_x, createEvent->m_y, createEvent->m_z, createEvent->m_o);
                break;
            }
            case SE_UNIT_CREATE2:
            {
                uint32 const guid = itr.second->GetSourceObject().m_guid;
                visibleCreatures.insert(guid);
                auto createEvent = std::static_pointer_cast<SniffedEvent_UnitCreate2>(itr.second);
                creaturePositions[guid] = Position(createEvent->m_x, createEvent->m_y, createEvent->m_z, createEvent->m_o);
                break;
            }
            case SE_UNIT_DESTROY:
            {
                visibleCreatures.erase(itr.second->GetSourceObject().m_guid);
                break;
            }
            case SE_UNIT_SERVERSIDE_MOVEMENT:
            {
                uint32 const guid = itr.second->GetSourceObject().m_guid;
                auto moveEvent = std::static_pointer_cast<SniffedEvent_ServerSideMovement>(itr.second);
                creaturePositions[guid] = Position(moveEvent->m_x, moveEvent->m_y, moveEvent->m_z, moveEvent->m_o);
                break;
            }
            case SE_UNIT_UPDATE_ENTRY:
            case SE_UNIT_UPDATE_SCALE:
            case SE_UNIT_UPDATE_DISPLAY_ID:
            case SE_UNIT_UPDATE_MOUNT:
            case SE_UNIT_UPDATE_FACTION:
            case SE_UNIT_UPDATE_LEVEL:
            case SE_UNIT_UPDATE_AURA_STATE:
            case SE_UNIT_UPDATE_EMOTE_STATE:
            case SE_UNIT_UPDATE_STAND_STATE:
            case SE_UNIT_UPDATE_VIS_FLAGS:
            case SE_UNIT_UPDATE_SHEATH_STATE:
            case SE_UNIT_UPDATE_SHAPESHIFT_FORM:
            case SE_UNIT_UPDATE_NPC_FLAGS:
            case SE_UNIT_UPDATE_UNIT_FLAGS:
            case SE_UNIT_UPDATE_CURRENT_HEALTH:
            case SE_UNIT_UPDATE_MAX_HEALTH:
            case SE_UNIT_UPDATE_CURRENT_MANA:
            case SE_UNIT_UPDATE_MAX_MANA:
            case SE_UNIT_UPDATE_BOUNDING_RADIUS:
            case SE_UNIT_UPDATE_COMBAT_REACH:
            case SE_UNIT_UPDATE_MAIN_HAND_ATTACK_TIME:
            case SE_UNIT_UPDATE_OFF_HAND_ATTACK_TIME:
            case SE_UNIT_UPDATE_SPEED:
            case SE_UNIT_UPDATE_GUID_VALUE:
            case SE_UNIT_UPDATE_AURAS:
            case SE_SPELL_CHANNEL_START:
            case SE_SPELL_CHANNEL_UPDATE:
            {
                itr.second->Execute();
                break;
            }
        }
    }

    for (const auto& itr : creaturePositions)
    {
        if (Creature* pCreature = GetCreature(itr.first))
        {
            if (pCreature->IsInWorld() && pCreature->GetPosition() != itr.second)
            {
                pCreature->DisableSpline();
                pCreature->GetMap()->CreatureRelocation(pCreature, itr.second.x, itr.second.y, itr.second.z, itr.second.o);
            }
        }
    }

    for (const auto itr : visibleCreatures)
        if (Creature* pCreature = GetCreature(itr))
            pCreature->SetVisibility(VISIBILITY_ON);
}

void ReplayMgr::ResetGameObjectToInitialState(GameObject* pGo)
{
    if (auto data = pGo->GetGOData())
    {
        if (pGo->GetGoState() != data->go_state)
            pGo->SetGoState(GOState(data->go_state));
        if (pGo->GetUInt32Value(GAMEOBJECT_FLAGS) != data->flags)
            pGo->SetUInt32Value(GAMEOBJECT_FLAGS, data->flags);
        if (data->faction && pGo->GetUInt32Value(GAMEOBJECT_FACTION) != data->faction)
            pGo->SetUInt32Value(GAMEOBJECT_FACTION, data->faction);
        if (data->display_id && pGo->GetUInt32Value(GAMEOBJECT_DISPLAYID) != data->display_id)
            pGo->SetUInt32Value(GAMEOBJECT_DISPLAYID, data->display_id);
        if (pGo->GetUInt32Value(GAMEOBJECT_LEVEL) != data->level)
            pGo->SetUInt32Value(GAMEOBJECT_LEVEL, data->level);
        if (pGo->GetGoArtKit() != data->artkit)
            pGo->SetGoArtKit(data->artkit);

        ObjectGuid creatorGuid;
        if (!data->creatorGuid.IsEmpty())
            if (WorldObject* pObject = GetStoredObject(data->creatorGuid))
                creatorGuid = pObject->GetObjectGuid();
        pGo->SetGuidValue(OBJECT_FIELD_CREATED_BY, creatorGuid);
    }
}

void ReplaySetTimeGameObjectWorker::run()
{
    sReplayMgr.UpdateGameObjectsForCurrentTime();
}

void ReplayMgr::UpdateGameObjectsForCurrentTime()
{
    for (const auto itr : m_gameobjects)
    {
        if (itr.second->IsVisible())
            itr.second->SetVisible(false);

        ResetGameObjectToInitialState(itr.second);
    }

    std::set<uint32> visibleGameObjects;
    for (const auto& itr : m_eventsMap)
    {
        if (itr.first > m_currentSniffTimeMs)
            break;

        switch (itr.second->GetType())
        {
            case SE_GAMEOBJECT_CREATE1:
            case SE_GAMEOBJECT_CREATE2:
            {
                visibleGameObjects.insert(itr.second->GetSourceObject().m_guid);
                break;
            }
            case SE_GAMEOBJECT_DESTROY:
            {
                visibleGameObjects.erase(itr.second->GetSourceObject().m_guid);
                break;
            }
            case SE_GAMEOBJECT_UPDATE_FLAGS:
            case SE_GAMEOBJECT_UPDATE_STATE:
            {
                itr.second->Execute();
                break;
            }
        }
    }

    for (const auto itr : visibleGameObjects)
        if (GameObject* pGo = GetGameObject(itr))
            pGo->SetVisible(true);
}

void ReplayMgr::UpdateObjectStateAndVisiblityForCurrentTime()
{
    // Initial world states
    if (m_initialWorldStateSendTime)
    {
        g_defaultWorldStates.clear();
        for (const auto itr : m_initialWorldStates)
            g_defaultWorldStates[itr.first] = itr.second;
    }
    for (const auto& itr : m_eventsMap)
    {
        if (itr.first > m_currentSniffTimeMs)
            break;

        if (itr.second->GetType() == SE_WORLD_STATE_UPDATE)
        {
            auto updateEvent = std::static_pointer_cast<SniffedEvent_WorldStateUpdate>(itr.second);
            g_defaultWorldStates[updateEvent->m_variable] = updateEvent->m_value;
        }
    }

    std::unique_ptr<ACE_Based::Thread> pPlayerUpdater = std::make_unique<ACE_Based::Thread>(new ReplaySetTimePlayerWorker());
    std::unique_ptr<ACE_Based::Thread> pCreatureUpdater = std::make_unique<ACE_Based::Thread>(new ReplaySetTimeCreatureWorker());
    std::unique_ptr<ACE_Based::Thread> pGameObjectUpdater = std::make_unique<ACE_Based::Thread>(new ReplaySetTimeGameObjectWorker());
    pPlayerUpdater->wait();
    pCreatureUpdater->wait();
    pGameObjectUpdater->wait();
    pPlayerUpdater->destroy();
    pCreatureUpdater->destroy();
    pGameObjectUpdater->destroy();
}

void ReplayMgr::SetPlayTime(uint32 unixtime, bool updateObjectsState)
{
    uint32 const currentTime = time(nullptr);
    if (unixtime > currentTime)
    {
        sLog.outError("[ReplayMgr] Sniff time is later than current time!");
        return;
    }
    else
    {
        std::string message = "[ReplayMgr] Sniff time has been set to " + std::to_string(unixtime);
        sLog.outInfo(message.c_str());
        sWorld.SendGlobalText(message.c_str(), nullptr);
    }

    m_startTimeSniff = unixtime;
    m_currentSniffTime = unixtime;
    m_currentSniffTimeMs = uint64(unixtime) * 1000;
    m_timeDifference = currentTime - m_startTimeSniff;

    if (updateObjectsState)
        UpdateObjectStateAndVisiblityForCurrentTime();
}

void ReplayMgr::StartPlaying()
{
    if (!m_initialized)
    {
        if (m_eventsMap.empty())
        {
            sLog.outError("[ReplayMgr] Events map is empty!");
            return;
        }

        if (!m_startTimeSniff)
        {
            uint32 earliestEventTime = m_eventsMap.begin()->first / IN_MILLISECONDS;
            SetPlayTime(earliestEventTime);
        }
        
        SpawnCharacters();
        m_initialized = true;
    }
    sLog.outInfo("[ReplayMgr] Sniff replay started");
    sWorld.SendGlobalText("[ReplayMgr] Sniff replay started", nullptr);
    m_enabled = true;
}

void ReplayMgr::StopPlaying()
{
    sLog.outInfo("[ReplayMgr] Sniff replay stopped");
    sWorld.SendGlobalText("[ReplayMgr] Sniff replay stopped", nullptr);
    m_enabled = false;
}

Player* ReplayMgr::GetPlayer(uint32 guid)
{
    auto const itr = m_playerBots.find(guid);
    if (itr != m_playerBots.end())
        return itr->second->me;
    return nullptr;
}

bool ChatHandler::HandleSniffPlayCommand(char* args)
{
    if (sReplayMgr.IsPlaying())
        SendSysMessage("Sniff replay is already playing.");

    uint32 unixtime;
    if (ExtractUInt32(&args, unixtime))
        sReplayMgr.SetPlayTime(unixtime);

    sReplayMgr.StartPlaying();
    return true;
}

bool ChatHandler::HandleSniffStopCommand(char* args)
{
    if (!sReplayMgr.IsPlaying())
        SendSysMessage("Sniff replay is already stopped.");

    sReplayMgr.StopPlaying();
    return true;
}

bool ChatHandler::HandleSniffSetTimeCommand(char* args)
{
    uint32 unixtime;
    if (!ExtractUInt32(&args, unixtime))
        return false;
    sReplayMgr.SetPlayTime(unixtime);
    return true;
}

bool ChatHandler::HandleSniffGetTimeCommand(char* args)
{
    PSendSysMessage("Current sniff time: %u", sReplayMgr.GetCurrentSniffTime());
    return true;
}

bool ChatHandler::HandleSniffGoToClientCommand(char* args)
{
    WorldLocation loc;
    if (sReplayMgr.GetCurrentClientPosition(loc))
        m_session->GetPlayer()->TeleportTo(loc);
    else
        SendSysMessage("Cannot identify client's position.");
    return true;
}

std::string ReplayMgr::ListSniffedEventsForObject(KnownObject object)
{
    std::stringstream eventsList;
    for (const auto& itr : m_eventsMap)
    {
        if (itr.second->GetSourceObject() == object)
        {
            eventsList << itr.first << " - " << GetSniffedEventName(itr.second->GetType()) << "\n";
        }
    }
    return eventsList.str();
}

bool ChatHandler::HandleUnitListEventsCommand(char* /*args*/)
{
    Unit* pUnit = GetSelectedUnit();
    if (!pUnit)
    {
        PSendSysMessage(LANG_SELECT_CHAR_OR_CREATURE);
        SetSentErrorMessage(true);
        return false;
    }

    uint32 guid = 0;
    uint32 entry = 0;
    if (pUnit->IsCreature() && static_cast<Creature*>(pUnit)->HasStaticDBSpawnData())
    {
        guid = pUnit->GetGUIDLow();
        entry = pUnit->GetEntry();
    }
    else if (Player* pPlayer = pUnit->ToPlayer())
    {
        if (auto pAI = pPlayer->AI())
        {
            if (auto pBotAI = dynamic_cast<ReplayBotAI*>(pAI))
            {
                guid = pBotAI->m_guid;
            }
        }
    }

    if (!guid)
    {
        PSendSysMessage("There are no events for that unit.");
        SetSentErrorMessage(true);
        return false;
    }

    KnownObject objectGuid = KnownObject(guid, entry, TypeID(pUnit->GetTypeId()));
    std::string eventsList = sReplayMgr.ListSniffedEventsForObject(objectGuid);
    if (eventsList.empty())
        SendSysMessage("No events for target.");
    else
    {
        PSendSysMessage("Events for %s", pUnit->GetObjectGuid().GetString().c_str());
        SendSysMessage(eventsList.c_str());
    }

    return true;
}
