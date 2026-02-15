#pragma once
#include "poker_networking.hpp"

class Client;

struct ServerState
{
    std::set<std::shared_ptr<Client>> clients;

    int nextId = 0;
    int pot = 0;
    int currentBet = 0;
    int minRaise = 0;
    int lastAggressor = -1;
    int toAct = 0;

    std::unordered_map<int, std::string> idToName;

    GameState gameState = GameState::WaitingForPlayers;

    void broadcast_all(const std::string &msg);

    bool all_ready() const;

    void reset_game();
};

class Client : public std::enable_shared_from_this<Client>
{
public:
    Client(tcp::socket s, ServerState *state);

    void start();
    void broadcast(const std::string &msg);
    void send_to(const std::string &msg, int id);
    void send(std::shared_ptr<std::string> msg);

    std::function<void()> play_game_ptr;
    std::string display_name() const;

    bool ready;

    int id = -1;
    int money = 1000;
    bool inHand;
    bool allin;
    int betThisRound = 0;

private:
    tcp::socket socket;
    boost::asio::streambuf inbuf;
    ServerState *serverState;

    std::string name;

    std::deque<std::shared_ptr<std::string>> outbox;

    void read_line();

    void do_write();

    void handle_line(const std::string &line);
};