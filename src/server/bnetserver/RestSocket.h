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

#ifndef _RESTSOCKET_H
#define _RESTSOCKET_H

#include "Common.h"
#include "ByteBuffer.h"
#include "BufferedSocket.h"
#include "Login.pb.h"
#include "http_parser.h"
#include <memory>

struct ParsedHttpPacket
{
    uint32 method = 0;
    std::string url;
    std::string body;
};

// Handle login commands
class RestSocket: public BufferedSocket
{
    public:
        RestSocket() = default;
        ~RestSocket();

        // Called from BufferedSocket
        void OnAccept();
        void OnRead();

        // Called from http-parser
        void SetParsedUrl(uint32 method, char const* at, size_t length);
        void SetParsedBody(uint32 method, char const* at, size_t length);
        void SetParsingDone(uint32 method);
        
    private:
        void WriteResponseHeader(ByteBuffer& buffer, std::string const& content);
        void SendResponse(google::protobuf::Message const& response);
        void HandleGetForm();
        void HandlePostLogin(std::string const& body);
        std::unique_ptr<http_parser_settings> m_settings;
        std::unique_ptr<http_parser> m_parser;
        ParsedHttpPacket m_parsedHttpPacket;
};
#endif
// @}
