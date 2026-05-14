#include "poker_networking.hpp"
#include "client_in_server.hpp"
#include "deck.h"

class Server
{
public:
    Server();
    ~Server();

    void start();
    void end();
    void play_game();
    void StartBettingRound();
    void AdvanceBetting();
    void onPlayerAction(int playerId, PlayerActionType action, int actionAmount);

private:
    struct SidePot
    {
        int amount;
        std::vector<int> eligiblePlayers;
    };

    boost::asio::io_context io;
    ServerState state;
    Deck deck;
    bool gameInProgress = false;

    shared_ptr<Client> find_client_by_id(int id);
    std::vector<shared_ptr<Client>> activePlayers();

    void promoteWaitingPlayers();
    void gameEndedReset();
    void dealFlop();
    void dealTurnorRiver();
    void runOutToFive();
    void doShowdown();
    void removeBrokePlayers();
    void handleDisconnect(int playerId);
    void removeDisconnectedClients();
    void broadcastGameState();
    void broadcastShowCardsOf(vector<Server::SidePot> &sidePots);
    void broadcastBettingUpdate(int toCall);
    void broadcastActionResult(int playerId, PlayerActionType action, int actionAmount, bool ok);
    void broadcastSpectatingUpdate(shared_ptr<Client> c);
    void advanceDealer(const std::vector<int> &activeIds);
    void chooseBlinds(const std::vector<int> &activeIds);
    void postBlind(int playerId, int amount);

    int countInHand();
    int CountCanAct();
    int countInHand(ServerState &st);
    int nextIdNeedingAction(int startId);
    int indexOfPlayerId(const std::vector<int> &playerIds, int id);
    int nextId(const std::vector<int> &playerIds, int id);

    string findNameById(int id);

    std::vector<int> orderedActiveIds();
    std::vector<shared_ptr<Client>> orderedActivePlayers();

    std::vector<SidePot> buildSidePots();
};
