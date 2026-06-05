#include "mainmenu.hpp"

void MainMenu::Init(Font *mainFont)
{
    this->mainFont = mainFont;
    nameBox = GetVirtualCenteredRectangle(500, 270, 60);
    codeBox = GetVirtualCenteredRectangle(500, 380, 60);
    joinButton = GetVirtualCenteredRectangle(280, 480, 70);
    hostButton = GetVirtualCenteredRectangle(280, 580, 70);
}

void MainMenu::Update()
{
    int key = GetCharPressed();
    while (key > 0)
    {
        if (key > 32 && key <= 126)
        {
            if (typingName && name.size() < 10)
            {
                name += static_cast<char>(key);
            }
            else if (typingCode && code.size() < 8)
            {
                if (key >= 97 && key <= 122) // Convert lowercase to uppercase
                {
                    key -= 32;
                }
                code += static_cast<char>(key);
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
        else if (typingCode && !code.empty())
        {
            code.pop_back();
        }
    }

    if (IsKeyPressed(KEY_TAB))
    {
        typingName = !typingName;
        typingCode = !typingCode;
    }

    if (IsKeyPressed(KEY_ENTER))
    {
        if (IsKeyPressed(KEY_LEFT_SHIFT) || IsKeyPressed(KEY_RIGHT_SHIFT))
        {
            hostGame = true;
            return;
        }
        joinRequestSent = true;
    }

    Vector2 mouse = GetVirtualMousePosition();

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        if (CheckCollisionPointRec(mouse, nameBox))
        {
            typingName = true;
            typingCode = false;
        }
        else if (CheckCollisionPointRec(mouse, codeBox))
        {
            typingName = false;
            typingCode = true;
        }
        else if (CheckCollisionPointRec(mouse, joinButton))
        {
            joinRequestSent = true;
        }
        else if (CheckCollisionPointRec(mouse, hostButton))
        {
            hostGame = true;
        }
    }
}

void MainMenu::Draw()
{
    DrawMainMenuTextCenteredWithMainFont("ALL IN POKER", 80, 80, GOLD);

    Vector2 mouse = GetVirtualMousePosition();
    bool hovered = CheckCollisionPointRec(mouse, joinButton);

    bool hostHovered = CheckCollisionPointRec(mouse, hostButton);

    DrawMainMenuTextCenteredWithMainFont("Name", 230, 28, WHITE);
    DrawRectangleRounded(nameBox, 0.2f, 8, Fade(BLACK, 0.7f));
    DrawRectangleLinesEx(nameBox, 3, typingName ? GOLD : GRAY);
    DrawMainMenuTextCenteredWithMainFont(name, nameBox.y + 15, 28, WHITE);

    DrawMainMenuTextCenteredWithMainFont("Room Code", 345, 28, WHITE);
    DrawRectangleRounded(codeBox, 0.2f, 8, Fade(BLACK, 0.7f));
    DrawRectangleLinesEx(codeBox, 3, typingCode ? GOLD : GRAY);
    DrawMainMenuTextCenteredWithMainFont(code, codeBox.y + 15, 28, WHITE);

    DrawRectangleRounded(joinButton, 0.25f, 10, hovered ? GOLD : DARKGRAY);
    DrawTextCentered(joinButton, "JOIN GAME", 32, *mainFont, hovered ? BLACK : WHITE);

    DrawRectangleRounded(hostButton, 0.25f, 10, hostHovered ? GOLD : DARKGRAY);
    DrawTextCentered(hostButton, "HOST GAME", 32, *mainFont, hostHovered ? BLACK : WHITE);

    DrawMainMenuTextCenteredWithMainFont("TAB to switch field, ENTER to join, SHIFT+ENTER to host", 710, 24, LIGHTGRAY);

    if (!errorMessage.empty())
    {
        DrawMainMenuTextCenteredWithMainFont(errorMessage, 750, 24, RED);
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

bool MainMenu::shouldHost() const
{
    return hostGame;
}

void MainMenu::clearHostRequest()
{
    hostGame = false;
}

std::string MainMenu::getPlayerName() const
{
    return name;
}

std::string MainMenu::getCode() const
{
    return code;
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
