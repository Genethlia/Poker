#include "raylib_client.hpp"
#include "assets.h"
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

    InitWindow(1600, 900, "Poker Game");
    SetTargetFPS(60);

    client.connect_to("127.0.0.1", "6767");
    client.join_us(playerName);
    client.start();

    for (int i = 0; i < 4; i++)
        suitTextures[i].LoadSuit(i);
    gameImages.LoadMatHiddenCardAndHome();
    cardFont = LoadFontFromMemory(".ttf", cardfont_ttf, cardfont_ttf_len, 32, nullptr, 0);

    while (!WindowShouldClose() && client.running)
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
    {
        client.sendAction(PlayerActionType::Raise, raiseAmount);
        raiseAmount = 0;
    }
}

void Game::update()
{
    currentState = client.getClientStateCopy();
    clearCardsIfNecessary();
    shouldNewCardBeMade();

    switch (currentState.gameState)
    {
    case GameState::WaitingForPlayers:
        break;
    case GameState::PreFlop:
    case GameState::Flop:
    case GameState::Turn:
    case GameState::River:
    case GameState::Showdown:
        for (auto &card : myCards)
        {
            card.Update();
        }
        for (auto &card : opponentCards)
        {
            card.Update();
        }
        for (auto &card : communityCards)
        {
            card.Update();
        }
        break;
    default:
        break;
    }
}

void Game::draw()
{
    DrawTexture(gameImages.matTexture, 0, 0, WHITE);
    DrawText(TextFormat("My ID: %d", currentState.myId), 20, 20, 24, WHITE);
    DrawText(TextFormat("Pot: %d", currentState.potAmount), 20, 60, 24, WHITE);
    DrawText(TextFormat("To Call: %d", currentState.toCall), 20, 100, 24, WHITE);
    DrawText(TextFormat("Current Bet: %d", currentState.currentBet), 20, 140, 24, WHITE);
    DrawText(TextFormat("Min Raise: %d", currentState.minRaise), 20, 180, 24, WHITE);
    DrawText(TextFormat("To Act: %d", currentState.toAct), 20, 220, 24, WHITE);
    DrawText(TextFormat("Money: %d", currentState.playerMoney[currentState.myId]), 20, 260, 24, WHITE);

    for (auto &card : myCards)
    {
        card.Draw();
    }
    for (auto &card : opponentCards)
    {
        card.Draw();
    }
    for (auto &card : communityCards)
    {
        card.Draw();
    }

    DrawText("R = Ready", 20, 300, 20, YELLOW);
    DrawText("Q = Fold", 20, 330, 20, YELLOW);
    DrawText("K = Check", 20, 360, 20, YELLOW);
    DrawText("C = Call", 20, 390, 20, YELLOW);
    DrawText("1/2 = Increase Raise", 20, 420, 20, YELLOW);
    DrawText("ENTER = Raise", 20, 450, 20, YELLOW);

    DrawText(TextFormat("Raise Amount: %d", raiseAmount), 20, 500, 24, ORANGE);
}

void Game::shouldNewCardBeMade()
{
    while (currentState.myCards.size() > myCards.size())
    {
        valRank cardInfo = currentState.myCards[myCards.size()];
        holeCardsDrownCount[currentState.myId]++;
        myCards.emplace_back(getPlayerCardBasePos(currentState.myId).x + holeCardsDrownCount[currentState.myId] * 100, getPlayerCardBasePos(currentState.myId).y, cardInfo, &suitTextures[cardInfo.suit], &cardFont, &gameImages);
    }
    while (currentState.opponentCards.size() > opponentCards.size())
    {
        auto &[id, cardInfo] = currentState.opponentCards[opponentCards.size()];
        holeCardsDrownCount[id]++;
        opponentCards.emplace_back(getPlayerCardBasePos(id).x + holeCardsDrownCount[id] * 100, getPlayerCardBasePos(id).y, cardInfo, &suitTextures[cardInfo.suit], &cardFont, &gameImages);
    }
    while (currentState.communityCards.size() > communityCards.size())
    {
        valRank cardInfo = currentState.communityCards[communityCards.size()];
        communityCards.emplace_back(350 + communityCards.size() * 150, 20, cardInfo, &suitTextures[cardInfo.suit], &cardFont, &gameImages);
    }
}

void Game::clearCardsIfNecessary()
{
    if (myCards.size() > currentState.myCards.size() || opponentCards.size() > currentState.opponentCards.size() || communityCards.size() > currentState.communityCards.size())
    {
        myCards.clear();
        opponentCards.clear();
        communityCards.clear();
        holeCardsDrownCount.clear();
    }
}

pos Game::getPlayerCardBasePos(int id)
{
    return currentState.PlayerPosition.at(id);
}
