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

#ifndef _BANMANAGER_H
#define _BANMANAGER_H

#include "Common.h"
#include <shared_mutex>

class BanManager
{
public:
    static BanManager& Instance();
    void LoadIPBanList(bool silent = false);
    bool IsIPBanned(std::string const& ip) const;
private:
    std::map<std::string, uint32> m_ipBanned;
    mutable std::shared_timed_mutex m_ipBannedMutex;
};

#define sBanMgr BanManager::Instance()

#endif
