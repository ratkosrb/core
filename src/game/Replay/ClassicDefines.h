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

#ifndef MANGOS_H_CLASSICDEFINES
#define MANGOS_H_CLASSICDEFINES

#include "Common.h"
#include "SharedDefines.h"
#include <string>

enum ClassicWeatherState : uint32
{
    WEATHER_STATE_FINE              = 0,
    WEATHER_STATE_FOG               = 1,
    WEATHER_STATE_DRIZZLE           = 2,
    WEATHER_STATE_LIGHT_RAIN        = 3,
    WEATHER_STATE_MEDIUM_RAIN       = 4,
    WEATHER_STATE_HEAVY_RAIN        = 5,
    WEATHER_STATE_LIGHT_SNOW        = 6,
    WEATHER_STATE_MEDIUM_SNOW       = 7,
    WEATHER_STATE_HEAVY_SNOW        = 8,
    WEATHER_STATE_LIGHT_SANDSTORM   = 22,
    WEATHER_STATE_MEDIUM_SANDSTORM  = 41,
    WEATHER_STATE_HEAVY_SANDSTORM   = 42,
    WEATHER_STATE_THUNDERS          = 86,
    WEATHER_STATE_BLACKRAIN         = 90,
    WEATHER_STATE_BLACKSNOW         = 106
};

enum ClassicNPCFlags : uint32
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

enum ClassicSpellHitInfo
{
    CLASSIC_HITINFO_UNK0 = 0x00000001, // unused - debug flag, probably debugging visuals, no effect in non-ptr client
    CLASSIC_HITINFO_AFFECTS_VICTIM = 0x00000002,
    CLASSIC_HITINFO_OFFHAND = 0x00000004,
    CLASSIC_HITINFO_UNK3 = 0x00000008, // unused (3.3.5a)
    CLASSIC_HITINFO_MISS = 0x00000010,
    CLASSIC_HITINFO_FULL_ABSORB = 0x00000020,
    CLASSIC_HITINFO_PARTIAL_ABSORB = 0x00000040,
    CLASSIC_HITINFO_FULL_RESIST = 0x00000080,
    CLASSIC_HITINFO_PARTIAL_RESIST = 0x00000100,
    CLASSIC_HITINFO_CRITICALHIT = 0x00000200,
    CLASSIC_HITINFO_UNK10 = 0x00000400,
    CLASSIC_HITINFO_UNK11 = 0x00000800,
    CLASSIC_HITINFO_UNK12 = 0x00001000,
    CLASSIC_HITINFO_BLOCK = 0x00002000,
    CLASSIC_HITINFO_UNK14 = 0x00004000, // set only if meleespellid is present//  no world text when victim is hit for 0 dmg(HideWorldTextForNoDamage?)
    CLASSIC_HITINFO_UNK15 = 0x00008000, // player victim?// something related to blod sprut visual (BloodSpurtInBack?)
    CLASSIC_HITINFO_GLANCING = 0x00010000,
    CLASSIC_HITINFO_CRUSHING = 0x00020000,
    CLASSIC_HITINFO_NO_ANIMATION = 0x00040000, // set always for melee spells and when no hit animation should be displayed
    CLASSIC_HITINFO_UNK19 = 0x00080000,
    CLASSIC_HITINFO_UNK20 = 0x00100000,
    CLASSIC_HITINFO_UNK21 = 0x00200000, // unused (3.3.5a)
    CLASSIC_HITINFO_UNK22 = 0x00400000,
    CLASSIC_HITINFO_RAGE_GAIN = 0x00800000,
    CLASSIC_HITINFO_FAKE_DAMAGE = 0x01000000, // enables damage animation even if no damage done, set only if no damage
    CLASSIC_HITINFO_UNK25 = 0x02000000,
    CLASSIC_HITINFO_UNK26 = 0x04000000
};

