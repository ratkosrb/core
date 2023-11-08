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

#include <ace/OS_NS_unistd.h>
#include <ace/OS_NS_fcntl.h>
#include <ace/OS_NS_sys_stat.h>

RestSocket::~RestSocket()
{
    
}

int on_url(http_parser* parser, const char* at, size_t length)
{
    printf("Method: %d, Url: %.*s", parser->method, (int)length, at);
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
