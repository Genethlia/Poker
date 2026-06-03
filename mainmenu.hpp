#pragma once
#include "raylib.h"
#include "raylib_helpers.h"
#include <string>

class MainMenu
{
public:
    MainMenu() = default;
    void Init(Font *mainFont);
    void Update();
    void Draw();

    bool shouldJoin() const;
    void clearJoinRequest();

    bool shouldHost() const;
    void clearHostRequest();

    std::string getPlayerName() const;
    std::string getCode() const;

    void setErrorMessage(const std::string &error);

    void DrawMainMenuTextCenteredWithMainFont(const std::string &text, float y, float fontSize, Color color = BLACK);

private:
    Font *mainFont;

    std::string name;
    std::string code;
    std::string errorMessage;

    bool typingName = true;
    bool typingCode = false;
    bool joinRequestSent = false;
    bool hostGame = false;

    Rectangle nameBox;
    Rectangle codeBox;
    Rectangle joinButton;
    Rectangle hostButton;
};