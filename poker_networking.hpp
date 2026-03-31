#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NOGDI
#define NOUSER
#include <boost/asio.hpp>
#include <map>
#include <unordered_map>
#include <iostream>
#include <memory>
#include <vector>
#include <set>
#include <unordered_set>
#include <functional>
#include <deque>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <sstream>
using boost::asio::ip::tcp;

struct valRank
{
    int value;
    int suit;
};
using hand = std::pair<valRank, valRank>;

enum class MessageTypeClientToServer
{
    Join,
    Ready,
    Chat,
    Action,
    RequestState,
    Leave,
    AdminPlay,
    RejectAck
};

enum class MessageTypeServerToClient
{
    Welcome,
    Reject,
    PlayerJoined,
    PlayerLeft,
    PlayerReady,
    ChatFrom,
    GameState,
    ActionResult,
    BettingUpdate,
    CommunityCard,
    PlayerHand,
    PotUpdate,
    Showdown,
    SpectatingUpdate,
    NewPlayerUpdateGraphics
};

enum class PlayerActionType
{
    Fold,
    Check,
    Call,
    Raise,
    Failed
};

enum class GameState
{
    WaitingForPlayers,
    PreFlop,
    Flop,
    Turn,
    River,
    Showdown,
    GameOver
};

enum class reason_for_rejection
{
    GameInProgress,
    TooManyPlayers
};

struct MessageClientToServer
{
    MessageTypeClientToServer type;

    std::string name;     // For JOIN
    std::string chatText; // For CHAT

    PlayerActionType action; // For ACTION
    int actionAmount = 0;    // For BET/RAISE
};

struct MessageServerToClient
{
    MessageTypeServerToClient type;

    int playerId = -1;
    int playerSum = 0;
    int potAmount = 0;

    std::string name;

    std::unordered_map<int, std::string> playerNames = {};
    std::unordered_map<int, int> playerMoney = {};
    std::unordered_map<int, bool> isSpectatorMap = {};
    std::unordered_map<int, bool> isSeatedMap = {};
    std::unordered_map<int, hand> playerHands = {};

    std::string chatText = "";

    GameState gameState = GameState::WaitingForPlayers;

    bool isSpectator = false; // For Welcome message
    bool isSeated = true;     // For Welcome message

    PlayerActionType action = PlayerActionType::Failed;
    int actionAmount = 0;

    std::string cards; // later can be vector<Card>

    std::vector<int> idWinners = {};          // For Showdown
    std::vector<valRank> communityCards = {}; // For Showdown

    int toAct = -1;     // For GameState
    int toCall = 0;     // For GameState
    int currentBet = 0; // For GameState
    int minRaise = 0;   // For GameState

    int winPower = 0;

    reason_for_rejection rejectionReason = reason_for_rejection::TooManyPlayers; // For Reject
};

inline std::string serialize_client(const MessageClientToServer &m)
{
    std::ostringstream out;

    switch (m.type)
    {
    case MessageTypeClientToServer::Join:
        out << "JOIN " << m.name;
        break;
    case MessageTypeClientToServer::Ready:
        out << "READY";
        break;
    case MessageTypeClientToServer::Chat:
        out << "CHAT " << m.chatText;
        break;
    case MessageTypeClientToServer::Action:
        out << "ACTION " << int(m.action) << " " << m.actionAmount;
        break;
    case MessageTypeClientToServer::RequestState:
        out << "REQUEST_STATE ";
        break;
    case MessageTypeClientToServer::Leave:
        out << "LEAVE ";
        break;
    case MessageTypeClientToServer::AdminPlay:
        out << "ADMIN_PLAY ";
        break;
    case MessageTypeClientToServer::RejectAck:
        out << "REJECT_ACK ";
        break;
    default:
        break;
    }
    out << "\n";

    return out.str();
}

