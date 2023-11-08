/*
 * Copyright (C) 2005-2011 MaNGOS <http://getmangos.com/>
 * Copyright (C) 2009-2011 MaNGOSZero <https://github.com/mangos/zero>
 * Copyright (C) 2011-2016 Nostalrius <https://nostalrius.org>
 * Copyright (C) 2016-2017 Elysium Project <https://github.com/elysium-project>
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
 */

/** \file
    \ingroup realmd
*/

#include "Common.h"
#include "Auth/Hmac.h"
#include "Auth/base32.h"
#include "Database/DatabaseEnv.h"
#include "Config/Config.h"
#include "Log.h"
#include "RealmList.h"
#include "BNetSocket.h"
#include "SystemConfig.h"
#include "Util.h"
#include "BattlenetRpcErrorCodes.h"
#include "Service.h"
#include "ServiceDispatcher.h"
#include "JSON/ProtobufJSON.h"
#include "RealmList.pb.h"
#include <google/protobuf/message.h>

#ifdef USE_SENDGRID
#include "MailerService.h"
#include "SendgridMail.h"
#endif

#include <openssl/md5.h>
#include <ctime>

#include <ace/OS_NS_unistd.h>
#include <ace/OS_NS_fcntl.h>
#include <ace/OS_NS_sys_stat.h>

enum AccountFlags
{
    ACCOUNT_FLAG_GM         = 0x00000001,
    ACCOUNT_FLAG_TRIAL      = 0x00000008,
    ACCOUNT_FLAG_PROPASS    = 0x00800000,
};

BNetSocket::~BNetSocket()
{
    
}

AccountTypes BNetSocket::GetSecurityOn(uint32 realmId) const
{
    AccountSecurityMap::const_iterator it = m_accountSecurityOnRealm.find(realmId);
    if (it == m_accountSecurityOnRealm.end())
        return m_accountDefaultSecurityLevel;
    return it->second;
}

void BNetSocket::OnAccept()
{
    sLog.Out(LOG_BASIC, LOG_LVL_BASIC, "[BNetSocket::OnAccept] Accepting connection from '%s'", get_remote_address().c_str());
}

// Read the packet from the client
void BNetSocket::OnRead()
{
    sLog.Out(LOG_BASIC, LOG_LVL_MINIMAL, "[BNetSocket::OnRead] Received %u bytes", recv_len());

    if (m_packetBuffer.currentState == BNET_PACKET_SIZE)
    {
        if (recv_len() < sizeof(uint16))
            return;

        recv((char*)&m_packetBuffer.headerSize, sizeof(uint16));
        EndianConvertReverse(m_packetBuffer.headerSize);
        m_packetBuffer.currentState = BNET_PACKET_HEADER;
        sLog.Out(LOG_BASIC, LOG_LVL_MINIMAL, "[BNetSocket::OnRead] Header size is %u", m_packetBuffer.headerSize);
    }

    if (m_packetBuffer.currentState == BNET_PACKET_HEADER)
    {
        if (recv_len() < m_packetBuffer.headerSize)
            return;

        m_packetBuffer.dataBuffer.resize(m_packetBuffer.headerSize);
        recv((char*)m_packetBuffer.dataBuffer.contents(), m_packetBuffer.headerSize);

        m_packetBuffer.header = std::make_unique<Header>();
        if (!m_packetBuffer.header->ParseFromArray(m_packetBuffer.dataBuffer.contents(), m_packetBuffer.headerSize))
        {
            m_packetBuffer.Reset();
            recv_skip(recv_len());
            return;
        }

        m_packetBuffer.dataBuffer.clear();
        m_packetBuffer.currentState = BNET_PACKET_DATA;
        sLog.Out(LOG_BASIC, LOG_LVL_MINIMAL, "[BNetSocket::OnRead] Service id is %u, service hash is %u, size is %u", m_packetBuffer.header->service_id(), m_packetBuffer.header->service_hash(), m_packetBuffer.header->size());
    }

    if (m_packetBuffer.currentState == BNET_PACKET_DATA)
    {
        if (recv_len() < m_packetBuffer.header->size())
            return;

        m_packetBuffer.dataBuffer.resize(m_packetBuffer.header->size());
        recv((char*)m_packetBuffer.dataBuffer.contents(), m_packetBuffer.header->size());

        MessageBuffer msgBuffer;
        msgBuffer.Write(m_packetBuffer.dataBuffer.contents(), m_packetBuffer.dataBuffer.size());

        if (m_packetBuffer.header->service_id() != 0xFE)
        {
            sServiceDispatcher.Dispatch(this, m_packetBuffer.header->service_hash(), m_packetBuffer.header->token(), m_packetBuffer.header->method_id(), std::move(msgBuffer));
        }

        m_packetBuffer.Reset();
    }
}

