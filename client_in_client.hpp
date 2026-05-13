#pragma once
#include "poker_networking.hpp"
#include "cards.h"

enum class popUpMessageType
{
    PlayerJoined,
    PlayerLeft,
    PlayerReady,
    ChatMessage,
    ActionResult,
    BettingUpdate,
    GameWon,
    GameLost,
    Error
};

using pop = std::pair<std::string, popUpMessageType>;

struct pos
{
    float x;
    float y;
};

enum class Seat
{
    Unassigned,
    Top,
    Left,
    Right,
    Bottom // Self
};

class PokerClient
{
public:
    PokerClient();

    ~PokerClient();

    void connect_to(const std::string &host, const std::string &port);
    void join_us(const std::string &name);
    void start();
    void stop();

    void sendReady();
    void requestState();
    void leaveGame();
    void sendAction(PlayerActionType action, int amount = 0);
    void startGame();
    void sendChat(const std::string &chat);

    std::string nameOf(int id);
    std::string nameOfUnsafe(int id);
    std::atomic<bool> running;

    struct ClientState
    {
        std::unordered_map<int, std::string> playerNames;
        int myId = -1;

        GameState gameState = GameState::WaitingForPlayers;
        int potAmount = 0;
        std::unordered_map<int, int> playerMoney = {}; // player id -> money
        std::unordered_map<int, bool> isSpectator = {};
        std::unordered_map<int, bool> isSeated = {};
        std::vector<valRank> communityCards;
        std::vector<valRank> myCards;
        std::vector<std::pair<int, valRank>> opponentCards;
        std::vector<int> idToShowCardsOf;
        hand myHand;
        int toAct = -1;
        int toCall = 0;
        int currentBet = 0;
        int minRaise = 50;
        int dealerId = -1;
        int smallBlindId = -1;
        int bigBlindId = -1;
        std::map<int, std::pair<Seat, int>> PlayerPosition = {};
    };
    ClientState getClientStateCopy();

    pop createPopUpMessage(MessageServerToClient msg);

    struct popUpMessage
    {
        std::string text;
        double timer = 0.0;
        popUpMessageType type;

        popUpMessage(const std::string &t, popUpMessageType tpe) : text(t), timer(GetTime()), type(tpe)
        {
            std::cout << "Pop-up message created: " << text << "\n";
        }
    };

    std::deque<pop> popUpMessages;
    std::mutex popUpMessagesMutex;
    std::deque<pop> getAndClearPopUpMessages();

private:
    boost::asio::io_context io;
    tcp::socket socket;

    ClientState state;
    std::mutex stateMutex;

    std::thread readerThread;
    void write_line(const std::string &s);
    void readerLoop();

    void handle_line(const std::string &line);
    void newGame();

    valRank extractCardValueSuit(const MessageServerToClient &msg);

    std::atomic<bool> gameRunning = false;

    std::vector<std::pair<Seat, int>>
        playerCardPositionsAndAngles = {
            {Seat::Right, 270}, // position for player 2
            {Seat::Top, 0},     // position for player 0
            {Seat::Left, 90},   // position for player 1
            {Seat::Bottom, 0},  // position for self
    };

    void rebuildPlayerPositions();

    string winPowerTranslation(int winPower);
};