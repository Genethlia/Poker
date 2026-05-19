#pragma once
#include <string>
#include <functional>
#include <raylib.h>
#include <iostream>

class Button
{
public:
    Button() = default;
    virtual void Init(int x, int y, int width, int height, const std::string &text, std::function<void()> onClick);
    virtual void Draw();
    virtual void Update();
    Vector2 getPosition() const;
    Rectangle rect;
    std::string text;
    bool isButtonClicked() const;
    bool isButtonHovered() const;

private:
    int x, y, width, height;

    std::function<void()> onClick;
};

class ActionButton : public Button
{
public:
    ActionButton() = default;
    void Init(int x, int y, int width, int height, const std::string &text, std::function<void()> onClick, int *toAct, int *myId);
    void Draw() override;
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

class RaiseAmountButton : public ActionButton
{
public:
    RaiseAmountButton() = default;
    void Init(int x, int y, int width, int height, int *toAct, int *myId, int *minRaise, int *money, int *raiseAmount);
    void Update() override;
    void Draw() override;

private:
    int *minRaise;
    int *money;
    int *raiseAmount;
    float percentageRaised;
    Rectangle rect1;
};