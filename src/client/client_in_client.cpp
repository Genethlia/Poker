#include "client_in_client.hpp"
using namespace std;

PokerClient::ClientState PokerClient::getClientStateCopy()
{
    lock_guard<mutex> lock(stateMutex);
    return state;
}

pop PokerClient::createPopUpMessage(MessageServerToClient msg)
{
    switch (msg.type)
    {
    case MessageTypeServerToClient::PlayerJoined:
        return pop(nameOfUnsafe(msg.playerId) + " joined the game.", popUpMessageType::PlayerJoined);
    case MessageTypeServerToClient::PlayerLeft:
        return pop(msg.name + " left the game.", popUpMessageType::PlayerLeft);
    case MessageTypeServerToClient::PlayerReady:
        return pop(nameOfUnsafe(msg.playerId) + " is ready.", popUpMessageType::PlayerReady);
    case MessageTypeServerToClient::ChatFrom:
        return pop("You have a new chat message from " + nameOfUnsafe(msg.playerId), popUpMessageType::ChatMessage);
    case MessageTypeServerToClient::ActionResult:
    {
        string actionStr;
        switch (msg.action)
        {
        case PlayerActionType::Fold:
            actionStr = "folded";
            break;
        case PlayerActionType::Check:
            actionStr = "checked";
            break;
        case PlayerActionType::Call:
            actionStr = "called";
            break;
        case PlayerActionType::Raise:
            actionStr = "raised to " + to_string(msg.actionAmount);
            break;
        default:
            actionStr = "performed an illegal action";
            break;
        }
        return pop(nameOfUnsafe(msg.playerId) + " " + actionStr + ".", popUpMessageType::ActionResult);
        break;
    }
    case MessageTypeServerToClient::BettingUpdate:
    {
        if (state.myId == msg.toAct)
        {
            string t = (msg.toCall == 0) ? "" : "You need to call " + to_string(msg.toCall) + " to stay in the hand.";
            return pop("It's your turn to act! " + t, popUpMessageType::BettingUpdate);
        }
        return pop("", popUpMessageType::BettingUpdate);
        break;
    }
    case MessageTypeServerToClient::Showdown:
    {
        if (msg.idWinners.size() == 1 && msg.idWinners[0] == state.myId)
            return pop("Congratulations! You won the game with a " + winPowerTranslation(msg.winPower) + "!", popUpMessageType::GameWon);
        else if (find(msg.idWinners.begin(), msg.idWinners.end(), state.myId) != msg.idWinners.end())
        {
            string winnerNames;
            for (int id : msg.idWinners)
            {
                if (id != state.myId)
                    winnerNames += nameOfUnsafe(id) + ", ";
            }
            if (!winnerNames.empty())
            {
                winnerNames.pop_back(); // Remove trailing space
                winnerNames.pop_back(); // Remove trailing comma
            }
            return pop("You tied for the win with a " + winPowerTranslation(msg.winPower) + " with players: " + winnerNames, popUpMessageType::GameWon);
        }
        else
        {
            string winnerNames;
            for (int id : msg.idWinners)
            {
                winnerNames += nameOfUnsafe(id) + ", ";
            }
            if (!winnerNames.empty())
            {
                winnerNames.pop_back(); // Remove trailing space
                winnerNames.pop_back(); // Remove trailing comma
            }
            if (msg.idWinners.size() == 1)
                return pop("Player " + winnerNames + " won the game with a " + winPowerTranslation(msg.winPower) + ". Better luck next time!", popUpMessageType::GameWon);
            else
                return pop("Players " + winnerNames + " tied for the win with a " + winPowerTranslation(msg.winPower) + ". Better luck next time!", popUpMessageType::GameWon);
        }
        break;
    }
    default:
        return pop("", popUpMessageType::Error);
    }
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
    boost::system::error_code ec;
    if (socket.is_open())
        socket.close(ec);

    socket = tcp::socket(io);
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

void PokerClient::requestUnorderedMapUpdates()
{
    MessageClientToServer msg;
    msg.type = MessageTypeClientToServer::RequestUnorderedMapUpdates;

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
        popUpMessages.push_back(pop("It's not your turn to act!", popUpMessageType::Error));
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

void PokerClient::resetLocalState()
{
    {
        lock_guard<mutex> lock(stateMutex);
        state = ClientState();
    }
    {
        lock_guard<mutex> lock(popUpMessagesMutex);
        popUpMessages.clear();
    }
    {
        lock_guard<mutex> lock(chatMessagesMutex);
        chatMessages.clear();
    }
    gameRunning = false;
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
    try
    {
        boost::asio::write(socket, boost::asio::buffer(s));
    }
    catch (const std::exception &e)
    {
        cerr << "Error sending message to server: " << e.what() << '\n';
        stop();
    }
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

std::deque<pop> PokerClient::getAndClearPopUpMessages()
{
    lock_guard<std::mutex> lock(popUpMessagesMutex);
    std::deque<pop> messages = std::move(popUpMessages);
    popUpMessages.clear();
    return messages;
}

std::deque<PokerClient::chatMessage> PokerClient::getAndClearChatMessages()
{
    lock_guard<std::mutex> lock(chatMessagesMutex);
    std::deque<PokerClient::chatMessage> messages = std::move(chatMessages);
    chatMessages.clear();
    return messages;
}

void PokerClient::handle_line(const string &line)
{

    bool createPopUp = true;
    bool shouldRequestState = false;
    bool shouldRequestMapUpdates = false;
    bool shouldSendRejectAck = false;

    MessageServerToClient msg;

    try
    {
        msg = deserialize_server(line);
    }
    catch (const std::exception &e)
    {
        cerr << "Error deserializing message from server: " << e.what() << '\n';
        cerr << "Message content: " << line << "\n";
        return;
    }

    {
        lock_guard<std::mutex> lock(stateMutex);
        switch (msg.type)
        {
        case MessageTypeServerToClient::Welcome:
        {
            state.playerNames.clear();
            state.playerMoney.clear();
            state.isSpectator.clear();
            state.isSeated.clear();
            for (auto &[id, name] : msg.playerNames)
            {
                if (msg.playerMoney.find(id) != msg.playerMoney.end())
                    state.playerMoney[id] = msg.playerMoney[id];
                if (msg.playerNames.find(id) != msg.playerNames.end())
                    state.playerNames[id] = msg.playerNames[id];
                if (msg.isSpectatorMap.find(id) != msg.isSpectatorMap.end())
                    state.isSpectator[id] = msg.isSpectatorMap[id];
                if (msg.isSeatedMap.find(id) != msg.isSeatedMap.end())
                    state.isSeated[id] = msg.isSeatedMap[id];
                if (msg.betThisRoundMap.find(id) != msg.betThisRoundMap.end())
                    state.betThisRound[id] = msg.betThisRoundMap[id];
            }
            state.myId = msg.playerId;
            state.roomCode = msg.roomCode;
            rebuildPlayerPositions();
            createPopUp = false;
            break;
        }
        case MessageTypeServerToClient::PlayerJoined:
        {
            state.playerNames[msg.playerId] = msg.name;
            state.playerMoney[msg.playerId] = 1000; // Initialize player money for the new player
            state.isSpectator[msg.playerId] = msg.isSpectator;
            state.isSeated[msg.playerId] = msg.isSeated;
            rebuildPlayerPositions();
            break;
        }
        case MessageTypeServerToClient::PlayerLeft:
        {
            state.playerNames.erase(msg.playerId);
            state.playerMoney.erase(msg.playerId); // Remove player money for the player who left
            state.isSpectator.erase(msg.playerId); // Remove spectator status for the player who left
            state.isSeated.erase(msg.playerId);    // Remove seated status for the player who left
            removeOpponentCardsAndIdToShowCards(msg.playerId);
            rebuildPlayerPositions();
            break;
        }
        case MessageTypeServerToClient::PlayerReady:
        {
            break;
        }
        case MessageTypeServerToClient::ChatFrom:
        {
            break;
        }
        case MessageTypeServerToClient::GameState:
        {
            state.gameState = msg.gameState;
            state.potAmount = msg.potAmount; // Update pot amount in the client state
            if (msg.gameState == GameState::PreFlop && !gameRunning)
            {
                newGame();
            }
            createPopUp = false;
            break;
        }
        case MessageTypeServerToClient::ActionResult:
        {
            break;
        }
        case MessageTypeServerToClient::CommunityCard:
        {
            state.communityCards.push_back(extractCardValueSuit(msg));
            createPopUp = false;
            break;
        }
        case MessageTypeServerToClient::PlayerHand:
        {
            rebuildPlayerPositions();
            auto temp = extractCardValueSuit(msg);
            cout << "Received hand card for player " << msg.playerId << ": Value = " << temp.value << ", Suit = " << temp.suit << "\n";
            if (msg.playerId == state.myId)
            {
                state.myCards.push_back(temp);
            }
            else
            {
                state.opponentCards.push_back({msg.playerId, temp});
            }
            createPopUp = false;
            break;
        }
        case MessageTypeServerToClient::PotUpdate:
        {
            state.potAmount = msg.potAmount; // Update the client state with the new pot amount
            createPopUp = false;
            break;
        }
        case MessageTypeServerToClient::Showdown:
        {
            gameRunning = false;
            shouldRequestMapUpdates = true;
            break;
        }
        case MessageTypeServerToClient::BettingUpdate:
        {
            state.toAct = msg.toAct; // Update the client state with the new player to act
            if (state.myId != msg.toAct)
                createPopUp = false;
            state.toCall = msg.toCall;             // Update the client state with the new amount to call
            state.currentBet = msg.currentBet;     // Update the client state with the new current bet
            state.minRaise = msg.minRaise;         // Update the client state with the new minimum raise
            state.potAmount = msg.potAmount;       // Update the client state with the new pot amount
            state.dealerId = msg.dealerId;         // Update the client state with the new dealer ID
            state.smallBlindId = msg.smallBlindId; // Update the client state with the new small blind ID
            state.bigBlindId = msg.bigBlindId;     // Update the client state with the new big blind ID
            shouldRequestMapUpdates = true;
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
            shouldSendRejectAck = true;
            createPopUp = false;
            break;
        }
        case MessageTypeServerToClient::NewPlayerUpdateGraphics:
        {
            state.opponentCards.clear();
            state.communityCards.clear();
            state.idToShowCardsOf.clear();
            state.toAct = msg.toAct;               // Update the client state with the new player to act
            state.toCall = msg.toCall;             // Update the client state with the new amount to call
            state.currentBet = msg.currentBet;     // Update the client state with the new current bet
            state.minRaise = msg.minRaise;         // Update the client state with the new minimum raise
            state.dealerId = msg.dealerId;         // Update the client state with the new dealer ID
            state.smallBlindId = msg.smallBlindId; // Update the client state with the new small blind ID
            state.bigBlindId = msg.bigBlindId;     // Update the client state with the new big blind ID
            for (auto &[id, hand] : msg.playerHands)
            {
                cout << "Graphics hand for id " << id
                     << " hasName=" << state.playerNames.count(id)
                     << " seated=" << (state.isSeated.count(id) ? state.isSeated[id] : false)
                     << " spectator=" << (state.isSpectator.count(id) ? state.isSpectator[id] : false)
                     << " hasPosition=" << state.PlayerPosition.count(id)
                     << "\n";

                state.opponentCards.push_back({id, hand.first});
                state.opponentCards.push_back({id, hand.second});
            }
            for (auto &card : msg.communityCards)
            {
                state.communityCards.push_back(card);
            }
            gameRunning = true;
            createPopUp = false;
            shouldRequestState = true;
            shouldRequestMapUpdates = true;
            break;
        }
        case MessageTypeServerToClient::UnorderedMapUpdate:
        {
            createPopUp = false;
            state.playerMoney = msg.playerMoney;
            state.playerNames = msg.playerNames;
            state.isSpectator = msg.isSpectatorMap;
            state.isSeated = msg.isSeatedMap;
            state.betThisRound = msg.betThisRoundMap;
            rebuildPlayerPositions();
            break;
        }
        case MessageTypeServerToClient::ShowCardsOf:
        {
            createPopUp = false;
            state.idToShowCardsOf.push_back(msg.playerId);
            break;
        }
        case MessageTypeServerToClient::SpectatingUpdate:
        {
            createPopUp = false;
            state.isSpectator[msg.playerId] = msg.isSpectator;
            state.isSeated[msg.playerId] = msg.isSeated;
            rebuildPlayerPositions();
            break;
        }
        default:
            createPopUp = false;
            cout << "Unknown message type received.\n";
            cout << "Message content: " << line << "\n";
            break;
        }
    }
    if (msg.type == MessageTypeServerToClient::ChatFrom)
    {
        lock_guard<std::mutex> lock(chatMessagesMutex);
        chatMessages.push_back(PokerClient::chatMessage(msg.playerId, msg.chatText));
    }

    if (createPopUp)
    {
        pop temp;
        {
            lock_guard<std::mutex> stateLock(stateMutex);
            temp = createPopUpMessage(msg);
        }
        {
            lock_guard<std::mutex> popLock(popUpMessagesMutex);
            popUpMessages.push_back(temp);
        }
    }
    if (shouldRequestState)
    {
        requestState();
    }
    if (shouldRequestMapUpdates)
    {
        requestUnorderedMapUpdates();
    }
    if (shouldSendRejectAck)
    {
        MessageClientToServer ack;
        ack.type = MessageTypeClientToServer::RejectAck;
        write_line(serialize_client(ack));
    }
}

void PokerClient::newGame()
{
    gameRunning = true;

    state.communityCards.clear();
    state.myCards.clear();
    state.opponentCards.clear();
    state.idToShowCardsOf.clear();
    state.toAct = -1;
    state.toCall = 0;
    state.currentBet = 0;
    state.minRaise = 50;
    state.potAmount = 0;
}

valRank PokerClient::extractCardValueSuit(const MessageServerToClient &msg)
{
    int value = stoi(msg.cards.substr(0, msg.cards.find(',')));
    int suit = stoi(msg.cards.substr(msg.cards.find(',') + 1));
    return {value, suit};
}

void PokerClient::rebuildPlayerPositions()
{
    state.PlayerPosition.clear();
    std::vector<int> ids;
    for (auto &[id, name] : state.playerNames)
    {
        bool spectator = state.isSpectator.count(id) && state.isSpectator[id];
        bool seated = state.isSeated.count(id) && state.isSeated[id];
        if (spectator && !seated)
            continue;

        ids.push_back(id);
    }
    std::sort(ids.begin(), ids.end());

    if (ids.empty())
        return;

    int selfIndex = -1;

    for (size_t i = 0; i < ids.size(); i++)
    {
        if (ids[i] == state.myId)
        {
            selfIndex = i;
            break;
        }
    }

    vector<int> visualOrder{};

    if (selfIndex != -1)
    {
        for (size_t i = 0; i < ids.size(); i++)
        {
            int index = (selfIndex + i) % ids.size();
            visualOrder.push_back(ids[index]);
        }

        int otherPlayerCount = 0;

        for (int id : visualOrder)
        {
            if (id == state.myId)
            {
                state.PlayerPosition[id] = playerCardPositionsAndAngles.back(); // Position for self
                // state.PlayerPosition[id] = {Seat::Left, 90}; // Debug: Temporarily set self to Left seat for chat testing.
            }
            else
            {
                if (otherPlayerCount >= (int)playerCardPositionsAndAngles.size() - 1)
                    break;
                state.PlayerPosition[id] = playerCardPositionsAndAngles[otherPlayerCount++]; // Positions for opponents
            }
        }
    }
    else
    {
        for (size_t i = 0; i < ids.size() && i < playerCardPositionsAndAngles.size(); i++)
        {
            state.PlayerPosition[ids[i]] = playerCardPositionsAndAngles[i];
        }
    }
}

void PokerClient::removeOpponentCardsAndIdToShowCards(const int &id)
{
    state.opponentCards.erase(std::remove_if(state.opponentCards.begin(), state.opponentCards.end(), [&](const auto &cardPair)
                                             { return cardPair.first == id; }),
                              state.opponentCards.end());
    state.idToShowCardsOf.erase(std::remove(state.idToShowCardsOf.begin(), state.idToShowCardsOf.end(), id), state.idToShowCardsOf.end());
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
