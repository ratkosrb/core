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
#include "AuthSocket.h"
#include "AuthCodes.h"
#include "PatchHandler.h"
#include "Util.h"
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
//#include "Util.h" -- for commented utf8ToUpperOnlyLatin

#include <ace/OS_NS_unistd.h>
#include <ace/OS_NS_fcntl.h>
#include <ace/OS_NS_sys_stat.h>

enum AccountFlags
{
    ACCOUNT_FLAG_GM         = 0x00000001,
    ACCOUNT_FLAG_TRIAL      = 0x00000008,
    ACCOUNT_FLAG_PROPASS    = 0x00800000,
};

// Close patch file descriptor before leaving
AuthSocket::~AuthSocket()
{
    
}

AccountTypes AuthSocket::GetSecurityOn(uint32 realmId) const
{
    AccountSecurityMap::const_iterator it = m_accountSecurityOnRealm.find(realmId);
    if (it == m_accountSecurityOnRealm.end())
        return m_accountDefaultSecurityLevel;
    return it->second;
}

constexpr auto TCP_SSL_VERSION_LIST = "tlsv1,tlsv1.1,tlsv1.2,tlsv1.3";

void AuthSocket::InitTcpSSL()
{
    ACE_SSL_Context::instance()->certificate("bnetserver.cert.pem", SSL_FILETYPE_PEM);
    ACE_SSL_Context::instance()->private_key("bnetserver.key.pem", SSL_FILETYPE_PEM);

    ACE_SSL_Context::instance()->filter_versions(TCP_SSL_VERSION_LIST);
    auto sslHandler = ACE_SSL_Context::instance()->context();
    // Enable ECDH cipher
    if (!SSL_CTX_set_ecdh_auto(sslHandler, 1))
    {
        sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "SSL_CTX_set_ecdh_auto  failed: %s", std::strerror(errno));
    }
    auto ciphers = "ALL:!RC4:!SSLv3:+HIGH:!MEDIUM:!LOW";
    // auto ciphers = "HIGH:!aNULL:!eNULL:!kECDH:!aDH:!RC4:!3DES:!CAMELLIA:!MD5:!PSK:!SRP:!KRB5:@STRENGTH";
    if (!SSL_CTX_set_cipher_list(sslHandler, ciphers))
    {
        sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "SSL_CTX_set_cipher_list  failed: %s", std::strerror(errno));
    }
    SSL_CTX_clear_options(sslHandler, SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION);
}

// Accept the connection and set the s random value for SRP6
void AuthSocket::OnAccept()
{
    sLog.Out(LOG_BASIC, LOG_LVL_BASIC, "Accepting connection from '%s'", get_remote_address().c_str());
}

// Read the packet from the client
void AuthSocket::OnRead()
{
    size_t recvLen = recv_len();
    std::vector<uint8> buf;
    buf.resize(recvLen);
    recv((char*)&buf[0], recvLen);
    sLog.Out(LOG_BASIC, LOG_LVL_MINIMAL, "Received %u bytes", recvLen);

    ASSERT(buf.size() > sizeof(uint16));
    uint16 headerSize = *((uint16*)buf.data());
    EndianConvertReverse(headerSize);
    sLog.Out(LOG_BASIC, LOG_LVL_MINIMAL, "Header size is %u", headerSize);

    Header header;
    ASSERT(header.ParseFromArray(buf.data() + sizeof(uint16), headerSize));
    sLog.Out(LOG_BASIC, LOG_LVL_MINIMAL, "Service id is %u, service hash is %u", header.service_id(), header.service_hash());

    MessageBuffer msgBuffer;
    msgBuffer.Write(buf.data() + sizeof(uint16) + header.size(), recvLen - (sizeof(uint16) + header.size()));

    if (header.service_id() != 0xFE)
    {
        sServiceDispatcher.Dispatch(this, header.service_hash(), header.token(), header.method_id(), std::move(msgBuffer));
    }
}

void AuthSocket::SendResponse(uint32 token, pb::Message const* response)
{
    sLog.Out(LOG_BASIC, LOG_LVL_MINIMAL, "SendResponse: token %u", token);

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

    send((char*)packet.GetBasePointer(), packet.GetBufferSize());
}

void AuthSocket::SendResponse(uint32 token, uint32 status)
{
    sLog.Out(LOG_BASIC, LOG_LVL_MINIMAL, "SendResponse: token %u status %u", token, status);

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

    send((char*)packet.GetBasePointer(), packet.GetBufferSize());
}

