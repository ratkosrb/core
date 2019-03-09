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

#ifndef _ACCMGR_H
#define _ACCMGR_H

#include "Common.h"
#include "Policies/Singleton.h"
#include <string>
#include <functional>
#include "SharedDefines.h"

enum AccountOpResult
{
    AOR_OK,
    AOR_NAME_TOO_LONG,
    AOR_PASS_TOO_LONG,
    AOR_NAME_ALREDY_EXIST,
    AOR_NAME_NOT_EXIST,
    AOR_DB_INTERNAL_ERROR
};


class WorldSession;
class ChatHandler;
class MasterPlayer;

#define MAX_ACCOUNT_STR 16

class AccountPersistentData
{
/// WHISP FLOOD
public:
    struct WhisperData
    {
        WhisperData() : first_whisp(time(NULL)), score(0), whispers_count(0) {}
        time_t first_whisp;
        uint32 score;
        uint32 whispers_count;
    };
    void WhisperedBy(MasterPlayer* whisperer);
    uint32 CountWhispersTo(MasterPlayer* from, MasterPlayer* player);
    uint32 GetWhisperScore(MasterPlayer* from, MasterPlayer* player) const;
    uint32 CountDifferentWhispTargets() const { return _whisperTargets.size(); }

    typedef std::map<uint32 /*lowguid*/, WhisperData> WhispersMap;
    WhispersMap _whisperTargets;

/// MAIL FLOOD
public:
    void JustMailed(uint32 toAccount);
    bool CanMail(uint32 targetAccount);
protected:
    typedef std::map<uint32, time_t> MailsSentMap;
    MailsSentMap _mailsSent;
};

enum DelayedActionType
{
    DAA_SILENCE,
    DAA_KICK,
    DAA_BAN
};

class DelayedAction
{
public:
    DelayedActionType GetActionType() const { return _action; }
    virtual void Execute() = 0;
    uint32 GetDelay() const { return _delay; }
    uint32 GetTimer() const { return _timer; }

protected:
    DelayedAction(DelayedActionType action, uint32 delay) : _action(action), _delay(delay)
    {
        _timer = time(nullptr) + delay;
    }

    DelayedActionType _action;
    uint32 _delay;
    uint32 _timer;
};

class DelayedBanAction : public DelayedAction
{
public:
    DelayedBanAction(uint32 banAccountId, const std::string& source, uint32 duration, const std::string& reason, uint32 delay);
    BanMode GetBanMode() const { return _mode; }
    uint32 GetDuration() const { return _duration; }
    std::string& GetReason() { return _reason; }
    std::string& GetAuthor() { return _author; }
    uint32 GetBanAccountId() { return _banAccountId; }

    void Execute() override;

private:
    DelayedActionType _action;
    BanMode _mode;
    uint32 _duration;
    std::string _reason;
    std::string _author;
    uint32 _banAccountId;
};

class DelayedKickAction : public DelayedAction
{
public:
    DelayedKickAction(uint32 kickAccountId, uint32 delay)
        : DelayedAction(DAA_KICK, delay), _kickAccountId(kickAccountId)
    {
    }

    void Execute() override;
private:
    uint32 _kickAccountId;
};

class DelayedSilenceAction : public DelayedAction
{
public:
    DelayedSilenceAction(uint32 silenceAccountId, uint32 delay)
        : DelayedAction(DAA_SILENCE, delay), _silenceAccountId(silenceAccountId)
    {
    }

    void Execute() override;
private:
    uint32 _silenceAccountId;
};

class AccountMgr
{
    public:
        AccountMgr();
        ~AccountMgr();

        AccountOpResult CreateAccount(std::string username, std::string password);
        AccountOpResult DeleteAccount(uint32 accid);
        AccountOpResult ChangeUsername(uint32 accid, std::string new_uname, std::string new_passwd);
        AccountOpResult ChangePassword(uint32 accid, std::string new_passwd, std::string username="");
        bool CheckPassword(uint32 accid, std::string passwd, std::string username="");

        uint32 GetId(std::string username);

        void Load();
        AccountTypes GetSecurity(uint32 acc_id);
        void SetSecurity(uint32 accId, AccountTypes sec);

        bool GetName(uint32 acc_id, std::string &name);
        uint32 GetCharactersCount(uint32 acc_id);
        std::string CalculateShaPassHash(std::string& name, std::string& password);

        static bool normalizeString(std::string& utf8str);
        // Nostalrius
        void Update(uint32 diff);
        void LoadIPBanList(bool silent=false);
        void LoadAccountBanList(bool silent=false);
        void BanIP(std::string const& ip, uint32 unbandate) { _ipBanned[ip] = unbandate; }
        void UnbanIP(std::string const& ip) { _ipBanned.erase(ip); }
        void BanAccount(uint32 account, uint32 unbandate) { _accountBanned[account] = unbandate; }
        void UnbanAccount(uint32 acc) { _accountBanned.erase(acc); }
        bool IsIPBanned(std::string const& ip) const;
        bool IsAccountBanned(uint32 acc) const;
        // Max instance reset per account per hour
        bool CheckInstanceCount(uint32 accountId, uint32 instanceId, uint32 maxCount);
        void AddInstanceEnterTime(uint32 accountId, uint32 instanceId, time_t enterTime);

        void AddDelayedAction(DelayedAction* action);

        AccountPersistentData& GetAccountPersistentData(uint32 accountId) { return _accountPersistentData[accountId]; }
    protected:
        std::map<uint32, AccountTypes> _accountSecurity;
        uint32 _banlistUpdateTimer;
        std::map<std::string, uint32> _ipBanned;
        std::map<uint32, uint32> _accountBanned;
        typedef std::map<uint32 /* instanceId */, time_t /* enter time */> InstanceEnterTimesMap;
        typedef std::map<uint32 /* accountId */, InstanceEnterTimesMap> AccountInstanceEnterTimesMap;
        AccountInstanceEnterTimesMap _instanceEnterTimes;
        std::map<uint32, AccountPersistentData> _accountPersistentData;

        void ProcessDelayedActions();

        ACE_Thread_Mutex _delayedActionMutex;
        std::set<DelayedAction*, std::function<bool(const DelayedAction* lhs, const DelayedAction* rhs)>> _delayedActions;
};

#define sAccountMgr MaNGOS::Singleton<AccountMgr>::Instance()
#endif
