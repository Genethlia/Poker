#pragma once
#include "raylib.h"
#include "client_in_client.hpp"
#include "buttons.h"
#include "cards.h"
static constexpr int VIRTUAL_WIDTH = 1600;
static constexpr int VIRTUAL_HEIGHT = 900;
static pos center = {VIRTUAL_WIDTH / 2, VIRTUAL_HEIGHT / 2};

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
    void updateFullScreen();
    void updateDimensions();
    void drawShades();
    void drawPlayers();
    void drawSinglePlayer(int id);
    void draw();
    void drawInput();
    void drawBackground();
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
    static Font cardFontStatic; // Static variable to hold the card font for use in static functions
    struct VisualState
    {
        std::vector<Card> myCards;
        std::vector<std::pair<int, Card>> opponentCards;
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
    std::unordered_map<int, int> DrawnCount = {};
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
    static void drawSingleChip(int x, int y, int radius, Color color, bool isLastChip = false);
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
    string getPlayerName(int id);

    RenderTexture2D target;
    std::vector<std::vector<Card>> allCards = {}; // for testing

    unordered_map<string, Color> all_Colors = {
        {"woodColor", {60, 30, 10, 255}},
        {"tableRed", {200, 33, 42, 255}},
        {"background", {82, 36, 0, 255}}};
};