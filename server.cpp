#include "poker_networking.hpp"
#include "client_in_server.hpp"
#include "deck.h"
using namespace std;

class Server
{
public:
    Server()
    {
        start();
    }
    ~Server()
    {
        end();
    }

    void start()
    {
        try
        {

            tcp::acceptor acceptor(
                io,
                tcp::endpoint(tcp::v4(), 6767));

            cout << "Server running on port 6767...\n";

            function<void()> accept_loop;

            accept_loop = [&]()
            {
                acceptor.async_accept(
                    [&](boost::system::error_code ec, tcp::socket socket)
                    {
                        if (!ec)
                        {
                            auto client =
                                make_shared<Client>(move(socket), &state);

                            client->play_game_ptr = [this]()
                            { this->play_game(); };
                            client->on_action_ptr = [this](int playerId, PlayerActionType action, int actionAmount)
                            { this->onPlayerAction(playerId, action, actionAmount); };
                            client->on_disconnect_ptr = [this](int playerId)
                            { this->handleDisconnect(playerId); };

                            state.clients.insert(client);
                            client->start();

                            cout << "Client connected. Total: "
                                 << state.clients.size() << endl;
                        }
                        accept_loop();
                    });
            };

            accept_loop();
            io.run();
        }
        catch (exception &e)
        {
            cerr << "Error: " << e.what() << endl;
        }
    }

    void
    end()
    {
        io.stop();
    }
    void play_game()
    {
        promoteWaitingPlayers();
        vector<shared_ptr<Client>> players;

        for (auto &c : state.clients)
        {
            if (c->connected && c->ready && !c->spectator && c->seated)
            {
                players.push_back(c);
            }
        }
        sort(players.begin(), players.end(), [](const shared_ptr<Client> &a, const shared_ptr<Client> &b)
             { return a->id < b->id; });

        if (players.size() < 2)
        {
            cout << "Not enough players to start the game.\n";
            return;
        }
        if (!state.all_ready())
        {
            cout << "Not all players are ready.\n";
            return;
        }
        if (gameInProgress)
        {
            cout << "Game already in progress.\n";
            return;
        }
        removeDisconnectedClients();

        for (auto &client : state.clients)
        {
            client->inHand = false;
            client->allin = false;
            client->betThisRound = 0;
        }

        gameInProgress = true;
        state.pot = 0;
        state.gameState = GameState::PreFlop;
        state.currentBet = 50; // BB for now
        state.minRaise = 50;
        state.needsAction.clear();

        state.handstate.clear();
        state.handstate.active = true;
        state.handstate.street = 0;

        state.broadcast_all(serialize_server(MessageServerToClient{
            .type = MessageTypeServerToClient::GameState,
            .gameState = state.gameState}));
        state.broadcast_all(serialize_server(MessageServerToClient{
            .type = MessageTypeServerToClient::PotUpdate,
            .potAmount = state.pot}));

        for (auto &client : players)
        {
            client->inHand = true;
            client->allin = false;
            client->betThisRound = 0;
            state.handstate.orderedPlayerIds.push_back(client->id);
        }

        state.handstate.hole.clear();

        cout << "All players are ready. Starting game...\n";

        cout << "Dealing cards...\n";
        for (int round = 0; round < 2; round++)
        {
            for (size_t j = 0; j < players.size(); j++)
            {
                auto card = deck.DrawCard();
                cout << "Dealt card " << card.value << " of suit " << card.suit << " to player " << players[j]->display_name() << endl;
                if (round == 0)
                {
                    state.handstate.hole[players[j]->id].first = card;
                }
                else
                {
                    state.handstate.hole[players[j]->id].second = card;
                }
                state.broadcast_all(serialize_server(MessageServerToClient{
                    .type = MessageTypeServerToClient::PlayerHand,
                    .playerId = players[j]->id,
                    .cards = to_string(card.value) + "," + to_string(card.suit)}));
            }
        }
        state.gameState = GameState::PreFlop;
        StartBettingRound();
    }