inline uint32 ConvertClassicHitInfoFlagToVanilla(uint32 flag)
{
    switch (flag)
    {
        case CLASSIC_HITINFO_UNK0:
            return HITINFO_UNK0;
        case CLASSIC_HITINFO_AFFECTS_VICTIM:
            return HITINFO_AFFECTS_VICTIM;
        case CLASSIC_HITINFO_OFFHAND:
            return HITINFO_LEFTSWING;
        case CLASSIC_HITINFO_UNK3:
            return HITINFO_UNK3;
        case CLASSIC_HITINFO_MISS:
            return HITINFO_MISS;
        case CLASSIC_HITINFO_FULL_ABSORB:
            return HITINFO_ABSORB;
        case CLASSIC_HITINFO_PARTIAL_ABSORB:
            return HITINFO_ABSORB;
        case CLASSIC_HITINFO_FULL_RESIST:
            return HITINFO_RESIST;
        case CLASSIC_HITINFO_PARTIAL_RESIST:
            return HITINFO_RESIST;
        case CLASSIC_HITINFO_CRITICALHIT:
            return HITINFO_CRITICALHIT;
        case CLASSIC_HITINFO_GLANCING:
            return HITINFO_GLANCING;
        case CLASSIC_HITINFO_CRUSHING:
            return HITINFO_CRUSHING;
        case CLASSIC_HITINFO_NO_ANIMATION:
            return HITINFO_NOACTION;
    }

    return 0;
}

inline uint32 ConvertClassicHitInfoFlagsToVanilla(uint32 flags)
{
    uint32 newFlags = 0;
    for (uint32 i = 0; i < 32; i++)
    {
        uint32 flag = (uint32)pow(2, i);
        if (flags & flag)
        {
            newFlags |= ConvertClassicHitInfoFlagToVanilla(flag);
        }
    }
    return newFlags;
}

enum class ClassicMovementFlag : uint32
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

inline uint32 ConvertMovementFlags(uint32 flags)
{
    uint32 newFlags = 0;
    if (flags & (uint32)ClassicMovementFlag::Forward)
        newFlags |= MOVEFLAG_FORWARD;
    if (flags & (uint32)ClassicMovementFlag::Backward)
        newFlags |= MOVEFLAG_BACKWARD;
    if (flags & (uint32)ClassicMovementFlag::StrafeLeft)
        newFlags |= MOVEFLAG_STRAFE_LEFT;
    if (flags & (uint32)ClassicMovementFlag::StrafeRight)
        newFlags |= MOVEFLAG_STRAFE_RIGHT;
    if (flags & (uint32)ClassicMovementFlag::Left)
        newFlags |= MOVEFLAG_TURN_LEFT;
    if (flags & (uint32)ClassicMovementFlag::Right)
        newFlags |= MOVEFLAG_TURN_RIGHT;
    if (flags & (uint32)ClassicMovementFlag::PitchUp)
        newFlags |= MOVEFLAG_PITCH_UP;
    if (flags & (uint32)ClassicMovementFlag::PitchDown)
        newFlags |= MOVEFLAG_PITCH_DOWN;
    if (flags & (uint32)ClassicMovementFlag::Walking)
        newFlags |= MOVEFLAG_WALK_MODE;
    if (flags & (uint32)ClassicMovementFlag::Root)
        newFlags |= MOVEFLAG_ROOT;
    if (flags & (uint32)ClassicMovementFlag::Falling)
        newFlags |= MOVEFLAG_JUMPING;
    if (flags & (uint32)ClassicMovementFlag::FallingFar)
        newFlags |= MOVEFLAG_FALLINGFAR;
    if (flags & (uint32)ClassicMovementFlag::Swimming)
        newFlags |= MOVEFLAG_SWIMMING;
    if (flags & (uint32)ClassicMovementFlag::CanFly)
        newFlags |= MOVEFLAG_CAN_FLY;
    if (flags & (uint32)ClassicMovementFlag::Flying)
        newFlags |= MOVEFLAG_FLYING;
    if (flags & (uint32)ClassicMovementFlag::Waterwalking)
        newFlags |= MOVEFLAG_WATERWALKING;
    if (flags & (uint32)ClassicMovementFlag::FallingSlow)
        newFlags |= MOVEFLAG_SAFE_FALL;
    if (flags & (uint32)ClassicMovementFlag::Hover)
        newFlags |= MOVEFLAG_HOVER;
    return newFlags;
}

