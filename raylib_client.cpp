#include "raylib_client.hpp"
#include "assets.h"
using namespace std;

Font Game::cardFontStatic{}; // Definition of the static card font variable

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
                                   { client.sendAction(PlayerActionType::Raise, raiseAmount);
                                raiseAmount = 0; }));
    actionButtons.push_back(Button(0, 550, 200, 50, "Raise +50", [this]()
                                   { raiseAmount += 50; }));
    actionButtons.push_back(Button(0, 490, 200, 50, "Raise +100", [this]()
                                   { raiseAmount += 100; }));
    actionButtons.push_back(Button(0, 430, 200, 50, "Raise -50", [this]()
                                   { raiseAmount = std::max(0, raiseAmount - 50); }));
}

Game::~Game()
{
    client.stop();
    UnloadRenderTexture(target);
}

void Game::start()
{
    std::cout << "Enter your name: ";
    std::getline(std::cin, playerName);
    while (playerName.empty())
    {
        std::cout << "Name cannot be empty. Please enter your name: ";
        std::getline(std::cin, playerName);
    }

    std::cout << "Enter server IP (default: 127.0.0.1): ";
    std::string serverIP;
    std::getline(std::cin, serverIP);
    if (serverIP.empty() || !authenticateIP(serverIP))
    {
        serverIP = "127.0.0.1";
    }

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(VIRTUAL_WIDTH, VIRTUAL_HEIGHT, "Poker Game");
    SetTargetFPS(60);

    target = LoadRenderTexture(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);

    // ToggleFullscreen();

    client.connect_to(serverIP, "6767");
    client.join_us(playerName);
    client.start();

    for (int i = 0; i < 4; i++)
        suitTextures[i].LoadSuit(i);
    gameImages.LoadMatHiddenCard();
    cardFont = LoadFontFromMemory(".ttf", cardfont_ttf, cardfont_ttf_len, 32, nullptr, 0);
    Game::cardFontStatic = cardFont; // Initialize the static card font variable

    /*
    for (int i = 2; i <= 14; i++)
    {
        vector<Card> suitCards;
        for (int j = 0; j < 4; j++)
        {
            Card tempCard(200 + i * 75, 100 + j * 150, {i, j}, &suitTextures[j], &cardFont, &gameImages);
            suitCards.push_back(tempCard);
        }
        allCards.push_back(suitCards);
    }
    */

    while (!WindowShouldClose() && client.running)
    {

        update();

        BeginTextureMode(target);
        ClearBackground(all_Colors["background"]);
        draw();
        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);

        float scale = std::min(
            (float)GetScreenWidth() / VIRTUAL_WIDTH,
            (float)GetScreenHeight() / VIRTUAL_HEIGHT);

        float destWidth = VIRTUAL_WIDTH * scale;
        float destHeight = VIRTUAL_HEIGHT * scale;
        float offsetX = (GetScreenWidth() - destWidth) / 2.0f;
        float offsetY = (GetScreenHeight() - destHeight) / 2.0f;

        Rectangle source = {0, 0, (float)VIRTUAL_WIDTH, -(float)VIRTUAL_HEIGHT};
        Rectangle dest = {offsetX, offsetY, destWidth, destHeight};

        DrawTexturePro(target.texture, source, dest, {0, 0}, 0.0f, WHITE);

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

    /*
    for (int i = 0; i <= 12; i++)
    {
        for (int j = 0; j < 4; j++)
        {

            allCards[i][j].Update();
        }
    }
    */

    clearCardsIfNecessary();
    shouldNewCardBeMade();
    updatePopUpMessages();
    updateGameState();
    updateFullScreen();
    updateDimensions();
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

void Game::updateFullScreen()
{
    if (IsKeyPressed(KEY_F11) || IsKeyPressed(KEY_KP_ENTER) && IsKeyDown(KEY_LEFT_ALT))
    {
        ToggleFullscreen();
    }
}

void Game::updateDimensions()
{
    if (IsWindowResized() && !IsWindowFullscreen())
    {
        int screenWidth = GetScreenWidth();
        int screenHeight = min(GetMonitorHeight(GetCurrentMonitor()) + 0.0f, screenWidth * (9 / 16.0f));
        screenWidth = screenHeight * (16 / 9.0f);
        if (GetScreenHeight() != screenHeight)
            SetWindowSize(screenWidth, screenHeight);
    }
}

void Game::drawShades()
{
    int shadeWidth = VIRTUAL_WIDTH / 200; // 1800
    for (int i = 0; i < VIRTUAL_WIDTH; i += shadeWidth)
    {
        DrawRectangle(i, 0, shadeWidth, VIRTUAL_HEIGHT, Fade(WHITE, i / float(VIRTUAL_WIDTH) / 5.0f));
    }
}

void Game::drawPlayers()
{
    for (auto &[id, name] : currentState.playerNames)
    {
        if (currentState.isSpectator[id])
            continue;
        if (!currentState.isSeated[id])
            continue;
        if (!currentState.PlayerPosition.count(id))
            continue;
        drawSinglePlayer(id);
    }
}

void Game::drawSinglePlayer(int id)
{
    int rotationAngle = getPlayerCardRotationAngle(id);

    std::string name = getPlayerName(id);
    int money = currentState.playerMoney.count(id) ? currentState.playerMoney[id] : 0;

    pos boxPos = getPlayerPosition(id);
    Seat seat = getPlayerSeat(id);

    int offsetX = 0, offsetY = 0;

    switch (seat)
    {
    case Seat::Top:
        offsetX = -300;
        offsetY = -30;
        break;
    case Seat::Left:
        offsetX = -280;
        break;
    case Seat::Right:
        offsetX = 100;
        offsetY = 110;
        break;
    case Seat::Bottom:
        offsetX = -100;
        offsetY = 30;
        break;
    }

    Color bgColor = BLACK;

    if (id == currentState.toAct)
        bgColor = SKYBLUE;

    DrawRectangleRounded(
        {boxPos.x + offsetX, boxPos.y + offsetY, 200, 65},
        0.25f,
        8,
        bgColor);

    offsetX += 10;
    DrawText(name.c_str(), boxPos.x + offsetX, boxPos.y + offsetY, 22, WHITE);
    DrawText(TextFormat("$%d", money), boxPos.x + offsetX, boxPos.y + offsetY + 28, 20, GOLD);

    if (id == currentState.myId)
    {
        DrawText("YOU", boxPos.x + offsetX + 150, boxPos.y + offsetY, 18, DARKBLUE);
        for (auto &card : visualState.myCards)
        {
            card.Draw();
        }
    }
    bgColor = (bgColor.r == SKYBLUE.r) ? BLACK : WHITE;
    if (id == currentState.dealerId)
    {
        DrawText("D", boxPos.x + offsetX + 140, boxPos.y + offsetY + 40, 18, bgColor);
    }
    if (id == currentState.smallBlindId)
    {
        DrawText("SB", boxPos.x + offsetX + 160, boxPos.y + offsetY + 40, 18, bgColor);
    }
    if (id == currentState.bigBlindId)
    {
        DrawText("BB", boxPos.x + offsetX + 160, boxPos.y + offsetY + 40, 18, bgColor);
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
        card.second.Update();
    }
    for (auto &card : communityCards)
    {
        card.Update();
    }
}

void Game::draw()
{
    drawBackground();

    visualState.drawCards();
    drawPlayers();
    drawMoneyChips();

    drawInput();
    drawPopUpMessages();
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
        string waitingText = "Waiting for players...";
        int textWidth = MeasureText(waitingText.c_str(), 24);
        DrawText(waitingText.c_str(), VIRTUAL_WIDTH / 2 - textWidth / 2, VIRTUAL_HEIGHT / 2, 24, YELLOW);
    }
    else if (visualState.gameState == GameState::GameOver)
    {
        playAgainButton.Draw();
        string gameOverText = "Game Over!";
        int textWidth = MeasureText(gameOverText.c_str(), 24);
        DrawText(gameOverText.c_str(), VIRTUAL_WIDTH / 2 - textWidth / 2, VIRTUAL_HEIGHT / 2, 24, YELLOW);
    }
    else
    {
        for (auto &button : actionButtons)
        {
            button.Draw();
        }
    }
    /*
   for (int i = 0; i <= 12; i++)
   {
       for (int j = 0; j < 4; j++)
       {

           allCards[i][j].Draw();
       }
   }
    */
}

