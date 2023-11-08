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
#include "RestSocket.h"
#include "http_parser.h"
#include "ProtobufJSON.h"
#include "Login.pb.h"
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

int on_url(http_parser* parser, const char* at, size_t length)
{
    printf("Method: %d, Url: %.*s", parser->method, (int)length, at);
    if (parser->method == HTTP_GET)
    {
        if (length == (sizeof(g_urlLoginForm) - 1) &&
            memcmp(at, g_urlLoginForm, length) == 0)
        {
            ((RestSocket*)parser->data)->HandleGetForm();
        }
    }
    return 0;
}

void RestSocket::OnAccept()
{
    sLog.Out(LOG_BASIC, LOG_LVL_BASIC, "[RestSocket::OnAccept] Accepting connection from '%s'", get_remote_address().c_str());

    // Initialize http parser
    m_settings.on_url = on_url;
    http_parser_init(&m_parser, HTTP_REQUEST);
    m_parser.data = this;
}

// Read the packet from the client
void RestSocket::OnRead()
{
    sLog.Out(LOG_BASIC, LOG_LVL_MINIMAL, "[RestSocket::OnRead] Received %u bytes", recv_len());

    std::vector<char> buf;
    buf.resize(recv_len());
    recv(buf.data(), buf.size());

    int nparsed = http_parser_execute(&m_parser, &m_settings, buf.data(), buf.size());

    if (m_parser.upgrade)
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
    std::string jsonResponse = JSON::Serialize(response);

    ByteBuffer buffer;
    WriteResponseHeader(buffer, jsonResponse);
    send((char const*)buffer.contents(), buffer.size());;
}

void RestSocket::HandleGetForm()
{
    return SendResponse(g_formInputs);
}
