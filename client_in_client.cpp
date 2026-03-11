#include "client_in_client.hpp"
using namespace std;

PokerClient::ClientState PokerClient::getClientStateCopy()
{
    lock_guard<mutex> lock(stateMutex);
    return state;
}

PokerClient::PokerClient() : socket(io), running(false)
{
}
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
}

void PokerClient::sendReady()
{
    MessageClientToServer msg;
    msg.type = MessageTypeClientToServer::Ready;

    write_line(serialize_client(msg));
}

void PokerClient::requestState()
{
    MessageClientToServer msg;
    msg.type = MessageTypeClientToServer::RequestState;

    write_line(serialize_client(msg));
}

void PokerClient::leaveGame()
{
    MessageClientToServer msg;
    msg.type = MessageTypeClientToServer::Leave;

    write_line(serialize_client(msg));
    stop();
}

void PokerClient::sendAction(PlayerActionType action, int amount)
{
    ClientState snapshot = getClientStateCopy();
    if (snapshot.toAct != snapshot.myId)
    {
        cout << "It's not your turn to act!\n";
        return;
    }

    MessageClientToServer msg;
    msg.type = MessageTypeClientToServer::Action;
    msg.action = action;
    msg.actionAmount = amount;

    write_line(serialize_client(msg));
}

void PokerClient::startGame()
{
    MessageClientToServer msg;
    msg.type = MessageTypeClientToServer::AdminPlay;
    write_line(serialize_client(msg));
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

void PokerClient::UpdateMoney(const MessageServerToClient &msg)
{
    switch (msg.action)
    {
    case PlayerActionType::Call:
        state.playerMoney[msg.playerId] -= state.toCall;
        break;
    case PlayerActionType::Raise:
        state.playerMoney[msg.playerId] -= msg.actionAmount;
        break;
    default:
        break;
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

        handle_line(line);
    }
}

string PokerClient::nameOf(int id)
{
    lock_guard<std::mutex> lock(stateMutex);
    return nameOfUnsafe(id);
}

std::string PokerClient::nameOfUnsafe(int id)
{
    auto it = state.playerNames.find(id);
    if (it != state.playerNames.end() && !it->second.empty())
        return it->second;
    return "ID:  " + to_string(id);
}

void PokerClient::handle_line(const string &line)
{
    lock_guard<std::mutex> lock(stateMutex);
    MessageServerToClient msg = deserialize_server(line);
    switch (msg.type)
    {
    case MessageTypeServerToClient::Welcome:
        cout << "Welcome, " << msg.name << "! Your player ID is " << msg.playerId << ". There are " << msg.playerSum << " players in the game.\n";
        for (int i = 0; i < msg.playerSum; i++)
        {
            cout << "Player " << msg.playerNames[i] << " (ID: " << i << ") is in the game.";
            state.playerMoney[i] = 1000; // Initialize player money, can be updated later with actual values from the server
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
        state.playerMoney[msg.playerId] = 1000; // Initialize player money for the new player
        break;
    case MessageTypeServerToClient::PlayerLeft:
        cout << "Player left: " << nameOfUnsafe(msg.playerId) << "\n";
        state.playerNames.erase(msg.playerId);
        state.playerMoney.erase(msg.playerId); // Remove player money for the player who left
        break;
    case MessageTypeServerToClient::PlayerReady:
        cout << "Player ready: " << nameOfUnsafe(msg.playerId) << "\n";
        break;
    case MessageTypeServerToClient::ChatFrom:
        cout << nameOfUnsafe(msg.playerId) << ": " << msg.chatText << "\n";
        break;
    case MessageTypeServerToClient::GameState:
        cout << "Game state changed: " << int(msg.gameState) << "\n";
        state.gameState = msg.gameState;
        break;
    case MessageTypeServerToClient::ActionResult:
        cout << "Action result for player " << nameOfUnsafe(msg.playerId) << ": " << int(msg.action) << "\n";
        UpdateMoney(msg); // Update player money based on the action result
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
            cout << nameOfUnsafe(id) << " (ID: " << id << ") \n";
            state.playerMoney[id] += msg.potAmount / msg.idWinners.size(); // Distribute pot among winners
        }
        state.playerMoney[state.toAct] += msg.potAmount % msg.idWinners.size();
        break;
    case MessageTypeServerToClient::BettingUpdate:
        cout << "Betting update: To Act: " << nameOfUnsafe(msg.toAct) << " (ID: " << msg.toAct << "), To Call: $" << msg.toCall << ", Current Bet: $" << msg.currentBet << ", Min Raise: $" << msg.minRaise << ", Pot: $" << msg.potAmount << "\n";
        state.toAct = msg.toAct;           // Update the client state with the new player to act
        state.toCall = msg.toCall;         // Update the client state with the new amount to call
        state.currentBet = msg.currentBet; // Update the client state with the new current bet
        state.minRaise = msg.minRaise;     // Update the client state with the new minimum raise
        state.potAmount = msg.potAmount;   // Update the client state with the new pot amount
        break;
    default:
        cout << "Unknown message type received.\n";
        break;
    }
}