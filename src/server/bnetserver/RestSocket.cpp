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
#include "Database/DatabaseEnv.h"
#include "Config/Config.h"
#include "Log.h"
#include "Util.h"
#include "RestSocket.h"
#include "LockFlag.h"
#include "http_parser.h"
#include "ProtobufJSON.h"
#include "Login.pb.h"
#include "SRP6/SRP6.h"
#include <string>
#include <iostream>
#include <sstream>
#include <ace/OS_NS_unistd.h>
#include <ace/OS_NS_fcntl.h>
#include <ace/OS_NS_sys_stat.h>

static Battlenet::JSON::Login::FormInputs BuildLoginForm()
{
    // set up form inputs
    Battlenet::JSON::Login::FormInput* input;
    Battlenet::JSON::Login::FormInputs formInputs;

    formInputs.set_type(Battlenet::JSON::Login::LOGIN_FORM);
    input = formInputs.add_inputs();
    input->set_input_id("account_name");
    input->set_type("text");
    input->set_label("E-mail");
    input->set_max_length(320);

    input = formInputs.add_inputs();
    input->set_input_id("password");
    input->set_type("password");
    input->set_label("Password");
    input->set_max_length(16);

    input = formInputs.add_inputs();
    input->set_input_id("log_in_submit");
    input->set_type("submit");
    input->set_label("Log In");
    return formInputs;
}

static Battlenet::JSON::Login::FormInputs g_formInputs = BuildLoginForm();
static char const g_urlLoginForm[] = "/bnetserver/login/";

RestSocket::~RestSocket()
{
    
}

int HttpParserOnUrl(http_parser* parser, const char* at, size_t length)
{
    sLog.Out(LOG_BASIC, LOG_LVL_BASIC, "[HttpParserOnUrl] Method: %d, Url: %.*s", parser->method, (int)length, at);
    ((RestSocket*)parser->data)->SetParsedUrl(parser->method, at, length);
    return 0;
}

void RestSocket::SetParsedUrl(uint32 method, char const* at, size_t length)
{
    m_parsedHttpPacket.method = method;
    m_parsedHttpPacket.url = std::string(at, length);
}

int HttpParserOnBody(http_parser* parser, const char* at, size_t length)
{
    sLog.Out(LOG_BASIC, LOG_LVL_BASIC, "[HttpParserOnBody] Method: %d, Body: %.*s", parser->method, (int)length, at);
    ((RestSocket*)parser->data)->SetParsedBody(parser->method, at, length);
    return 0;
}

void RestSocket::SetParsedBody(uint32 method, char const* at, size_t length)
{
    ASSERT(m_parsedHttpPacket.method == method);
    m_parsedHttpPacket.body = std::string(at, length);
}

int HttpParserOnMessageComplete(http_parser* parser)
{
    sLog.Out(LOG_BASIC, LOG_LVL_BASIC, "[HttpParserOnMessageComplete] Method %d", parser->method);
    ((RestSocket*)parser->data)->SetParsingDone(parser->method);
    return 0;
}

void RestSocket::SetParsingDone(uint32 method)
{
    ASSERT(m_parsedHttpPacket.method == method);
    if (method == HTTP_GET)
    {
        if (m_parsedHttpPacket.url == g_urlLoginForm)
        {
            HandleGetForm();
        }
    }
    else if (method == HTTP_POST)
    {
        if (m_parsedHttpPacket.url == g_urlLoginForm)
        {
            HandlePostLogin(m_parsedHttpPacket.body);
        }
    }
}

void RestSocket::OnAccept()
{
    sLog.Out(LOG_BASIC, LOG_LVL_BASIC, "[RestSocket::OnAccept] Accepting connection from '%s'", get_remote_address().c_str());

    // Initialize http parser
    m_settings = std::make_unique<http_parser_settings>();
    m_settings->on_url = HttpParserOnUrl;
    m_settings->on_body = HttpParserOnBody;
    m_settings->on_message_complete = HttpParserOnMessageComplete;
    m_parser = std::make_unique<http_parser>();
    http_parser_init(m_parser.get(), HTTP_REQUEST);
    m_parser->data = this;
}

