#include "client_in_client.hpp"
using namespace std;

PokerClient::PokerClient() : socket(io), running(false) {}
PokerClient::~PokerClient()
{
    stop();
}
void PokerClient::connect_to(const string &host, const string &port)
{
    tcp::resolver resolver(io);
    auto endpoints = resolver.resolve(host, port);
    boost::asio::connect(socket, endpoints);
    cout << "Connected to server!\n";
}

void PokerClient::join_us(const string &name)
{
    MessageClientToServer join;
    join.type = MessageTypeClientToServer::Join;
    join.name = name;

    write_line(serialize_client(join));
}

void PokerClient::start()
{
    running = true;

    readerThread = thread([this]()
                          { readerLoop(); });
    input();
}

void PokerClient::input()
{
    string line;
    while (running)
    {
        getline(cin, line);
        terminal(line);
    }
}

void PokerClient::terminal(const string &input)
{
    if (input.empty())
        return;
    if (input[0] == '/' && input.size() > 1)
    {
        string command, arg;
        istringstream iss(input);
        iss >> command;
        getline(iss, arg);
        if (command == "/ready")
        {
            MessageClientToServer msg;
            msg.type = MessageTypeClientToServer::Ready;
            write_line(serialize_client(msg));
        }
        else if (command == "/request_state")
        {
            MessageClientToServer msg;
            msg.type = MessageTypeClientToServer::RequestState;
            write_line(serialize_client(msg));
        }
        else if (command == "/leave")
        {
            MessageClientToServer msg;
            msg.type = MessageTypeClientToServer::Leave;
            write_line(serialize_client(msg));
            stop();
        }
        else if (command == "/action")
        {
            istringstream actionStream(arg);
            string actionStr;
            int actionAmount = 0;
            actionStream >> actionStr >> actionAmount;

            PlayerActionType actionType = PlayerActionType::Failed;
            if (actionStr == "fold")
                actionType = PlayerActionType::Fold;
            else if (actionStr == "check")
                actionType = PlayerActionType::Check;
            else if (actionStr == "call")
                actionType = PlayerActionType::Call;
            else if (actionStr == "raise")
                actionType = PlayerActionType::Raise;

            MessageClientToServer msg;
            msg.type = MessageTypeClientToServer::Action;
            msg.action = actionType;
            msg.actionAmount = actionAmount;

            write_line(serialize_client(msg));
        }
        else if (command == "/admin_play")
        {
            MessageClientToServer msg;
            msg.type = MessageTypeClientToServer::AdminPlay;
            write_line(serialize_client(msg));
        }
        else
        {
            cout << "Unknown command.\n";
        }
    }
    else
    {
        sendChat(input);
    }
}

void PokerClient::sendChat(const string &chat)
{
    MessageClientToServer msg;
    msg.type = MessageTypeClientToServer::Chat;
    msg.chatText = chat;

    write_line(serialize_client(msg));
}

void PokerClient::stop()
{
    bool expected = true;
    if (!running.compare_exchange_strong(expected, false))
    {
        return;
    }
    boost::system::error_code ec;

    socket.close(ec);
    if (readerThread.joinable())
    {
        readerThread.join();
    }
}

void PokerClient::write_line(const string &s)
{
    boost::asio::write(socket, boost::asio::buffer(s));
}
void PokerClient::readerLoop()
{
    boost::asio::streambuf buf;

    while (running)
    {
        boost::system::error_code ec;

        boost::asio::read_until(socket, buf, '\n', ec);

        if (ec)
        {
            if (running)
                cout << "Disconnected.\n";
            break;
        }

        istream is(&buf);
        string line;
        getline(is, line);

        handle_line(line, state);
    }
}

string PokerClient::nameOf(int id)
{
    auto it = state.playerNames.find(id);
    if (it != state.playerNames.end() && !it->second.empty())
        return it->second;
    return "ID:  " + to_string(id);
}

void PokerClient::handle_line(const string &line, ClientState &state)
{
    MessageServerToClient msg = deserialize_server(line);
    switch (msg.type)
    {
    case MessageTypeServerToClient::Welcome:
        cout << "Welcome, " << msg.name << "! Your player ID is " << msg.playerId << ". There are " << msg.playerSum << " players in the game.\n";
        for (int i = 0; i < msg.playerSum; i++)
        {
            cout << "Player " << msg.playerNames[i] << " (ID: " << i << ") is in the game.";
            if (i == msg.playerId)
                cout << " (You)";
            state.playerNames[i] = msg.playerNames[i];
            cout << "\n";
        }
        state.myId = msg.playerId;
        break;
    case MessageTypeServerToClient::PlayerJoined:
        cout << "Player joined: " << msg.name << " (ID: " << msg.playerId << ")\n";
        state.playerNames[msg.playerId] = msg.name;
        break;
    case MessageTypeServerToClient::PlayerLeft:
        cout << "Player left: " << nameOf(msg.playerId) << "\n";
        state.playerNames.erase(msg.playerId);
        break;
    case MessageTypeServerToClient::PlayerReady:
        cout << "Player ready: " << nameOf(msg.playerId) << "\n";
        break;
    case MessageTypeServerToClient::ChatFrom:
        cout << nameOf(msg.playerId) << ": " << msg.chatText << "\n";
        break;
    case MessageTypeServerToClient::GameState:
        cout << "Game state changed: " << int(msg.gameState) << "\n";
        break;
    case MessageTypeServerToClient::ActionResult:
        cout << "Action result for player " << nameOf(msg.playerId) /*<< ": " << int(msg.actionResult)*/ << "\n";
        break;
    case MessageTypeServerToClient::CommunityCard:
        cout << "Community cards updated: " << msg.cards << "\n";
        break;
    case MessageTypeServerToClient::PlayerHand:
        cout << "Your hand: " << msg.cards << "\n";
        break;
    case MessageTypeServerToClient::PotUpdate:
        cout << "Pot updated: $" << msg.potAmount << "\n";
        break;
    case MessageTypeServerToClient::Showdown:
        cout << "Showdown! Pot: $" << msg.potAmount << ". Winners: ";
        for (int id : msg.idWinners)
        {
            cout << nameOf(id) << " (ID: " << id << ") \n";
        }
        break;
    case MessageTypeServerToClient::BettingUpdate:
        cout << "Betting update: To Act: " << nameOf(msg.toAct) << " (ID: " << msg.toAct << "), To Call: $" << msg.toCall << ", Current Bet: $" << msg.currentBet << ", Min Raise: $" << msg.minRaise << ", Pot: $" << msg.potAmount << "\n";
        break;
    default:
        cout << "Unknown message type received.\n";
        break;
    }
}