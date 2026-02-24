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
                tcp::endpoint(tcp::v4(), 12345));

            cout << "Server running on port 12345...\n";

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
                            client->on_action_ptr = [this](int playerId, PlayerActionType action, int amount)
                            { this->onPlayerAction(playerId, action, amount); };

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

    void end()
    {
        io.stop();
    }
    void play_game()
    {
        vector<shared_ptr<Client>> players;
        for (auto &client : state.clients)
            players.push_back(client);

        sort(players.begin(), players.end(), [](const shared_ptr<Client> &a, const shared_ptr<Client> &b)
             { return a->id < b->id; });

        if (state.clients.size() < 2)
        {
            cout << "Not enough players to start the game.\n";
            return;
        }
        if (!state.all_ready())
        {
            cout << "Not all players are ready.\n";
            return;
        }

        state.pot = 0;
        state.gameState = GameState::PreFlop;
        state.currentBet = 0;
        state.minRaise = 50;
        state.needsAction.clear();

        state.handstate.clear();
        state.handstate.active = true;
        state.handstate.street = 0;

        for (auto &client : players)
        {
            client->inHand = true;
            client->allin = false;
            client->betThisRound = 0;
            state.handstate.playersOrderd.push_back(client->id);
        }

        state.handstate.hole.resize(players.size());

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
                    state.handstate.hole[j].first = card;
                }
                else
                {
                    state.handstate.hole[j].second = card;
                }
                players[j]->send_to(serialize_server(MessageServerToClient{
                                        .type = MessageTypeServerToClient::PlayerHand,
                                        .cards = to_string(card.value) + "." + to_string(card.suit)}),
                                    players[j]->id);
            }
        }
        state.gameState = GameState::PreFlop;
        StartBettingRound(state.handstate.playersOrderd[0]);
    }

    void StartBettingRound(int firstToAct)
    {
        state.currentBet = 0;
        state.minRaise = 50; // BB for now
        state.needsAction.clear();

        for (auto &c : state.clients)
        {
            c->betThisRound = 0;
            if (c->inHand && !c->allin)
                state.needsAction.insert(c->id);
        }

        state.toAct = firstToAct;

        AdvanceBetting();
    }
    void AdvanceBetting()
    {
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
                }
            }
            state.gameState = GameState::Showdown;
            state.broadcast_all(serialize_server(MessageServerToClient{
                .type = MessageTypeServerToClient::Showdown,
                .potAmount = state.pot,
                .idWinners = {winnerId}}));
        }

        if (CountCanAct() == 0 && countInHand() > 1)
        {
            runOutToFive();
            doShowdown();
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
                return;
            }

            int next = nextIdNeedingAction(state, state.handstate.playersOrderd[0]);
            state.toAct = next;

            auto p = find_client_by_id(next);
            if (!p)
                return;

            state.broadcast_all(serialize_server(MessageServerToClient{
                .type = MessageTypeServerToClient::GameState,
                .playerId = next,
                .toCall = toCall,
                .currentBet = state.currentBet,
                .minRaise = state.minRaise,
                .potAmount = state.pot}));
        }

        return;
    }

private:
    boost::asio::io_context io;
    ServerState state;
    Deck deck;

    shared_ptr<Client> find_client_by_id(int id)
    {
        for (auto &client : state.clients)
        {
            if (client->id == id)
                return client;
        }
        return nullptr;
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
                .cards = to_string(card.value) + "." + to_string(card.suit)}));
        }
    }
    void dealTurnorRiver()
    {
        auto card = deck.DrawCard();
        state.handstate.communityCards.push_back(card);
        cout << "Dealing community card " << card.value << " of suit " << card.suit << endl;
        state.broadcast_all(serialize_server(MessageServerToClient{
            .type = MessageTypeServerToClient::CommunityCard,
            .cards = to_string(card.value) + "." + to_string(card.suit)}));
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
                .cards = to_string(card.value) + "." + to_string(card.suit)}));
        }
    }

    void doShowdown()
    {
        vector<hand> hands = state.handstate.hole;
        vector<valRank> community = state.handstate.communityCards;

        auto winners = determine_winner(hands, community);
        state.gameState = GameState::Showdown;
        state.broadcast_all(serialize_server(MessageServerToClient{
            .type = MessageTypeServerToClient::Showdown,
            .potAmount = state.pot,
            .idWinners = winners}));

        if (winners.size() == 1)
        {
            auto winner = find_client_by_id(winners[0]);
            if (winner)
            {
                winner->money += state.pot;
                cout << "Player " << winner->display_name() << " wins the pot of " << state.pot << " with a showdown!\n";
            }
        }
        state.handstate.active = false;
    }
    shared_ptr<Client> findClientById(ServerState &st, int pid)
    {
        for (auto &c : st.clients)
            if (c->id == pid)
                return c;
        return nullptr;
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
};

int main()
{
    Server server;
}