void BNetSocket::SendResponse(uint32 token, pb::Message const* response)
{
    sLog.Out(LOG_BASIC, LOG_LVL_MINIMAL, "[BNetSocket::SendResponse] token %u", token);

    Header header;
    header.set_token(token);
    header.set_service_id(0xFE);
    header.set_size(response->ByteSize());

    uint16 headerSize = header.ByteSize();
    EndianConvertReverse(headerSize);

    MessageBuffer packet;
    packet.Write(&headerSize, sizeof(headerSize));
    uint8* ptr = packet.GetWritePointer();
    packet.WriteCompleted(header.ByteSize());
    header.SerializeToArray(ptr, header.ByteSize());
    ptr = packet.GetWritePointer();
    packet.WriteCompleted(response->ByteSize());
    response->SerializeToArray(ptr, response->ByteSize());

    send((char*)packet.GetBasePointer(), packet.GetActiveSize());
}

void BNetSocket::SendResponse(uint32 token, uint32 status)
{
    sLog.Out(LOG_BASIC, LOG_LVL_MINIMAL, "[BNetSocket::SendResponse] token %u status %u", token, status);

    Header header;
    header.set_token(token);
    header.set_status(status);
    header.set_service_id(0xFE);

    uint16 headerSize = header.ByteSize();
    EndianConvertReverse(headerSize);

    MessageBuffer packet;
    packet.Write(&headerSize, sizeof(headerSize));
    uint8* ptr = packet.GetWritePointer();
    packet.WriteCompleted(header.ByteSize());
    header.SerializeToArray(ptr, header.ByteSize());

    send((char*)packet.GetBasePointer(), packet.GetActiveSize());
}

void BNetSocket::SendRequest(uint32 serviceHash, uint32 methodId, pb::Message const* request)
{
    sLog.Out(LOG_BASIC, LOG_LVL_MINIMAL, "[BNetSocket::SendRequest] serviceHash %u methodId %u", serviceHash, methodId);

    Header header;
    header.set_service_id(0);
    header.set_service_hash(serviceHash);
    header.set_method_id(methodId);
    header.set_size(request->ByteSize());
    header.set_token(_requestToken++);

    uint16 headerSize = header.ByteSize();
    EndianConvertReverse(headerSize);

    MessageBuffer packet;
    packet.Write(&headerSize, sizeof(headerSize));
    uint8* ptr = packet.GetWritePointer();
    packet.WriteCompleted(header.ByteSize());
    header.SerializeToArray(ptr, header.ByteSize());
    ptr = packet.GetWritePointer();
    packet.WriteCompleted(request->ByteSize());
    request->SerializeToArray(ptr, request->ByteSize());

    send((char*)packet.GetBasePointer(), packet.GetActiveSize());
}