// Read the packet from the client
void RestSocket::OnRead()
{
    sLog.Out(LOG_BASIC, LOG_LVL_BASIC, "[RestSocket::OnRead] Received %u bytes", recv_len());

    std::vector<char> buf;
    buf.resize(recv_len());
    recv(buf.data(), buf.size());

    int nparsed = http_parser_execute(m_parser.get(), m_settings.get(), buf.data(), buf.size());

    if (m_parser->upgrade)
    {
        sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "[RestSocket::OnRead] Attempt to upgrade to new protocol!");
        close_connection();
    }
    else if (nparsed != buf.size())
    {
        sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "[RestSocket::OnRead] Failed to parse http request!");
        close_connection();
    }
}

void RestSocket::WriteResponseHeader(ByteBuffer& buffer, std::string const& content)
{
    std::stringstream ss;
    ss << "HTTP/1.1 200 Ok\r\n";
    ss << "Content-Length: " << content.length() << "\r\n";
    ss << "Content-Type: application/json;charset=UTF-8\r\n";
    ss << "\r\n";
    ss << content << "\r\n";

    std::string response = ss.str();
    buffer.append(response.c_str(), response.length());
}

void RestSocket::SendResponse(google::protobuf::Message const& response)
{
    sLog.Out(LOG_BASIC, LOG_LVL_BASIC, "[RestSocket::SendResponse] Sending %s", response.GetTypeName().c_str());

    std::string jsonResponse = JSON::Serialize(response);

    ByteBuffer buffer;
    WriteResponseHeader(buffer, jsonResponse);
    send((char const*)buffer.contents(), buffer.size());;
}

void RestSocket::HandleGetForm()
{
    return SendResponse(g_formInputs);
}

