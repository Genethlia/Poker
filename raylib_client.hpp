#pragma once
#include "raylib.h"
#include "client_in_client.hpp"
#include "cards.h"

class Game
{
public:
    Game();
    ~Game();

    void start();

private:
    void input();
    void update();
    void draw();
    void shouldNewCardBeMade();
    void clearCardsIfNecessary(); // Check if cards have been cleared in the client state and clear them in the game if that's the case

    PokerClient client = PokerClient();
    PokerClient::ClientState currentState;
    std::string playerName;

    Images suitTextures[4];
    Images gameImages;

    Font cardFont;

    std::vector<Card> myCards;
    std::vector<Card> opponentCards;
    std::vector<Card> communityCards;

    int raiseAmount;

    pos getPlayerCardBasePos(int id);
    int getPlayerCardRotationAngle(int id);
    std::unordered_map<int, int> holeCardsDrownCount = {};
};