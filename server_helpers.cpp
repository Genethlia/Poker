#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>

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

std::vector<shared_ptr<Client>> Server::orderedActivePlayers()
{
    std::vector<shared_ptr<Client>> players;
    for (auto &c : state.clients)
    {
        if (c->connected && c->ready && !c->spectator && c->seated)
        {
            players.push_back(c);
        }
    }
    return players;
}

std::vector<Server::SidePot> Server::buildSidePots()
{
    vector<shared_ptr<Client>> players = orderedActivePlayers();

    players.erase(remove_if(players.begin(), players.end(), [this](const shared_ptr<Client> &c)
                            { return c->totalBetThisHand == 0; }),
                  players.end());

    sort(players.begin(), players.end(), [](const shared_ptr<Client> &a, const shared_ptr<Client> &b)
         { return a->totalBetThisHand < b->totalBetThisHand; });

    vector<SidePot> sidePots;

    int lastLevel = 0;

    for (auto &p : players)
    {
        int level = p->totalBetThisHand;

        if (level == lastLevel)
            continue;

        int gap = level - lastLevel;

        vector<int> contributors;
        vector<int> eligible;

        for (auto &c : players)
        {
            if (c->totalBetThisHand >= level)
            {
                contributors.push_back(c->id);
                if (c->inHand)
                    eligible.push_back(c->id);
            }
        }

        SidePot sp;

        sp.amount = gap * contributors.size();
        sp.eligiblePlayers = eligible;

        if (sp.amount > 0 && !sp.eligiblePlayers.empty())
            sidePots.push_back(sp);

        lastLevel = level;
    }
    return sidePots;
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

string Server::getLocalIp()
{
    ULONG bufferSize = 15000;
    vector<char> buffer(bufferSize);

    IP_ADAPTER_ADDRESSES *addresses =
        reinterpret_cast<IP_ADAPTER_ADDRESSES *>(buffer.data());

    DWORD result = GetAdaptersAddresses(
        AF_INET,
        GAA_FLAG_SKIP_ANYCAST |
            GAA_FLAG_SKIP_MULTICAST |
            GAA_FLAG_SKIP_DNS_SERVER,
        nullptr,
        addresses,
        &bufferSize);

    if (result == ERROR_BUFFER_OVERFLOW)
    {
        buffer.resize(bufferSize);
        addresses = reinterpret_cast<IP_ADAPTER_ADDRESSES *>(buffer.data());

        result = GetAdaptersAddresses(
            AF_INET,
            GAA_FLAG_SKIP_ANYCAST |
                GAA_FLAG_SKIP_MULTICAST |
                GAA_FLAG_SKIP_DNS_SERVER,
            nullptr,
            addresses,
            &bufferSize);
    }

    if (result != NO_ERROR)
    {
        cerr << "GetAdaptersAddresses failed. Error: " << result << "\n";
        return "127.0.0.1";
    }

    for (IP_ADAPTER_ADDRESSES *adapter = addresses; adapter != nullptr; adapter = adapter->Next)
    {
        if (adapter->OperStatus != IfOperStatusUp)
            continue;

        for (IP_ADAPTER_UNICAST_ADDRESS *addr = adapter->FirstUnicastAddress;
             addr != nullptr;
             addr = addr->Next)
        {
            sockaddr_in *ipv4 = reinterpret_cast<sockaddr_in *>(addr->Address.lpSockaddr);

            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(ipv4->sin_addr), ip, INET_ADDRSTRLEN);

            string ipString = ip;

            if (ipString == "127.0.0.1")
                continue;

            // Prefer normal LAN IPs
            if (ipString.rfind("192.168.", 0) == 0 ||
                ipString.rfind("10.", 0) == 0 ||
                ipString.rfind("172.", 0) == 0)
            {
                return ipString;
            }
        }
    }

    return "127.0.0.1";
}

std::string Server::ipToRoomCode(const std::string &ip)
{
    stringstream ss(ip);
    string part;
    stringstream code;

    while (getline(ss, part, '.'))
    {
        int num = stoi(part);
        code << char('A' + num / 16) << char('A' + num % 16);
    }
    return code.str();
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

void Server::broadcastShowCardsOf(vector<Server::SidePot> &sidePots)
{
    set<int> playerIdsToShow;

    for (const auto &p : sidePots)
    {
        for (int id : p.eligiblePlayers)
        {
            auto p = find_client_by_id(id);
            if (p && p->inHand)
            {
                playerIdsToShow.insert(id);
            }
        }
    }

    for (int id : playerIdsToShow)
    {

        state.broadcast_all(serialize_server(MessageServerToClient{
            .type = MessageTypeServerToClient::ShowCardsOf,
            .playerId = id}));
    }
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
    p->totalBetThisHand += actualAmount;
    state.pot += actualAmount;

    if (p->money == 0)
        p->allin = true;
}

void Server::scheduleNextGame()
{
    nextGameTimer.expires_after(std::chrono::seconds(5));
    nextGameTimer.async_wait([this](const boost::system::error_code &ec)
                             {
                                 if (ec)
                                 {
                                     return;
                                 }
                                 if(!state.all_ready())
                                 {
                                     return;
                                 }
                                 if(gameInProgress)
                                 {
                                     return;
                                 }
                                play_game(); });
}

void Server::broadcastUnorderedMapUpdates()
{
    state.broadcast_all(serialize_server(MessageServerToClient{
        .type = MessageTypeServerToClient::UnorderedMapUpdate,
        .playerNames = state.buildNameSnapshot(),
        .playerMoney = state.buildMoneySnapshot(),
        .isSpectatorMap = state.buildSpectatorSnapshot(),
        .isSeatedMap = state.buildSeatedSnapshot(),
        .betThisRoundMap = state.buildBetThisRoundSnapshot()}));
}
