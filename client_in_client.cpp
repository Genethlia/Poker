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
    cout << "Waiting server response...\n";
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
        popUpMessages.push_back("It's not your turn to act!");
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

std::deque<string> PokerClient::getAndClearPopUpMessages()
{
    lock_guard<std::mutex> lock(popUpMessagesMutex);
    std::deque<string> messages = std::move(popUpMessages);
    popUpMessages.clear();
    return messages;
}

void PokerClient::handle_line(const string &line)
{
    lock_guard<std::mutex> lock(stateMutex);
    lock_guard<std::mutex> lock2(popUpMessagesMutex);
    MessageServerToClient msg = deserialize_server(line);
    switch (msg.type)
    {
    case MessageTypeServerToClient::Welcome:
    {
        rebuildPlayerPositions();
        state.playerNames.clear();
        state.playerMoney.clear();
        state.isSpectator.clear();
        state.isSeated.clear();
        auto temp = "Welcome, " + msg.name + "! Your player ID is " + to_string(msg.playerId) + ". There are " + to_string(msg.playerSum) + " players in the game.";
        popUpMessages.push_back(temp);
        for (auto &[id, name] : msg.playerNames)
        {
            if (msg.playerMoney.find(id) != msg.playerMoney.end())
                state.playerMoney[id] = msg.playerMoney[id]; // Initialize player money, can be updated later with actual values from the server
            if (msg.playerNames.find(id) != msg.playerNames.end())
                state.playerNames[id] = msg.playerNames[id];
            if (msg.isSpectatorMap.find(id) != msg.isSpectatorMap.end())
                state.isSpectator[id] = msg.isSpectatorMap[id];
            if (msg.isSeatedMap.find(id) != msg.isSeatedMap.end())
                state.isSeated[id] = msg.isSeatedMap[id];
            auto temp = "Player " + name + " (ID: " + to_string(id) + ") is in the game." + (id == msg.playerId ? " (You)" : "");
            popUpMessages.push_back(temp);
        }
        rebuildPlayerPositions();
        state.myId = msg.playerId;
        break;
    }
    case MessageTypeServerToClient::PlayerJoined:
    {
        auto temp = "Player joined: " + msg.name + " (ID: " + to_string(msg.playerId) + ")";
        popUpMessages.push_back(temp);
        state.playerNames[msg.playerId] = msg.name;
        state.playerMoney[msg.playerId] = 1000; // Initialize player money for the new player
        state.isSpectator[msg.playerId] = msg.isSpectator;
        state.isSeated[msg.playerId] = msg.isSeated;
        rebuildPlayerPositions();
        break;
    }
    case MessageTypeServerToClient::PlayerLeft:
    {
        auto temp = "Player left: " + nameOfUnsafe(msg.playerId);
        popUpMessages.push_back(temp);
        state.playerNames.erase(msg.playerId);
        state.playerMoney.erase(msg.playerId); // Remove player money for the player who left
        state.isSpectator.erase(msg.playerId); // Remove spectator status for the player who left
        state.isSeated.erase(msg.playerId);    // Remove seated status for the player who left
        rebuildPlayerPositions();
        break;
    }
    case MessageTypeServerToClient::PlayerReady:
    {
        auto temp = "Player ready: " + nameOfUnsafe(msg.playerId);
        popUpMessages.push_back(temp);
        break;
    }
    case MessageTypeServerToClient::ChatFrom:
    {
        auto temp = nameOfUnsafe(msg.playerId) + ": " + msg.chatText;
        popUpMessages.push_back(temp);
        break;
    }
    case MessageTypeServerToClient::GameState:
    {
        auto temp = "Game state changed: " + to_string(int(msg.gameState));
        popUpMessages.push_back(temp);
        state.gameState = msg.gameState;
        state.potAmount = msg.potAmount; // Update pot amount in the client state
        if (msg.gameState == GameState::PreFlop && !gameRunning)
        {
            newGame();
        }
        break;
    }
    case MessageTypeServerToClient::ActionResult:
    {
        auto temp = "Action result for player " + nameOfUnsafe(msg.playerId) + ": " + to_string(int(msg.action));
        popUpMessages.push_back(temp);
        UpdateMoney(msg); // Update player money based on the action result
        break;
    }
    case MessageTypeServerToClient::CommunityCard:
    {
        auto temp = "Community cards updated: " + msg.cards;
        popUpMessages.push_back(temp);
        state.communityCards.push_back(extractCardValueSuit(msg));
        break;
    }
    case MessageTypeServerToClient::PlayerHand:
    {
        auto temp = extractCardValueSuit(msg);
        if (msg.playerId == state.myId)
        {
            auto tempMessage = "Your hand updated: " + msg.cards;
            popUpMessages.push_back(tempMessage);
            state.myCards.push_back(temp);
        }
        else
        {
            auto tempMessage = nameOfUnsafe(msg.playerId) + "'s hand updated.";
            popUpMessages.push_back(tempMessage);

            state.opponentCards.push_back({msg.playerId, temp});
        }
        break;
    }
    case MessageTypeServerToClient::PotUpdate:
    {
        auto tempMessage = "Pot updated: $" + to_string(msg.potAmount);
        popUpMessages.push_back(tempMessage);
        state.potAmount = msg.potAmount; // Update the client state with the new pot amount
        break;
    }
    case MessageTypeServerToClient::Showdown:
    {
        auto tempMessage = "Showdown! Pot: $" + to_string(msg.potAmount) + ". Winners: ";
        popUpMessages.push_back(tempMessage);
        for (int id : msg.idWinners)
        {
            auto temp1 = nameOfUnsafe(id) + " (ID: " + to_string(id) + ") ";
            popUpMessages.push_back(temp1);
            state.playerMoney[id] += msg.potAmount / msg.idWinners.size(); // Distribute pot among winners
        }
        auto temp2 = "Won with hand power: " + winPowerTranslation(msg.winPower);
        popUpMessages.push_back(temp2);
        state.playerMoney[msg.idWinners[0]] += msg.potAmount % msg.idWinners.size();
        gameRunning = false;
        break;
    }
    case MessageTypeServerToClient::BettingUpdate:
    {
        auto tempMessage = "Betting update: To Act: " + nameOfUnsafe(msg.toAct) + " (ID: " + to_string(msg.toAct) + "), To Call: $" + to_string(msg.toCall) + ", Current Bet: $" + to_string(msg.currentBet) + ", Min Raise: $" + to_string(msg.minRaise) + ", Pot: $" + to_string(msg.potAmount);
        popUpMessages.push_back(tempMessage);
        state.toAct = msg.toAct;           // Update the client state with the new player to act
        state.toCall = msg.toCall;         // Update the client state with the new amount to call
        state.currentBet = msg.currentBet; // Update the client state with the new current bet
        state.minRaise = msg.minRaise;     // Update the client state with the new minimum raise
        state.potAmount = msg.potAmount;   // Update the client state with the new pot amount
        break;
    }
    case MessageTypeServerToClient::Reject:
    {
        if (msg.rejectionReason == reason_for_rejection::TooManyPlayers)
        {
            running = false;
            cout << "Connection rejected: Too many players in the game.\n";
        }
        else if (msg.rejectionReason == reason_for_rejection::GameInProgress)
        {
            running = false;
            cout << "Connection rejected: Game already in progress.\n";
        }
        MessageClientToServer ack;
        ack.type = MessageTypeClientToServer::RejectAck;
        write_line(serialize_client(ack));
        break;
    }
    case MessageTypeServerToClient::NewPlayerUpdateGraphics:
    {

        auto tempMessage = "Graphics update for new player: To Act: " + nameOfUnsafe(msg.toAct) + " (ID: " + to_string(msg.toAct) + "), To Call: $" + to_string(msg.toCall) + ", Current Bet: $" + to_string(msg.currentBet) + ", Min Raise: $" + to_string(msg.minRaise);
        popUpMessages.push_back(tempMessage);
        state.toAct = msg.toAct;           // Update the client state with the new player to act
        state.toCall = msg.toCall;         // Update the client state with the new amount to call
        state.currentBet = msg.currentBet; // Update the client state with the new current bet
        state.minRaise = msg.minRaise;     // Update the client state with the new minimum raise
        for (auto &[id, hand] : msg.playerHands)
        {
            auto tempMessage = "Player " + nameOfUnsafe(id) + "'s hand updated.";
            popUpMessages.push_back(tempMessage);
            state.opponentCards.push_back({id, hand.first});
            state.opponentCards.push_back({id, hand.second});
        }
        for (auto &card : msg.communityCards)
        {
            auto tempMessage = "Community card updated: Value = " + to_string(card.value) + ", Suit = " + to_string(card.suit);
            popUpMessages.push_back(tempMessage);
            state.communityCards.push_back(card);
        }
        gameRunning = true;
        requestState();
        break;
    }
    default:
        cout << "Unknown message type received.\n";
        cout << "Message content: " << line << "\n";
        break;
    }
}

