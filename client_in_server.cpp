#include "client_in_server.hpp"
using namespace std;

void ServerState::send_to(const string &msg, int id)
{
    auto target = find_if(clients.begin(), clients.end(),
                          [id](const shared_ptr<Client> &c)
                          { return c->id == id; });
    if (target != clients.end() && (*target)->connected)
        (*target)->send(make_shared<string>(msg));
}

bool ServerState::all_ready() const
{
    int counter = 0;
    for (const auto &client : clients)
    {
        if (!client->connected || client->spectator || !client->seated)
            continue; // skip disconnected clients
        counter++;
        if (!client->ready)
            return false;
    }
    return counter >= 2;
}

void ServerState::reset_game()
{
    pot = 0;
    gameState = GameState::WaitingForPlayers;
    for (const auto &client : clients)
    {
        client->betThisRound = 0;
        client->totalBetThisHand = 0;
    }
}

void ServerState::broadcast_all(const string &msg)
{
    for (auto &client : clients)
    {
        if (client->connected)
            client->send(make_shared<string>(msg));
    }
}

Client::Client(tcp::socket s, ServerState *state)
    : socket(move(s)), serverState(state)
{
    ready = false;
    inHand = false;
    allin = false;
    hasPendingAction = false;
    connected = true;
    disconnectedHandled = false;
    betThisRound = 0;
    totalBetThisHand = 0;
}

void Client::start()
{
    read_line();
}

string Client::display_name() const
{
    if (id >= 0 && !name.empty())
        return name + "#" + to_string(id);
    return "UnAuthenticatedClient";
}

void Client::read_line()
{
    auto self = shared_from_this();

    boost::asio::async_read_until(
        socket,
        inbuf,
        '\n',
        [this, self](boost::system::error_code ec, size_t)
        {
            if (ec)
            {
                cout << "[" << display_name() << "]" << ec.message() << " disconnected\n";
                handleDisconnectOnce();
                return;
            }

            istream is(&inbuf);
            string line;
            getline(is, line);
            trim(line);

            handle_line(line);

            read_line();
        });
}

void Client::send(shared_ptr<string> msg)
{
    bool writing = !outbox.empty();
    outbox.push_back(std::move(msg));

    if (!writing)
    {
        do_write();
    }
}

void Client::do_write()
{
    auto self = shared_from_this();
    auto msg = outbox.front();

    boost::asio::async_write(
        socket,
        boost::asio::buffer(*msg),
        [this, self, msg](boost::system::error_code ec, size_t)
        {
            if (ec)
            {
                cout << "[" << display_name() << "] write error:" << ec.message() << "\n";
                handleDisconnectOnce();
                return;
            }

            outbox.pop_front();
            if (!outbox.empty())
            {
                do_write();
            }
        });
}

void Client::handleDisconnectOnce()
{
    if (disconnectedHandled)
        return;
    disconnectedHandled = true;
    connected = false;

    boost::system::error_code ec;
    socket.close(ec);

    if (on_disconnect_ptr)
        on_disconnect_ptr(id);
}

void Client::broadcast(const string &msg)
{
    auto buffer = make_shared<string>(msg);
    for (auto &c : serverState->clients)
    {
        if (c->connected)
            c->send(buffer);
    }
}

void Client::send_to(const std::string &msg, int id)
{
    auto target = find_if(serverState->clients.begin(), serverState->clients.end(),
                          [id](const shared_ptr<Client> &c)
                          { return c->id == id; });
    if (target != serverState->clients.end() && (*target)->connected)
    {
        (*target)->send(make_shared<string>(msg));
    }
}

