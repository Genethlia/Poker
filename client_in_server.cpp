#include "client_in_server.hpp"
using namespace std;

bool ServerState::all_ready() const
{
    for (const auto &client : clients)
    {
        if (!client->ready)
            return false;
    }
    return true;
}

void ServerState::reset_game()
{
    pot = 0;
    gameState = GameState::WaitingForPlayers;
    for (const auto &client : clients)
    {
        client->ready = false;
    }
}

void ServerState::broadcast_all(const string &msg)
{
    for (auto &client : clients)
    {
        client->send(make_shared<string>(msg));
    }
}

Client::Client(tcp::socket s, ServerState *state)
    : socket(move(s)), serverState(state)
{
    ready = false;
    inHand = true;
    allin = false;
    hasPendingAction = false;
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
                serverState->clients.erase(self);
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
                serverState->clients.erase(self);
                return;
            }

            outbox.pop_front();
            if (!outbox.empty())
            {
                do_write();
            }
        });
}

void Client::broadcast(const string &msg)
{
    auto buffer = make_shared<string>(msg);
    for (auto &c : serverState->clients)
    {
        c->send(buffer);
    }
}

void Client::send_to(const std::string &msg, int id)
{
    auto target = find_if(serverState->clients.begin(), serverState->clients.end(),
                          [id](const shared_ptr<Client> &c)
                          { return c->id == id; });
    if (target != serverState->clients.end())
        (*target)->send(make_shared<string>(msg));
}

void Client::handle_line(const string &line)
{
    MessageClientToServer msg = deserialize_client(line);
    MessageServerToClient response{};
    bool validMessage = true;
    switch (msg.type)
    {
    case MessageTypeClientToServer::Join:
        name = msg.name;
        id = serverState->nextId++;
        serverState->idToName[id] = name;
        cout << "[" << display_name() << "] joined\n";

        send(make_shared<string>(serialize_server(
            MessageServerToClient{
                .type = MessageTypeServerToClient::Welcome,
                .playerId = id,
                .playerSum = int(serverState->idToName.size()),
                .name = name,
                .playerNames = serverState->idToName})));

        response.type = MessageTypeServerToClient::PlayerJoined;
        response.playerId = id;
        response.name = name;
        break;
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
            cout << "[" << display_name() << "] attempted action is invalid\n";
            response.type = MessageTypeServerToClient::ActionResult;
            response.playerId = id;
            response.action = PlayerActionType::Failed;
            break;
        }

        cout << "[" << display_name() << "] action: " << int(msg.action) << " amount: " << msg.amount << "\n";
        response.type = MessageTypeServerToClient::ActionResult;
        response.playerId = id;
        response.action = msg.action;
        response.amount = msg.amount;
        break;
    case MessageTypeClientToServer::RequestState:
        cout << "[" << display_name() << "] requested game state\n";
        response.type = MessageTypeServerToClient::GameState;
        response.gameState = GameState::WaitingForPlayers; // TODO: actual game state
        break;
    case MessageTypeClientToServer::Leave:
        cout << "[" << display_name() << "] left\n";
        response.type = MessageTypeServerToClient::PlayerLeft;
        response.playerId = id;
        break;
    case MessageTypeClientToServer::AdminPlay:
        cout << "[" << display_name() << "] triggered admin play\n";
        if (play_game_ptr)
            play_game_ptr();
        validMessage = false; // Don't broadcast this message to other clients
        break;
    default:
        validMessage = false;
        break;
    }
    if (validMessage)
        broadcast(serialize_server(response));
}