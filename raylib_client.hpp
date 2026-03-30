#pragma once
#include "raylib.h"
#include "client_in_client.hpp"
#include "buttons.h"
#include "cards.h"

class Game
{
public:
    Game();
    ~Game();

    void start();

private:
    void updateActionButtons();
    void update();
    void updateGameState();

    void draw();
    void drawInput();
    void drawMoneyChips();
    void shouldNewCardBeMade();
    void clearCardsIfNecessary(); // Check if cards have been cleared in the client state and clear them in the game if that's the case
    void updatePopUpMessages();
    void drawPopUpMessages();
    void onServerStateChange(GameState newState);

    PokerClient client = PokerClient();
    PokerClient::ClientState currentState;
    std::string playerName;

    Images suitTextures[4];
    Images gameImages;

    Font cardFont;
    struct VisualState
    {
        std::vector<Card> myCards;
        std::vector<Card> opponentCards;
        std::vector<Card> communityCards;

        GameState gameState = GameState::WaitingForPlayers;
        double showdownTimerStartTime = 0.0;

        void updateCards();
        void drawCards();
    };

    VisualState visualState;

    std::deque<PokerClient::popUpMessage> popUpMessages;
    bool hasEnoughTimePassed(double &lastTime, double delay);

    int raiseAmount;
    bool sendReady = false;

    bool authenticateIP(std::string ip);

    pos getPlayerCardBasePos(int id);
    int getPlayerCardRotationAngle(int id);
    std::unordered_map<int, int> holeCardsDrownCount = {};
    Button readyButton = Button(0, 550, 200, 50, "Ready", [this]()
                                {
                                if (!sendReady)
                                {
                                    client.sendReady();
                                    sendReady = true;
                                }
                                else
                                {
                                    popUpMessages.push_back(PokerClient::popUpMessage("You have already sent ready for this game."));
                                } });
    Button playAgainButton = Button(0, 610, 200, 50, "Play Again", [this]()
                                    {
                                    if (visualState.gameState == GameState::GameOver)
                                    {
                                        client.startGame();
                                    }
                                    else
                                    {
                                        popUpMessages.push_back(PokerClient::popUpMessage("You can only start a new game once the current game is over."));
                                    } });
    Button startGameButton = Button(0, 670, 200, 50, "Start Game", [this]()
                                    {
                                    if (visualState.gameState == GameState::WaitingForPlayers)
                                    {
                                        client.startGame();
                                    }
                                    else
                                    {
                                        popUpMessages.push_back(PokerClient::popUpMessage("You can only start the game when it's in the waiting for players state."));
                                    } });
    std::vector<Button> actionButtons;

    struct ChipVisual
    {
        int value;
        Color color;
    };
    static void drawSingleChip(int x, int y, int radius, Color color);
    static void drawStackOfChips(pos basePos, int amount, Color color, int rotationAngle);

    struct MoneyChips
    {
        int amount10 = 0;
        int amount50 = 0;
        int amount100 = 0;
        int amount250 = 0;

        void reset();
        void buildChips(int money);
        void drawChips(pos basePos, int rotationAngle);
    };

    unordered_map<int, MoneyChips> playerMoneyChips; // player id -> money chips

    void buildMoneyChips();
};