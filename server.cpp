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
        cout << "All players are ready. Starting game...\n";
        state.gameState = GameState::PreFlop;
        cout << "Dealing cards...\n";
        vector<hand> playerHands(state.clients.size());
        for (int round = 0; round < 2; round++)
        {
            for (size_t j = 0; j < players.size(); j++)
            {
                auto card = deck.DrawCard();
                cout << "Dealt card " << card.value << " of suit " << card.suit << " to player " << players[j]->display_name() << endl;
                if (round == 0)
                {
                    playerHands[j].first = card;
                }
                else
                {
                    playerHands[j].second = card;
                }
                players[j]->send_to(serialize_server(MessageServerToClient{
                                        .type = MessageTypeServerToClient::PlayerHand,
                                        .cards = to_string(card.value) + "." + to_string(card.suit)}),
                                    players[j]->id);
            }
        }
        // bets
        cout << "Dealing community cards...\n";
        vector<valRank> communityCards;
        for (int i = 0; i < 3; i++)
        {
            auto card = deck.DrawCard();
            communityCards.push_back(card);
            cout << "Dealing community card " << card.value << " of suit " << card.suit << endl;
        }
        for (auto &card : communityCards)
        {
            state.broadcast_all(serialize_server(MessageServerToClient{
                .type = MessageTypeServerToClient::CommunityCard,
                .cards = to_string(card.value) + "." + to_string(card.suit)}));
        }
        // more bets
        auto card1 = deck.DrawCard();
        communityCards.push_back(card1);
        cout << "Dealing turn card: " << card1.value << " of suit " << card1.suit << endl;
        state.broadcast_all(serialize_server(MessageServerToClient{
            .type = MessageTypeServerToClient::CommunityCard,
            .cards = to_string(card1.value) + "." + to_string(card1.suit)}));

        // more bets
        auto card2 = deck.DrawCard();
        communityCards.push_back(card2);
        cout << "Dealing river card: " << card2.value << " of suit " << card2.suit << endl;
        state.broadcast_all(serialize_server(MessageServerToClient{
            .type = MessageTypeServerToClient::CommunityCard,
            .cards = to_string(card2.value) + "." + to_string(card2.suit)}));
        // last round of bets

        cout << "Showdown! Determining winner...\n";
        vector<int> winners = determine_winner(playerHands, communityCards);
        state.broadcast_all(serialize_server(MessageServerToClient{
            .type = MessageTypeServerToClient::Showdown,
            .potAmount = state.pot,
            .idWinners = winners}));
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
};

int main()
{
    Server server;
    server.start();
}
