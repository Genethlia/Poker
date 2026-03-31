#pragma once
#include "poker_networking.hpp"
#include "game_logic.hpp"

class Client;

struct HandState
{
    bool active = false;

    std::vector<int> orderedPlayerIds;
    std::unordered_map<int, hand> hole;
    std::vector<valRank> communityCards;

    int street = 0; // 0: PreFlop, 1: Flop, 2: Turn, 3: River

    void clear()
    {
        active = false;
        orderedPlayerIds.clear();
        hole.clear();
        communityCards.clear();
    }
};

struct ServerState
{
    std::set<std::shared_ptr<Client>> clients;

    HandState handstate;

    int nextId = 0;
    int pot = 0;
    int currentBet = 0;
    int minRaise = 0;
    int lastAggressor = -1;
    int toAct = -1;
    int toCall = 0;

    std::unordered_set<int> needsAction; // player ids that need to act in the current betting round

    std::unordered_map<int, std::string> buildNameSnapshot() const;
    std::unordered_map<int, int> buildMoneySnapshot() const;
    std::unordered_map<int, bool> buildSpectatorSnapshot() const;
    std::unordered_map<int, bool> buildSeatedSnapshot() const;

    GameState gameState = GameState::WaitingForPlayers;

    void broadcast_all(const std::string &msg);

    void send_to(const std::string &msg, int id);

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
    bool connected;
    bool disconnectedHandled;
    bool spectator = false;
    bool wantsToPlay = false;
    bool seated = true;

    std::string getName() const { return name; }
    int id = -1;
    int money = 1000;
    bool inHand;
    bool allin;
    bool hasPendingAction;
    std::string PendingAction;
    std::function<void(int /*playerId*/, PlayerActionType, int /*actionAmount*/)> on_action_ptr;
    std::function<void(int /*playerId*/)> on_disconnect_ptr;
    int betThisRound = 0;

private:
    tcp::socket socket;
    boost::asio::streambuf inbuf;
    ServerState *serverState;
    std::string name;

    std::deque<std::shared_ptr<std::string>> outbox;

    void read_line();

    void do_write();

    void handleDisconnectOnce();

    void handle_line(const std::string &line);
};