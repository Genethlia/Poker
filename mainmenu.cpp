#include "mainmenu.hpp"

void MainMenu::Init(Font *mainFont)
{
    this->mainFont = mainFont;
}

void MainMenu::Update()
{
    int key = GetCharPressed();
    while (key > 0)
    {
        if (key >= 32 && key <= 126)
        {
            if (typingName && name.size() < 10)
            {
                name += static_cast<char>(key);
            }
            else if (typingIP && IP.size() < 15)
            {
                IP += static_cast<char>(key);
            }
        }
        key = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE))
    {
        if (typingName && !name.empty())
        {
            name.pop_back();
        }
        else if (typingIP && !IP.empty())
        {
            IP.pop_back();
        }
    }

    if (IsKeyPressed(KEY_TAB))
    {
        typingName = !typingName;
        typingIP = !typingIP;
    }

    if (IsKeyPressed(KEY_ENTER))
    {
        joinRequestSent = true;
    }

    Vector2 mouse = GetVirtualMousePosition();
    Rectangle nameBox = {650, 350, 500, 60};
    Rectangle ipBox = {650, 450, 500, 60};
    Rectangle joinButton = {760, 560, 280, 70};

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        if (CheckCollisionPointRec(mouse, nameBox))
        {
            typingName = true;
            typingIP = false;
        }
        else if (CheckCollisionPointRec(mouse, ipBox))
        {
            typingName = false;
            typingIP = true;
        }
        else if (CheckCollisionPointRec(mouse, joinButton))
        {
            joinRequestSent = true;
        }
    }
}

void MainMenu::Draw()
{
    DrawTextEx(*mainFont, "POKER", {760, 160}, 80, 2, GOLD);

    DrawTextEx(*mainFont, "Name", {650, 310}, 28, 1, WHITE);
    DrawRectangleRounded({650, 350, 500, 60}, 0.2f, 8, Fade(BLACK, 0.7f));
    DrawRectangleLinesEx({650, 350, 500, 60}, 3, typingName ? GOLD : GRAY);
    DrawTextEx(*mainFont, name.c_str(), {670, 365}, 28, 1, WHITE);

    DrawTextEx(*mainFont, "Server IP", {650, 410}, 28, 1, WHITE);
    DrawRectangleRounded({650, 450, 500, 60}, 0.2f, 8, Fade(BLACK, 0.7f));
    DrawRectangleLinesEx({650, 450, 500, 60}, 3, typingIP ? GOLD : GRAY);
    DrawTextEx(*mainFont, IP.c_str(), {670, 465}, 28, 1, WHITE);

    Rectangle joinButton = {760, 560, 280, 70};
    Vector2 mouse = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mouse, joinButton);

    DrawRectangleRounded(joinButton, 0.25f, 10, hovered ? GOLD : DARKGRAY);
    DrawTextCentered(joinButton, "JOIN GAME", 32, *mainFont, hovered ? BLACK : WHITE);

    DrawTextEx(*mainFont, "TAB to switch field, ENTER to join", {650, 660}, 24, 1, LIGHTGRAY);

    if (!errorMessage.empty())
    {
        DrawTextEx(*mainFont, errorMessage.c_str(), {650, 720}, 24, 1, RED);
    }
}

bool MainMenu::shouldJoin() const
{
    return joinRequestSent;
}

void MainMenu::clearJoinRequest()
{
    joinRequestSent = false;
}

std::string MainMenu::getPlayerName() const
{
    return name;
}

std::string MainMenu::getServerIP() const
{
    return IP;
}

void MainMenu::setErrorMessage(const std::string &error)
{
    errorMessage = error;
}
