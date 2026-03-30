#include "raylib_client.hpp"
#include "assets.h"
using namespace std;

Game::Game()
{
    raiseAmount = 0;
    actionButtons.push_back(Button(0, 610, 200, 50, "Fold", [this]()
                                   { client.sendAction(PlayerActionType::Fold); }));
    actionButtons.push_back(Button(0, 670, 200, 50, "Check", [this]()
                                   { client.sendAction(PlayerActionType::Check); }));
    actionButtons.push_back(Button(0, 730, 200, 50, "Call", [this]()
                                   { client.sendAction(PlayerActionType::Call); }));
    actionButtons.push_back(Button(0, 790, 200, 50, "Raise", [this]()
                                   { client.sendAction(PlayerActionType::Raise, raiseAmount); }));
    actionButtons.push_back(Button(0, 550, 200, 50, "Raise +50", [this]()
                                   { raiseAmount += 50; }));
    actionButtons.push_back(Button(0, 490, 200, 50, "Raise +100", [this]()
                                   { raiseAmount += 100; }));
    actionButtons.push_back(Button(0, 430, 200, 50, "Remove 50 from Raise", [this]()
                                   { raiseAmount = std::max(0, raiseAmount - 50); }));
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

    std::cout << "Enter server IP (default: 127.0.0.1): ";
    std::string serverIP;
    std::getline(std::cin, serverIP);
    if (serverIP.empty() || !authenticateIP(serverIP))
    {
        serverIP = "127.0.0.1";
    }

    InitWindow(1800, 900, "Poker Game");
    SetTargetFPS(60);

    client.connect_to(serverIP, "6767");
    client.join_us(playerName);
    client.start();

    for (int i = 0; i < 4; i++)
        suitTextures[i].LoadSuit(i);
    gameImages.LoadMatHiddenCard();
    cardFont = LoadFontFromMemory(".ttf", cardfont_ttf, cardfont_ttf_len, 32, nullptr, 0);

    while (!WindowShouldClose() && client.running)
    {

        update();

        BeginDrawing();
        ClearBackground(DARKGREEN);

        draw();

        EndDrawing();
    }
    CloseWindow();
}

void Game::updateActionButtons()
{
    for (auto &button : actionButtons)
    {
        button.Update();
    }
}

void Game::update()
{
    clearCardsIfNecessary();
    shouldNewCardBeMade();
    updatePopUpMessages();
    updateGameState();
    if (!sendReady || visualState.gameState == GameState::WaitingForPlayers)
    {
        readyButton.Update();
    }
    if (sendReady && visualState.gameState == GameState::WaitingForPlayers)
    {
        startGameButton.Update();
    }
    else if (visualState.gameState == GameState::GameOver)
    {
        playAgainButton.Update();
    }
    if (visualState.gameState != GameState::WaitingForPlayers && visualState.gameState != GameState::GameOver)
    {
        updateActionButtons();
        visualState.updateCards();
    }
}

void Game::updateGameState()
{
    auto newState = client.getClientStateCopy();
    if (newState.gameState != currentState.gameState)
    {
        onServerStateChange(newState.gameState);
    }
    buildMoneyChips();
    currentState = newState;
    if (visualState.gameState == GameState::Showdown)
    {
        if (hasEnoughTimePassed(visualState.showdownTimerStartTime, 5.0))
        {
            visualState.gameState = GameState::GameOver;
        }
    }
}

void Game::VisualState::updateCards()
{
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

    visualState.drawCards();

    DrawText(TextFormat("Raise Amount: %d", raiseAmount), 20, 500, 24, ORANGE);

    drawInput();
    drawPopUpMessages();
    drawMoneyChips();
    DrawRectangleLines(0, 0, 1800, 1010, BLACK);
    DrawLine(200, 0, 200, 1010, BLACK);
}

void Game::drawInput()
{
    if (!sendReady || visualState.gameState == GameState::WaitingForPlayers)
    {
        readyButton.Draw();
    }
    if (visualState.gameState == GameState::WaitingForPlayers)
    {
        startGameButton.Draw();
        DrawText("Waiting for players...", 800, 555, 24, YELLOW);
    }
    else if (visualState.gameState == GameState::GameOver)
    {
        playAgainButton.Draw();
        DrawText("Game Over!", 800, 555, 24, YELLOW);
    }
    else
    {
        for (auto &button : actionButtons)
        {
            button.Draw();
        }
    }
}

void Game::drawMoneyChips()
{
    for (auto &[id, chips] : playerMoneyChips)
    {
        if (!currentState.isSeated[id] || currentState.isSpectator[id])
            continue; // Only draw chips for seated players who are not spectators
        pos basePos = getPlayerCardBasePos(id);
        int rotationAngle = getPlayerCardRotationAngle(id);
        chips.drawChips({basePos.x, basePos.y}, rotationAngle);
    }
    DrawText(TextFormat("chip entries: %i", (int)playerMoneyChips.size()), 700, 20, 20, WHITE);
    DrawText(TextFormat("players: %d", (int)currentState.playerMoney.size()), 700, 330, 20, WHITE);
    DrawText(TextFormat("IsPlayerSeated entries: %d", (int)currentState.isSeated.size()), 700, 360, 20, WHITE);
    DrawText(TextFormat("IsPlayerSpectator entries: %d", (int)currentState.isSpectator.size()), 700, 390, 20, WHITE);
}

void Game::VisualState::drawCards()
{
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
}

