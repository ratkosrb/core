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

// \addtogroup realmd
// @{
// \file

#ifndef _AUTHSOCKET_H
#define _AUTHSOCKET_H

#include "Common.h"
#include "Auth/BigNumber.h"
#include "Auth/Sha1.h"
#include "SRP6/SRP6.h"
#include "ByteBuffer.h"
#include "Utilities/MessageBuffer.h"
#include "BufferedSocket.h"
#include "rpc_types.pb.h"
#include <google/protobuf/message.h>

namespace pb = google::protobuf;

class ServiceBase;

namespace bgs
{
    namespace protocol
    {
        class Variant;

        namespace account
        {
            namespace v1
            {
                class GetAccountStateRequest;
                class GetAccountStateResponse;
                class GetGameAccountStateRequest;
                class GetGameAccountStateResponse;
            }
        }

        namespace authentication
        {
            namespace v1
            {
                class LogonRequest;
                class VerifyWebCredentialsRequest;
            }
        }

        namespace game_utilities
        {
            namespace v1
            {
                class ClientRequest;
                class ClientResponse;
                class GetAllValuesForAttributeRequest;
                class GetAllValuesForAttributeResponse;
            }
        }
    }
}

using namespace bgs::protocol;

struct PINData
{
    uint8 salt[16];
    uint8 hash[20];
};

enum LockFlag
{
    NONE            = 0x00,
    IP_LOCK         = 0x01,
    FIXED_PIN       = 0x02,
    TOTP            = 0x04,
    ALWAYS_ENFORCE  = 0x08,
    GEO_COUNTRY     = 0x10,
    GEO_CITY        = 0x20
};

enum BNetPacketState
{
    BNET_PACKET_SIZE,
    BNET_PACKET_HEADER,
    BNET_PACKET_DATA
};

struct BNetPacketBuffer
{
    BNetPacketState currentState = BNET_PACKET_SIZE;
    uint16 headerSize = 0;
    std::unique_ptr<Header> header;
    ByteBuffer dataBuffer;

    void Reset()
    {
        currentState = BNET_PACKET_SIZE;
        headerSize = 0;
        header.reset();
        dataBuffer.clear();
    }
};

// Handle login commands
class BNetSocket: public BufferedSocket
{
    public:
        const static int s_BYTE_SIZE = 32;

        BNetSocket() = default;
        ~BNetSocket();

        static void InitTcpSSL();
        void SendResponse(uint32 token, pb::Message const* response);
        void SendResponse(uint32 token, uint32 status);
        void SendRequest(uint32 serviceHash, uint32 methodId, pb::Message const* request, std::function<void(MessageBuffer)> callback)
        {
            _responseCallbacks[_requestToken] = std::move(callback);
            SendRequest(serviceHash, methodId, request);
        }
        void SendRequest(uint32 serviceHash, uint32 methodId, pb::Message const* request);

        uint32 HandleLogon(authentication::v1::LogonRequest const* logonRequest, std::function<void(ServiceBase*, uint32, ::google::protobuf::Message const*)>& continuation);
        uint32 HandleVerifyWebCredentials(authentication::v1::VerifyWebCredentialsRequest const* verifyWebCredentialsRequest, std::function<void(ServiceBase*, uint32, ::google::protobuf::Message const*)>& continuation);

        void OnAccept();
        void OnRead();
        void LoadRealmlist(ByteBuffer &pkt);
        bool VerifyPinData(uint32 pin, const PINData& clientData);
        uint32 GenerateTotpPin(const std::string& secret, int interval);

    private:

        bool VerifyVersion();
        uint32 VerifyWebCredentials(std::string const& webCredentials, std::function<void(ServiceBase*, uint32, ::google::protobuf::Message const*)>& continuation);

        SRP6 srp;
        BigNumber m_reconnectProof;

        bool m_promptPin = false;

        BNetPacketBuffer m_packetBuffer;

        std::string m_login;
        std::string m_safelogin;
        std::string m_securityInfo;
        std::string m_lastIP;
        std::string m_email;

        BigNumber m_serverSecuritySalt;
        LockFlag m_lockFlags = NONE;
        uint32 m_gridSeed = 0;
        uint32 m_geoUnlockPIN = 0;

        std::string m_os;
        std::string m_locale;
        uint32 m_accountId = 0;
        uint32 m_lastRealmListRequest = 0;

        // Since GetLocaleByName() is _NOT_ bijective, we have to store the locale as a string. Otherwise we can't differ
        // between enUS and enGB, which is important for the patch system
        std::string m_localizationName;
        uint32 m_build = 0;

        AccountTypes GetSecurityOn(uint32 realmId) const;
        void LoadAccountSecurityLevels(uint32 accountId);
        bool GeographicalLockCheck();

        AccountTypes m_accountDefaultSecurityLevel = SEC_PLAYER;
        typedef std::map<uint32, AccountTypes> AccountSecurityMap;
        AccountSecurityMap m_accountSecurityOnRealm;

        std::unordered_map<uint32, std::function<void(MessageBuffer)>> _responseCallbacks;
        uint32 _requestToken = 0;
};
#endif
// @}
