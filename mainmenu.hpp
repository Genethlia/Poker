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

    std::string getPlayerName() const;
    std::string getServerIP() const;

    void setErrorMessage(const std::string &error);

private:
    Font *mainFont;

    std::string name;
    std::string IP;
    std::string errorMessage;

    bool typingName = true;
    bool typingIP = false;
    bool joinRequestSent = false;
};