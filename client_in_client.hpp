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

    std::string nameOf(int id);
    std::string nameOfUnsafe(int id);

    struct ClientState
    {
        std::unordered_map<int, std::string> playerNames;
        int myId = -1;

        GameState gameState = GameState::WaitingForPlayers;
        int potAmount = 0;
        std::unordered_map<int, int> playerMoney; // player id -> money
        std::vector<valRank> communityCards;
        hand myHand;
        std::vector<hand> playerHands; // indexed by player id, -1 if unknown
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
};