    void StartBettingRound()
    {
        state.minRaise = 50;                                       // BB for now
        state.currentBet = (state.handstate.street == 0) ? 50 : 0; // PreFlop, start with BB, otherwise start with 0
        state.needsAction.clear();

        for (auto &c : state.clients)
        {
            if (!c->connected)
                continue;
            c->betThisRound = 0;
            if (c->inHand && !c->allin)
                state.needsAction.insert(c->id);
        }

        AdvanceBetting();
    }
    void AdvanceBetting()
    {
        if (!gameInProgress)
            return;

        for (auto &c : state.clients)
        {
            if (!c->inHand || c->allin)
            {
                state.needsAction.erase(c->id);
            }
        }
        if (countInHand() <= 1)
        {
            int winnerId = -1;
            for (auto &c : state.clients)
            {
                if (c->inHand)
                {
                    winnerId = c->id;
                    break;
                }
            }

            if (winnerId != -1)
            {
                auto winner = find_client_by_id(winnerId);
                if (winner)
                {
                    winner->money += state.pot;
                    cout << "Player " << winner->display_name() << " wins the pot of " << state.pot << " by everyone else folding!\n";
                    // state.idToMoney[winner->id] = winner->money;
                }
            }
            state.gameState = GameState::Showdown;
            state.broadcast_all(serialize_server(MessageServerToClient{
                .type = MessageTypeServerToClient::Showdown,
                .potAmount = state.pot,
                .idWinners = {winnerId},
                .winPower = 0}));
            state.broadcast_all(serialize_server(MessageServerToClient{
                .type = MessageTypeServerToClient::GameState,
                .gameState = state.gameState}));
            gameInProgress = false;
            cout << "Game ended. Waiting for players to be ready for the next game...\n";
            gameEndedReset();
            return;
        }

        if (CountCanAct() == 0 && countInHand() > 1)
        {
            runOutToFive();
            doShowdown();
            gameInProgress = false;
            gameEndedReset();
            cout << "Game ended. Waiting for players to be ready for the next game...\n";

            return;
        }

        if (state.needsAction.empty())
        {
            if (state.handstate.street == 0)
            {
                dealFlop();
                state.handstate.street = 1;
                state.gameState = GameState::Flop;
            }
            else if (state.handstate.street == 1)
            {
                dealTurnorRiver();
                state.handstate.street = 2;
                state.gameState = GameState::Turn;
            }
            else if (state.handstate.street == 2)
            {
                dealTurnorRiver();
                state.handstate.street = 3;
                state.gameState = GameState::River;
            }
            else
            {
                doShowdown();
                gameInProgress = false;
                gameEndedReset();
                cout << "Game ended. Waiting for players to be ready for the next game...\n";
                return;
            }
            state.broadcast_all(serialize_server(MessageServerToClient{
                .type = MessageTypeServerToClient::GameState,
                .potAmount = state.pot,
                .gameState = state.gameState}));

            StartBettingRound();
            removeDisconnectedClients(); // Remove disconnected clients between betting rounds
            return;
        }

        int next = nextIdNeedingAction(state, state.toAct);
        if (next == -1)
        {
            cout << "ERROR: no active ids, cannot pick toAct\n";
            return;
        }
        state.toAct = next;

        auto p = find_client_by_id(next);
        if (!p)
            return;

        int toCall = max(0, state.currentBet - p->betThisRound);

        cout << "Next to act: " << state.toAct << " toCall=" << toCall << " currentBet=" << state.currentBet << " minRaise=" << state.minRaise << "\n";

        state.broadcast_all(serialize_server(MessageServerToClient{
            .type = MessageTypeServerToClient::BettingUpdate,
            .potAmount = state.pot,
            .toAct = state.toAct,
            .toCall = toCall,
            .currentBet = state.currentBet,
            .minRaise = state.minRaise,
        }));
    }
    void onPlayerAction(int playerId, PlayerActionType action, int actionAmount)
    {
        if (playerId != state.toAct)
        {

            auto it = find_client_by_id(playerId);
            cout << "Received action from player " << (it ? it->display_name() : "Unknown") << " but it's not their turn.\n";
            state.send_to(serialize_server(MessageServerToClient{
                              .type = MessageTypeServerToClient::ActionResult,
                              .playerId = playerId,
                              .action = PlayerActionType::Failed,
                              .actionAmount = 0}),
                          playerId);
            return;
        }
        auto p = find_client_by_id(playerId);
        if (!p)
        {
            cout << "Received action from unknown player " << playerId << ".\n";
            return;
        }
        if (!p->inHand || p->allin)
        {
            cout << "Received action from player " << playerId << " who is not in hand or already all-in.\n";
            return;
        }
        if (p->spectator)
        {
            cout << "Received action from player " << playerId << " who is a spectator.\n";
            return;
        }

        int toCall = max(0, state.currentBet - p->betThisRound);

        bool ok = true;

        auto pay = [&](int amt)
        {
            int act = min(amt, p->money);
            p->money -= act;
            p->betThisRound += act;
            state.pot += act;
            if (p->money == 0)
                p->allin = true;
            return act;
        };

        switch (action)
        {
        case PlayerActionType::Fold:
            p->inHand = false;
            state.needsAction.erase(playerId);
            break;
        case PlayerActionType::Check:
            if (toCall != 0)
            {
                ok = false;
                break;
            }
            state.needsAction.erase(playerId);
            break;
        case PlayerActionType::Call:
            if (toCall > 0)
                pay(toCall);
            state.needsAction.erase(playerId);
            break;
        case PlayerActionType::Raise:
        {
            int raiseTo = actionAmount;
            int minTo = state.currentBet + state.minRaise;
            int maxTo = p->betThisRound + p->money;
            if (raiseTo < minTo)
            {
                ok = false;
                break;
            }
            if (raiseTo > maxTo)
            {
                raiseTo = maxTo;
            }

            int additional = raiseTo - p->betThisRound;
            if (additional <= 0)
            {
                ok = false;
                break;
            }
            pay(additional);

            int newTotalBet = p->betThisRound;
            int raiseAmount = newTotalBet - state.currentBet;
            if (raiseAmount > 0)
                state.minRaise = raiseAmount;
            state.currentBet = max(state.currentBet, newTotalBet);
            state.needsAction.clear();
            for (auto &c : state.clients)
            {
                if (c->inHand && !c->allin && c->id != playerId)
                    state.needsAction.insert(c->id);
            }
            break;
        }
        default:
            ok = false;
            break;
        }
        state.broadcast_all(serialize_server(MessageServerToClient{
            .type = MessageTypeServerToClient::ActionResult,
            .playerId = playerId,
            .potAmount = state.pot,
            .action = ok ? action : PlayerActionType::Failed,
            .actionAmount = actionAmount,
        }));
        cout << "Player " << p->display_name() << " performed action " << int(action) << " with amount " << actionAmount << (ok ? "" : " (invalid)") << ". Pot is now " << state.pot << ".\n";
        AdvanceBetting();
    }

private:
    boost::asio::io_context io;
    ServerState state;
    Deck deck;
    bool gameInProgress = false;

