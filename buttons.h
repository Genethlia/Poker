#pragma once
#include "client_in_client.hpp"
#include "colors.h"
#include <string>
#include <functional>
#include <raylib.h>
#include <algorithm>
#include <iostream>

enum class quickBetButtonPressed
{
    None,
    Min,
    Pot,
    AllIn
};

class Button
{
public:
    Button() = default;
    virtual void Init(int x, int y, int width, int height, const std::string &text, std::function<void()> onClick, int colorScheme);
    void Draw(int textSize = 20);
    virtual void Update();
    Vector2 getPosition() const;
    float getWidth() const;
    float getHeight() const;
    Rectangle rect;
    std::string text;
    bool isButtonClicked() const;
    int colorScheme;

private:
    int x, y, width, height;
    bool isButtonHovered() const;
    std::function<void()> onClick;
};

class ActionButton : public Button
{
public:
    ActionButton() = default;
    void Init(int x, int y, int width, int height, const std::string &text, std::function<void()> onClick, int *toAct, int *myId, int colorScheme);
    virtual void Draw();
    void Update() override;
    int *toAct;
    int *myId;
};

class CheckCallButton : public ActionButton
{
public:
    CheckCallButton() = default;
    void Init(int x, int y, int width, int height, const std::string &text, std::function<void()> onClick, int *toAct, int *myId, int *toCall);
    void Update() override;

private:
    int *toCall;
};

class RaiseSliderButton : public ActionButton
{
public:
    RaiseSliderButton() = default;
    void Init(int x, int y, int width, int height, PokerClient::ClientState *currentState, int *raiseAmount, bool *buttonInteractionFlag, quickBetButtonPressed *quick);
    void Update() override;
    void Draw() override;

private:
    PokerClient::ClientState *currentState;
    int *raiseAmount;
    bool *buttonInteractionFlag;
    float percentageRaised;
    quickBetButtonPressed *quick;
    bool isButtonPressed() const;
    Rectangle smallRect;
    Rectangle barRect;
};

class RaiseAmountButton : public ActionButton
{
public:
    RaiseAmountButton() = default;
    void Draw() override;
};