void AuthSocket::SendRequest(uint32 serviceHash, uint32 methodId, pb::Message const* request)
{
    sLog.Out(LOG_BASIC, LOG_LVL_MINIMAL, "SendResponse: serviceHash %u methodId %u", serviceHash, methodId);

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

    send((char*)packet.GetBasePointer(), packet.GetBufferSize());
}

void AuthSocket::LoadRealmlist(ByteBuffer &pkt)
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

// Verify PIN entry data
bool AuthSocket::VerifyPinData(uint32 pin, const PINData& clientData)
{
    // remap the grid to match the client's layout
    std::vector<uint8> grid { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    std::vector<uint8> remappedGrid(grid.size());

    uint8* remappedIndex = remappedGrid.data();
    uint32 seed = m_gridSeed;

    for (size_t i = grid.size(); i > 0; --i)
    {
        auto remainder = seed % i;
        seed /= i;
        *remappedIndex = grid[remainder];

        size_t copySize = i;
        copySize -= remainder;
        --copySize;

        uint8* srcPtr = grid.data() + remainder + 1;
        uint8* dstPtr = grid.data() + remainder;

        std::copy(srcPtr, srcPtr + copySize, dstPtr);
        ++remappedIndex;
    }

    // convert the PIN to bytes (for ex. '1234' to {1, 2, 3, 4})
    std::vector<uint8> pinBytes;

    while (pin != 0)
    {
        pinBytes.push_back(pin % 10);
        pin /= 10;
    }

    std::reverse(pinBytes.begin(), pinBytes.end());

    // validate PIN length
    if (pinBytes.size() < 4 || pinBytes.size() > 10)
        return false; // PIN outside of expected range

    // remap the PIN to calculate the expected client input sequence
    for (size_t i = 0; i < pinBytes.size(); ++i)
    {
        auto index = std::find(remappedGrid.begin(), remappedGrid.end(), pinBytes[i]);
        pinBytes[i] = std::distance(remappedGrid.begin(), index);
    }

    // convert PIN bytes to their ASCII values
    for (size_t i = 0; i < pinBytes.size(); ++i)
        pinBytes[i] += 0x30;

    // validate the PIN, x = H(client_salt | H(server_salt | ascii(pin_bytes)))
    Sha1Hash sha;
    sha.UpdateData(m_serverSecuritySalt.AsByteArray());
    sha.UpdateData(pinBytes.data(), pinBytes.size());
    sha.Finalize();

    BigNumber hash, clientHash;
    hash.SetBinary(sha.GetDigest(), sha.GetLength());
    clientHash.SetBinary(clientData.hash, 20);

    sha.Initialize();
    sha.UpdateData(clientData.salt, sizeof(clientData.salt));
    sha.UpdateData(hash.AsByteArray());
    sha.Finalize();
    hash.SetBinary(sha.GetDigest(), sha.GetLength());

    return !memcmp(hash.AsDecStr(), clientHash.AsDecStr(), 20);
}

uint32 AuthSocket::GenerateTotpPin(const std::string& secret, int interval) {
    std::vector<uint8> decoded_key((secret.size() + 7) / 8 * 5);
    int key_size = base32_decode((const uint8_t*)secret.data(), decoded_key.data(), decoded_key.size());

    if (key_size == -1)
    {
        sLog.Out(LOG_BASIC, LOG_LVL_DEBUG, "Unable to base32 decode TOTP key for user %s", m_safelogin.c_str());
        return -1;
    }

    // not guaranteed by the standard to be the UNIX epoch but it is on all supported platforms
    auto time = std::time(nullptr);
    uint64 now = static_cast<uint64>(time);
    uint64 step = static_cast<uint64>((floor(now / 30))) + interval;
    EndianConvertReverse(step);

    HmacHash hmac(decoded_key.data(), key_size);
    hmac.UpdateData((uint8*)&step, sizeof(step));
    hmac.Finalize();

    auto hmac_result = hmac.GetDigest();

    unsigned int offset = hmac_result[19] & 0xF;
    std::uint32_t pin = (hmac_result[offset] & 0x7f) << 24 | (hmac_result[offset + 1] & 0xff) << 16
        | (hmac_result[offset + 2] & 0xff) << 8 | (hmac_result[offset + 3] & 0xff);
    EndianConvert(pin);

    pin &= 0x7FFFFFFF;
    pin %= 1000000;
    return pin;
}

void AuthSocket::LoadAccountSecurityLevels(uint32 accountId)
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

bool AuthSocket::GeographicalLockCheck()
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

bool AuthSocket::VerifyVersion(uint8 const* a, int32 aLength, uint8 const* versionProof, bool isReconnect)
{
    std::vector<RealmBuildInfo const*> allowedClients = FindBuildInfo(m_build, m_os, m_platform);
    if (allowedClients.empty())
        return false;

    return true;
}