    shared_ptr<Client> find_client_by_id(int id)
    {
        for (auto &client : state.clients)
        {
            if (client->id == id)
                return client;
        }
        return nullptr;
    }

    void promoteWaitingPlayers()
    {
        int activePlayers = 0;
        for (auto &c : state.clients)
        {
            if (c->connected && !c->spectator)
                activePlayers++;
        }
        for (auto &c : state.clients)
        {
            if (activePlayers >= 4)
                break;

            if (c->connected && c->spectator)
            {
                c->spectator = false;
                c->wantsToPlay = false;
                c->seated = true;
                activePlayers++;
                state.broadcast_all(serialize_server(MessageServerToClient{
                    .type = MessageTypeServerToClient::SpectatingUpdate,
                    .playerId = c->id,
                    .isSpectator = c->spectator,
                    .isSeated = c->seated}));
            }
        }
    }

    void gameEndedReset()
    {
        for (auto &c : state.clients)
        {
            c->inHand = false;
            c->allin = false;
            c->betThisRound = 0;
        }
        state.pot = 0;
        state.currentBet = 0;
        state.minRaise = 0;
        state.toAct = -1;
        state.needsAction.clear();
        state.handstate.clear();

        removeBrokePlayers();
        promoteWaitingPlayers();

        // play_game();
    }

