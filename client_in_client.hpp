#pragma once
#include "poker_networking.hpp"
#include "cards.h"

struct pos
{
    int x;
    int y;
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
        hand myHand;
        int toAct = -1;
        int toCall = 0;
        int currentBet = 0;
        int minRaise = 50;
        std::map<int, std::pair<pos, int>> PlayerPosition = {};
    };
    ClientState getClientStateCopy();

    struct popUpMessage
    {
        std::string text;
        double timer = 0.0;

        popUpMessage(const std::string &t) : text(t), timer(GetTime())
        {
            cout << "Pop-up message created: " << text << "\n";
        }
    };

    std::deque<string> popUpMessages;
    std::mutex popUpMessagesMutex;
    std::deque<string> getAndClearPopUpMessages();

private:
    boost::asio::io_context io;
    tcp::socket socket;

    ClientState state;
    std::mutex stateMutex;

    std::thread readerThread;

    void UpdateMoney(const MessageServerToClient &msg);

    void write_line(const std::string &s);
    void readerLoop();

    void handle_line(const std::string &line);
    void newGame();

    valRank find_valRank(const MessageServerToClient &msg);

    std::atomic<bool> gameRunning = false;

    std::vector<std::pair<pos, int>> playerCardPositionsAndAngles = {
        {{686, 20}, 0},     // position for player 0
        {{56, 300}, 90},    // position for player 1
        {{1446, 300}, 270}, // position for player 2
        {{686, 700}, 0},    // position for self
    };

    void rebuildPlayerPositions();

    string winPowerTranslation(int winPower);
};