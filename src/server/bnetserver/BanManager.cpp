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

#include "BanManager.h"
#include "Log.h"
#include "Database/DatabaseEnv.h"

BanManager& BanManager::Instance()
{
    static BanManager banMgr;
    return banMgr;
}

void BanManager::LoadIPBanList(bool silent)
{
    if (!silent)
        sLog.Out(LOG_BASIC, LOG_LVL_MINIMAL, "Loading ip_banned ...");

    std::unique_ptr<QueryResult> banresult(LoginDatabase.PQuery("SELECT `ip`, `unbandate`, `bandate` FROM `ip_banned` WHERE (`unbandate` > UNIX_TIMESTAMP() OR `bandate` = `unbandate`)"));

    if (!banresult)
    {
        if (!silent)
        {
            sLog.Out(LOG_BASIC, LOG_LVL_MINIMAL, "");
            sLog.Out(LOG_BASIC, LOG_LVL_MINIMAL, ">> Loaded 0 ip bans");
        }
        return;
    }

    std::map<std::string, uint32> ipBanned;

    do
    {
        Field* fields = banresult->Fetch();
        uint32 unbandate = fields[1].GetUInt32();
        uint32 bandate = fields[2].GetUInt32();
        if (unbandate == bandate)
            unbandate = 0xFFFFFFFF;
        ipBanned[fields[0].GetString()] = unbandate;
    } while (banresult->NextRow());

    std::lock_guard<std::shared_timed_mutex> lock(m_ipBannedMutex);
    std::swap(ipBanned, m_ipBanned);

    if (!silent)
    {
        sLog.Out(LOG_BASIC, LOG_LVL_MINIMAL, "");
        sLog.Out(LOG_BASIC, LOG_LVL_MINIMAL, ">> Loaded %u ip bans", m_ipBanned.size());
    }
}

bool BanManager::IsIPBanned(std::string const& ip) const
{
    std::shared_lock<std::shared_timed_mutex> lock(m_ipBannedMutex);
    std::map<std::string, uint32>::const_iterator it = m_ipBanned.find(ip);
    return !(it == m_ipBanned.end() || it->second < time(nullptr));
}
