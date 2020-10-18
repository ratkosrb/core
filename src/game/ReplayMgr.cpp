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
#include "Language.h"

INSTANTIATE_SINGLETON_1(ReplayMgr);

void ReplayMgr::LoadCharacterTemplates()
{
    uint32 count = 0;

    //                                                               0       1          2       3       4        5         6        7     8        9              10              11             12            13            14            15     16             17        18        19
    std::unique_ptr<QueryResult> result(SniffDatabase.Query("SELECT `guid`, `account`, `name`, `race`, `class`, `gender`, `level`, `xp`, `money`, `playerBytes`, `playerBytes2`, `playerFlags`, `position_x`, `position_y`, `position_z`, `map`, `orientation`, `health`, `power1`, `equipmentCache` FROM `characters`"));

    if (!result)
    {
        BarGoLink bar(1);
        bar.step();

        sLog.outString();
        sLog.outErrorDb(">> Loaded 0 character definitions. DB table `characters` is empty.");
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
        character.name = fields[2].GetCppString();
        character.raceId = fields[3].GetUInt8();
        character.classId = fields[4].GetUInt8();
        character.gender = fields[5].GetUInt8();
        character.level = fields[6].GetUInt32();
        character.playerBytes = fields[9].GetUInt32();
        character.playerBytes2 = fields[10].GetUInt32();
        character.position.x = fields[12].GetFloat();
        character.position.y = fields[13].GetFloat();
        character.position.z = fields[14].GetFloat();
        character.position.mapId = fields[15].GetUInt16();
        character.position.o = fields[16].GetFloat();
        character.health = fields[17].GetUInt32();
        character.mana = fields[18].GetUInt32();
        std::string equipmentCache = fields[19].GetCppString();
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

enum class ModernMovementFlag : uint32
{
    None = 0x00000000,
    Forward = 0x00000001,
    Backward = 0x00000002,
    StrafeLeft = 0x00000004,
    StrafeRight = 0x00000008,
    Left = 0x00000010,
    Right = 0x00000020,
    PitchUp = 0x00000040,
    PitchDown = 0x00000080,
    Walking = 0x00000100,
    DisableGravity = 0x00000200,
    Root = 0x00000400,
    Falling = 0x00000800,
    FallingFar = 0x00001000,
    PendingStop = 0x00002000,
    PendingStrafeStop = 0x00004000,
    PendingForward = 0x00008000,
    PendingBackward = 0x00010000,
    PendingStrafeLeft = 0x00020000,
    PendingStrafeRight = 0x00040000,
    PendingRoot = 0x00080000,
    Swimming = 0x00100000,
    Ascending = 0x00200000,
    Descending = 0x00400000,
    CanFly = 0x00800000,
    Flying = 0x01000000,
    SplineElevation = 0x02000000,
    Waterwalking = 0x04000000,
    FallingSlow = 0x08000000,
    Hover = 0x10000000,
    DisableCollision = 0x20000000,
};

uint32 ConvertMovementFlags(uint32 flags)
{
    uint32 newFlags = 0;
    if (flags & (uint32)ModernMovementFlag::Forward)
        newFlags |= MOVEFLAG_FORWARD;
    if (flags & (uint32)ModernMovementFlag::Backward)
        newFlags |= MOVEFLAG_BACKWARD;
    if (flags & (uint32)ModernMovementFlag::StrafeLeft)
        newFlags |= MOVEFLAG_STRAFE_LEFT;
    if (flags & (uint32)ModernMovementFlag::StrafeRight)
        newFlags |= MOVEFLAG_STRAFE_RIGHT;
    if (flags & (uint32)ModernMovementFlag::Left)
        newFlags |= MOVEFLAG_TURN_LEFT;
    if (flags & (uint32)ModernMovementFlag::Right)
        newFlags |= MOVEFLAG_TURN_RIGHT;
    if (flags & (uint32)ModernMovementFlag::PitchUp)
        newFlags |= MOVEFLAG_PITCH_UP;
    if (flags & (uint32)ModernMovementFlag::PitchDown)
        newFlags |= MOVEFLAG_PITCH_DOWN;
    if (flags & (uint32)ModernMovementFlag::Walking)
        newFlags |= MOVEFLAG_WALK_MODE;
    if (flags & (uint32)ModernMovementFlag::Root)
        newFlags |= MOVEFLAG_ROOT;
    if (flags & (uint32)ModernMovementFlag::Falling)
        newFlags |= MOVEFLAG_JUMPING;
    if (flags & (uint32)ModernMovementFlag::FallingFar)
        newFlags |= MOVEFLAG_FALLINGFAR;
    if (flags & (uint32)ModernMovementFlag::Swimming)
        newFlags |= MOVEFLAG_SWIMMING;
    if (flags & (uint32)ModernMovementFlag::CanFly)
        newFlags |= MOVEFLAG_CAN_FLY;
    if (flags & (uint32)ModernMovementFlag::Flying)
        newFlags |= MOVEFLAG_FLYING;
    if (flags & (uint32)ModernMovementFlag::Waterwalking)
        newFlags |= MOVEFLAG_WATERWALKING;
    if (flags & (uint32)ModernMovementFlag::FallingSlow)
        newFlags |= MOVEFLAG_SAFE_FALL;
    if (flags & (uint32)ModernMovementFlag::Hover)
        newFlags |= MOVEFLAG_HOVER;
    return newFlags;
}

void ReplayMgr::LoadCharacterMovements()
{
    uint32 count = 0;

    //                                                               0       1         2            3             4      5             6             7             8              9
    std::unique_ptr<QueryResult> result(SniffDatabase.Query("SELECT `guid`, `opcode`, `move_time`, `move_flags`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `unixtimems` FROM `character_movement`"));

    if (!result)
    {
        BarGoLink bar(1);
        bar.step();

        sLog.outString();
        sLog.outErrorDb(">> Loaded 0 character movements. DB table `character_movement` is empty.");
        return;
    }

    BarGoLink bar(result->GetRowCount());

    do
    {
        Field* fields = result->Fetch();
        bar.step();

        uint32 guid = fields[0].GetUInt32();
        std::string opcodeName = fields[1].GetCppString();
        uint16 opcode = ConvertMovementOpcode(opcodeName);
        if (!opcode)
            continue;

        uint64 unixtimems = fields[9].GetUInt64();
        CharacterMovementEntry& moveData = m_characterMovements[guid][unixtimems];
        moveData.opcode = opcode;
        moveData.moveTime = fields[2].GetUInt32();
        moveData.moveFlags = ConvertMovementFlags(fields[3].GetUInt32());
        moveData.position.mapId = fields[4].GetUInt32();
        moveData.position.x = fields[5].GetFloat();
        moveData.position.y = fields[6].GetFloat();
        moveData.position.z = fields[7].GetFloat();
        moveData.position.o = fields[8].GetFloat();

        ++count;
    } while (result->NextRow());

    sLog.outString(">> Loaded %u sniffed character movements", count);
}

void ReplayMgr::LoadActivePlayer()
{
    uint32 count = 0;

    //                                                               0       1
    std::unique_ptr<QueryResult> result(SniffDatabase.Query("SELECT `guid`, `unixtime` FROM `character_active_player`"));

    if (!result)
    {
        BarGoLink bar(1);
        bar.step();

        sLog.outString();
        sLog.outErrorDb(">> No active player in sniff.");
        return;
    }

    BarGoLink bar(result->GetRowCount());

    do
    {
        Field* fields = result->Fetch();
        bar.step();

        uint32 guid = fields[0].GetUInt32();
        uint32 unixtime = fields[1].GetUInt32();

        m_activePlayers.insert({ unixtime, guid });

    } while (result->NextRow());
}

void ReplayMgr::SpawnCharacters()
{
    for (const auto& itr : m_characterTemplates)
    {
        CharacterMovementMap const* movementMap = nullptr;
        auto movement = m_characterMovements.find(itr.first);
        if (movement != m_characterMovements.end())
            movementMap = &movement->second;
        ReplayBotAI* ai = new ReplayBotAI(itr.first, &itr.second, movementMap);
        m_playerBots[itr.first] = ai;
        sPlayerBotMgr.AddBot(ai);
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

    uint32 oldSniffTime = m_currentSniffTime;
    m_currentSniffTimeMs += diff;
    m_currentSniffTime = m_currentSniffTimeMs / 1000;

    if (oldSniffTime == m_currentSniffTime)
        return;

    for (const auto& itr : m_eventsMap)
    {
        if (itr.first <= oldSniffTime)
            continue;

        if (itr.first > m_currentSniffTime)
            return;

        itr.second->Execute();
    }

    if (m_currentSniffTime > m_eventsMap.rbegin()->first)
    {
        sLog.outInfo("[ReplayMgr] Sniff replay is over.");
        m_enabled = false;
    }
}

void ReplayMgr::UpdateObjectVisiblityForCurrentTime()
{
    // Creatures
    {
        std::map<uint32, Position> creaturePositions;
        for (const auto itr : m_creatures)
        {
            if (itr.second->GetVisibility() != VISIBILITY_OFF)
                itr.second->SetVisibility(VISIBILITY_OFF);

            if (auto data = itr.second->GetCreatureData())
            {
                creaturePositions[itr.first] = data->position.ToPosition();

                if (!itr.second->IsAlive() && data->current_health > 0)
                    itr.second->Respawn();
                else
                {
                    if (itr.second->GetMaxHealth() != data->max_health)
                        itr.second->SetMaxHealth(data->max_health);
                    if (itr.second->GetHealth() != data->current_health)
                        itr.second->SetHealth(data->current_health);
                } 

                if (itr.second->GetMaxPower(POWER_MANA) != data->max_mana)
                    itr.second->SetMaxPower(POWER_MANA, data->max_mana);
                if (itr.second->GetPower(POWER_MANA) != data->current_mana)
                    itr.second->SetPower(POWER_MANA, data->current_mana);

                if (itr.second->GetUInt32Value(OBJECT_FIELD_ENTRY) != data->creature_id[0])
                    itr.second->SetUInt32Value(OBJECT_FIELD_ENTRY, data->creature_id[0]);
                if (itr.second->GetDisplayId() != data->display_id)
                    itr.second->SetDisplayId(data->display_id);
                if (itr.second->GetFactionTemplateId() != data->faction)
                    itr.second->SetFactionTemplateId(data->faction);
                if (itr.second->GetUInt32Value(UNIT_FIELD_FLAGS) != data->unit_flags)
                    itr.second->SetUInt32Value(UNIT_FIELD_FLAGS, data->unit_flags);
                if (itr.second->GetUInt32Value(UNIT_NPC_FLAGS) != data->npc_flags)
                    itr.second->SetUInt32Value(UNIT_NPC_FLAGS, data->npc_flags);

                if (itr.second->GetSpeedRate(MOVE_WALK) != data->speed_walk)
                    itr.second->SetSpeedRateDirect(MOVE_WALK, data->speed_walk);
                if (itr.second->GetSpeedRate(MOVE_RUN) != data->speed_run)
                    itr.second->SetSpeedRateDirect(MOVE_RUN, data->speed_run);
            }

            if (auto addon = itr.second->GetCreatureAddon())
            {
                if (itr.second->GetSheath() != addon->sheath_state)
                    itr.second->SetSheath(SheathState(addon->sheath_state));
                if (itr.second->GetStandState() != addon->stand_state)
                    itr.second->SetStandState(addon->stand_state);
                if (itr.second->GetUInt32Value(UNIT_NPC_EMOTESTATE) != addon->emote)
                    itr.second->SetUInt32Value(UNIT_NPC_EMOTESTATE, addon->emote);
                if (itr.second->GetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID) != addon->mount)
                    itr.second->SetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID, addon->mount);
            }

            itr.second->SetChannelObjectGuid(ObjectGuid());
            itr.second->SetUInt32Value(UNIT_CHANNEL_SPELL, 0);
        }

        std::set<uint32> visibleCreatures;
        for (const auto& itr : m_eventsMap)
        {
            if (itr.first > m_currentSniffTime)
                break;

            if (itr.second->GetSourceObject().m_type != TYPEID_UNIT)
                continue;

            switch (itr.second->GetType())
            {
                case SE_CREATURE_CREATE1:
                {
                    uint32 const guid = itr.second->GetSourceObject().m_guid;
                    visibleCreatures.insert(guid);
                    auto createEvent = std::static_pointer_cast<SniffedEvent_CreatureCreate1>(itr.second);
                    creaturePositions[guid] = Position(createEvent->m_x, createEvent->m_y, createEvent->m_z, createEvent->m_o);
                    break;
                }
                case SE_CREATURE_CREATE2:
                {
                    uint32 const guid = itr.second->GetSourceObject().m_guid;
                    visibleCreatures.insert(guid);
                    auto createEvent = std::static_pointer_cast<SniffedEvent_CreatureCreate2>(itr.second);
                    creaturePositions[guid] = Position(createEvent->m_x, createEvent->m_y, createEvent->m_z, createEvent->m_o);
                    break;
                }
                case SE_CREATURE_DESTROY:
                {
                    visibleCreatures.erase(itr.second->GetSourceObject().m_guid);
                    break;
                }
                case SE_CREATURE_MOVEMENT:
                {
                    uint32 const guid = itr.second->GetSourceObject().m_guid;
                    auto moveEvent = std::static_pointer_cast<SniffedEvent_CreatureMovement>(itr.second);
                    creaturePositions[guid] = Position(moveEvent->m_x, moveEvent->m_y, moveEvent->m_z, moveEvent->m_o);
                    break;
                }
                case SE_UNIT_UPDATE_ENTRY:
                case SE_UNIT_UPDATE_SCALE:
                case SE_UNIT_UPDATE_DISPLAY_ID:
                case SE_UNIT_UPDATE_MOUNT:
                case SE_UNIT_UPDATE_FACTION:
                case SE_UNIT_UPDATE_EMOTE_STATE:
                case SE_UNIT_UPDATE_STAND_STATE:
                case SE_UNIT_UPDATE_NPC_FLAGS:
                case SE_UNIT_UPDATE_UNIT_FLAGS:
                case SE_UNIT_UPDATE_CURRENT_HEALTH:
                case SE_UNIT_UPDATE_MAX_HEALTH:
                case SE_UNIT_UPDATE_CURRENT_MANA:
                case SE_UNIT_UPDATE_MAX_MANA:
                case SE_UNIT_UPDATE_SPEED:
                case SE_UNIT_TARGET_CHANGE:
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
    // GameObjects
    {
        for (const auto itr : m_gameobjects)
        {
            if (itr.second->IsVisible())
                itr.second->SetVisible(false);

            if (auto data = itr.second->GetGOData())
            {
                if (itr.second->GetGoState() != data->go_state)
                    itr.second->SetGoState(GOState(data->go_state));
                if (itr.second->GetUInt32Value(GAMEOBJECT_FLAGS) != data->flags)
                    itr.second->SetUInt32Value(GAMEOBJECT_FLAGS, data->flags);
            }
        }

        std::set<uint32> visibleGameObjects;
        for (const auto& itr : m_eventsMap)
        {
            if (itr.first > m_currentSniffTime)
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
    // Players
    if (m_initialized)
    {
        std::map<uint32, WorldLocation> playerPositions;
        for (const auto& itr : m_characterTemplates)
        {
            playerPositions[itr.first] = itr.second.position;

            if (Player* pPlayer = GetPlayer(itr.first))
            {
                pPlayer->SetObjectScale(pPlayer->GetNativeScale());
                pPlayer->SetUInt32Value(UNIT_FIELD_DISPLAYID, pPlayer->GetNativeDisplayId());
                pPlayer->SetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID, 0);
                pPlayer->SetFactionForRace(pPlayer->GetRace());
                pPlayer->SetUInt32Value(UNIT_NPC_EMOTESTATE, 0);
                pPlayer->SetStandState(UNIT_STAND_STATE_STAND);
                pPlayer->SetUInt32Value(UNIT_FIELD_FLAGS, 0);
                pPlayer->SetMaxHealth(itr.second.health);
                pPlayer->SetMaxPower(POWER_MANA, itr.second.mana);
                pPlayer->ClearTarget();
                pPlayer->SetChannelObjectGuid(ObjectGuid());
                pPlayer->SetUInt32Value(UNIT_CHANNEL_SPELL, 0);
            }
        }

        for (const auto& itr : m_eventsMap)
        {
            if (itr.first > m_currentSniffTime)
                break;

            if (itr.second->GetSourceObject().m_type != TYPEID_PLAYER)
                continue;

            switch (itr.second->GetType())
            {
                case SE_UNIT_UPDATE_ENTRY:
                case SE_UNIT_UPDATE_SCALE:
                case SE_UNIT_UPDATE_DISPLAY_ID:
                case SE_UNIT_UPDATE_MOUNT:
                case SE_UNIT_UPDATE_FACTION:
                case SE_UNIT_UPDATE_EMOTE_STATE:
                case SE_UNIT_UPDATE_STAND_STATE:
                case SE_UNIT_UPDATE_NPC_FLAGS:
                case SE_UNIT_UPDATE_UNIT_FLAGS:
                case SE_UNIT_UPDATE_CURRENT_HEALTH:
                case SE_UNIT_UPDATE_MAX_HEALTH:
                case SE_UNIT_UPDATE_CURRENT_MANA:
                case SE_UNIT_UPDATE_MAX_MANA:
                case SE_UNIT_UPDATE_SPEED:
                case SE_UNIT_TARGET_CHANGE:
                case SE_SPELL_CHANNEL_START:
                case SE_SPELL_CHANNEL_UPDATE:
                {
                    itr.second->Execute();
                    break;
                }
            }
        }

        for (const auto& itr : m_characterMovements)
        {
            for (const auto itr2 : itr.second)
            {
                if (itr2.first > m_currentSniffTimeMs)
                    break;

                playerPositions[itr.first] = itr2.second.position;
            }
        }

        for (const auto& itr : playerPositions)
        {
            if (Player* pPlayer = GetPlayer(itr.first))
            {
                if (pPlayer->GetMapId() != itr.second.mapId ||
                    pPlayer->GetPosition() != itr.second.ToPosition())
                {
                    pPlayer->TeleportTo(itr.second);
                }
            }
        }
    }
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
        sLog.outInfo("[ReplayMgr] Sniff time has been set to %u", unixtime);

    m_startTimeSniff = unixtime;
    m_currentSniffTime = unixtime;
    m_currentSniffTimeMs = uint64(unixtime) * 1000;
    m_timeDifference = currentTime - m_startTimeSniff;

    if (updateObjectsState)
        UpdateObjectVisiblityForCurrentTime();
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
            uint32 earliestEventTime = m_eventsMap.begin()->first;
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
        PSendSysMessage(eventsList.c_str());
    }

    return true;
}
