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

#ifndef MANGOS_H_REPLAYMGR
#define MANGOS_H_REPLAYMGR

#include "Common.h"
#include "SharedDefines.h"
#include "Timer.h"
#include "SniffedEvent.h"
#include <memory>
#include "WaypointManager.h"

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
    uint32 playerFlags = 0;
    float scale = 1;
    uint32 display_id = 0;
    uint32 native_display_id = 0;
    uint32 mount_display_id = 0;
    uint32 faction = 0;
    uint32 unit_flags = 0;
    uint32 current_health = 0;
    uint32 max_health = 0;
    uint32 current_mana = 0;
    uint32 max_mana = 0;
    uint32 aura_state = 0;
    uint32 emote_state = 0;
    uint8 stand_state = 0;
    uint8 vis_flags = 0;
    uint8 sheath_state = 0;
    uint8 shapeshift_form = 0;
    uint32 move_flags = 0;
    float speed_walk = 1;
    float speed_run = 1;
    float speed_run_back = 1;
    float speed_swim = 1;
    float speed_swim_back = 1;
    float bounding_radius = 1;
    float combat_reach = 1;
    uint32 main_hand_attack_time = 0;
    uint32 off_hand_attack_time = 0;
    uint32 ranged_attack_time = 0;
    KnownObject charmGuid;
    KnownObject summonGuid;
    KnownObject charmerGuid;
    KnownObject creatorGuid;
    KnownObject summonerGuid;
    KnownObject targetGuid;
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

struct CreatureText
{
    uint32 creatureId = 0;
    uint32 groupId = 0;
    std::string text;
    uint32 chatType = 0;
};

class ReplayBotAI;

class ReplaySetTimePlayerWorker : public ACE_Based::Runnable
{
public:
    ReplaySetTimePlayerWorker() = default;
    virtual void run();
};

class ReplaySetTimeCreatureWorker : public ACE_Based::Runnable
{
public:
    ReplaySetTimeCreatureWorker() = default;
    virtual void run();
};

class ReplaySetTimeGameObjectWorker : public ACE_Based::Runnable
{
public:
    ReplaySetTimeGameObjectWorker() = default;
    virtual void run();
};

typedef std::map<uint32 /*guid*/, std::map<uint32 /*parent_point*/, std::vector<G3D::Vector3>>> SplinesMap;

class ReplayMgr
{
    public:
        ReplayMgr() {};
        ~ReplayMgr() {};

        void LoadEverything()
        {
            LoadPlayers();
            LoadInitialPlayerGuidValues();
            LoadCharacterMovements();
            LoadActivePlayer();
            LoadInitialWorldStates();
            LoadSniffedEvents();
        }
        void LoadSniffedEvents();
        void LoadPlayers();
        void LoadInitialPlayerGuidValues();
        void LoadCharacterMovements();
        void LoadActivePlayer();
        void LoadInitialWorldStates();

        void LoadUnitCreate1(char const* tableName, TypeID typeId);
        void LoadUnitCreate2(char const* tableName, TypeID typeId);
        void LoadUnitDestroy(char const* tableName, TypeID typeId);
        void LoadCreatureClientSideMovement();
        void LoadServerSideMovement(char const* tableName, TypeID typeId, SplinesMap const& splinesMap);
        void LoadServerSideMovementSplines(char const* tableName, SplinesMap& splinesMap);
        void LoadCreatureTextTemplate();
        void LoadCreatureText();
        void LoadCreatureEquipmentUpdate();
        void LoadUnitEmote(char const* tableName, TypeID typeId);
        void LoadUnitAttackLog(char const* tableName, uint32 typeId);
        template <class T>
        void LoadUnitAttackToggle(char const* tableName, uint32 typeId);
        template <class T>
        void LoadCreatureValuesUpdate(char const* fieldName);
        template <class T>
        void LoadCreatureValuesUpdate_float(char const* fieldName);
        template <class T>
        void LoadPlayerValuesUpdate(char const* fieldName);
        template <class T>
        void LoadPlayerValuesUpdate_float(char const* fieldName);
        void LoadUnitGuidValuesUpdate(char const* tableName, uint32 typeId);
        void LoadCreatureSpeedUpdate(uint32 speedType);
        void LoadPlayerSpeedUpdate(uint32 speedType);
        void LoadUnitAurasUpdate(char const* tableName, uint32 typeId);
        void LoadDynamicObjectCreate();
        void LoadGameObjectCreate1();
        void LoadGameObjectCreate2();
        void LoadGameObjectCustomAnim();
        void LoadGameObjectDespawnAnim();
        void LoadGameObjectDestroy();
        template <class T>
        void LoadGameObjectUpdate(char const* fieldName);
        void LoadSpellCastFailed();
        void LoadSpellCastStart();
        void LoadSpellCastGo();
        void LoadSpellCastGoTargets();
        void LoadSpellCastGoPositions();
        void LoadSpellChannelStart();
        void LoadSpellChannelUpdate();
        void LoadPlayerChat();
        void LoadPlayerEquipmentUpdate();
        void LoadPlayMusic();
        void LoadPlaySound();
        void LoadPlaySpellVisualKit();
        void LoadWeatherUpdates();
        void LoadWorldText();
        void LoadQuestAcceptTimes();
        void LoadQuestCompleteTimes();
        void LoadCreatureInteractTimes();
        void LoadGameObjectUseTimes();
        void LoadItemUseTimes();
        void LoadReclaimCorpseTimes();
        void LoadReleaseSpiritTimes();
        void LoadWorldStateUpdates();
        void LoadQuestUpdateComplete();
        void LoadQuestUpdateFailed();
        void LoadXPGainLog();
        void LoadFactionStandingUpdates();

