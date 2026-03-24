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
    gameImages.LoadMatHiddenCard();
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
    updatePopUpMessages();
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

    drawPopUpMessages();
}

void Game::shouldNewCardBeMade()
{
    while (currentState.myCards.size() > myCards.size())
    {
        valRank cardInfo = currentState.myCards[myCards.size()];
        myCards.emplace_back(getPlayerCardBasePos(currentState.myId).x + holeCardsDrownCount[currentState.myId] * 120, getPlayerCardBasePos(currentState.myId).y, cardInfo, &suitTextures[cardInfo.suit], &cardFont, &gameImages);
        holeCardsDrownCount[currentState.myId]++;
    }
    while (currentState.opponentCards.size() > opponentCards.size())
    {
        auto &[id, cardInfo] = currentState.opponentCards[opponentCards.size()];
        pos basePos = getPlayerCardBasePos(id);
        int rotationAngle = getPlayerCardRotationAngle(id);
        int offsetX = (rotationAngle == 0) ? holeCardsDrownCount[id] * 120 : 0;
        int offsetY = (rotationAngle == 0) ? 0 : holeCardsDrownCount[id] * 120;
        opponentCards.emplace_back(basePos.x + offsetX, basePos.y + offsetY, cardInfo, &suitTextures[cardInfo.suit], &cardFont, &gameImages, rotationAngle);
        holeCardsDrownCount[id]++;
    }
    while (currentState.communityCards.size() > communityCards.size())
    {
        valRank cardInfo = currentState.communityCards[communityCards.size()];
        communityCards.emplace_back(446 + communityCards.size() * 150, 360, cardInfo, &suitTextures[cardInfo.suit], &cardFont, &gameImages);
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

void Game::drawPopUpMessages()
{
    int yOffset = 0;
    for (auto &message : popUpMessages)
    {
        if (message.text.length() >= 30)
        {
            size_t splitIndex = message.text.find(' ', message.text.length() / 2);
            DrawText(message.text.substr(0, splitIndex).c_str(), 400, 20 + yOffset, 64, RED);
            yOffset += 30;
            DrawText(message.text.substr(splitIndex).c_str(), 400, 20 + yOffset, 64, RED);
        }
        else
        {
            DrawText(message.text.c_str(), 400, 20 + yOffset, 20, LIGHTGRAY);
        }
        yOffset += 30;
    }
}

bool Game::hasEnoughTimePassed(double &lastTime, double delay)
{
    return (GetTime() - lastTime) >= delay;
}

pos Game::getPlayerCardBasePos(int id)
{
    return currentState.PlayerPosition.at(id).first;
}

int Game::getPlayerCardRotationAngle(int id)
{
    return currentState.PlayerPosition.at(id).second;
}

void Game::updatePopUpMessages()
{
    auto newMessages = client.getAndClearPopUpMessages();
    for (auto &message : newMessages)
    {
        popUpMessages.push_back(PokerClient::popUpMessage(message));
    }
    for (auto it = popUpMessages.begin(); it != popUpMessages.end();)
    {
        if (hasEnoughTimePassed(it->timer, 3.0))
        {
            it = popUpMessages.erase(it);
        }
        else
        {
            ++it;
        }
    }
}