uint32 BNetSocket::HandleLogon(authentication::v1::LogonRequest const* logonRequest, std::function<void(ServiceBase*, uint32, ::google::protobuf::Message const*)>& continuation)
{
    m_locale = logonRequest->locale();
    m_os = logonRequest->platform();
    m_build = logonRequest->application_version();
    
    if (logonRequest->program() != "WoW")
    {
        sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "[BNetSocket::LogonRequest] %s attempted to log in with game other than WoW (using %s)!", get_remote_address().c_str(), logonRequest->program().c_str());
        return ERROR_BAD_PROGRAM;
    }

    if (m_os != "Win" && m_os != "Wn64" && m_os != "Mc64")
    {
        sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "[BNetSocket::LogonRequest] %s attempted to log in from an unsupported platform (using %s)!", get_remote_address().c_str(), logonRequest->platform().c_str());
        return ERROR_BAD_PLATFORM;
    }

    if (GetLocaleByName(m_locale) == LOCALE_enUS && m_locale != "enUS")
    {
        sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "[BNetSocket::LogonRequest] %s attempted to log in with unsupported locale (using %s)!", get_remote_address().c_str(), logonRequest->locale().c_str());
        return ERROR_BAD_LOCALE;
    }

    if (!VerifyVersion())
    {
        sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "[BNetSocket::LogonRequest] %s attempted to log in with unsupported client build (using %u)!", get_remote_address().c_str(), m_build);
        return ERROR_BAD_VERSION;
    }

    sLog.Out(LOG_BASIC, LOG_LVL_BASIC, "[BNetSocket::LogonRequest] %s trying to login. OS: %s Build: %u Locale: %s", get_remote_address().c_str(), m_os.c_str(), m_build, m_locale.c_str());

    if (logonRequest->has_cached_web_credentials())
        return VerifyWebCredentials(logonRequest->cached_web_credentials(), continuation);

    challenge::v1::ChallengeExternalRequest externalChallenge;
    externalChallenge.set_payload_type("web_auth_url");
    std::string restAddress = "https://" + sConfig.GetStringDefault("LoginREST.ExternalAddress", "127.0.0.1") + ":" + std::to_string(sConfig.GetIntDefault("LoginREST.Port", DEFAULT_REST_PORT)) + "/bnetserver/login/";
    externalChallenge.set_payload(restAddress);
    Battlenet::Service<challenge::v1::ChallengeListener>(this).OnExternalChallenge(&externalChallenge);
    return ERROR_OK;
}

uint32 BNetSocket::HandleVerifyWebCredentials(authentication::v1::VerifyWebCredentialsRequest const* verifyWebCredentialsRequest, std::function<void(ServiceBase*, uint32, ::google::protobuf::Message const*)>& continuation)
{
    if (verifyWebCredentialsRequest->has_web_credentials())
        return VerifyWebCredentials(verifyWebCredentialsRequest->web_credentials(), continuation);

    return ERROR_DENIED;
}

