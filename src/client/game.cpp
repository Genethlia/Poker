#include "game.hpp"
#include "assets.h"
using namespace std;

Font Game::cardFontStatic{}; // Definition of the static card font variable

Game::Game()
{
    leaveButton = {VIRTUAL_WIDTH - 230, 110, 200, 40};
}

Game::~Game()
{
    client.stop();
    server.end();

    if (serverThread.joinable())
        serverThread.join();

    UnloadRenderTexture(target);
    UnloadFont(cardFont);
    UnloadFont(mainFont);
    UnloadFont(buttonFont);
}

void Game::start()
{

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(VIRTUAL_WIDTH, VIRTUAL_HEIGHT, "All In Poker");
    SetTargetFPS(60);

    target = LoadRenderTexture(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);

    // ToggleFullscreen();

    for (int i = 0; i < 4; i++)
        suitTextures[i].LoadSuit(i);
    hiddenCardImage.LoadHiddenCard();
    chatImage.LoadChatTexture();

    cardFont = LoadFontFromMemory(".ttf", cardfont_ttf, cardfont_ttf_len, 64, nullptr, 0);
    mainFont = LoadFontFromMemory(".ttf", mainfont_ttf, mainfont_ttf_len, 64, nullptr, 0);
    buttonFont = LoadFontFromMemory(".ttf", buttonfont_ttf, buttonfont_ttf_len, 64, nullptr, 0);

    Game::cardFontStatic = cardFont; // Initialize the static card font variable
    chat.Init(&chatImage, &client, &mainFont);
    uiButton.Init(&client, &popUpMessages, &visualState.gameState, &currentState, &buttonFont);
    mainMenu.Init(&mainFont);

    // try
    // {
    //     client.connect_to(serverIP, "6767");
    //     client.join_us(playerName);
    //     client.start();
    // }
    // catch (const exception &e)
    // {
    //     cout << "Failed to connect: " << e.what() << "\n";
    // }

    while (!WindowShouldClose())
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

void Game::update()
{
    if (screenState == ScreenState::MainMenu)
    {
        mainMenu.Update();
        if (mainMenu.shouldJoin())
        {
            tryJoin();
            mainMenu.clearJoinRequest();
        }
        else if (mainMenu.shouldHost())
        {
            tryHost();
            mainMenu.clearHostRequest();
        }
    }
    else
    {
        updateLeaveButton();
        updateGameState();
        clearCardsIfNecessary();
        updatePopUpMessages();
        updateFullScreen();
        updateDimensions();
        uiButton.Update();
        updateChat();

        if (visualState.gameState != GameState::WaitingForPlayers && visualState.gameState != GameState::GameOver)
        {
            updateVisualState();
            visualState.updateCards();
        }

        if (!client.running)
        {
            leaveToMainMenu();
            mainMenu.setErrorMessage("Disconnected from server. Please try joining again.");
        }
    }
}

void Game::updateGameState()
{
    auto newState = client.getClientStateCopy();

    bool positionsChanged = currentState.PlayerPosition != newState.PlayerPosition;

    if (newState.gameState != currentState.gameState)
    {
        onServerStateChange(newState.gameState);
    }
    buildMoneyChips();
    currentState = newState;

    if (positionsChanged)
    {
        visualState.reset();
        DrawnCount.clear();
    }

    if (visualState.gameState == GameState::Showdown)
    {
        if (hasEnoughTimePassed(visualState.showdownTimerStartTime, 5.0))
        {
            visualState.gameState = GameState::GameOver;
            resetForNewGame();
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
    std::string name = getPlayerName(id);
    int money = currentState.playerMoney.count(id) ? currentState.playerMoney[id] : 0;

    pos boxPos = getSeatLayout(id).nameBoxPos;
    Seat seat = getPlayerSeat(id);

    if (currentState.betThisRound.count(id) && currentState.betThisRound[id] > 0)
    {
        drawBetOfPlayer(id);
    }

    Color bgColor = BLACK;

    if (id == currentState.toAct)
        bgColor = {122, 0, 0, 255}; // DARK RED

    DrawRectangleRounded(
        {boxPos.x, boxPos.y, 200, 65},
        0.25f,
        8,
        bgColor);

    int offsetX = 10;
    DrawTextEx(mainFont, name.c_str(), {boxPos.x + offsetX, boxPos.y}, 22, 1.0f, WHITE);
    DrawTextEx(mainFont, TextFormat("$%d", money), {boxPos.x + offsetX, boxPos.y + 40}, 20, 1.0f, GOLD);

    if (id == currentState.myId)
    {
        DrawTextEx(mainFont, "YOU", {boxPos.x + 150, boxPos.y}, 18, 1.0f, WHITE);
    }
    bgColor = WHITE;
    if (id == currentState.dealerId)
    {
        DrawTextEx(mainFont, "D", {boxPos.x + 140, boxPos.y + 40}, 18, 1.0f, bgColor);
    }
    if (id == currentState.smallBlindId)
    {
        DrawTextEx(mainFont, "SB", {boxPos.x + 160, boxPos.y + 40}, 18, 1.0f, bgColor);
    }
    if (id == currentState.bigBlindId)
    {
        DrawTextEx(mainFont, "BB", {boxPos.x + 160, boxPos.y + 40}, 18, 1.0f, bgColor);
    }
}

Game::VisualState::VisualState()
{
    myCards = {};
    opponentCards = {};
    communityCards = {};
    idToShowCardsOf = {};

    gameState = GameState::WaitingForPlayers;
    showdownTimerStartTime = 0.0;
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
    if (screenState == ScreenState::MainMenu)
    {
        mainMenu.Draw();
    }
    else
    {
        drawBackground();
        drawChat();
        visualState.drawCards();
        drawPlayers();
        drawMoneyChips();

        drawInput();
        drawChat();
        drawPopUpMessages();
        drawCodeAndSpectators();
        uiButton.Draw();
        drawLeaveButton();
        drawCurrentHandText();
    }
}

void Game::drawInput()
{
    if (visualState.gameState == GameState::WaitingForPlayers)
    {
        string waitingText = "Waiting for players...";
        int textWidth = MeasureText(waitingText.c_str(), 24);
        DrawTextCentered({0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT}, waitingText.c_str(), 24, mainFont, YELLOW);
    }
    else if (visualState.gameState == GameState::GameOver)
    {

        string gameOverText = "Game Over!";
        int textWidth = MeasureText(gameOverText.c_str(), 24);
        DrawTextCentered({0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT}, gameOverText.c_str(), 24, mainFont, YELLOW);
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
        pos basePos = getSeatLayout(id).chipPos;
        int rotationAngle = getPlayerCardRotationAngle(id);
        chips.drawChips(basePos, rotationAngle);
    }
}

void Game::VisualState::drawCards()
{
    for (auto &card : myCards)
    {
        card.Draw();
    }
    for (auto &card : opponentCards)
    {
        card.second.Draw();
    }
    for (auto &card : communityCards)
    {
        card.Draw();
    }
}

void Game::VisualState::reset()
{
    myCards.clear();
    opponentCards.clear();
    communityCards.clear();
    idToShowCardsOf.clear();
}

void Game::updateVisualState()
{
    while (currentState.myCards.size() > visualState.myCards.size())
    {
        seatLayout layout;
        if (!tryGetSeatLayout(currentState.myId, layout))
        {
            cout << "No seat layout for me\n";
            return;
        };
        valRank cardInfo = currentState.myCards[visualState.myCards.size()];
        visualState.myCards.emplace_back(getSeatLayout(currentState.myId).cardPos.x + DrawnCount[currentState.myId] * 25, getSeatLayout(currentState.myId).cardPos.y + DrawnCount[currentState.myId] * 5, cardInfo, &suitTextures[cardInfo.suit], &cardFont, &hiddenCardImage);
        DrawnCount[currentState.myId]++;
    }
    while (currentState.opponentCards.size() > visualState.opponentCards.size())
    {
        auto &[id, cardInfo] = currentState.opponentCards[visualState.opponentCards.size()];

        seatLayout layout;
        if (!tryGetSeatLayout(id, layout))
        {
            cout << "Waiting for seat layout before drawing cards for player ID "
                 << id << "\n";
            return;
        }
        pos basePos = getSeatLayout(id).cardPos;
        int offsetX = DrawnCount[id] * 80;
        visualState.opponentCards.emplace_back(id, Card(basePos.x + offsetX, basePos.y, cardInfo, &suitTextures[cardInfo.suit], &cardFont, &hiddenCardImage, true));
        DrawnCount[id]++;
    }
    while (currentState.communityCards.size() > visualState.communityCards.size())
    {
        valRank cardInfo = currentState.communityCards[visualState.communityCards.size()];
        visualState.communityCards.emplace_back(446 + visualState.communityCards.size() * 150, 360, cardInfo, &suitTextures[cardInfo.suit], &cardFont, &hiddenCardImage);
    }
    while (currentState.idToShowCardsOf.size() > visualState.idToShowCardsOf.size())
    {
        int id = currentState.idToShowCardsOf[visualState.idToShowCardsOf.size()];
        visualState.idToShowCardsOf.push_back(id);
        if (id == currentState.myId)
            continue;
        for (auto &[opponentId, card] : visualState.opponentCards)
        {
            if (opponentId == id)
            {
                card.SetHidden(false);
            }
        }
    }
}

void Game::drawBetOfPlayer(int id)
{
    int betAmount = currentState.betThisRound[id];
    pos basePos = getSeatLayout(id).betPos;
    DrawRectangleRounded({basePos.x - 20, basePos.y - 10, 80, 30}, 0.25f, 8, Fade(BLACK, 0.8f));
    DrawTextCentered({basePos.x - 20, basePos.y - 10, 80, 30}, TextFormat("$%d", betAmount), 20, mainFont, GOLD);
}

void Game::clearCardsIfNecessary()
{
    bool hasCardWithoutPosition = false;

    for (auto &[id, card] : visualState.opponentCards)
    {
        if (!currentState.PlayerPosition.count(id))
        {
            hasCardWithoutPosition = true;
            break;
        }
    }

    if (hasCardWithoutPosition || visualState.myCards.size() > currentState.myCards.size() || visualState.opponentCards.size() > currentState.opponentCards.size() || visualState.communityCards.size() > currentState.communityCards.size() || visualState.idToShowCardsOf.size() > currentState.idToShowCardsOf.size())
    {
        visualState.myCards.clear();
        visualState.opponentCards.clear();
        visualState.communityCards.clear();
        visualState.idToShowCardsOf.clear();
        DrawnCount.clear();
    }
}

void Game::drawPopUpMessages()
{
    int yOffset = 0;
    Color bgColor;
    for (auto &message : popUpMessages)
    {
        bgColor = getColorForPopUpMessageType(message.type);
        if (message.text.length() >= 30)
        {
            size_t splitIndex = message.text.find(' ', message.text.length() / 2);
            int textWidth1 = MeasureTextEx(mainFont, message.text.substr(0, splitIndex).c_str(), 20, 1.0f).x;
            int textWidth2 = MeasureTextEx(mainFont, message.text.substr(splitIndex).c_str(), 20, 1.0f).x;
            int textWidth = max(textWidth1, textWidth2); // find biggest width for the background rectangle
            DrawRectangleRounded({400, 20.0f + yOffset, (float)textWidth + 20, 59}, 0.5f, 4, Fade(BLACK, 0.5f));
            DrawTextEx(mainFont, message.text.substr(0, splitIndex).c_str(), {410.0f, 20.0f + yOffset}, 20, 1.0f, bgColor);
            yOffset += 30;
            DrawTextEx(mainFont, message.text.substr(splitIndex).c_str(), {410.0f, 20.0f + yOffset}, 20, 1.0f, bgColor);
        }
        else
        {
            int textWidth = MeasureTextEx(mainFont, message.text.c_str(), 20, 1.0f).x;
            DrawRectangleRounded({400, 20.0f + yOffset, (float)textWidth + 20, 29}, 0.5f, 4, Fade(BLACK, 0.5f));
            DrawTextEx(mainFont, message.text.c_str(), {410.0f, 20.0f + yOffset}, 20, 1.0f, bgColor);
        }
        yOffset += 30;
    }
}

void Game::onServerStateChange(GameState newState)
{
    if (newState == GameState::PreFlop)
    {
        resetForNewGame();
    }

    visualState.gameState = newState;

    if (newState == GameState::Showdown)
    {
        visualState.showdownTimerStartTime = GetTime();
    }
}

void Game::resetForNewGame()
{
    visualState.communityCards.clear();
    visualState.opponentCards.clear();
    visualState.myCards.clear();
    visualState.idToShowCardsOf.clear();

    currentState.communityCards.clear();
    currentState.opponentCards.clear();
    currentState.myCards.clear();
    currentState.idToShowCardsOf.clear();

    visualState.showdownTimerStartTime = 0.0;
    DrawnCount.clear();
}

void Game::drawChat()
{
    chat.Draw();
}

void Game::updateChat()
{
    chat.Update();
}

void Game::drawCodeAndSpectators()
{
    int spectatorCount = 0;
    for (auto &[id, isSpectator] : currentState.isSpectator)
    {
        if (isSpectator)
            spectatorCount++;
    }
    string roomCodeText = TextFormat("Room Code: %s", currentState.roomCode.c_str());
    float textWidth = MeasureTextEx(mainFont, roomCodeText.c_str(), 20, 1.0f).x;
    DrawTextEx(mainFont, roomCodeText.c_str(), {VIRTUAL_WIDTH - textWidth - 30.0f, 70}, 20, 1.0f, WHITE);
    if (spectatorCount > 0)
    {
        string spectatorText = TextFormat("Spectators: %d", spectatorCount);
        DrawTextCentered({VIRTUAL_WIDTH - textWidth - 30.0f, 30.0f, textWidth, 20.0f}, spectatorText.c_str(), 20, mainFont, WHITE);
    }
}

void Game::drawLeaveButton()
{
    Vector2 mouse = GetVirtualMousePosition();
    Color bg = CheckCollisionPointRec(mouse, leaveButton) ? buttonColorSchemes[0].first : buttonColorSchemes[0].second;
    DrawRectangleRounded(leaveButton, 0.5f, 16, bg);
    DrawTextCentered(leaveButton, "LEAVE", 30, mainFont, BLACK);
}

void Game::updateLeaveButton()
{
    Vector2 mouse = GetVirtualMousePosition();
    bool clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, leaveButton);
    if (clicked)
    {
        leaveToMainMenu();
    }
}

void Game::drawCurrentHandText()
{
    vector<valRank> visibleCards;

    for (auto &card : visualState.myCards)
        visibleCards.push_back({card.getValue(), card.getSuit()});

    for (auto &card : visualState.communityCards)
        visibleCards.push_back({card.getValue(), card.getSuit()});

    if (visibleCards.empty())
        return;

    Score currentScore = scoreVisibleCards(visibleCards);

    string text = scoreName(currentScore[0]);

    Rectangle hand = GetVirtualCenteredRectangle(200, 800, 80);
    DrawRectangleRounded(
        hand,
        0.5f,
        16,
        dark_Gold);

    DrawTextCentered(hand, text.c_str(), 30, mainFont, BLACK);
}

Color Game::getColorForPopUpMessageType(popUpMessageType type)
{
    switch (type)
    {
    case popUpMessageType::ChatMessage:
        return SKYBLUE;
    case popUpMessageType::ActionResult:
    case popUpMessageType::BettingUpdate:
        return YELLOW;
    case popUpMessageType::GameWon:
        return GREEN;
    case popUpMessageType::GameLost:
        return RED;
    case popUpMessageType::Error:
        return RED;
    default:
        return LIGHTGRAY;
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

seatLayout Game::getSeatLayout(int id)
{
    Seat seat = getPlayerSeat(id);
    auto it = seatLayouts.find(seat);
    if (it == seatLayouts.end())
    {
        std::cout << "ERROR: Invalid seat layout for player id: " << id << std::endl;
        return seatLayout();
    }
    return it->second;
}

bool Game::tryGetSeatLayout(int id, seatLayout &out)
{
    auto posIt = currentState.PlayerPosition.find(id);
    if (posIt == currentState.PlayerPosition.end())
        return false;

    Seat seat = posIt->second.first;

    auto layoutIt = seatLayouts.find(seat);
    if (layoutIt == seatLayouts.end())
        return false;

    out = layoutIt->second;
    return true;
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

void Game::tryJoin()
{
    client.stop();
    client.resetLocalState();

    currentState = PokerClient::ClientState{};
    visualState = VisualState{};
    uiButton.reset();
    popUpMessages.clear();
    playerMoneyChips.clear();
    DrawnCount.clear();

    std::string name = mainMenu.getPlayerName();
    std::string code = mainMenu.getCode();

    if (name.empty())
    {
        mainMenu.setErrorMessage("Name cannot be empty.");
        return;
    }

    std::string serverIP = roomCodeToIP(code);

    if (serverIP.empty() || !authenticateIP(serverIP))
    {
        serverIP = "127.0.0.1"; // Localhost
    }

    try
    {
        isHosting = false;
        playerName = name;
        client.connect_to(serverIP, "6767");
        client.join_us(playerName);
        client.start();

        screenState = ScreenState::InGame;
    }
    catch (const exception &e)
    {
        mainMenu.setErrorMessage("Failed to connect: False room code");
        screenState = ScreenState::MainMenu;
    }
}

void Game::tryHost()
{
    client.stop();
    client.resetLocalState();

    currentState = PokerClient::ClientState{};
    visualState = VisualState{};
    uiButton.reset();
    popUpMessages.clear();
    playerMoneyChips.clear();
    DrawnCount.clear();

    stopHostedServerIfNeeded();

    std::string name = mainMenu.getPlayerName();
    if (name.empty())
    {
        mainMenu.setErrorMessage("Name cannot be empty. ");
        return;
    }
    try
    {
        serverThread = thread([this]()
                              { server.start(); });
        this_thread::sleep_for(chrono::milliseconds(100)); // Give the server a moment to start

        isHosting = true;
        playerName = name;
        client.connect_to("127.0.0.1", "6767");
        client.join_us(playerName);
        client.start();

        screenState = ScreenState::InGame;
    }
    catch (const exception &e)
    {
        isHosting = false;
        mainMenu.setErrorMessage("Failed to start server: " + string(e.what()));
        screenState = ScreenState::MainMenu;
        return;
    }
}

std::string Game::roomCodeToIP(std::string roomCode)
{
    if (roomCode.size() != 8)
        return "";

    string ip;

    for (int i = 0; i < 8; i += 2)
    {
        int A = roomCode[i] - 'A';
        int B = roomCode[i + 1] - 'A';
        if (A < 0 || A > 15 || B < 0 || B > 15)
            return "";

        int num = A * 16 + B;

        ip += to_string(num);
        if (i < 6)
            ip += '.';
    }
    return ip;
}

void Game::leaveToMainMenu()
{
    client.leaveGame();
    client.resetLocalState();

    stopHostedServerIfNeeded();

    currentState = PokerClient::ClientState();
    visualState = VisualState();
    popUpMessages.clear();
    playerMoneyChips.clear();
    DrawnCount.clear();

    screenState = ScreenState::MainMenu;
    mainMenu.setErrorMessage("");
}

void Game::stopHostedServerIfNeeded()
{
    if (!isHosting)
        return;

    server.end();
    if (serverThread.joinable())
        serverThread.join();

    isHosting = false;
}

void Game::updatePopUpMessages()
{
    auto newMessages = client.getAndClearPopUpMessages();
    for (auto &message : newMessages)
    {
        popUpMessages.push_back(PokerClient::popUpMessage(message.first, message.second));
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