#include "mainmenu.hpp"

void MainMenu::Init(Font *mainFont)
{
    this->mainFont = mainFont;
    nameBox = GetVirtualCenteredRectangle(500, 350, 60);
    ipBox = GetVirtualCenteredRectangle(500, 450, 60);
    joinButton = GetVirtualCenteredRectangle(280, 560, 70);
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
    DrawMainMenuTextCenteredWithMainFont("POKER", 160, 80, GOLD);

    Vector2 mouse = GetVirtualMousePosition();
    bool hovered = CheckCollisionPointRec(mouse, joinButton);

    DrawMainMenuTextCenteredWithMainFont("Name", 310, 28, WHITE);
    DrawRectangleRounded(nameBox, 0.2f, 8, Fade(BLACK, 0.7f));
    DrawRectangleLinesEx(nameBox, 3, typingName ? GOLD : GRAY);
    DrawMainMenuTextCenteredWithMainFont(name, nameBox.y + 15, 28, WHITE);

    DrawMainMenuTextCenteredWithMainFont("Server IP", 415, 28, WHITE);
    DrawRectangleRounded(ipBox, 0.2f, 8, Fade(BLACK, 0.7f));
    DrawRectangleLinesEx(ipBox, 3, typingIP ? GOLD : GRAY);
    DrawMainMenuTextCenteredWithMainFont(IP, ipBox.y + 15, 28, WHITE);

    DrawRectangleRounded(joinButton, 0.25f, 10, hovered ? GOLD : DARKGRAY);
    DrawTextCentered(joinButton, "JOIN GAME", 32, *mainFont, hovered ? BLACK : WHITE);

    DrawMainMenuTextCenteredWithMainFont("TAB to switch field, ENTER to join", 660, 24, LIGHTGRAY);

    if (!errorMessage.empty())
    {
        DrawMainMenuTextCenteredWithMainFont(errorMessage, 720, 24, RED);
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

void MainMenu::DrawMainMenuTextCenteredWithMainFont(const std::string &text, float y, float fontSize, Color color)
{
    if (!mainFont)
        return;

    DrawMainMenuTextCentered(text, y, fontSize, *mainFont, color);
}
