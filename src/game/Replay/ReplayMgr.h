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
    float speed_walk = 1;
    float speed_run = 1;
    float bounding_radius = 1;
    float combat_reach = 1;
    float mod_melee_haste = 1;
    float mod_ranged_haste = 1;
    uint32 base_attack_time = 0;
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

enum NPCFlags_Classic : uint32
{
    CLASSIC_UNIT_NPC_FLAG_GOSSIP                = 0x00000001,     // 100%
    CLASSIC_UNIT_NPC_FLAG_QUESTGIVER            = 0x00000002,     // 100%
    CLASSIC_UNIT_NPC_FLAG_UNK1                  = 0x00000004,
    CLASSIC_UNIT_NPC_FLAG_UNK2                  = 0x00000008,
    CLASSIC_UNIT_NPC_FLAG_TRAINER               = 0x00000010,     // 100%
    CLASSIC_UNIT_NPC_FLAG_TRAINER_CLASS         = 0x00000020,     // 100%
    CLASSIC_UNIT_NPC_FLAG_TRAINER_PROFESSION    = 0x00000040,     // 100%
    CLASSIC_UNIT_NPC_FLAG_VENDOR                = 0x00000080,     // 100%
    CLASSIC_UNIT_NPC_FLAG_VENDOR_AMMO           = 0x00000100,     // 100%, general goods vendor
    CLASSIC_UNIT_NPC_FLAG_VENDOR_FOOD           = 0x00000200,     // 100%
    CLASSIC_UNIT_NPC_FLAG_VENDOR_POISON         = 0x00000400,     // guessed
    CLASSIC_UNIT_NPC_FLAG_VENDOR_REAGENT        = 0x00000800,     // 100%
    CLASSIC_UNIT_NPC_FLAG_REPAIR                = 0x00001000,     // 100%
    CLASSIC_UNIT_NPC_FLAG_FLIGHTMASTER          = 0x00002000,     // 100%
    CLASSIC_UNIT_NPC_FLAG_SPIRITHEALER          = 0x00004000,     // guessed
    CLASSIC_UNIT_NPC_FLAG_SPIRITGUIDE           = 0x00008000,     // guessed
    CLASSIC_UNIT_NPC_FLAG_INNKEEPER             = 0x00010000,     // 100%
    CLASSIC_UNIT_NPC_FLAG_BANKER                = 0x00020000,     // 100%
    CLASSIC_UNIT_NPC_FLAG_PETITIONER            = 0x00040000,     // 100% 0xC0000 = guild petitions, 0x40000 = arena team petitions
    CLASSIC_UNIT_NPC_FLAG_TABARDDESIGNER        = 0x00080000,     // 100%
    CLASSIC_UNIT_NPC_FLAG_BATTLEMASTER          = 0x00100000,     // 100%
    CLASSIC_UNIT_NPC_FLAG_AUCTIONEER            = 0x00200000,     // 100%
    CLASSIC_UNIT_NPC_FLAG_STABLEMASTER          = 0x00400000,     // 100%
    CLASSIC_UNIT_NPC_FLAG_GUILD_BANKER          = 0x00800000,     //
    CLASSIC_UNIT_NPC_FLAG_SPELLCLICK            = 0x01000000,     //
    CLASSIC_UNIT_NPC_FLAG_PLAYER_VEHICLE        = 0x02000000,     // players with mounts that have vehicle data should have it set
    CLASSIC_UNIT_NPC_FLAG_MAILBOX               = 0x04000000,     // mailbox
    CLASSIC_UNIT_NPC_FLAG_ARTIFACT_POWER_RESPEC = 0x08000000,     // artifact powers reset
    CLASSIC_UNIT_NPC_FLAG_TRANSMOGRIFIER        = 0x10000000,     // transmogrification
    CLASSIC_UNIT_NPC_FLAG_VAULTKEEPER           = 0x20000000,     // void storage
    CLASSIC_UNIT_NPC_FLAG_WILD_BATTLE_PET       = 0x40000000,     // Pet that player can fight (Battle Pet)
    CLASSIC_UNIT_NPC_FLAG_BLACK_MARKET          = 0x80000000,     // black market
    CLASSIC_MAX_NPC_FLAGS                       = 32
};

inline uint32 ConvertClassicNpcFlagToVanilla(uint32 flag)
{
    switch (flag)
    {
        case CLASSIC_UNIT_NPC_FLAG_GOSSIP:
            return UNIT_NPC_FLAG_GOSSIP;
        case CLASSIC_UNIT_NPC_FLAG_QUESTGIVER:
            return UNIT_NPC_FLAG_QUESTGIVER;
        case CLASSIC_UNIT_NPC_FLAG_TRAINER:
        case CLASSIC_UNIT_NPC_FLAG_TRAINER_CLASS:
        case CLASSIC_UNIT_NPC_FLAG_TRAINER_PROFESSION:
            return UNIT_NPC_FLAG_TRAINER;
        case CLASSIC_UNIT_NPC_FLAG_VENDOR:
        case CLASSIC_UNIT_NPC_FLAG_VENDOR_AMMO:
        case CLASSIC_UNIT_NPC_FLAG_VENDOR_FOOD:
        case CLASSIC_UNIT_NPC_FLAG_VENDOR_POISON:
        case CLASSIC_UNIT_NPC_FLAG_VENDOR_REAGENT:
            return UNIT_NPC_FLAG_VENDOR;
        case CLASSIC_UNIT_NPC_FLAG_REPAIR:
            return UNIT_NPC_FLAG_REPAIR;
        case CLASSIC_UNIT_NPC_FLAG_FLIGHTMASTER:
            return UNIT_NPC_FLAG_FLIGHTMASTER;
        case CLASSIC_UNIT_NPC_FLAG_SPIRITHEALER:
            return UNIT_NPC_FLAG_SPIRITHEALER;
        case CLASSIC_UNIT_NPC_FLAG_SPIRITGUIDE:
            return UNIT_NPC_FLAG_SPIRITGUIDE;
        case CLASSIC_UNIT_NPC_FLAG_INNKEEPER:
            return UNIT_NPC_FLAG_INNKEEPER;
        case CLASSIC_UNIT_NPC_FLAG_BANKER:
            return UNIT_NPC_FLAG_BANKER;
        case CLASSIC_UNIT_NPC_FLAG_PETITIONER:
            return UNIT_NPC_FLAG_PETITIONER;
        case CLASSIC_UNIT_NPC_FLAG_TABARDDESIGNER:
            return UNIT_NPC_FLAG_TABARDDESIGNER;
        case CLASSIC_UNIT_NPC_FLAG_BATTLEMASTER:
            return UNIT_NPC_FLAG_BATTLEMASTER;
        case CLASSIC_UNIT_NPC_FLAG_AUCTIONEER:
            return UNIT_NPC_FLAG_AUCTIONEER;
        case CLASSIC_UNIT_NPC_FLAG_STABLEMASTER:
            return UNIT_NPC_FLAG_STABLEMASTER;
    }
    return 0;
}

inline uint32 ConvertClassicNpcFlagsToVanilla(uint32 flags)
{
    uint32 newFlags = 0;
    for (uint32 i = 0; i < CLASSIC_MAX_NPC_FLAGS; i++)
    {
        uint32 flag = (uint32)pow(2, i);
        if (flags & flag)
        {
            newFlags |= ConvertClassicNpcFlagToVanilla(flag);
        }
    }
    return newFlags;
}

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
        std::string ListSniffedEventsForObject(KnownObject object);

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
