#include "uibuttons.h"

void UiButton::Init(PokerClient *client, deque<PokerClient::popUpMessage> *popUpMessages, GameState *gamestate, PokerClient::ClientState *currentState)
{
    this->client = client;
    this->popUpMessages = popUpMessages;
    this->gamestate = gamestate;
    this->currentState = currentState;
    sendReady = false;
    readyButton.Init(0, 550, 200, 50, "Ready", [this]()
                     {
                                if (!sendReady)
                                {
                                    this->client->sendReady();
                                    sendReady = true;
                                }
                                else
                                {
                                    this->popUpMessages->push_back(PokerClient::popUpMessage("You have already sent ready for this game.", popUpMessageType::Error));
                                } });
    playAgainButton.Init(0, 610, 200, 50, "Play Again", [this]()
                         {
                                    if (*this->gamestate == GameState::GameOver)
                                    {
                                        this->client->startGame();
                                    }
                                    else
                                    {
                                        this->popUpMessages->push_back(PokerClient::popUpMessage("You can only start a new game once the current game is over.", popUpMessageType::Error));
                                    } });
    startGameButton.Init(0, 670, 200, 50, "Start Game", [this]()
                         {
                                    if (*this->gamestate == GameState::WaitingForPlayers)
                                    {
                                        this->client->startGame();
                                    }
                                    else
                                    {
                                        this->popUpMessages->push_back(PokerClient::popUpMessage("You can only start the game when it's in the waiting for players state.", popUpMessageType::Error));
                                    } });
    raiseAmount = 0;
    foldButton.Init(0, 610, 200, 50, "Fold", [this]()
                    { this->client->sendAction(PlayerActionType::Fold); }, &currentState->toAct, &currentState->myId);
    checkCallButton.Init(0, 730, 200, 50, "Check", [this, currentState]()
                         {
        if (currentState->toCall > 0)
        {
            this->client->sendAction(PlayerActionType::Call, currentState->toCall);
        }
        else
        {
            this->client->sendAction(PlayerActionType::Check);
        } }, &currentState->toAct, &currentState->myId, &currentState->toCall);
    raiseButton.Init(0, 790, 200, 50, &currentState->toAct, &currentState->myId, &currentState->minRaise, &currentState->playerMoney[currentState->myId], &raiseAmount);
    confirmRaiseButton.Init(0, 850, 200, 50, "Confirm Raise", [this]()
                            {
                                                if (raiseAmount > 0)
                                                {
                                                    this->client->sendAction(PlayerActionType::Raise, raiseAmount);
                                                    raiseAmount = 0;
                                                }
                                                else
                                                {
                                                    this->popUpMessages->push_back(PokerClient::popUpMessage("Raise amount must be greater than 0.", popUpMessageType::Error));
                                                } }, &currentState->toAct, &currentState->myId);
}

void UiButton::Draw()
{
    if (!sendReady || *gamestate == GameState::WaitingForPlayers)
    {
        readyButton.Draw();
    }
    if (sendReady && *gamestate == GameState::WaitingForPlayers)
    {
        startGameButton.Draw();
    }
    else if (*gamestate == GameState::GameOver)
    {
        playAgainButton.Draw();
    }
    else if (*gamestate != GameState::WaitingForPlayers && *gamestate != GameState::GameOver)
    {
        foldButton.Draw();
        checkCallButton.Draw();
        raiseButton.Draw();
        confirmRaiseButton.Draw();
    }
}

void UiButton::Update()
{
    if (!sendReady || *gamestate == GameState::WaitingForPlayers)
    {
        readyButton.Update();
    }
    if (sendReady && *gamestate == GameState::WaitingForPlayers)
    {
        startGameButton.Update();
    }
    else if (*gamestate == GameState::GameOver)
    {
        playAgainButton.Update();
    }
    else if (*gamestate != GameState::WaitingForPlayers && *gamestate != GameState::GameOver)
    {
        foldButton.Update();
        checkCallButton.Update();
        raiseButton.Update();
        confirmRaiseButton.Update();
    }
}