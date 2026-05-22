#pragma once
#include "poker_networking.hpp"
#include "buttons.h"
#include "client_in_client.hpp"

class UiButton
{
public:
    UiButton() = default;
    void Init(PokerClient *client, deque<PokerClient::popUpMessage> *popUpMessages, GameState *gamestate, PokerClient::ClientState *currentState);
    void Draw();
    void Update();

private:
    PokerClient *client;
    deque<PokerClient::popUpMessage> *popUpMessages;
    GameState *gamestate;
    PokerClient::ClientState *currentState;
    ActionButton foldButton;
    CheckCallButton checkCallButton;
    RaiseSliderButton raiseButton;
    ActionButton confirmRaiseButton;
    RaiseAmountButton minRaiseButton;
    RaiseAmountButton potRaiseButton;
    RaiseAmountButton allInButton;
    Button playAgainButton;
    Button startGameButton;
    Button readyButton;
    bool sendReady;
    int raiseAmount;
    bool buttonInteractionFlag;
    quickBetButtonPressed quick;
};