void PokerClient::newGame()
{
    gameRunning = true;

    state.communityCards.clear();
    state.myCards.clear();
    state.opponentCards.clear();
    state.toAct = -1;
    state.toCall = 0;
    state.currentBet = 0;
    state.minRaise = 50;
    state.potAmount = 0;
}

valRank PokerClient::extractCardValueSuit(const MessageServerToClient &msg)
{
    cout << msg.cards << "\n";
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
    int otherPlayerCount = 0;
    for (int i = 0; i < ids.size() && i < playerCardPositionsAndAngles.size(); i++)
    {
        if (ids[i] == state.myId)
        {
            state.PlayerPosition[ids[i]] = playerCardPositionsAndAngles.back(); // Position for self
        }
        else
        {
            state.PlayerPosition[ids[i]] = playerCardPositionsAndAngles[otherPlayerCount++]; // Positions for opponents
        }
    }
}

string PokerClient::winPowerTranslation(int winPower)
{
    string result = "";
    switch (winPower)
    {
    case 1:
        result = "High Card";
        break;
    case 2:
        result = "One Pair";
        break;
    case 3:
        result = "Two Pair";
        break;
    case 4:
        result = "Three of a Kind";
        break;
    case 5:
        result = "Straight";
        break;
    case 6:
        result = "Flush";
        break;
    case 7:
        result = "Full House";
        break;
    case 8:
        result = "Four of a Kind";
        break;
    case 9:
        result = "Straight Flush";
        break;
    case 10:
        result = "Royal Flush";
        break;
    default:
        result = "Unknown Hand Power";
        break;
    }
    return result;
}