void RestSocket::HandlePostLogin(std::string const& body)
{
    Battlenet::JSON::Login::LoginForm loginForm;
    if (body.empty() || !JSON::Deserialize(body, &loginForm))
    {
        Battlenet::JSON::Login::LoginResult loginResult;
        loginResult.set_authentication_state(Battlenet::JSON::Login::LOGIN);
        loginResult.set_error_code("UNABLE_TO_DECODE");
        loginResult.set_error_message("There was an internal error while connecting to Battle.net. Please try again later.");
        SendResponse(loginResult);
        return;
    }

    std::string login;
    std::string password;

    for (int32 i = 0; i < loginForm.inputs_size(); ++i)
    {
        if (loginForm.inputs(i).input_id() == "account_name")
            login = loginForm.inputs(i).value();
        else if (loginForm.inputs(i).input_id() == "password")
            password = loginForm.inputs(i).value();
    }

    normalizeString(login);
    normalizeString(password);

    sLog.Out(LOG_BASIC, LOG_LVL_BASIC, "[RestSocket::HandlePostLogin] Name %s Password %s", login.c_str(), password.c_str());

    std::string safelogin = login;
    LoginDatabase.escape_string(safelogin);

    // Get the account details from the account table
    // No SQL injection (escaped user name)
    //                                                                0     1         2          3    4    5               6                      7              8       9
    std::unique_ptr<QueryResult> result(LoginDatabase.PQuery("SELECT `id`, `locked`, `last_ip`, `v`, `s`, `login_ticket`, `login_ticket_expiry`, `email_verif`, `email`, UNIX_TIMESTAMP(`joindate`) FROM `account` WHERE `username` = '%s'", safelogin.c_str()));
    if (result)
    {
        Field* fields = result->Fetch();

        uint32 accountId = fields[0].GetUInt32();

        // Prevent login if the user's email address has not been verified
        bool requireVerification = sConfig.GetBoolDefault("ReqEmailVerification", false);
        int32 requireEmailSince = sConfig.GetIntDefault("ReqEmailSince", 0);
        bool verified = fields[7].GetBool();

        // Prevent login if the user's join date is bigger than the timestamp in configuration
        if (requireEmailSince > 0)
        {
            uint32 t = fields[9].GetUInt32();
            requireVerification = requireVerification && (t >= uint32(requireEmailSince));
        }

        if (requireVerification && !verified)
        {
            sLog.Out(LOG_BASIC, LOG_LVL_BASIC, "[RestSocket::HandlePostLogin] Account '%s' using IP '%s' tries to login before verifying email", login.c_str(), get_remote_address().c_str());
            return;
        }

        // If the IP is 'locked', check that the player comes indeed from the correct IP address
        LockFlag lockFlags = (LockFlag)fields[1].GetUInt32();
        std::string lastIP = fields[2].GetString();
        std::string email = fields[8].GetCppString();

        if (lockFlags & IP_LOCK)
        {
            sLog.Out(LOG_BASIC, LOG_LVL_DEBUG, "[RestSocket::HandlePostLogin] Account '%s' is locked to IP - '%s'", login.c_str(), lastIP.c_str());
            sLog.Out(LOG_BASIC, LOG_LVL_DEBUG, "[RestSocket::HandlePostLogin] Player address is '%s'", get_remote_address().c_str());

            if (lastIP != get_remote_address())
            {
                sLog.Out(LOG_BASIC, LOG_LVL_DEBUG, "[RestSocket::HandlePostLogin] Account IP differs");
                return;
            }
            else
            {
                sLog.Out(LOG_BASIC, LOG_LVL_DEBUG, "[RestSocket::HandlePostLogin] Account IP matches");
            }
        }
        else
        {
            sLog.Out(LOG_BASIC, LOG_LVL_DEBUG, "[RestSocket::HandlePostLogin] Account '%s' is not locked to ip", login.c_str());
        }

        std::string databaseV = fields[3].GetCppString();
        std::string databaseS = fields[4].GetCppString();

        SRP6 serverSrp;
        if (!serverSrp.SetVerifier(databaseV.c_str()) || !serverSrp.SetSalt(databaseS.c_str()))
        {
            sLog.Out(LOG_BASIC, LOG_LVL_BASIC, "[RestSocket::HandlePostLogin] Broken v/s values in database for account '%s'!", login.c_str());
            return;
        }

        SRP6 clientSrp;
        clientSrp.CalculateVerifier(SRP6::CalculateShaPassHash(login, password), databaseS.c_str());

        if (clientSrp.GetVerifier().AsHexStr() != databaseV)
        {
            sLog.Out(LOG_BASIC, LOG_LVL_BASIC, "[RestSocket::HandlePostLogin] Account '%s' tries to login with wrong password!", login.c_str());
            return;
        }

        std::string loginTicket = fields[5].GetString();
        uint32 loginTicketExpiry = fields[6].GetUInt32();

        if (loginTicket.empty() || loginTicketExpiry < time(nullptr))
        {
            BigNumber ticket;
            ticket.SetRand(20 * 8);

            loginTicket = "TC-" + ByteArrayToHexStr(ticket.AsByteArray(20).data(), 20);
        }

        loginTicketExpiry = time(nullptr) + sConfig.GetIntDefault("LoginREST.TicketDuration", 3600);

        LoginDatabase.DirectPExecute("UPDATE `account` SET `login_ticket`='%s', `login_ticket_expiry`=%u WHERE `id`=%u", loginTicket.c_str(), loginTicketExpiry, accountId);

        Battlenet::JSON::Login::LoginResult loginResult;
        loginResult.set_authentication_state(Battlenet::JSON::Login::DONE);
        loginResult.set_login_ticket(loginTicket);
        SendResponse(loginResult);
    }
    else
    {
        sLog.Out(LOG_BASIC, LOG_LVL_BASIC, "[RestSocket::HandlePostLogin] %s tries to login to unknown account %s.", get_remote_address().c_str(), login.c_str());
    }
}
