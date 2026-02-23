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
        state.gameState = GameState::PreFlop;
        StartBettingRound(state.handstate.playersOrderd[0]);

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
        // bets
        cout << "Dealing community cards...\n";
        for (int i = 0; i < 3; i++)
        {
            auto card = deck.DrawCard();
            state.handstate.communityCards.push_back(card);
            cout << "Dealing community card " << card.value << " of suit " << card.suit << endl;
        }
        for (auto &card : state.handstate.communityCards)
        {
            state.broadcast_all(serialize_server(MessageServerToClient{
                .type = MessageTypeServerToClient::CommunityCard,
                .cards = to_string(card.value) + "." + to_string(card.suit)}));
        }
        // more bets
        auto card1 = deck.DrawCard();
        state.handstate.communityCards.push_back(card1);
        cout << "Dealing turn card: " << card1.value << " of suit " << card1.suit << endl;
        state.broadcast_all(serialize_server(MessageServerToClient{
            .type = MessageTypeServerToClient::CommunityCard,
            .cards = to_string(card1.value) + "." + to_string(card1.suit)}));

        // more bets
        auto card2 = deck.DrawCard();
        state.handstate.communityCards.push_back(card2);
        cout << "Dealing river card: " << card2.value << " of suit " << card2.suit << endl;
        state.broadcast_all(serialize_server(MessageServerToClient{
            .type = MessageTypeServerToClient::CommunityCard,
            .cards = to_string(card2.value) + "." + to_string(card2.suit)}));
        // last round of bets

        /*cout << "\n--- HAND MAPPING DEBUG ---\n";
        for (size_t j = 0; j < players.size(); j++)
        {
            cout << "index " << j
                 << " = " << players[j]->display_name()
                 << " id=" << players[j]->id
                 << " hole=(" << playerHands[j].first.value << "." << playerHands[j].first.suit
                 << "," << playerHands[j].second.value << "." << playerHands[j].second.suit
                 << ")\n";
        }*/

        cout << "Showdown! Determining winner...\n";
        vector<int> winners = determine_winner(state.handstate.hole, state.handstate.communityCards);
        state.broadcast_all(serialize_server(MessageServerToClient{
            .type = MessageTypeServerToClient::Showdown,
            .potAmount = state.pot,
            .idWinners = winners}));
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
};

int main()
{
    Server server;
}
