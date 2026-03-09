#include "raylib_client.hpp"
using namespace std;

Game::Game()
{
    raiseAmount = 0;
}

Game::~Game()
{
    client.stop();
    CloseWindow();
}

void Game::start()
{
    std::cout << "Enter your name: ";
    std::getline(std::cin, playerName);

    InitWindow(1000, 700, "Poker Game");
    SetTargetFPS(60);

    client.connect_to("127.0.0.1", "6767");
    client.join_us(playerName);
    client.start();

    while (!WindowShouldClose())
    {
        input();
        update();

        BeginDrawing();
        ClearBackground(DARKGREEN);
        draw();
        EndDrawing();
    }
    CloseWindow();
}

void Game::input()
{
    if (IsKeyPressed(KEY_P))
        client.startGame();

    if (IsKeyPressed(KEY_R))
        client.sendReady();

    if (IsKeyPressed(KEY_Q))
        client.sendAction(PlayerActionType::Fold);

    if (IsKeyPressed(KEY_C))
        client.sendAction(PlayerActionType::Call);

    if (IsKeyPressed(KEY_K))
        client.sendAction(PlayerActionType::Check);

    if (IsKeyPressed(KEY_ONE))
        raiseAmount += 50;

    if (IsKeyPressed(KEY_TWO))
        raiseAmount += 100;

    if (IsKeyPressed(KEY_BACKSPACE))
        raiseAmount = std::max(0, raiseAmount - 50);

    if (IsKeyPressed(KEY_ENTER))
        client.sendAction(PlayerActionType::Raise, raiseAmount);
}

void Game::update()
{
    currentState = client.getClientStateCopy();
}

void Game::draw()
{
    DrawText(TextFormat("My ID: %d", currentState.myId), 20, 20, 24, WHITE);
    DrawText(TextFormat("Pot: %d", currentState.potAmount), 20, 60, 24, WHITE);
    DrawText(TextFormat("To Call: %d", currentState.toCall), 20, 100, 24, WHITE);
    DrawText(TextFormat("Current Bet: %d", currentState.currentBet), 20, 140, 24, WHITE);
    DrawText(TextFormat("Min Raise: %d", currentState.minRaise), 20, 180, 24, WHITE);
    DrawText(TextFormat("To Act: %d", currentState.toAct), 20, 220, 24, WHITE);

    DrawText("R = Ready", 20, 300, 20, YELLOW);
    DrawText("Q = Fold", 20, 330, 20, YELLOW);
    DrawText("K = Check", 20, 360, 20, YELLOW);
    DrawText("C = Call", 20, 390, 20, YELLOW);
    DrawText("1/2 = Increase Raise", 20, 420, 20, YELLOW);
    DrawText("ENTER = Raise", 20, 450, 20, YELLOW);

    DrawText(TextFormat("Raise Amount: %d", raiseAmount), 20, 500, 24, ORANGE);
}
