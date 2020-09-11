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
#include "SniffedEvent.h"
#include <memory>

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
            LoadActivePlayer();
            LoadSniffedEvents();
        }
        void LoadSniffedEvents()
        {
            LoadCreatureCreate1();
            LoadCreatureCreate2();
            LoadCreatureDestroy();
            LoadCreatureMovement();
        }
        void LoadCharacterTemplates();
        void LoadCharacterMovements();
        void LoadActivePlayer();

        void LoadCreatureCreate1();
        void LoadCreatureCreate2();
        void LoadCreatureDestroy();
        void LoadCreatureMovement();

        void Update(uint32 const diff);
        void SpawnCharacters();
        void SetPlayTime(uint32 unixtime);
        void UpdateObjectVisiblityForCurrentTime();
        void StartPlaying();
        void StopPlaying() { m_enabled = false; }
        bool IsPlaying() { return m_enabled; }

        uint32 GetCurrentSniffTime() { return m_currentSniffTime; }
        uint64 GetCurrentSniffTimeMs() { return m_currentSniffTimeMs; }
        uint32 GetStartTimeSniff() { return m_startTimeSniff; }
        uint32 GetTimeDifference() { return m_timeDifference; }

        Player* GetPlayer(uint32 guid);
        ReplayBotAI* GetPlayerBot(uint32 guid)
        {
            auto const itr = m_playerBots.find(guid);
            if (itr != m_playerBots.end())
                return itr->second;
            return nullptr;
        }
        void StoreCreature(uint32 guid, Creature* pCreature)
        {
            m_creatures.insert({ guid, pCreature });
        }
        Creature* GetCreature(uint32 guid)
        {
            auto const itr = m_creatures.find(guid);
            if (itr != m_creatures.end())
                return itr->second;
            return nullptr;
        }
        void StoreGameObject(uint32 guid, GameObject* pGo)
        {
            m_gameobjects.insert({ guid, pGo });
        }
        GameObject* GetGameObject(uint32 guid)
        {
            auto const itr = m_gameobjects.find(guid);
            if (itr != m_gameobjects.end())
                return itr->second;
            return nullptr;
        }
        bool GetCurrentClientPosition(WorldLocation& loc);
        uint32 GetCreatureEntryFromGuid(uint32 guid);

    protected:
        bool m_enabled = false;
        bool m_initialized = false;
        uint32 m_currentSniffTime = 0;
        uint64 m_currentSniffTimeMs = 0;
        uint32 m_startTimeSniff = 0;
        uint32 m_timeDifference = 0;
        std::unordered_map<uint32 /*guid*/, Creature*> m_creatures;
        std::unordered_map<uint32 /*guid*/, GameObject*> m_gameobjects;
        std::map<uint32 /*unixtime*/, uint32 /*guid*/> m_activePlayers;
        std::unordered_map<uint32 /*guid*/, ReplayBotAI*> m_playerBots;
        std::unordered_map<uint32 /*guid*/, CharacterTemplateEntry> m_characterTemplates;
        std::unordered_map<uint32 /*guid*/, CharacterMovementMap> m_characterMovements;
        std::multimap<uint32, std::shared_ptr<SniffedEvent>> m_eventsMap;
};

#define sReplayMgr MaNGOS::Singleton<ReplayMgr>::Instance()

#endif