    void removeBrokePlayers()
    {
        for (auto &c : state.clients)
        {
            if (c->money <= 0)
            {
                c->spectator = true;
                c->seated = false;
                c->wantsToPlay = false;
                state.broadcast_all(serialize_server(MessageServerToClient{
                    .type = MessageTypeServerToClient::SpectatingUpdate,
                    .playerId = c->id,
                    .isSpectator = c->spectator,
                    .isSeated = c->seated}));
            }
        }
    }

    vector<shared_ptr<Client>> activePlayers()
    {
        vector<shared_ptr<Client>> active;
        for (auto &client : state.clients)
        {
            if (client->inHand && !client->allin)
                active.push_back(client);
        }
        return active;
    }

    int countInHand()
    {
        int count = 0;
        for (auto &client : state.clients)
        {
            if (client->inHand)
                count++;
        }
        return count;
    }
    int CountCanAct()
    {
        int n = 0;
        for (auto &c : state.clients)
        {
            if (c->inHand && !c->allin)
                n++;
        }
        return n;
    }
    void dealFlop()
    {
        for (int i = 0; i < 3; i++)
        {
            auto card = deck.DrawCard();
            state.handstate.communityCards.push_back(card);
            cout << "Dealing community card " << card.value << " of suit " << card.suit << endl;
            state.broadcast_all(serialize_server(MessageServerToClient{
                .type = MessageTypeServerToClient::CommunityCard,
                .cards = to_string(card.value) + "," + to_string(card.suit)}));
        }
    }
    void dealTurnorRiver()
    {
        auto card = deck.DrawCard();
        state.handstate.communityCards.push_back(card);
        cout << "Dealing community card " << card.value << " of suit " << card.suit << endl;
        state.broadcast_all(serialize_server(MessageServerToClient{
            .type = MessageTypeServerToClient::CommunityCard,
            .cards = to_string(card.value) + "," + to_string(card.suit)}));
    }
    void runOutToFive()
    {
        while (state.handstate.communityCards.size() < 5)
        {
            auto card = deck.DrawCard();
            state.handstate.communityCards.push_back(card);
            cout << "Dealing community card " << card.value << " of suit " << card.suit << endl;
            state.broadcast_all(serialize_server(MessageServerToClient{
                .type = MessageTypeServerToClient::CommunityCard,
                .cards = to_string(card.value) + "," + to_string(card.suit)}));
        }
    }