inline MessageClientToServer deserialize_client(const std::string &line)
{
    std::istringstream in(line);
    std::string command;
    in >> command;

    if (command.empty())
        return MessageClientToServer{}; // Return default if command is empty

    MessageClientToServer msg;

    switch (command[0])
    {
    case 'J': // JOIN
        msg.type = MessageTypeClientToServer::Join;
        std::getline(in >> std::ws, msg.name);
        break;
    case 'R': // READY or REQUEST_STATE
        if (command == "READY")
            msg.type = MessageTypeClientToServer::Ready;
        else if (command == "REJECT_ACK")
            msg.type = MessageTypeClientToServer::RejectAck;
        else
            msg.type = MessageTypeClientToServer::RequestState;
        break;
    case 'C': // CHAT
        msg.type = MessageTypeClientToServer::Chat;
        std::getline(in >> std::ws, msg.chatText);
        break;
    case 'A': // ACTION
        if (command == "ADMIN_PLAY")
        {
            msg.type = MessageTypeClientToServer::AdminPlay;
            break;
        }
        msg.type = MessageTypeClientToServer::Action;
        int tempAction;
        in >> tempAction >> msg.actionAmount;
        msg.action = static_cast<PlayerActionType>(tempAction);
        break;
    case 'L': // LEAVE
        msg.type = MessageTypeClientToServer::Leave;
        break;
    default:
        break;
    }
    return msg;
}

inline std::string serialize_server(const MessageServerToClient &m)
{
    std::ostringstream out;

    switch (m.type)
    {
    case MessageTypeServerToClient::Welcome:
        out << "WELCOME " << m.playerId << " " << m.name << " " << m.playerSum;
        for (auto it = m.playerNames.begin(); it != m.playerNames.end(); it++)
        {
            out << " " << it->first << " " << it->second;
        }
        for (auto it = m.playerMoney.begin(); it != m.playerMoney.end(); it++)
        {
            out << " " << it->first << " " << it->second;
        }
        for (auto it = m.isSpectatorMap.begin(); it != m.isSpectatorMap.end(); it++)
        {
            out << " " << it->first << " " << it->second;
        }
        for (auto it = m.isSeatedMap.begin(); it != m.isSeatedMap.end(); it++)
        {
            out << " " << it->first << " " << it->second;
        }
        break;
    case MessageTypeServerToClient::PlayerJoined:
        out << "PLAYER_JOINED " << m.playerId << " " << m.name << " " << m.isSpectator << " " << m.isSeated;
        break;
    case MessageTypeServerToClient::PlayerLeft:
        out << "PLAYER_LEFT " << m.playerId;
        break;
    case MessageTypeServerToClient::PlayerReady:
        out << "PLAYER_READY " << m.playerId;
        break;
    case MessageTypeServerToClient::ChatFrom:
        out << "CHAT_FROM " << m.playerId << " " << m.chatText;
        break;
    case MessageTypeServerToClient::GameState:
        out << "GAME_STATE " << int(m.gameState);
        out << " " << m.potAmount;
        break;
    case MessageTypeServerToClient::ActionResult:
        out << "ACTION_RESULT " << m.playerId << " " << int(m.action) << " " << m.actionAmount;
        break;
    case MessageTypeServerToClient::CommunityCard:
        out << "COMMUNITY_CARD " << m.cards;
        break;
    case MessageTypeServerToClient::PlayerHand:
        out << "PLAYER_HAND " << m.playerId << " " << m.cards;
        break;
    case MessageTypeServerToClient::PotUpdate:
        out << "POT_UPDATE " << m.potAmount;
        break;
    case MessageTypeServerToClient::Showdown:
        out << "SHOWDOWN ";
        out << m.potAmount << " " << m.winPower << " " << m.idWinners.size();
        ;
        for (const auto &id : m.idWinners)
        {
            out << " " << id;
        }
        break;
    case MessageTypeServerToClient::BettingUpdate:
        out << "BETTING_UPDATE " << m.toAct << " " << m.toCall << " " << m.currentBet << " " << m.minRaise << " " << m.potAmount;
        break;
    case MessageTypeServerToClient::Reject:
        out << "REJECT " << int(m.rejectionReason);
        break;
    case MessageTypeServerToClient::SpectatingUpdate:
        out << "SPECTATING_UPDATE " << m.playerId << " " << m.isSpectator << " " << m.isSeated;
        break;
    case MessageTypeServerToClient::NewPlayerUpdateGraphics:
        out << "NEW_PLAYER_UPDATE_GRAPHICS";
        out << " " << m.toAct << " " << m.toCall << " " << m.currentBet << " " << m.minRaise << " " << m.playerHands.size() << " " << m.communityCards.size();
        for (auto it = m.playerHands.begin(); it != m.playerHands.end(); it++)
        {
            out << " " << it->first << " " << it->second.first.value << " " << it->second.first.suit << " " << it->second.second.value << " " << it->second.second.suit;
        }
        for (const auto &card : m.communityCards)
        {
            out << " " << card.value << " " << card.suit;
        }
        break;
    default:
        out << "UNKNOWN_MESSAGE";
        break;
    }
    out << "\n";
    return out.str();
}

