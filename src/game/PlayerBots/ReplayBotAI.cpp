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

        // Fix zone in who window
        uint32 newzone, newarea;
        me->GetZoneAndAreaId(newzone, newarea);
        me->UpdateZone(newzone, newarea);

        m_lastMoveUnixTimeMs = uint64(sReplayMgr.GetCurrentSniffTime()) * 1000;
        m_lastMoveWorldMsTime = WorldTimer::getMSTime();
        if (m_guid == 2)
            printf("starting with %llu\n", m_lastMoveUnixTimeMs);
        return;
    }

    if (!sReplayMgr.IsPlaying())
        return;

    UpdateMovement();
}

void ReplayBotAI::UpdateMovement()
{
    if (!m_movementMap)
        return;

    uint32 currentWorldMsTime = WorldTimer::getMSTime();
    uint32 msExpired = currentWorldMsTime - m_lastMoveWorldMsTime;
    uint64 maxUnixTimeMs = m_lastMoveUnixTimeMs + msExpired;
    if (m_guid == 2)
    {
        printf("---------------\n");
        printf("msExpired = %u, maxUnixTimeMs = %llu\n", msExpired, maxUnixTimeMs);
    }
        
    for (const auto& itr : *m_movementMap)
    {
        if (itr.first <= m_lastMoveUnixTimeMs)
            continue;

        if (itr.first > maxUnixTimeMs)
        {
            m_lastMoveUnixTimeMs += msExpired;
            m_lastMoveWorldMsTime = currentWorldMsTime;
            return;
        }

        if (m_guid == 2)
            printf("moving\n");
        // send the movement
        if (itr.second.position.mapId == me->GetMapId())
        {
            me->Relocate(itr.second.position.x, itr.second.position.y, itr.second.position.z, itr.second.position.o);
            me->m_movementInfo.SetMovementFlags(MovementFlags(itr.second.moveFlags));
            me->m_movementInfo.UpdateTime(itr.second.moveTime);
            WorldPacket data(itr.second.opcode);
#if SUPPORTED_CLIENT_BUILD > CLIENT_BUILD_1_8_4
            data << me->GetPackGUID();
#else
            data << me->GetGUID();
#endif
            data << me->m_movementInfo;
            me->SendMovementMessageToSet(std::move(data), false);
        }
        else
        {
            me->TeleportTo(itr.second.position);
        }

        m_lastMoveUnixTimeMs = itr.first;
        m_lastMoveWorldMsTime = currentWorldMsTime;
    }
}