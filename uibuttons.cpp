#include "uibuttons.h"

void UiButton::Init(PokerClient *client, deque<PokerClient::popUpMessage> *popUpMessages, GameState *gamestate, PokerClient::ClientState *currentState, Font *buttonFont)
{
    this->client = client;
    this->popUpMessages = popUpMessages;
    this->gamestate = gamestate;
    this->currentState = currentState;
    this->buttonFont = buttonFont;
    sendReady = false;
    readyButton.Init(10, 560, 200, 50, "Ready", [this]()
                     {
                                if (!sendReady)
                                {
                                    this->client->sendReady();
                                    sendReady = true;
                                }
                                else
                                {
                                    this->popUpMessages->push_back(PokerClient::popUpMessage("You have already sent ready for this game.", popUpMessageType::Error));
                                } }, 2, buttonFont); // Green
    playAgainButton.Init(10, 630, 200, 50, "Play Again", [this]()
                         {
                                    if (*this->gamestate == GameState::GameOver)
                                    {
                                        this->client->startGame();
                                    }
                                    else
                                    {
                                        this->popUpMessages->push_back(PokerClient::popUpMessage("You can only start a new game once the current game is over.", popUpMessageType::Error));
                                    } }, 1, buttonFont); // Blue
    startGameButton.Init(10, 630, 200, 50, "Start Game", [this]()
                         {
                                    if (*this->gamestate == GameState::WaitingForPlayers)
                                    {
                                        this->client->startGame();
                                    }
                                    else
                                    {
                                        this->popUpMessages->push_back(PokerClient::popUpMessage("You can only start the game when it's in the waiting for players state.", popUpMessageType::Error));
                                    } }, 2, buttonFont); // Green
    raiseAmount = 0;
    buttonInteractionFlag = false;
    quick = quickBetButtonPressed::None;
    foldButton.Init(10, 800, 200, 80, "Fold", [this]()
                    { this->client->sendAction(PlayerActionType::Fold); }, &currentState->toAct, &currentState->myId, 0, buttonFont); // Red
    checkCallButton.Init(230, 800, 200, 80, "Check", [this, currentState]()
                         {
        if (currentState->toCall > 0)
        {
            this->client->sendAction(PlayerActionType::Call, currentState->toCall);
        }
        else
        {
            this->client->sendAction(PlayerActionType::Check);
        } }, &currentState->toAct, &currentState->myId, &currentState->toCall, buttonFont);
    raiseButton.Init(1020, 820, 500, 50, currentState, &raiseAmount, &buttonInteractionFlag, &quick, buttonFont);
    confirmRaiseButton.Init(450, 800, 200, 80, "Bet", [this]()
                            {
                                                if (raiseAmount > 0)
                                                {
                                                    this->client->sendAction(PlayerActionType::Raise, raiseAmount);
                                                    raiseAmount = 0;
                                                    buttonInteractionFlag = false;
                                                }
                                                else
                                                {
                                                    this->popUpMessages->push_back(PokerClient::popUpMessage("Raise amount must be greater than 0.", popUpMessageType::Error));
                                                } }, &currentState->toAct, &currentState->myId, 3, buttonFont); // Gold
    minRaiseButton.Init(1350, 620, 220, 50, "Min Raise", [this, currentState]()
                        {
        if(!currentState->playerMoney.count(currentState->myId))
            return;
        if(!currentState->betThisRound.count(currentState->myId))
            return;
        int money = currentState->playerMoney[currentState->myId];
        int betthisRound = currentState->betThisRound[currentState->myId];
        int minTotalRaise = currentState->minRaise + currentState->currentBet;
        raiseAmount = std::min(minTotalRaise,money+betthisRound);
        quick=quickBetButtonPressed::Min; }, &currentState->toAct, &currentState->myId, 2, buttonFont); // Green

    potRaiseButton.Init(1350, 680, 220, 50, "Pot Raise", [this, currentState]()
                        {
                            if(!currentState->playerMoney.count(currentState->myId))
                                return;
                            if(!currentState->betThisRound.count(currentState->myId))
                                return;
                            int money = currentState->playerMoney[currentState->myId];
                            int betthisRound = currentState->betThisRound[currentState->myId];
                            int potRaise = currentState->potAmount + currentState->currentBet;
                            raiseAmount = std::min(potRaise,money+betthisRound);
                            quick=quickBetButtonPressed::Pot; }, &currentState->toAct, &currentState->myId, 1, buttonFont); // Blue
    allInButton.Init(1350, 740, 220, 50, "All In", [this, currentState]()
                     {
                         if (!currentState->playerMoney.count(currentState->myId))
                             return;
                         if (!currentState->betThisRound.count(currentState->myId))
                             return;
                         int money = currentState->playerMoney[currentState->myId];
                         int betthisRound = currentState->betThisRound[currentState->myId];
                         raiseAmount = money + betthisRound;
                          quick=quickBetButtonPressed::AllIn; }, &currentState->toAct, &currentState->myId, 3, buttonFont); // Gold
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
        minRaiseButton.Draw();
        potRaiseButton.Draw();
        allInButton.Draw();
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
        minRaiseButton.Update();
        potRaiseButton.Update();
        allInButton.Update();
    }
}

void UiButton::reset()
{
    sendReady = false;
    raiseAmount = 0;
    buttonInteractionFlag = false;
    quick = quickBetButtonPressed::None;
}