enum class ClassicChatMessageType : uint8
{
    System = 0,
    Say = 1,
    Party = 2,
    Raid = 3,
    Guild = 4,
    Officer = 5,
    Yell = 6,
    Whisper = 7,
    Whisper2 = 8,
    WhisperInform = 9,
    Emote = 10,
    TextEmote = 11,
    MonsterSay = 12,
    MonsterParty = 13,
    MonsterYell = 14,
    MonsterWhisper = 15,
    MonsterEmote = 16,
    Channel = 17,
    ChannelJoin = 18,
    ChannelLeave = 19,
    ChannelList = 20,
    ChannelNotice = 21,
    ChannelNoticeUser = 22,
    Afk = 23,
    Dnd = 24,
    Ignored = 25,
    Skill = 26,
    Loot = 27,
    Money = 28,
    Opening = 29,
    Tradeskills = 30,
    PetInfo = 31,
    CombatMiscInfo = 32,
    CombatXpGain = 33,
    CombatHonorGain = 34,
    CombatFactionChange = 35,
    BgSystemNeutral = 36,
    BgSystemAlliance = 37,
    BgSystemHorde = 38,
    RaidLeader = 39,
    RaidWarning = 40,
    RaidBossEmote = 41,
    RaidBossWhisper = 42,
    Filtered = 43,
    Restricted = 44,
    //unused1 = 45,
    Achievement = 46,
    GuildAchievement = 47,
    //unused2 = 48,
    PartyLeader = 49,
    Targeticons = 50,
    BnWhisper = 51,
    BnWhisperInform = 52,
    BnConversation = 53,
    BnConversationNotice = 54,
    BnConversationList = 55,
    BnInlineToastAlert = 56,
    BnInlineToastBroadcast = 57,
    BnInlineToastBroadcastInform = 58,
    BnInlineToastConversation = 59,
    BnWhisperPlayerOffline = 60,
    CombatGuildXpGain = 61,
    Battleground = 62,
    BattlegroundLeader = 63,
    PetBattleCombatLog = 64,
    PetBattleInfo = 65,
    InstanceChat = 66,
    InstanceChatLeader = 67,
};

inline ChatMsg ConvertClassicChatTypeToVanilla(uint8 chatType)
{
    switch (ClassicChatMessageType(chatType))
    {
        case ClassicChatMessageType::Say:
            return CHAT_MSG_SAY;
        case ClassicChatMessageType::Party:
            return CHAT_MSG_PARTY;
        case ClassicChatMessageType::Raid:
            return CHAT_MSG_RAID;
        case ClassicChatMessageType::Guild:
            return CHAT_MSG_GUILD;
        case ClassicChatMessageType::Officer:
            return CHAT_MSG_OFFICER;
        case ClassicChatMessageType::Yell:
            return CHAT_MSG_YELL;
        case ClassicChatMessageType::Whisper:
            return CHAT_MSG_WHISPER;
        case ClassicChatMessageType::Whisper2:
            return CHAT_MSG_WHISPER;
        case ClassicChatMessageType::WhisperInform:
            return CHAT_MSG_WHISPER_INFORM;
        case ClassicChatMessageType::Emote:
            return CHAT_MSG_EMOTE;
        case ClassicChatMessageType::TextEmote:
            return CHAT_MSG_TEXT_EMOTE;
        case ClassicChatMessageType::Channel:
            return CHAT_MSG_CHANNEL;
        case ClassicChatMessageType::BgSystemNeutral:
            return CHAT_MSG_BG_SYSTEM_NEUTRAL;
        case ClassicChatMessageType::BgSystemAlliance:
            return CHAT_MSG_BG_SYSTEM_ALLIANCE;
        case ClassicChatMessageType::BgSystemHorde:
            return CHAT_MSG_BG_SYSTEM_HORDE;
        case ClassicChatMessageType::RaidLeader:
            return CHAT_MSG_RAID_LEADER;
        case ClassicChatMessageType::RaidWarning:
            return CHAT_MSG_RAID_WARNING;
        case ClassicChatMessageType::PartyLeader:
            return CHAT_MSG_PARTY;
        case ClassicChatMessageType::BnWhisper:
            return CHAT_MSG_WHISPER;
        case ClassicChatMessageType::BnWhisperInform:
            return CHAT_MSG_WHISPER_INFORM;
        case ClassicChatMessageType::Battleground:
            return CHAT_MSG_BATTLEGROUND;
        case ClassicChatMessageType::BattlegroundLeader:
            return CHAT_MSG_BATTLEGROUND_LEADER;
        case ClassicChatMessageType::InstanceChat:
            return CHAT_MSG_PARTY;
        case ClassicChatMessageType::InstanceChatLeader:
            return CHAT_MSG_PARTY;
    }
    return CHAT_MSG_SAY;
}

#endif
