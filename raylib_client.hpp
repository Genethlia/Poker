#pragma once
#include "raylib.h"
#include "client_in_client.hpp"
#include "cards.h"
#include "uibuttons.h"
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
        std::vector<int> idToShowCardsOf;

        GameState gameState = GameState::WaitingForPlayers;
        double showdownTimerStartTime = 0.0;

        void updateCards();
        void drawCards();
    };

    VisualState visualState;

    std::deque<PokerClient::popUpMessage> popUpMessages;
    Color getColorForPopUpMessageType(popUpMessageType type);
    bool hasEnoughTimePassed(double &lastTime, double delay);

    bool authenticateIP(std::string ip);

    Seat getPlayerSeat(int id);
    pos getPlayerPosition(int id);
    pos getPlayerCardPosition(int id);
    pos getPlayerBetPosition(int id);
    unordered_map<Seat, pos> seatPositions = {
        {Seat::Top, {VIRTUAL_WIDTH / 2 + 120, 200}},
        {Seat::Left, {290, VIRTUAL_HEIGHT / 2 - 40}},
        {Seat::Right, {VIRTUAL_WIDTH - 330, VIRTUAL_HEIGHT / 2 - 70}},
        {Seat::Bottom, {VIRTUAL_WIDTH / 2 + 50, VIRTUAL_HEIGHT - 200}}};
    unordered_map<Seat, pos> seatCardPositions = {
        {Seat::Top, {VIRTUAL_WIDTH / 2 - 160, 100}},
        {Seat::Left, {30, VIRTUAL_HEIGHT / 2 - 110}},
        {Seat::Right, {VIRTUAL_WIDTH - 200, VIRTUAL_HEIGHT / 2 - 40}},
        {Seat::Bottom, {VIRTUAL_WIDTH / 2 - 150, VIRTUAL_HEIGHT - 250}}};
    unordered_map<Seat, pos> seatBetPositions = {
        {Seat::Top, {VIRTUAL_WIDTH / 2 - 20, 240}},
        {Seat::Left, {340, VIRTUAL_HEIGHT / 2 - 10}},
        {Seat::Right, {VIRTUAL_WIDTH - 290, VIRTUAL_HEIGHT / 2 - 10}},
        {Seat::Bottom, {VIRTUAL_WIDTH / 2 - 20, VIRTUAL_HEIGHT - 250}}};

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

    RenderTexture2D target;
    std::vector<std::vector<Card>> allCards = {}; // for testing

    unordered_map<string, Color> all_Colors = {
        {"woodColor", {60, 30, 10, 255}},
        {"tableRed", {200, 33, 42, 255}},
        {"background", {82, 36, 0, 255}}};

    UiButton uiButton;
};