void Client::handle_line(const string &line)
{
    MessageClientToServer msg = deserialize_client(line);
    MessageServerToClient response{};
    bool validMessage = true;
    switch (msg.type)
    {
    case MessageTypeClientToServer::Join:
    {
        int joinedPlayers = 0;
        for (auto &c : serverState->clients)
            if (c->id >= 0)
                joinedPlayers++;

        if (joinedPlayers >= 8)
        {
            cout << "Rejected join from [" << msg.name << "]: max players reached.\n";
            MessageServerToClient rejectMsg;
            rejectMsg.type = MessageTypeServerToClient::Reject;
            rejectMsg.rejectionReason = reason_for_rejection::TooManyPlayers;
            send(make_shared<string>(serialize_server(rejectMsg)));
            return;
        }

        this->name = msg.name;
        this->id = serverState->nextId++;
        this->money = 1000;
        this->spectator = false;
        this->seated = true;

        if (serverState->gameState != GameState::WaitingForPlayers || joinedPlayers >= 4)
        {
            this->spectator = true;
            this->seated = false;

            cout << "[" << msg.name << "] joined as spectator \n";
            MessageServerToClient welcomeMesg;
            welcomeMesg.type = MessageTypeServerToClient::Welcome;
            welcomeMesg.playerId = id;
            welcomeMesg.playerSum = joinedPlayers + 1;
            welcomeMesg.name = name;
            welcomeMesg.playerNames = serverState->buildNameSnapshot();
            welcomeMesg.playerMoney = serverState->buildMoneySnapshot();
            welcomeMesg.isSpectatorMap = serverState->buildSpectatorSnapshot();
            welcomeMesg.isSeatedMap = serverState->buildSeatedSnapshot();
            welcomeMesg.betThisRoundMap = serverState->buildBetThisRoundSnapshot();
            send(make_shared<string>(serialize_server(welcomeMesg)));

            response.type = MessageTypeServerToClient::PlayerJoined;
            response.playerId = id;
            response.name = name;
            response.isSpectator = spectator;
            response.isSeated = seated;

            if (serverState->gameState != GameState::WaitingForPlayers)
            {
                MessageServerToClient graphicsUpdateMsg;
                graphicsUpdateMsg.type = MessageTypeServerToClient::NewPlayerUpdateGraphics;
                graphicsUpdateMsg.toAct = serverState->toAct;
                graphicsUpdateMsg.toCall = serverState->toCall;
                graphicsUpdateMsg.currentBet = serverState->currentBet;
                graphicsUpdateMsg.minRaise = serverState->minRaise;
                graphicsUpdateMsg.playerHands = serverState->handstate.hole;
                graphicsUpdateMsg.communityCards = serverState->handstate.communityCards;
                graphicsUpdateMsg.dealerId = serverState->dealerId;
                graphicsUpdateMsg.smallBlindId = serverState->smallBlindId;
                graphicsUpdateMsg.bigBlindId = serverState->bigBlindId;
                send(make_shared<string>(serialize_server(graphicsUpdateMsg)));
            }

            break;
        }

        spectator = false;
        seated = true;

        cout << "[" << display_name() << "] joined\n";

        auto names = serverState->buildNameSnapshot();
        send(make_shared<string>(serialize_server(
            MessageServerToClient{
                .type = MessageTypeServerToClient::Welcome,
                .playerId = id,
                .playerSum = (int)names.size(),
                .name = name,
                .playerNames = names,
                .playerMoney = serverState->buildMoneySnapshot(),
                .isSpectatorMap = serverState->buildSpectatorSnapshot(),
                .isSeatedMap = serverState->buildSeatedSnapshot(),
                .betThisRoundMap = serverState->buildBetThisRoundSnapshot()})));

        response.type = MessageTypeServerToClient::PlayerJoined;
        response.playerId = id;
        response.name = name;
        response.isSpectator = spectator;
        response.isSeated = seated;
        break;
    }
    case MessageTypeClientToServer::Ready:
        cout << "[" << display_name() << "] is ready\n";
        ready = true;
        response.type = MessageTypeServerToClient::PlayerReady;
        response.playerId = id;
        break;
    case MessageTypeClientToServer::Chat:
        cout << "[" << display_name() << "] says: " << msg.chatText << "\n";
        response.type = MessageTypeServerToClient::ChatFrom;
        response.chatText = msg.chatText;
        response.playerId = id;
        break;
    case MessageTypeClientToServer::Action:
        hasPendingAction = true;
        PendingAction = line;
        if (serverState->gameState == GameState::WaitingForPlayers || serverState->gameState == GameState::Showdown || serverState->toAct != id)
        {
            cout << "[" << display_name() << "] invalid action because " << "gameState=" << int(serverState->gameState) << " toAct=" << serverState->toAct << " myId=" << id << "\n";
            validMessage = false;
            break;
        }

        cout << "[" << display_name() << "] action: " << int(msg.action) << " actionAmount: " << msg.actionAmount << "\n";
        response.playerId = id;
        response.action = msg.action;
        response.actionAmount = msg.actionAmount;

        if (on_action_ptr)
            on_action_ptr(id, msg.action, msg.actionAmount);

        validMessage = false; // Don't broadcast the action message to other clients, the server will broadcast the result after processing the action
        break;
    case MessageTypeClientToServer::RequestState:
        cout << "[" << display_name() << "] requested game state\n";
        response.type = MessageTypeServerToClient::GameState;
        response.gameState = serverState->gameState;
        response.potAmount = serverState->pot;
        send(make_shared<string>(serialize_server(response)));
        validMessage = false; // Don't broadcast the request state message to other clients
        break;
    case MessageTypeClientToServer::Leave:
        cout << "[" << display_name() << "] left\n";
        handleDisconnectOnce();
        validMessage = false;
        break;
    case MessageTypeClientToServer::AdminPlay:
        cout << "[" << display_name() << "] triggered admin play\n";
        if (play_game_ptr)
            play_game_ptr();
        validMessage = false; // Don't broadcast this message to other clients
        break;
    case MessageTypeClientToServer::RejectAck:
    {
        cout << "[" << display_name() << "] acknowledged rejection\n";
        auto self = shared_from_this();
        socket.close();
        serverState->clients.erase(self);
        validMessage = false; // Don't broadcast this message to other clients
        break;
    }
    case MessageTypeClientToServer::RequestUnorderedMapUpdates:
        cout << "[" << display_name() << "] requested unordered map updates\n";
        response.type = MessageTypeServerToClient::UnorderedMapUpdate;
        response.playerMoney = serverState->buildMoneySnapshot();
        response.playerNames = serverState->buildNameSnapshot();
        response.isSpectatorMap = serverState->buildSpectatorSnapshot();
        response.isSeatedMap = serverState->buildSeatedSnapshot();
        response.betThisRoundMap = serverState->buildBetThisRoundSnapshot();
        send(make_shared<string>(serialize_server(response)));
        validMessage = false; // Don't broadcast this message to other clients
        break;
    default:
        validMessage = false;
        break;
    }
    if (validMessage)
        broadcast(serialize_server(response));
}