uint32 BNetSocket::VerifyWebCredentials(std::string const& webCredentials, std::function<void(ServiceBase*, uint32, ::google::protobuf::Message const*)>& continuation)
{
    if (webCredentials.empty())
        return ERROR_DENIED;

    sLog.Out(LOG_BASIC, LOG_LVL_BASIC, "[BNetSocket::VerifyWebCredentials] Login Ticket %s", webCredentials.c_str());

    // Verify that this IP is not in the ip_banned table
    // No SQL injection possible (paste the IP address as passed by the socket)
    std::string address = get_remote_address();
    LoginDatabase.escape_string(address);
    std::unique_ptr<QueryResult> result(LoginDatabase.PQuery("SELECT `unbandate` FROM `ip_banned` WHERE "
        //    permanent                    still banned
        "(`unbandate` = `bandate` OR `unbandate` > UNIX_TIMESTAMP()) AND `ip` = '%s'", address.c_str()));
    if (result)
    {
        sLog.Out(LOG_BASIC, LOG_LVL_BASIC, "[BNetSocket::VerifyWebCredentials] Banned ip '%s' tries to login!", get_remote_address().c_str());
        return ERROR_DENIED;
    }

    std::string safeTicket = webCredentials;
    LoginDatabase.escape_string(safeTicket);

    // Get the account details from the account table
    // No SQL injection (escaped login ticket)
    //                                         0     1           2         3          4                      5              6       7
    result.reset(LoginDatabase.PQuery("SELECT `id`, `username`, `locked`, `last_ip`, `login_ticket_expiry`, `email_verif`, `email`, UNIX_TIMESTAMP(`joindate`) FROM `account` WHERE `login_ticket` = '%s'", safeTicket.c_str()));
    if (!result)
        return ERROR_DENIED;

    Field* fields = result->Fetch();

    m_accountId = fields[0].GetUInt32();
    m_login = fields[1].GetCppString();
    LockFlag lockFlags = (LockFlag)fields[2].GetUInt32();
    m_lastIP = fields[3].GetString();
    uint32 loginTicketExpiry = fields[4].GetUInt32();
    bool verified = fields[5].GetBool();
    m_email = fields[6].GetCppString();
    uint32 joinDate = fields[7].GetUInt32();

    if (loginTicketExpiry < time(nullptr))
        return ERROR_TIMED_OUT;

    if (GeographicalLockCheck())
        return ERROR_GAME_ACCOUNT_LOCKED;

    // Prevent login if the user's email address has not been verified
    bool requireVerification = sConfig.GetBoolDefault("ReqEmailVerification", false);
    int32 requireEmailSince = sConfig.GetIntDefault("ReqEmailSince", 0);
    
    // Prevent login if the user's join date is bigger than the timestamp in configuration
    if (requireEmailSince > 0)
        requireVerification = requireVerification && (joinDate >= uint32(requireEmailSince));

    if (requireVerification && !verified)
    {
        sLog.Out(LOG_BASIC, LOG_LVL_BASIC, "[BNetSocket::VerifyWebCredentials] Account '%s' using IP '%s' tries to login before verifying email", m_login.c_str(), get_remote_address().c_str());
        return ERROR_GAME_ACCOUNT_LOCKED;
    }

    // If the IP is 'locked', check that the player comes indeed from the correct IP address
    if (lockFlags & IP_LOCK)
    {
        sLog.Out(LOG_BASIC, LOG_LVL_DEBUG, "[BNetSocket::VerifyWebCredentials] Account '%s' is locked to IP - '%s'", m_login.c_str(), m_lastIP.c_str());
        sLog.Out(LOG_BASIC, LOG_LVL_DEBUG, "[BNetSocket::VerifyWebCredentials] Player address is '%s'", get_remote_address().c_str());

        if (m_lastIP != get_remote_address())
        {
            sLog.Out(LOG_BASIC, LOG_LVL_DEBUG, "[BNetSocket::VerifyWebCredentials] Account IP differs");
            return ERROR_RISK_ACCOUNT_LOCKED;
        }
    }

    // If the account is banned, reject the logon attempt
    result.reset(LoginDatabase.PQuery("SELECT `bandate`, `unbandate` FROM `account_banned` WHERE "
        "`id` = %u AND `active` = 1 AND (`unbandate` > UNIX_TIMESTAMP() OR `unbandate` = `bandate`) LIMIT 1", m_accountId));

    if (result)
    {
        if ((*result)[0].GetUInt64() == (*result)[1].GetUInt64())
        {
            sLog.Out(LOG_BASIC, LOG_LVL_BASIC, "[BNetSocket::VerifyWebCredentials] Banned account '%s' using IP '%s' tries to login!", m_login.c_str(), get_remote_address().c_str());
            return ERROR_GAME_ACCOUNT_BANNED;
        }
        else
        {
            sLog.Out(LOG_BASIC, LOG_LVL_BASIC, "[BNetSocket::VerifyWebCredentials] Temporarily banned account '%s' using IP '%s' tries to login!", m_login.c_str(), get_remote_address().c_str());
            return ERROR_GAME_ACCOUNT_SUSPENDED;
        }
    }

    //                                         0          1
    result.reset(LoginDatabase.PQuery("SELECT `realmid`, `numchars` FROM `realmcharacters` WHERE `acctid`='%u'", m_accountId));
    if (result)
    {
        Field* fields = result->Fetch();
        uint32 realmId = fields[0].GetUInt32();
        uint8 count = fields[1].GetUInt8();
        m_characterCounts[Battlenet::RealmHandle{ 1, 1, realmId }.GetAddress()] = count;
    }

    //                                         0           1                 2                 3
    result.reset(LoginDatabase.PQuery("SELECT `realm_id`, `character_name`, `character_guid`, `last_played_time` FROM `last_played_character` WHERE `account_id`=%u", m_accountId));
    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            Battlenet::RealmHandle realmId{ 1, 1, fields[0].GetUInt32() };
            LastPlayedCharacterInfo& lastPlayedCharacter = m_lastPlayedCharacters[realmId.GetSubRegionAddress()];

            lastPlayedCharacter.RealmId = realmId;
            lastPlayedCharacter.CharacterName = fields[1].GetString();
            lastPlayedCharacter.CharacterGUID = fields[2].GetUInt64();
            lastPlayedCharacter.LastPlayedTime = fields[3].GetUInt32();

        } while (result->NextRow());
    }

    authentication::v1::LogonResult logonResult;
    logonResult.set_error_code(0);
    logonResult.mutable_account_id()->set_low(m_accountId);
    logonResult.mutable_account_id()->set_high(UI64LIT(0x100000000000000));

    //for (auto itr = _accountInfo->GameAccounts.begin(); itr != _accountInfo->GameAccounts.end(); ++itr)
    {
        EntityId* gameAccountId = logonResult.add_game_account_id();
        gameAccountId->set_low(m_accountId);
        gameAccountId->set_high(UI64LIT(0x200000200576F57));
    }

    BigNumber k;
    k.SetRand(8 * 64);
    logonResult.set_session_key(k.AsByteArray(64).data(), 64);

    m_authed = true;
    Battlenet::Service<authentication::v1::AuthenticationListener>(this).OnLogonComplete(&logonResult);
    return ERROR_OK;
}

