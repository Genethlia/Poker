#include "server.h"
using namespace std;

int Server::countInHand()
{
    int count = 0;
    for (auto &client : state.clients)
    {
        if (client->inHand)
            count++;
    }
    return count;
}
int Server::CountCanAct()
{
    int n = 0;
    for (auto &c : state.clients)
    {
        if (c->inHand && !c->allin)
            n++;
    }
    return n;
}

vector<shared_ptr<Client>> Server::activePlayers()
{
    vector<shared_ptr<Client>> active;
    for (auto &client : state.clients)
    {
        if (client->inHand && !client->allin)
            active.push_back(client);
    }
    return active;
}

int Server::countInHand(ServerState &st)
{
    int n = 0;
    for (auto &c : st.clients)
        if (c->inHand)
            n++;
    return n;
}

vector<int> Server::orderedActiveIds()
{
    vector<int> ids;
    for (auto &c : state.clients)
        if (c->inHand && !c->allin)
            ids.push_back(c->id);
    sort(ids.begin(), ids.end());
    return ids;
}

int Server::nextIdNeedingAction(int startId)
{
    auto ids = orderedActiveIds();
    if (ids.empty())
        return -1;

    int startIndex = 0;
    for (int i = 0; i < (int)ids.size(); i++)
        if (ids[i] == startId)
        {
            startIndex = i;
            break;
        }

    for (int k = 0; k < (int)ids.size(); k++)
    {
        int pid = ids[(startIndex + k) % ids.size()];
        if (state.needsAction.count(pid))
            return pid;
    }
    return ids[0];
}

int Server::indexOfPlayerId(const vector<int> &playerIds, int id)
{
    for (int i = 0; i < (int)playerIds.size(); i++)
    {
        if (playerIds[i] == id)
            return i;
    }
    return -1;
}

int Server::nextId(const vector<int> &playerIds, int id)
{
    if (playerIds.empty())
        return -1;

    int pos = indexOfPlayerId(playerIds, id);

    if (pos == -1)
        return playerIds[0];

    return playerIds[(pos + 1) % playerIds.size()];
}

string Server::findNameById(int id)
{
    auto p = find_client_by_id(id);
    if (p)
        return p->display_name();
    return "Unknown";
}

shared_ptr<Client> Server::find_client_by_id(int id)
{
    for (auto &client : state.clients)
    {
        if (client->id == id)
            return client;
    }
    return nullptr;
}

void Server::broadcastGameState()
{
    state.broadcast_all(serialize_server(MessageServerToClient{
        .type = MessageTypeServerToClient::GameState,
        .potAmount = state.pot,
        .gameState = state.gameState}));
}

void Server::broadcastBettingUpdate(int toCall)
{
    state.broadcast_all(serialize_server(MessageServerToClient{
        .type = MessageTypeServerToClient::BettingUpdate,
        .potAmount = state.pot,
        .toAct = state.toAct,
        .toCall = toCall,
        .currentBet = state.currentBet,
        .minRaise = state.minRaise,
        .dealerId = state.dealerId,
        .smallBlindId = state.smallBlindId,
        .bigBlindId = state.bigBlindId}));
}

void Server::broadcastActionResult(int playerId, PlayerActionType action, int actionAmount, bool ok)
{
    state.broadcast_all(serialize_server(MessageServerToClient{
        .type = MessageTypeServerToClient::ActionResult,
        .playerId = playerId,
        .potAmount = state.pot,
        .action = ok ? action : PlayerActionType::Failed,
        .actionAmount = actionAmount,
    }));
}

void Server::broadcastSpectatingUpdate(shared_ptr<Client> c)
{
    state.broadcast_all(serialize_server(MessageServerToClient{
        .type = MessageTypeServerToClient::SpectatingUpdate,
        .playerId = c->id,
        .isSpectator = c->spectator,
        .isSeated = c->seated}));
}

void Server::advanceDealer(const std::vector<int> &activeIds)
{
    if (activeIds.size() < 2)
    {
        state.dealerId = -1;
        return;
    }
    if (state.dealerId == -1 || indexOfPlayerId(activeIds, state.dealerId) == -1)
    {
        state.dealerId = activeIds[0];
        return;
    }

    state.dealerId = nextId(activeIds, state.dealerId);
}

void Server::chooseBlinds(const vector<int> &activeIds)
{
    int n = activeIds.size();
    if (n < 2)
    {
        state.smallBlindId = -1;
        state.bigBlindId = -1;
        return;
    }
    int dealerPos = indexOfPlayerId(activeIds, state.dealerId);

    if (dealerPos == -1)
    {
        state.dealerId = activeIds[0];
        dealerPos = 0;
    }

    if (n == 2)
    {
        state.smallBlindId = state.dealerId;
        state.bigBlindId = activeIds[(dealerPos + 1) % n];
    }
    else
    {
        state.smallBlindId = activeIds[(dealerPos + 1) % n];
        state.bigBlindId = activeIds[(dealerPos + 2) % n];
    }
}

void Server::postBlind(int playerId, int amount)
{
    auto p = find_client_by_id(playerId);
    if (!p)
        return;

    int actualAmount = min(amount, p->money);
    p->money -= actualAmount;
    p->betThisRound += actualAmount;
    state.pot += actualAmount;

    if (p->money == 0)
        p->allin = true;
}
