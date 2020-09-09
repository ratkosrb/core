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

#ifndef MANGOS_H_REPLAYMGR
#define MANGOS_H_REPLAYMGR

#include "Common.h"
#include "SharedDefines.h"
#include "Timer.h"

struct CharacterEquipment
{
    uint32 itemId = 0;
    uint32 enchantId = 0;
};

struct CharacterTemplateEntry
{
    uint32 guid = 0;
    std::string name;
    uint8 raceId = 0;
    uint8 classId = 0;
    uint8 gender = 0;
    uint32 level = 0;
    uint32 playerBytes = 0;
    uint32 playerBytes2 = 0;
    WorldLocation position;
    CharacterEquipment equipment[EQUIPMENT_SLOT_END] = {};
};

struct CharacterMovementEntry
{
    uint16 opcode = 0;
    uint32 moveTime = 0;
    uint32 moveFlags = 0;
    WorldLocation position;
    uint64 unixTimeMs = 0;
};

typedef std::map<uint64 /*unixtimems*/, CharacterMovementEntry> CharacterMovementMap;

class ReplayBotAI;

class ReplayMgr
{
    public:
        ReplayMgr() {};
        ~ReplayMgr() {};

        void LoadEverything()
        {
            LoadCharacterTemplates();
            LoadCharacterMovements();
        }
        void LoadCharacterTemplates();
        void LoadCharacterMovements();

        void SpawnCharacters();
        void SetPlayTime(uint32 unixtime);
        void StartPlaying();
        void StopPlaying() { m_enabled = false; }
        bool IsPlaying() { return m_enabled; }

        uint32 GetCurrentSniffTime() { return m_currentSniffTime; }
        uint32 GetStartTimeSniff() { return m_startTimeSniff; }
        uint32 GetStartTimeReal() { return m_startTimeReal; }
        uint32 GetStartTimeRealMs() { return m_startTimeRealMs; }
        uint32 GetTimeDifference() { return m_timeDifference; }

    protected:
        bool m_enabled = false;
        bool m_initialized = false;
        uint32 m_currentSniffTime = 0;
        uint32 m_startTimeSniff = 0;
        uint32 m_startTimeReal = 0;
        uint32 m_startTimeRealMs = 0;
        uint32 m_timeDifference = 0;
        std::unordered_map<uint32 /*guid*/, ReplayBotAI*> m_playerBots;
        std::unordered_map<uint32 /*guid*/, CharacterTemplateEntry> m_characterTemplates;
        std::unordered_map<uint32 /*guid*/, CharacterMovementMap> m_characterMovements;
};

#define sReplayMgr MaNGOS::Singleton<ReplayMgr>::Instance()

#endif
