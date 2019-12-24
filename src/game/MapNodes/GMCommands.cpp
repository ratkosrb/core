#include "Chat.h"
#include "NodesMgr.h"
#include "NodeSession.h"
#include "WorldSession.h"
#include "Player.h"

bool ChatHandler::HandleNodeServersListCommand(char*)
{
    sNodesMgr->ListServers(*this);
    return true;
}

bool ChatHandler::HandleNodeServersSwitchCommand(char* args)
{
    uint32 nodeServerId = 0;
    if (!ExtractUInt32(&args, nodeServerId))
        return false;
    NodeSession* node = sNodesMgr->GetNodeById(nodeServerId);
    SetSentErrorMessage(true);
    if (!node || !node->IsReady())
    {
        PSendSysMessage("Node #%u not found or not ready.", nodeServerId);
        return false;
    }
    Player* me = GetSession()->GetPlayer();
    if (!me)
    {
        SendSysMessage("You are not on the right node.");
        return false;
    }
    GetSession()->LoginPlayerToNode(node);
    return true;
}

void NodesMgr::ListServers(ChatHandler& handler)
{
    handler.PSendSysMessage("%u nodes.", m_nodes.size());
    for (const auto& node : m_nodes)
        handler.PSendSysMessage("[%3u][%s] %s", node.first, node.second->IsConnectedToMaster() ? "MSTR" : "NODE", node.second->GetName());
}
