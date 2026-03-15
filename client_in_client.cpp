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
void PokerClient::Init(Images suitTextures[4], Images *gameImages, Font *cardFont)
{
    for (int i = 0; i < 4; i++)
        this->suitTextures[i] = &suitTextures[i];
    this->gameImages = gameImages;
    this->cardFont = cardFont;
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
        rebuildPlayerPositions();
        state.playerNames.clear();
        state.playerMoney.clear();
        cout << "Welcome, " << msg.name << "! Your player ID is " << msg.playerId << ". There are " << msg.playerSum << " players in the game.\n";
        for (auto &[id, name] : msg.playerNames)
        {
            cout << "Player " << name << " (ID: " << id << ") is in the game.";
            if (msg.playerMoney.find(id) != msg.playerMoney.end())
                state.playerMoney[id] = msg.playerMoney[id]; // Initialize player money, can be updated later with actual values from the server
            if (id == msg.playerId)
                cout
                    << " (You)";
            if (msg.playerNames.find(id) != msg.playerNames.end())
                state.playerNames[id] = msg.playerNames[id];
            cout << "\n";
        }
        rebuildPlayerPositions();
        state.myId = msg.playerId;
        break;
    case MessageTypeServerToClient::PlayerJoined:
        cout << "Player joined: " << msg.name << " (ID: " << msg.playerId << ")\n";
        state.playerNames[msg.playerId] = msg.name;
        state.playerMoney[msg.playerId] = 1000; // Initialize player money for the new player
        rebuildPlayerPositions();
        break;
    case MessageTypeServerToClient::PlayerLeft:
        cout << "Player left: " << nameOfUnsafe(msg.playerId) << "\n";
        state.playerNames.erase(msg.playerId);
        state.playerMoney.erase(msg.playerId); // Remove player money for the player who left
        rebuildPlayerPositions();
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
        state.communityCards.push_back(find_valRank(msg));
        break;
    case MessageTypeServerToClient::PlayerHand:
    {
        auto temp = find_valRank(msg);
        if (msg.playerId == state.myId)
        {
            cout << "Your hand: " << msg.cards << "\n";
            state.myCards.push_back(temp);
        }
        else
        {
            cout << nameOfUnsafe(msg.playerId) << "'s hand updated.\n";

            state.opponentCards.push_back({msg.playerId, temp});
        }
        break;
    }
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

valRank PokerClient::find_valRank(const MessageServerToClient &msg)
{
    cout<<msg.cards<<"\n";
    int value = stoi(msg.cards.substr(0, msg.cards.find(',')));
    int suit = stoi(msg.cards.substr(msg.cards.find(',') + 1));
    cout << "Parsed card: Value = " << value << ", Suit = " << suit << "\n";
    return {value, suit};
}

void PokerClient::rebuildPlayerPositions()
{
    state.PlayerPosition.clear();
    std::vector<int> ids;
    for (auto &[id, name] : state.playerNames)
    {
        ids.push_back(id);
    }
    std::sort(ids.begin(), ids.end());
    for (int i = 0; i < ids.size() && i < playerCardPositions.size(); i++)
    {
        state.PlayerPosition[ids[i]] = playerCardPositions[i];
    }
}
