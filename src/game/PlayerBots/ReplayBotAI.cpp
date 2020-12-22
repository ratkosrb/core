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

#include "ReplayBotAI.h"
#include "Player.h"
#include "Log.h"
#include "MotionMaster.h"
#include "ObjectMgr.h"
#include "PlayerBotMgr.h"
#include "WorldPacket.h"
#include "Spell.h"
#include "SpellAuras.h"
#include "Chat.h"
#include "SocialMgr.h"

bool ReplayBotAI::OnSessionLoaded(PlayerBotEntry* entry, WorldSession* sess)
{
    ASSERT(botEntry);

    uint8 gender = m_template->gender;

    // Placeholders. We will set player bytes directly later.
    uint8 skin = 0;
    uint8 face = 0;
    uint8 hairStyle = 0;
    uint8 hairColor = 0;
    uint8 facialHair = 0;

    Player* newChar = new Player(sess);
    uint32 guid = botEntry->playerGUID;
    if (!newChar->Create(guid, m_template->name, m_template->raceId, m_template->classId, gender, skin, face, hairStyle, hairColor, facialHair))
    {
        sLog.outError("ReplayBotAI::OnSessionLoaded: Unable to create a player!");
        delete newChar;
        return false;
    }
    newChar->SetLocationMapId(m_template->position.mapId);
    newChar->SetLocationInstanceId(0);
    newChar->SetAutoInstanceSwitch(false);
    newChar->GetMotionMaster()->Initialize();
    newChar->SetCinematic(1);
    // Generate position
    Map* map = sMapMgr.FindMap(m_template->position.mapId, 0);
    if (!map)
    {
        sLog.outError("ReplayBotAI::OnSessionLoaded: Map (%u, %u) not found!", m_template->position.mapId, 0);
        delete newChar;
        return false;
    }
    newChar->Relocate(m_template->position.x, m_template->position.y, m_template->position.z, m_template->position.o);
    sObjectMgr.InsertPlayerInCache(newChar);
    newChar->SetMap(map);
    newChar->SaveRecallPosition();
    newChar->CreatePacketBroadcaster();
    MasterPlayer* mPlayer = new MasterPlayer(sess);
    mPlayer->LoadPlayer(newChar);
    mPlayer->SetSocial(sSocialMgr.LoadFromDB(nullptr, newChar->GetObjectGuid()));
    if (!newChar->GetMap()->Add(newChar))
    {
        sLog.outError("ReplayBotAI::OnSessionLoaded: Unable to add player to map!");
        delete newChar;
        return false;
    }
    sess->SetPlayer(newChar);
    sess->SetMasterPlayer(mPlayer);
    sObjectAccessor.AddObject(newChar);
    newChar->SetUInt32Value(PLAYER_BYTES, m_template->playerBytes);
    newChar->SetUInt32Value(PLAYER_BYTES_2, m_template->playerBytes2);
    newChar->SetCanModifyStats(true);
    newChar->UpdateAllStats();
    return true;
}

void ReplayBotAI::SendFakePacket(uint16 opcode)
{
    switch (opcode)
    {
        case MSG_MOVE_WORLDPORT_ACK:
        {
            me->GetSession()->HandleMoveWorldportAckOpcode();
            break;
        }
        case MSG_MOVE_TELEPORT_ACK:
        {
            WorldPacket data(MSG_MOVE_TELEPORT_ACK);
            data << me->GetObjectGuid();
#if SUPPORTED_CLIENT_BUILD > CLIENT_BUILD_1_9_4
            data << me->GetLastCounterForMovementChangeType(TELEPORT);
#endif
            data << uint32(time(nullptr));
            me->GetSession()->HandleMoveTeleportAckOpcode(data);
            break;
        }
        case CMSG_BATTLEFIELD_PORT:
        {
            for (uint32 i = BATTLEGROUND_QUEUE_AV; i <= BATTLEGROUND_QUEUE_AB; i++)
            {
                if (me->IsInvitedForBattleGroundQueueType(BattleGroundQueueTypeId(i)))
                {
                    WorldPacket data(CMSG_BATTLEFIELD_PORT);
#if SUPPORTED_CLIENT_BUILD > CLIENT_BUILD_1_8_4
                    data << uint32(GetBattleGrounMapIdByTypeId(BattleGroundTypeId(i)));
#endif
                    data << uint8(1);
                    me->GetSession()->HandleBattleFieldPortOpcode(data);
                    break;
                }
            }
            break;
        }
        case CMSG_BEGIN_TRADE:
        {
            WorldPacket data(CMSG_BEGIN_TRADE);
            me->GetSession()->HandleBeginTradeOpcode(data);
            break;
        }
        case CMSG_ACCEPT_TRADE:
        {
            if (Item* pItem = me->GetItemByPos(INVENTORY_SLOT_BAG_0, INVENTORY_SLOT_ITEM_START))
                me->DestroyItem(INVENTORY_SLOT_BAG_0, INVENTORY_SLOT_ITEM_START, true);

            WorldPacket data(CMSG_ACCEPT_TRADE);
            data << uint32(1);
            me->GetSession()->HandleAcceptTradeOpcode(data);
            break;
        }
        case CMSG_RESURRECT_RESPONSE:
        {
            WorldPacket data(CMSG_RESURRECT_RESPONSE);
            data << me->GetResurrector();
            data << uint8(1);
            me->GetSession()->HandleResurrectResponseOpcode(data);
            break;
        }
    }
}