void Game::drawBackground()
{
    float rx = VIRTUAL_WIDTH * 0.75;
    float ry = VIRTUAL_HEIGHT * 0.75;
    DrawRectangleRounded({(VIRTUAL_WIDTH - rx) / 2, (VIRTUAL_HEIGHT - ry) / 2, rx, ry}, 1.0f, 16, all_Colors["woodColor"]);
    DrawRectangleRounded({(VIRTUAL_WIDTH - rx + 80) / 2, (VIRTUAL_HEIGHT - ry + 80) / 2, rx - 80, ry - 80}, 1.0f, 16, all_Colors["tableRed"]);
    drawShades();
}

void Game::drawMoneyChips()
{
    for (auto &[id, chips] : playerMoneyChips)
    {
        if (!currentState.isSeated[id] || currentState.isSpectator[id])
            continue; // Only draw chips for seated players who are not spectators
        pos basePos = getPlayerPosition(id);
        int rotationAngle = getPlayerCardRotationAngle(id);
        chips.drawChips(basePos, rotationAngle);
    }
}

void Game::VisualState::drawCards()
{
    // for (auto &card : myCards)
    // {
    //     card.Draw();
    // }
    for (auto &card : opponentCards)
    {
        card.second.Draw();
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
        visualState.myCards.emplace_back(getPlayerCardPosition(currentState.myId).x + DrawnCount[currentState.myId] * 25, getPlayerCardPosition(currentState.myId).y + DrawnCount[currentState.myId] * 5, cardInfo, &suitTextures[cardInfo.suit], &cardFont, &gameImages);
        DrawnCount[currentState.myId]++;
    }
    while (currentState.opponentCards.size() > visualState.opponentCards.size())
    {
        auto &[id, cardInfo] = currentState.opponentCards[visualState.opponentCards.size()];
        pos basePos = getPlayerCardPosition(id);
        int rotationAngle = 0; // Show them normally
        int offsetX = DrawnCount[id] * 80;
        visualState.opponentCards.emplace_back(id, Card(basePos.x + offsetX, basePos.y, cardInfo, &suitTextures[cardInfo.suit], &cardFont, &gameImages));
        DrawnCount[id]++;
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
        DrawnCount.clear();
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
            int textWidth1 = MeasureText(message.text.substr(0, splitIndex).c_str(), 20);
            int textWidth2 = MeasureText(message.text.substr(splitIndex).c_str(), 20);
            int textWidth = max(textWidth1, textWidth2); // find biggest width for the background rectangle
            DrawRectangleRounded({400, 20.0f + yOffset, (float)textWidth + 20, 59}, 0.5f, 4, Fade(BLACK, 0.5f));
            DrawText(message.text.substr(0, splitIndex).c_str(), 410, 20 + yOffset, 20, LIGHTGRAY);
            yOffset += 30;
            DrawText(message.text.substr(splitIndex).c_str(), 410, 20 + yOffset, 20, LIGHTGRAY);
        }
        else
        {
            int textWidth = MeasureText(message.text.c_str(), 20);
            DrawRectangleRounded({400, 20.0f + yOffset, (float)textWidth + 20, 29}, 0.5f, 4, Fade(BLACK, 0.5f));
            DrawText(message.text.c_str(), 410, 20 + yOffset, 20, LIGHTGRAY);
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
    if (dotCount != 3 || ip.size() < 7 || ip.size() > 15)
        return false;
    if (ip.find_first_not_of("0123456789.") != std::string::npos)
        return false;

    std::stringstream ss(ip);
    std::string segment;
    for (int i = 0; i < 4; i++)
    {
        if (!std::getline(ss, segment, '.'))
            return false;
        if (segment.empty())
        {
            return false;
        }
        int num = stoi(segment);
        if (num < 0 || num > 255)
        {
            return false;
        }
    }
    return true;
}

Seat Game::getPlayerSeat(int id)
{
    auto it = currentState.PlayerPosition.find(id);
    if (it == currentState.PlayerPosition.end())
    {
        std::cout << "ERROR: Missing PlayerPosition for id: " << id << std::endl;
        return Seat::Unassigned;
    }
    return it->second.first;
}

pos Game::getPlayerPosition(int id)
{
    Seat seat = getPlayerSeat(id);

    auto it = seatPositions.find(seat);
    if (it == seatPositions.end())
    {
        std::cout << "ERROR: Invalid seat for player id: " << id << std::endl;
        return {0, 0};
    }

    return it->second;
}

pos Game::getPlayerCardPosition(int id)
{
    Seat seat = getPlayerSeat(id);

    auto it = seatCardPositions.find(seat);
    if (it == seatCardPositions.end())
    {
        std::cout << "ERROR: Invalid card seat for player id: " << id << std::endl;
        return {0, 0};
    }

    return it->second;
}

int Game::getPlayerCardRotationAngle(int id)
{
    auto it = currentState.PlayerPosition.find(id);

    if (it == currentState.PlayerPosition.end())
    {
        std::cout << "ERROR: Missing PlayerPosition for id: " << id << std::endl;
        return 0;
    }

    return it->second.second;
}

void Game::drawSingleChip(int x, int y, int radius, Color color, bool isLastChip, int rotationAngle)
{
    DrawCircle(x, y, radius, color);
    DrawCircleLines(x, y, radius, Fade(WHITE, 0.25f));
    if (isLastChip)
    {
        DrawCircle(x, y, radius - 5, WHITE);
        string chipValue;
        switch (color.b)
        {
        case 55: // Red chip
            chipValue = "250";
            break;
        case 241: // Blue chip
            chipValue = "100";
            break;
        case 48: // Green chip
            chipValue = "50";
            break;
        case 0: // Yellow chip
            chipValue = "10";
            break;
        }
        Vector2 TextSize = MeasureTextEx(cardFontStatic, chipValue.c_str(), 20, 1);
        float xoffset = TextSize.x / 2;
        float yoffset = TextSize.y / 2;

        DrawTextPro(cardFontStatic, chipValue.c_str(), {x - 0.0f, y - 0.0f}, {xoffset, yoffset}, rotationAngle, 20, 1, BLACK);
    }
    for (int i = 0; i < 360; i += 60)
    {
        float angle = i * DEG2RAD;
        int lineLength = 5;

        float px = x + cos(angle) * (radius - lineLength);
        float py = y + sin(angle) * (radius - lineLength);

        float ex = x + cos(angle) * (radius);
        float ey = y + sin(angle) * (radius);

        DrawLineEx({px, py}, {ex, ey}, 5, Fade(BLACK, 0.3f));
    }
}

void Game::drawStackOfChips(pos basePos, int amount, Color color, int rotationAngle)
{
    for (int i = 0; i < amount; i++)
    {
        if (i == amount - 1)
        {
            drawSingleChip(basePos.x, basePos.y, 20, color, true, rotationAngle);
        }
        else
        {
            drawSingleChip(basePos.x, basePos.y, 20, color); // (false,0) because not last chip
        }
        if (rotationAngle == 90)
            basePos.x += 2;
        else if (rotationAngle == 270)
            basePos.x -= 2;
        else if (rotationAngle == 0)
            basePos.y -= 2;
    }
}

void Game::buildMoneyChips()
{
    playerMoneyChips.clear();
    for (auto &[id, money] : currentState.playerMoney)
    {
        if (!currentState.isSeated[id] || currentState.isSpectator[id])
            continue; // Only build chips for players that are still in the hand
        MoneyChips chips;
        chips.buildChips(money);
        playerMoneyChips[id] = chips;
    }
    // playerMoneyChips.clear();

    // for (auto &[id, money] : currentState.playerMoney)
    // {
    //     playerMoneyChips[id].buildChips(money);
    // }
}

string Game::getPlayerName(int id)
{
    return playerName = (currentState.playerNames.count(id) > 0) ? currentState.playerNames.at(id) : ("Player " + to_string(id));
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
        if (hasEnoughTimePassed(it->timer, 5.0))
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