inline MessageServerToClient deserialize_server(const std::string &line)
{
    std::istringstream in(line);
    std::string command;
    in >> command;

    if (command.empty())
        return MessageServerToClient{}; // Return default if command is empty

    MessageServerToClient msg;

    switch (command[0])
    {
    case 'W': // WELCOME
        msg.type = MessageTypeServerToClient::Welcome;
        in >> msg.playerId >> msg.name >> msg.playerSum;
        for (int i = 0; i < msg.playerSum; i++)
        {
            int id;
            std::string name;
            in >> id >> name;
            msg.playerNames[id] = name;
        }
        for (int i = 0; i < msg.playerSum; i++)
        {
            int id;
            int money;
            in >> id >> money;
            msg.playerMoney[id] = money;
        }
        for (int i = 0; i < msg.playerSum; i++)
        {
            int id;
            bool isSpectator;
            in >> id >> isSpectator;
            msg.isSpectatorMap[id] = isSpectator;
        }
        for (int i = 0; i < msg.playerSum; i++)
        {
            int id;
            bool isSeated;
            in >> id >> isSeated;
            msg.isSeatedMap[id] = isSeated;
        }
        break;
    case 'P': // PLAYER_JOINED or PLAYER_LEFT or PLAYER_READY or PLAYER_HAND or POT_UPDATE
        if (command == "PLAYER_JOINED")
        {
            msg.type = MessageTypeServerToClient::PlayerJoined;
            in >> msg.playerId >> msg.name >> msg.isSpectator >> msg.isSeated;
        }
        else if (command == "PLAYER_LEFT")
        {
            msg.type = MessageTypeServerToClient::PlayerLeft;
            in >> msg.playerId;
        }
        else if (command == "PLAYER_READY")
        {
            msg.type = MessageTypeServerToClient::PlayerReady;
            in >> msg.playerId;
        }
        else if (command == "PLAYER_HAND")
        {
            msg.type = MessageTypeServerToClient::PlayerHand;
            in >> msg.playerId >> msg.cards;
        }
        else
        {
            msg.type = MessageTypeServerToClient::PotUpdate;
            in >> msg.potAmount;
        }
        break;
    case 'C': // CHAT_FROM or COMMUNITY_CARD
        if (command == "CHAT_FROM")
        {
            msg.type = MessageTypeServerToClient::ChatFrom;
            in >> msg.playerId;
            std::getline(in >> std::ws, msg.chatText);
        }
        else
        {
            msg.type = MessageTypeServerToClient::CommunityCard;
            in >> msg.cards;
        }
        break;
    case 'G': // GAME_STATE
        msg.type = MessageTypeServerToClient::GameState;
        int tempState;
        in >> tempState;
        msg.gameState = static_cast<GameState>(tempState);
        in >> msg.potAmount;
        break;
    case 'A': // ACTION_RESULT
        msg.type = MessageTypeServerToClient::ActionResult;
        int tempAction;
        in >> msg.playerId >> tempAction >> msg.actionAmount;
        msg.action = static_cast<PlayerActionType>(tempAction);
        break;
    case 'S': // SHOWDOWN
        if (command == "SPECTATING_UPDATE")
        {
            msg.type = MessageTypeServerToClient::SpectatingUpdate;
            in >> msg.playerId >> msg.isSpectator >> msg.isSeated;
            break;
        }
        msg.type = MessageTypeServerToClient::Showdown;
        int numWinners;
        in >> msg.potAmount >> msg.winPower >> numWinners;
        for (int i = 0; i < numWinners; i++)
        {
            int id;
            in >> id;
            msg.idWinners.push_back(id);
        }
        break;
    case 'B': // BETTING_UPDATE
        msg.type = MessageTypeServerToClient::BettingUpdate;
        in >> msg.toAct >> msg.toCall >> msg.currentBet >> msg.minRaise >> msg.potAmount;
        break;
    case 'R': // REJECT
        msg.type = MessageTypeServerToClient::Reject;
        int tempReason;
        in >> tempReason;
        msg.rejectionReason = static_cast<reason_for_rejection>(tempReason);
        break;
    case 'N': // NEW_PLAYER_UPDATE_GRAPHICS
        msg.type = MessageTypeServerToClient::NewPlayerUpdateGraphics;
        in >> msg.toAct >> msg.toCall >> msg.currentBet >> msg.minRaise;
        int numPlayerHands, numCommunityCards;
        in >> numPlayerHands >> numCommunityCards;
        for (int i = 0; i < numPlayerHands; i++)
        {
            int id;
            int value1, suit1, value2, suit2;
            in >> id >> value1 >> suit1 >> value2 >> suit2;
            valRank card1{value1, suit1};
            valRank card2{value2, suit2};
            msg.playerHands[id] = (hand{card1, card2});
        }
        for (int i = 0; i < numCommunityCards; i++)
        {
            int value, suit;
            in >> value >> suit;
            valRank card{value, suit};
            msg.communityCards.push_back(card);
        }
        break;
    default:
        break;
    }
    return msg;
}