void BNetSocket::LoadRealmlist(ByteBuffer &pkt)
{
    for (RealmList::RealmMap::const_iterator i = sRealmList.begin(); i != sRealmList.end(); ++i)
    {
        uint8 AmountOfCharacters;

        // No SQL injection. id of realm is controlled by the database.
        QueryResult* result = LoginDatabase.PQuery("SELECT `numchars` FROM `realmcharacters` WHERE `realmid` = '%d' AND `acctid`='%u'", i->second.m_ID, m_accountId);
        if (result)
        {
            Field* fields = result->Fetch();
            AmountOfCharacters = fields[0].GetUInt8();
            delete result;
        }
        else
            AmountOfCharacters = 0;

        bool ok_build = std::find(i->second.realmbuilds.begin(), i->second.realmbuilds.end(), m_build) != i->second.realmbuilds.end();

        RealmBuildInfo const* buildInfo = ok_build ? FindBuildInfo(m_build) : nullptr;
        if (!buildInfo)
            buildInfo = &i->second.realmBuildInfo;

        RealmFlags realmflags = i->second.realmflags;

        // 1.x clients not support explicitly REALM_FLAG_SPECIFYBUILD, so manually form similar name as show in more recent clients
        std::string name = i->first;
        if (realmflags & REALM_FLAG_SPECIFYBUILD)
        {
            char buf[20];
            snprintf(buf, 20, " (%u,%u,%u)", buildInfo->majorVersion, buildInfo->minorVersion, buildInfo->bugfixVersion);
            name += buf;
        }

        // Show offline state for unsupported client builds and locked realms (1.x clients not support locked state show)
        if (!ok_build || (i->second.allowedSecurityLevel > GetSecurityOn(i->second.m_ID)))
            realmflags = RealmFlags(realmflags | REALM_FLAG_OFFLINE);

        // WRITE REALM TO PACKET
    }
}

