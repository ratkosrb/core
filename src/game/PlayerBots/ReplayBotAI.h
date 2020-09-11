#ifndef MANGOS_COMBAT_BOT_BASE_H
#define MANGOS_COMBAT_BOT_BASE_H

#include "PlayerBotAI.h"
#include "ReplayMgr.h"

class ReplayBotAI : public PlayerBotAI
{
public:

    ReplayBotAI(uint32 guid, CharacterTemplateEntry const* charTemplate, CharacterMovementMap const* movementMap) : PlayerBotAI(nullptr), m_guid(guid), m_template(charTemplate), m_movementMap(movementMap)
    {

    }
    bool OnSessionLoaded(PlayerBotEntry* entry, WorldSession* sess) override;
    //void OnPlayerLogin() override;

    virtual void OnPacketReceived(WorldPacket const* packet) override;
    virtual void SendFakePacket(uint16 opcode) override;

    void UpdateAI(uint32 const diff) override;
    void UpdateMovement();
    uint16 DetermineCorrectMovementOpcode(CharacterMovementEntry const& moveData);

    bool m_initialized = false;
    uint32 m_guid = 0;
    uint64 m_lastMoveUnixTimeMs = 0;
    uint32 m_sniffStartTime = 0;
    CharacterTemplateEntry const* m_template = nullptr;
    CharacterMovementMap const* m_movementMap = nullptr;
};

#endif
