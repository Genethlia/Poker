#pragma once
#include "poker_networking.hpp"
#include "cards.h"

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
        std::vector<Card> communityCards;
        std::vector<Card> myCards;
        std::vector<Card> opponentCards;
        hand myHand;
        int toAct = -1;
        int toCall = 0;
        int currentBet = 0;
        int minRaise = 50;
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

    Card make_card(const MessageServerToClient &msg);

    struct pos
    {
        int x;
        int y;
    };

    vector<pos> playerCardPositions = {
        {20, 400},
        {200, 400},
        {380, 400},
        {560, 400},
        {740, 400},
        {920, 400}};

    vector<pos> communityCardPositions = {
        {200, 200},
        {380, 200},
        {560, 200},
        {740, 200},
        {920, 200}};

    std::unordered_map<int, pos> PlayerPosition = {};
    std::unordered_map<int, bool> firstCard = {{0, true}, {1, true}, {2, true}, {3, true}, {4, true}, {5, true}};

    int nextAvailablePosition = 0;

    Images *suitTextures[4];
    Images *gameImages;
    Font *cardFont;
};