void BNetSocket::LoadAccountSecurityLevels(uint32 accountId)
{
    QueryResult* result = LoginDatabase.PQuery("SELECT `gmlevel`, `RealmID` FROM `account_access` WHERE `id` = %u",
        accountId);
    if (!result)
        return;

    do
    {
        Field *fields = result->Fetch();
        AccountTypes security = AccountTypes(fields[0].GetUInt32());
        int realmId = fields[1].GetInt32();
        if (realmId < 0)
            m_accountDefaultSecurityLevel = security;
        else
            m_accountSecurityOnRealm[realmId] = security;
    } while (result->NextRow());

    delete result;
}

bool BNetSocket::GeographicalLockCheck()
{
    if (!sConfig.GetBoolDefault("GeoLocking", false))
    {
        return false;
    }

    if (m_lastIP.empty() || m_lastIP == get_remote_address())
    {
        return false;
    }

    if ((m_lockFlags & GEO_CITY) == 0 && (m_lockFlags & GEO_COUNTRY) == 0)
    {
        return false;
    }

    auto result = std::unique_ptr<QueryResult>(LoginDatabase.PQuery(
        "SELECT INET_ATON('%s') AS ip, network_start_integer, geoname_id, registered_country_geoname_id "
        "FROM geoip "
        "WHERE network_last_integer >= INET_ATON('%s') "
        "ORDER BY network_last_integer ASC LIMIT 1",
        get_remote_address().c_str(), get_remote_address().c_str())
        );

    auto result_prev = std::unique_ptr<QueryResult>(LoginDatabase.PQuery(
        "SELECT INET_ATON('%s') AS ip, network_start_integer, geoname_id, registered_country_geoname_id "
        "FROM geoip "
        "WHERE network_last_integer >= INET_ATON('%s') "
        "ORDER BY network_last_integer ASC LIMIT 1",
        m_lastIP.c_str(), m_lastIP.c_str())
        );

    if (!result && !result_prev)
    {
        return false;
    }

    // If only one of the queries returns a result, assume location has changed
    if ((result && !result_prev) || (!result && result_prev))
    {
        return true;
    }

    uint32_t net_start = result->Fetch()[1].GetUInt32();
    uint32_t net_start_prev = result_prev->Fetch()[1].GetUInt32();
    uint32_t ip = result->Fetch()[0].GetUInt32();
    uint32_t ip_prev = result_prev->Fetch()[0].GetUInt32();

    /* The optimised query will return the next highest range in the event
     * of the address not being found in the database. Therefore, we need
     * to perform a second check to ensure our address falls within
     * the returned range.
     * See: https://blog.jcole.us/2007/11/24/on-efficiently-geo-referencing-ips-with-maxmind-geoip-and-mysql-gis/
     */
    if (net_start > ip || net_start_prev > ip_prev)
    {
        return false;
    }

    std::string geoname_id = result->Fetch()[2].GetString();
    std::string country_geoname_id = result->Fetch()[3].GetString();
    std::string prev_geoname_id = result_prev->Fetch()[2].GetString();
    std::string prev_country_geoname_id = result_prev->Fetch()[3].GetString();

    if (m_lockFlags & GEO_CITY)
    {
        return geoname_id != prev_geoname_id;
    }
    else
    {
        return country_geoname_id != prev_country_geoname_id;
    }
}

bool BNetSocket::VerifyVersion()
{
    std::vector<RealmBuildInfo const*> allowedClients = FindBuildInfo(m_build, m_os);
    if (allowedClients.empty())
        return false;

    return true;
}
