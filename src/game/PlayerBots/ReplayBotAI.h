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

#ifndef MANGOS_COMBAT_BOT_BASE_H
#define MANGOS_COMBAT_BOT_BASE_H

#include "PlayerBotAI.h"
#include "Replay/ReplayMgr.h"

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