        void Update(uint32 const diff);
        void SpawnCharacters();
        void SetPlayTime(uint32 unixtime, bool updateObjectsState = true);
        void UpdatePlayersForCurrentTime();
        void UpdateCreaturesForCurrentTime();
        void UpdateGameObjectsForCurrentTime();
        void UpdateObjectStateAndVisiblityForCurrentTime();
        void ResetGameObjectToInitialState(GameObject* pGo);
        void ResetPlayerToInitialState(Player* pPlayer, CharacterTemplateEntry const& initialState);
        void UpdatePlayerToCurrentState(Player* pPlayer, CharacterTemplateEntry const& initialState);
        void MarkPlayersAsSpawned() { m_playersSpawned = true; }
        void StartPlaying();
        void StopPlaying();
        bool IsPlaying() { return m_enabled; }

        uint32 GetCurrentSniffTime() { return m_currentSniffTime; }
        uint64 GetCurrentSniffTimeMs() { return m_currentSniffTimeMs; }
        uint32 GetStartTimeSniff() { return m_startTimeSniff; }
        uint32 GetTimeDifference() { return m_timeDifference; }
        uint32 GetFirstEventTime() { return (m_eventsMap.empty() ? 0 : (m_eventsMap.begin()->first / IN_MILLISECONDS)); }

        Player* GetPlayer(uint32 guid);
        Player* GetActivePlayer();
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
        Unit* GetUnit(KnownObject const& object)
        {
            switch (object.m_type)
            {
                case TYPEID_UNIT:
                    return GetCreature(object.m_guid);
                case TYPEID_PLAYER:
                    return GetPlayer(object.m_guid);
            }
            return nullptr;
        }
        WorldObject* GetStoredObject(KnownObject const& object)
        {
            switch (object.m_type)
            {
                case TYPEID_UNIT:
                    return GetCreature(object.m_guid);
                case TYPEID_PLAYER:
                    return GetPlayer(object.m_guid);
                case TYPEID_GAMEOBJECT:
                    return GetGameObject(object.m_guid);
            }
            return nullptr;
        }
        bool GetCurrentClientPosition(WorldLocation& loc);
        uint32 GetCreatureEntryFromGuid(uint32 guid);
        uint32 GetGameObjectEntryFromGuid(uint32 guid);
        char const* GetCreatureName(uint32 entry);
        char const* GetGameObjectName(uint32 entry);
        char const* GetItemName(uint32 entry);
        std::string GetQuestName(uint32 entry);

        CreatureText const* GetCreatureTextTemplate(uint32 creatureId, uint32 groupId)
        {
            for (auto const& itr : m_creatureTextTemplates)
            {
                if (itr.creatureId == creatureId && itr.groupId == groupId)
                    return &itr;
            }
            return nullptr;
        }
        std::vector<KnownObject> const* GetSpellTargetList(uint32 listId)
        {
            auto const itr = m_spellCastGoTargets.find(listId);
            if (itr != m_spellCastGoTargets.end())
                return &itr->second;
            return nullptr;
        }
        G3D::Vector3* GetSpellPosition(uint32 id)
        {
            auto const itr = m_spellCastGoPositions.find(id);
            if (itr != m_spellCastGoPositions.end())
                return &itr->second;
            return nullptr;
        }

        std::shared_ptr<WaypointPath> GetOrCreateWaypoints(uint32 guid, bool useStartPosition);
        uint32 GetTotalMovementPointsForCreature(uint32 guid);
        std::string ListSniffedEventsForObject(KnownObject object, int32 sniffedEventType);

    protected:
        bool m_enabled = false;
        bool m_initialized = false;
        bool m_playersSpawned = false;
        uint32 m_currentSniffTime = 0;
        uint64 m_currentSniffTimeMs = 0;
        uint32 m_startTimeSniff = 0;
        uint32 m_timeDifference = 0;
        uint32 m_initialWorldStateSendTime = 0;
        std::map<uint32 /*state*/, uint32 /*value*/> m_initialWorldStates;
        SplinesMap m_characterMovementSplines;
        SplinesMap m_creatureMovementSplines;
        SplinesMap m_creatureMovementCombatSplines;
        std::map<uint32 /*guid*/, std::shared_ptr<WaypointPath>> m_creatureWaypoints;
        std::map<uint32 /*list_id*/, std::vector<KnownObject>> m_spellCastGoTargets;
        std::map<uint32 /*position_id*/, G3D::Vector3> m_spellCastGoPositions;
        std::unordered_map<uint32 /*guid*/, Creature*> m_creatures;
        std::unordered_map<uint32 /*guid*/, GameObject*> m_gameobjects;
        std::map<uint32 /*unixtime*/, uint32 /*guid*/> m_activePlayers;
        std::unordered_map<uint32 /*guid*/, ReplayBotAI*> m_playerBots;
        std::map<uint32 /*guid*/, CharacterTemplateEntry> m_characterTemplates;
        std::map<uint32 /*guid*/, CharacterMovementMap> m_characterMovements;
        std::vector<CreatureText> m_creatureTextTemplates;
        std::multimap<uint64, std::shared_ptr<SniffedEvent>> m_eventsMap;
};

#define sReplayMgr MaNGOS::Singleton<ReplayMgr>::Instance()

#endif
