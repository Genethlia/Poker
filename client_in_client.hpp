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
    void Init(Images suitTextures[4], Images *gameImages, Font *cardFont);

    std::string nameOf(int id);
    std::string nameOfUnsafe(int id);

    struct ClientState
    {
        std::unordered_map<int, std::string> playerNames;
        int myId = -1;

        GameState gameState = GameState::WaitingForPlayers;
        int potAmount = 0;
        std::unordered_map<int, int> playerMoney = {}; // player id -> money
        std::vector<valRank> communityCards;
        std::vector<valRank> myCards;
        std::vector<std::pair<int, valRank>> opponentCards;
        hand myHand;
        int toAct = -1;
        int toCall = 0;
        int currentBet = 0;
        int minRaise = 50;
        std::map<int, pos> PlayerPosition = {};
    };
    ClientState getClientStateCopy();

private:
    boost::asio::io_context io;
    tcp::socket socket;

    ClientState state;
    std::mutex stateMutex;

    std::atomic<bool> running;
    std::thread readerThread;

    void UpdateMoney(const MessageServerToClient &msg);

    void write_line(const std::string &s);
    void readerLoop();

    void handle_line(const std::string &line);

    valRank find_valRank(const MessageServerToClient &msg);

    std::vector<pos> playerCardPositions = {
        {20, 400},
        {200, 400},
        {380, 400},
        {560, 400},
        {740, 400},
        {920, 400}};

    std::vector<pos> communityCardPositions = {
        {200, 200},
        {380, 200},
        {560, 200},
        {740, 200},
        {920, 200}};

    void rebuildPlayerPositions();

    Images *suitTextures[4];
    Images *gameImages;
    Font *cardFont;
};