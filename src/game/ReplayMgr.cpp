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

    sLog.outString();
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
        sLog.outErrorDb(">> Loaded 0 character definitions. DB table `characters` is empty.");
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

    sLog.outString();
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

bool ReplayMgr::GetCurrentClientPosition(WorldLocation& loc)
{
    uint32 currentCharacterGuid = 0;
    for (const auto& itr : m_activePlayers)
    {
        if (itr.first < m_currentSniffTime)
            currentCharacterGuid = itr.second;
        else
            break;
    }

    if (Player* pPlayer = GetPlayer(currentCharacterGuid))
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
}

void ReplayMgr::UpdateObjectVisiblityForCurrentTime()
{
    for (const auto itr : m_creatures)
        if (itr.second->GetVisibility() != VISIBILITY_OFF)
            itr.second->SetVisibility(VISIBILITY_OFF);

    std::set<uint32> visibleCreatures;
    for (const auto& itr : m_eventsMap)
    {
        if (itr.first > m_currentSniffTime)
            break;

        switch (itr.second->GetType())
        {
            case SE_CREATURE_CREATE1:
            case SE_CREATURE_CREATE2:
            {
                visibleCreatures.insert(itr.second->GetSourceObject().m_guid);
                break;
            }
            case SE_CREATURE_DESTROY:
            {
                visibleCreatures.erase(itr.second->GetSourceObject().m_guid);
                break;
            }
        }
    }

    for (const auto itr : visibleCreatures)
        if (Creature* pCreature = GetCreature(itr))
            pCreature->SetVisibility(VISIBILITY_ON);
}

void ReplayMgr::SetPlayTime(uint32 unixtime)
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

    UpdateObjectVisiblityForCurrentTime();
}

void ReplayMgr::StartPlaying()
{
    if (!m_initialized)
    {
        SpawnCharacters();
        m_initialized = true;
    }
    m_enabled = true;
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

bool ChatHandler::HandleSniffGoToClientCommand(char* args)
{
    WorldLocation loc;
    if (sReplayMgr.GetCurrentClientPosition(loc))
        m_session->GetPlayer()->TeleportTo(loc);
    else
        SendSysMessage("Cannot identify client's position.");
    return true;
}