std::unordered_map<int, int> ServerState::buildMoneySnapshot() const
{
    std::unordered_map<int, int> moneySnapshot;
    for (const auto &client : clients)
    {
        if (client->id >= 0 && client->connected)
            moneySnapshot[client->id] = client->money;
    }
    return moneySnapshot;
}
std::unordered_map<int, std::string> ServerState::buildNameSnapshot() const
{
    std::unordered_map<int, std::string> nameSnapshot;
    for (const auto &client : clients)
    {
        if (client->id >= 0 && client->connected)
            nameSnapshot[client->id] = client->getName();
    }
    return nameSnapshot;
}

std::unordered_map<int, bool> ServerState::buildSpectatorSnapshot() const
{
    std::unordered_map<int, bool> spectatorSnapshot;
    for (const auto &client : clients)
    {
        if (client->id >= 0 && client->connected)
            spectatorSnapshot[client->id] = client->spectator;
    }
    return spectatorSnapshot;
}

std::unordered_map<int, bool> ServerState::buildSeatedSnapshot() const
{
    std::unordered_map<int, bool> seatedSnapshot;
    for (const auto &client : clients)
    {
        if (client->id >= 0 && client->connected)
            seatedSnapshot[client->id] = client->seated;
    }
    return seatedSnapshot;
}

std::unordered_map<int, int> ServerState::buildBetThisRoundSnapshot() const
{
    std::unordered_map<int, int> betThisRoundSnapshot;
    for (const auto &client : clients)
    {
        if (client->id >= 0 && client->connected)
            betThisRoundSnapshot[client->id] = client->betThisRound;
    }
    return betThisRoundSnapshot;
}