void Game::shouldNewCardBeMade()
{
    while (currentState.myCards.size() > visualState.myCards.size())
    {
        valRank cardInfo = currentState.myCards[visualState.myCards.size()];
        visualState.myCards.emplace_back(getPlayerCardBasePos(currentState.myId).x + holeCardsDrownCount[currentState.myId] * 120, getPlayerCardBasePos(currentState.myId).y, cardInfo, &suitTextures[cardInfo.suit], &cardFont, &gameImages);
        holeCardsDrownCount[currentState.myId]++;
    }
    while (currentState.opponentCards.size() > visualState.opponentCards.size())
    {
        auto &[id, cardInfo] = currentState.opponentCards[visualState.opponentCards.size()];
        pos basePos = getPlayerCardBasePos(id);
        int rotationAngle = getPlayerCardRotationAngle(id);
        int offsetX = (rotationAngle == 0) ? holeCardsDrownCount[id] * 120 : 0;
        int offsetY = (rotationAngle == 0) ? 0 : holeCardsDrownCount[id] * 120;
        visualState.opponentCards.emplace_back(basePos.x + offsetX, basePos.y + offsetY, cardInfo, &suitTextures[cardInfo.suit], &cardFont, &gameImages, rotationAngle);
        holeCardsDrownCount[id]++;
    }
    while (currentState.communityCards.size() > visualState.communityCards.size())
    {
        valRank cardInfo = currentState.communityCards[visualState.communityCards.size()];
        visualState.communityCards.emplace_back(446 + visualState.communityCards.size() * 150, 360, cardInfo, &suitTextures[cardInfo.suit], &cardFont, &gameImages);
    }
}

void Game::clearCardsIfNecessary()
{
    if (visualState.myCards.size() > currentState.myCards.size() || visualState.opponentCards.size() > currentState.opponentCards.size() || visualState.communityCards.size() > currentState.communityCards.size())
    {
        visualState.myCards.clear();
        visualState.opponentCards.clear();
        visualState.communityCards.clear();
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

void Game::onServerStateChange(GameState newState)
{
    visualState.gameState = newState;
    if (newState == GameState::Showdown)
    {
        visualState.showdownTimerStartTime = GetTime();
    }
}

bool Game::hasEnoughTimePassed(double &lastTime, double delay)
{
    return (GetTime() - lastTime) >= delay;
}

bool Game::authenticateIP(std::string ip)
{
    int dotCount = std::count(ip.begin(), ip.end(), '.');
    if (dotCount != 3)
        return false;
    int num1, num2, num3, num4;
    num1 = stoi(ip.substr(0, ip.find('.')));
    size_t pos1 = ip.find('.') + 1;
    num2 = stoi(ip.substr(pos1, ip.find('.', pos1) - pos1));
    size_t pos2 = ip.find('.', pos1) + 1;
    num3 = stoi(ip.substr(pos2, ip.find('.', pos2) - pos2));
    size_t pos3 = ip.find('.', pos2) + 1;
    num4 = stoi(ip.substr(pos3));
    if (num1 < 0 || num1 > 255 || num2 < 0 || num2 > 255 || num3 < 0 || num3 > 255 || num4 < 0 || num4 > 255)
        return false;
    return true;
}

pos Game::getPlayerCardBasePos(int id)
{
    return currentState.PlayerPosition.at(id).first;
}

int Game::getPlayerCardRotationAngle(int id)
{
    return currentState.PlayerPosition.at(id).second;
}

void Game::drawSingleChip(int x, int y, int radius, Color color)
{
    DrawCircle(x, y, radius, color);
    DrawCircleLines(x, y, radius, BLACK);
    DrawCircleLines(x, y, radius - 4, BLACK);
}

void Game::drawStackOfChips(pos basePos, int amount, Color color, int rotationAngle)
{
    for (int i = 0; i < amount; i++)
    {
        drawSingleChip(basePos.x, basePos.y, 20, color);
        if (rotationAngle == 90)
            basePos.x -= 5;
        else if (rotationAngle == 270)
            basePos.x += 5;
        else if (rotationAngle == 0)
            basePos.y -= 5;
        else
            basePos.y += 5;
    }
}

void Game::buildMoneyChips()
{
    // playerMoneyChips.clear();
    // for (auto &[id, money] : currentState.playerMoney)
    // {
    //     if (!currentState.isSeated[id] || currentState.isSpectator[id])
    //         continue; // Only build chips for players that are still in the hand
    //     MoneyChips chips;
    //     chips.buildChips(money);
    //     playerMoneyChips[id] = chips;
    // }
    playerMoneyChips.clear();

    for (auto &[id, money] : currentState.playerMoney)
    {
        playerMoneyChips[id].buildChips(money);
    }
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

void Game::MoneyChips::reset()
{
    amount10 = 0;
    amount50 = 0;
    amount100 = 0;
    amount250 = 0;
}

void Game::MoneyChips::buildChips(int money)
{
    reset();

    int stackSize = min(5, max(1, money / 200));

    amount250 = stackSize;
    amount100 = stackSize;
    amount50 = stackSize;
    amount10 = stackSize;
}

void Game::MoneyChips::drawChips(pos basePos, int rotationAngle)
{
    vector<ChipVisual> stacks;
    if (amount250 > 0)
        stacks.push_back({amount250, RED});
    if (amount100 > 0)
        stacks.push_back({amount100, BLUE});
    if (amount50 > 0)
        stacks.push_back({amount50, GREEN});
    if (amount10 > 0)
        stacks.push_back({amount10, YELLOW});
    if (stacks.empty())
        return;
    int spacing = 50;
    int totalWidth = (stacks.size() - 1) * spacing;

    for (int i = 0; i < stacks.size(); i++)
    {
        pos stackPos = {basePos.x, basePos.y};
        if (rotationAngle == 0 || rotationAngle == 180)
            stackPos.x += i * spacing - totalWidth / 2;
        else
            stackPos.y += i * spacing - totalWidth / 2;

        drawStackOfChips(stackPos, stacks[i].value, stacks[i].color, rotationAngle);
    }
}