inline void send_message_client(tcp::socket &socket, const MessageClientToServer &msg)
{
    try
    {
        std::string serialized = serialize_client(msg);
        boost::asio::write(socket, boost::asio::buffer(serialized));
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error sending message: " << e.what() << std::endl;
    }
}

inline MessageClientToServer receive_message_from_client(tcp::socket &socket)
{
    std::string line = "";
    try
    {
        boost::asio::streambuf buf;
        boost::asio::read_until(socket, buf, '\n');
        std::istream is(&buf);
        std::getline(is, line);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error receiving message: " << e.what() << std::endl;
        throw; // Re-raise the exception to signal an error condition
    }
    return deserialize_client(line);
}

inline void send_message_server(tcp::socket &socket, const MessageServerToClient &msg)
{
    try
    {
        std::string serialized = serialize_server(msg);
        boost::asio::write(socket, boost::asio::buffer(serialized));
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error sending message: " << e.what() << std::endl;
    }
}

inline MessageServerToClient receive_message_from_server(tcp::socket &socket)
{
    std::string line = "";
    try
    {
        boost::asio::streambuf buf;
        boost::asio::read_until(socket, buf, '\n');
        std::istream is(&buf);
        std::getline(is, line);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error receiving message: " << e.what() << std::endl;
        throw; // Re-raise the exception to signal an error condition
    }
    return deserialize_server(line);
}

static inline void trim(std::string &s)
{
    if (!s.empty() && s.back() == '\r')
        s.pop_back();
}