    void doShowdown()
    {
        vector<hand> hands;
        vector<int> handOwnerIds;
        for (size_t i = 0; i < state.handstate.orderedPlayerIds.size(); i++)
        {
            int pid = state.handstate.orderedPlayerIds[i];
            auto p = find_client_by_id(pid);
            if (p && p->inHand)
            {
                hands.push_back(state.handstate.hole[pid]);
                handOwnerIds.push_back(pid);
            }
        }

        if (hands.empty())
        {
            cout << "No hands left for showdown.\n";
            state.handstate.active = false;
            gameInProgress = false;
            removeDisconnectedClients();
            return;
        }

        vector<valRank> community = state.handstate.communityCards;

        int winPower = 0;
        auto winnerIndexes = determine_winner(hands, community, winPower);

        vector<int> winners;
        for (int idx : winnerIndexes)
        {
            if (idx >= 0 && idx < (int)handOwnerIds.size())
                winners.push_back(handOwnerIds[idx]);
        }

        state.gameState = GameState::Showdown;
        state.broadcast_all(serialize_server(MessageServerToClient{
            .type = MessageTypeServerToClient::GameState,
            .gameState = state.gameState}));
        state.broadcast_all(serialize_server(MessageServerToClient{
            .type = MessageTypeServerToClient::Showdown,
            .potAmount = state.pot,
            .idWinners = winners,
            .winPower = winPower}));

        if (winners.size() == 1)
        {
            auto winner = find_client_by_id(winners[0]);
            if (winner)
            {
                winner->money += state.pot;
                cout << "Player " << winner->display_name() << " wins the pot of " << state.pot << " with a showdown!\n";
            }
        }
        else
        {
            cout << "Players ";
            for (size_t i = 0; i < winners.size(); i++)
            {
                auto winner = find_client_by_id(winners[i]);
                if (winner)
                {
                    winner->money += state.pot / winners.size();
                    cout << winner->display_name() << " (ID: " << winner->id << ") ";
                }
            }
            cout << "split the pot of " << state.pot << " with a showdown!\n";
            auto remainder_winner = find_client_by_id(state.toAct);
            if (remainder_winner)
            {
                remainder_winner->money += state.pot % winners.size();
            }
        }

        state.handstate.active = false;
        gameInProgress = false;
        removeDisconnectedClients();
    }
    void handleDisconnect(int playerId)
    {
        auto p = find_client_by_id(playerId);
        if (!p)
            return;

        p->connected = false;
        p->ready = false;

        cout << "Handling disconnect for " << p->display_name() << "\n";

        if (p->inHand)
        {
            p->inHand = false;
            p->allin = false;
            state.needsAction.erase(playerId);
        }

        if (state.toAct == playerId)
            state.toAct = -1;

        // state.idToMoney[playerId] = p->money;

        MessageServerToClient msg;
        msg.type = MessageTypeServerToClient::PlayerLeft;
        msg.playerId = playerId;
        msg.name = findNameById(playerId);
        state.broadcast_all(serialize_server(msg));

        if (!gameInProgress)
        {
            removeDisconnectedClients();
            return;
        }

        AdvanceBetting();
    }
    void removeDisconnectedClients()
    {
        vector<shared_ptr<Client>> toRemove;
        for (auto &c : state.clients)
        {
            if (!c->connected)
                toRemove.push_back(c);
        }
        for (auto &c : toRemove)
        {
            state.needsAction.erase(c->id);
            state.clients.erase(c);
            cout << "Removed client " << c->display_name() << " from server.\n";
        }
    }

    int countInHand(ServerState &st)
    {
        int n = 0;
        for (auto &c : st.clients)
            if (c->inHand)
                n++;
        return n;
    }

    vector<int> orderedActiveIds(ServerState &st)
    {
        vector<int> ids;
        for (auto &c : st.clients)
            if (c->inHand && !c->allin)
                ids.push_back(c->id);
        sort(ids.begin(), ids.end());
        return ids;
    }

    int nextIdNeedingAction(ServerState &st, int startId)
    {
        auto ids = orderedActiveIds(st);
        if (ids.empty())
            return -1;

        int startIndex = 0;
        for (int i = 0; i < (int)ids.size(); i++)
            if (ids[i] == startId)
            {
                startIndex = i;
                break;
            }

        for (int k = 0; k < (int)ids.size(); k++)
        {
            int pid = ids[(startIndex + k) % ids.size()];
            if (st.needsAction.count(pid))
                return pid;
        }
        return ids[0];
    }

    string findNameById(int id)
    {
        auto p = find_client_by_id(id);
        if (p)
            return p->display_name();
        return "Unknown";
    }
};

int main()
{
    Server server;
}