void ReplayBotAI::OnPacketReceived(WorldPacket const* packet)
{
    // Must always check "me" player pointer here!
    //printf("Bot received %s\n", LookupOpcodeName(packet->GetOpcode()));
    switch (packet->GetOpcode())
    {
        case SMSG_NEW_WORLD:
        {
            botEntry->m_pendingResponses.push_back(MSG_MOVE_WORLDPORT_ACK);
            break;
        }
        case MSG_MOVE_TELEPORT_ACK:
        {
            botEntry->m_pendingResponses.push_back(MSG_MOVE_TELEPORT_ACK);
            break;
        }
        case SMSG_TRADE_STATUS:
        {
            uint32 status = *((uint32*)(*packet).contents());
            if (status == TRADE_STATUS_BEGIN_TRADE)
            {
                botEntry->m_pendingResponses.push_back(CMSG_BEGIN_TRADE);
            }
            else if (status == TRADE_STATUS_TRADE_ACCEPT)
            {
                botEntry->m_pendingResponses.push_back(CMSG_ACCEPT_TRADE);
            }
            break;
        }
        case SMSG_RESURRECT_REQUEST:
        {
            botEntry->m_pendingResponses.push_back(CMSG_RESURRECT_RESPONSE);
            break;
        }
    }
}

void ReplayBotAI::UpdateAI(uint32 const diff)
{
    if (!m_initialized)
    {
        m_initialized = true;

        if (m_template->level != me->GetLevel())
        {
            me->GiveLevel(m_template->level);
            me->InitTalentForLevel();
            me->SetUInt32Value(PLAYER_XP, 0);
        }

        // Unequip starting gear
        for (int i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
            me->AutoUnequipItemFromSlot(i);

        // Equip gear from db
        for (uint32 i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
        {
            if (uint32 itemId = m_template->equipment[i].itemId)
            {
                if (ItemPrototype const* pItem = sObjectMgr.GetItemPrototype(itemId))
                {
                    me->SatisfyItemRequirements(pItem);
                    me->StoreNewItemInBestSlots(itemId, 1, m_template->equipment[i].enchantId);
                }
            }  
        }

        sReplayMgr.ResetPlayerToInitialState(me, *m_template);

        // Fix zone in who window
        uint32 newzone, newarea;
        me->GetZoneAndAreaId(newzone, newarea);
        me->UpdateZone(newzone, newarea);

        m_sniffStartTime = sReplayMgr.GetStartTimeSniff();
        m_lastMoveUnixTimeMs = sReplayMgr.GetCurrentSniffTimeMs();
        return;
    }

    if (!sReplayMgr.IsPlaying())
        return;

    if (m_sniffStartTime != sReplayMgr.GetStartTimeSniff())
    {
        m_sniffStartTime = sReplayMgr.GetStartTimeSniff();
        m_lastMoveUnixTimeMs = sReplayMgr.GetCurrentSniffTimeMs();
        return;
    }

    UpdateMovement();
}

uint16 ReplayBotAI::DetermineCorrectMovementOpcode(CharacterMovementEntry const& moveData)
{
    // In classic all the movement for other players is sent in SMSG_MOVE_UPDATE.
    // That upcode does not exist in Vanilla, so we replace it with heartbeat.
    if (moveData.opcode == MSG_MOVE_HEARTBEAT)
    {
        // Determine more appropriate opcode based on movement flags.
        if (!me->m_movementInfo.HasMovementFlag(MOVEFLAG_FORWARD | MOVEFLAG_BACKWARD | MOVEFLAG_STRAFE_LEFT | MOVEFLAG_STRAFE_RIGHT | MOVEFLAG_TURN_LEFT | MOVEFLAG_TURN_RIGHT) &&
            (moveData.moveFlags & (MOVEFLAG_FORWARD | MOVEFLAG_BACKWARD | MOVEFLAG_STRAFE_LEFT | MOVEFLAG_STRAFE_RIGHT | MOVEFLAG_TURN_LEFT | MOVEFLAG_TURN_RIGHT)))
        {
            if (moveData.moveFlags & MOVEFLAG_FORWARD)
                return MSG_MOVE_START_FORWARD;
            else if (moveData.moveFlags & MOVEFLAG_BACKWARD)
                return MSG_MOVE_START_BACKWARD;
            else if (moveData.moveFlags & MOVEFLAG_STRAFE_LEFT)
                return MSG_MOVE_START_STRAFE_LEFT;
            else if (moveData.moveFlags & MOVEFLAG_STRAFE_RIGHT)
                return MSG_MOVE_START_STRAFE_RIGHT;
            else if (moveData.moveFlags & MOVEFLAG_TURN_LEFT)
                return MSG_MOVE_START_TURN_LEFT;
            else if (moveData.moveFlags & MOVEFLAG_TURN_RIGHT)
                return MSG_MOVE_START_TURN_RIGHT;
        }
        else if (me->m_movementInfo.HasMovementFlag(MOVEFLAG_JUMPING | MOVEFLAG_FALLINGFAR) && !(moveData.moveFlags & (MOVEFLAG_JUMPING | MOVEFLAG_FALLINGFAR)))
            return MSG_MOVE_FALL_LAND;
        else if (!me->m_movementInfo.HasMovementFlag(MOVEFLAG_JUMPING) && (moveData.moveFlags & MOVEFLAG_JUMPING))
            return MSG_MOVE_JUMP;
        else if (!(moveData.moveFlags & MOVEFLAG_MASK_MOVING))
        {
            if (me->m_movementInfo.HasMovementFlag(MOVEFLAG_STRAFE_LEFT | MOVEFLAG_STRAFE_RIGHT) && !(moveData.moveFlags & (MOVEFLAG_STRAFE_LEFT | MOVEFLAG_STRAFE_RIGHT)))
                return MSG_MOVE_STOP_STRAFE;
            else if (me->m_movementInfo.HasMovementFlag(MOVEFLAG_TURN_LEFT | MOVEFLAG_TURN_RIGHT) && !(moveData.moveFlags & (MOVEFLAG_TURN_LEFT | MOVEFLAG_TURN_RIGHT)))
                return MSG_MOVE_STOP_TURN;
            else if (me->m_movementInfo.HasMovementFlag(MOVEFLAG_PITCH_UP | MOVEFLAG_PITCH_DOWN) && !(moveData.moveFlags & (MOVEFLAG_PITCH_UP | MOVEFLAG_PITCH_DOWN)))
                return MSG_MOVE_STOP_PITCH;
            else if (me->m_movementInfo.HasMovementFlag(MOVEFLAG_SWIMMING) && !(moveData.moveFlags & MOVEFLAG_SWIMMING))
                return MSG_MOVE_STOP_SWIM;
            else if (me->m_movementInfo.HasMovementFlag(MOVEFLAG_MASK_MOVING) && (moveData.moveFlags == 0))
                return MSG_MOVE_STOP;
            else if ((moveData.moveFlags == 0) &&
                (moveData.position.x == me->m_movementInfo.pos.x) &&
                (moveData.position.y == me->m_movementInfo.pos.y) &&
                (moveData.position.z == me->m_movementInfo.pos.z) &&
                (moveData.position.o != me->m_movementInfo.pos.o))
                return MSG_MOVE_SET_FACING;
        }
    }
    return moveData.opcode;
}

void ReplayBotAI::UpdateMovement()
{
    if (!m_movementMap)
        return;

    uint64 maxUnixTimeMs = sReplayMgr.GetCurrentSniffTimeMs();

    for (const auto& itr : *m_movementMap)
    {
        if (itr.first <= m_lastMoveUnixTimeMs)
            continue;

        if (itr.first > maxUnixTimeMs)
            return;

        m_lastMoveUnixTimeMs = itr.first;

        // send the movement
        if (itr.second.position.mapId == me->GetMapId())
        {
            me->m_movementInfo.ChangePosition(itr.second.position.x, itr.second.position.y, itr.second.position.z, itr.second.position.o);
            me->m_movementInfo.SetMovementFlags(MovementFlags(itr.second.moveFlags));
            me->m_movementInfo.UpdateTime(itr.second.moveTime);
            WorldPacket data(DetermineCorrectMovementOpcode(itr.second));
#if SUPPORTED_CLIENT_BUILD > CLIENT_BUILD_1_8_4
            data << me->GetPackGUID();
#else
            data << me->GetGUID();
#endif
            data << me->m_movementInfo;
            me->SendMovementMessageToSet(std::move(data), false);
            me->SetPosition(itr.second.position.x, itr.second.position.y, itr.second.position.z, itr.second.position.o);
        }
        else
        {
            me->TeleportTo(itr.second.position);
            return;
        }
    }
}