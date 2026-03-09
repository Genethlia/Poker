#pragma once
#include "raylib.h"
#include "client_in_client.hpp"

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

    PokerClient client;
    PokerClient::ClientState currentState;
    std::string playerName;

    int raiseAmount;
};