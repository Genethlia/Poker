#pragma once
#include "raylib.h"
#include "raylib_helpers.h"
#include "client_in_client.hpp"
#include "cards.h"
#include "uibuttons.h"
#include "chat.h"
#include "mainmenu.hpp"
#include "server.h"

enum class ScreenState
{
    MainMenu,
    InGame
};

struct seatLayout
{
    pos nameBoxPos;
    pos cardPos;
    pos betPos;  // Number bet
    pos chipPos; // Chip bet
};

class Game
{
public:
    Game();
    ~Game();

    void start();

private:
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
    void updateVisualState();
    void drawBetOfPlayer(int id);
    void clearCardsIfNecessary(); // Check if cards have been cleared in the client state and clear them in the game if that's the case
    void updatePopUpMessages();
    void drawPopUpMessages();
    void onServerStateChange(GameState newState);
    void resetForNewGame();
    void drawChat();
    void updateChat();
    void drawCodeAndSpectators();
    void drawLeaveButton();
    void updateLeaveButton();
    void drawCurrentHandText();

    PokerClient client = PokerClient();
    PokerClient::ClientState currentState;
    std::string playerName;

    Images suitTextures[4];
    Images hiddenCardImage;
    Images chatImage;

    Font cardFont;
    Font mainFont;
    Font buttonFont;

    static Font cardFontStatic;
    static Font mainFontStatic;
    static Font buttonFontStatic;

    struct VisualState
    {
        VisualState();
        std::vector<Card> myCards{};
        std::vector<std::pair<int, Card>> opponentCards{};
        std::vector<Card> communityCards{};
        std::vector<int> idToShowCardsOf{};

        GameState gameState = GameState::WaitingForPlayers;
        double showdownTimerStartTime = 0.0;

        void updateCards();
        void drawCards();
        void reset();
    };

    VisualState visualState;

    std::deque<PokerClient::popUpMessage> popUpMessages;
    Color getColorForPopUpMessageType(popUpMessageType type);
    bool hasEnoughTimePassed(double &lastTime, double delay);

    bool authenticateIP(std::string ip);

    Seat getPlayerSeat(int id);
    seatLayout getSeatLayout(int id);

    unordered_map<Seat, seatLayout> seatLayouts = {
        {Seat::Top, {{VIRTUAL_WIDTH / 2 - 180, 170}, {VIRTUAL_WIDTH / 2 - 160, 100}, {VIRTUAL_WIDTH / 2 - 20, 240}, {VIRTUAL_WIDTH / 2 + 120, 200}}},
        {Seat::Left, {{10, VIRTUAL_HEIGHT / 2 + 40}, {30, VIRTUAL_HEIGHT / 2 - 40}, {340, VIRTUAL_HEIGHT / 2 - 10}, {290, VIRTUAL_HEIGHT / 2 - 30}}},
        {Seat::Right, {{VIRTUAL_WIDTH - 230, VIRTUAL_HEIGHT / 2 + 40}, {VIRTUAL_WIDTH - 200, VIRTUAL_HEIGHT / 2 - 40}, {VIRTUAL_WIDTH - 420, VIRTUAL_HEIGHT / 2 - 10}, {VIRTUAL_WIDTH - 290, VIRTUAL_HEIGHT / 2 - 30}}},
        {Seat::Bottom, {{VIRTUAL_WIDTH / 2 - 50, VIRTUAL_HEIGHT - 170}, {VIRTUAL_WIDTH / 2 - 150, VIRTUAL_HEIGHT - 250}, {VIRTUAL_WIDTH / 2 - 20, VIRTUAL_HEIGHT - 250}, {VIRTUAL_WIDTH / 2 + 50, VIRTUAL_HEIGHT - 200}}}};

    int getPlayerCardRotationAngle(int id);
    std::unordered_map<int, int> DrawnCount = {};

    struct ChipVisual
    {
        int value;
        Color color;
    };
    static void drawSingleChip(int x, int y, int radius, Color color, bool isLastChip = false, int rotationAngle = 0);
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

    RenderTexture2D target{};
    std::vector<std::vector<Card>> allCards = {}; // for testing

    UiButton uiButton;
    Chat chat;

    ScreenState screenState = ScreenState::MainMenu;
    MainMenu mainMenu;
    void tryJoin();
    void tryHost();

    Server server;
    thread serverThread;

    std::string roomCodeToIP(std::string roomCode);

    bool isHosting = false;

    void leaveToMainMenu();
    void stopHostedServerIfNeeded();

    Rectangle leaveButton;
};