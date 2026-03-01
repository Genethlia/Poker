#pragma once
#include "poker_networking.hpp"

class PokerClient
{
public:
    PokerClient();
    ~PokerClient();

    void connect_to(const std::string &host, const std::string &port);
    void join_us(const std::string &name);
    void start();
    void input();
    void terminal(const std::string &input);
    void sendChat(const std::string &chat);
    void stop();

private:
    struct ClientState
    {
        std::unordered_map<int, std::string> playerNames;
        int myId = -1;

        GameState gameState = GameState::WaitingForPlayers;
        int potAmount = 0;
        std::vector<valRank> communityCards;
        hand myHand;
        std::vector<hand> playerHands; // indexed by player id, -1 if unknown
        int toAct = -1;
        int toCall = 0;
        int currentBet = 0;
        int minRaise = 50;
    };

    boost::asio::io_context io;
    tcp::socket socket;

    ClientState state;

    std::atomic<bool> running;
    std::thread readerThread;

    void write_line(const std::string &s);
    void readerLoop();

    std::string nameOf(int id);

    void handle_line(const std::string &line, ClientState &state);
};