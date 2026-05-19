#include "buttons.h"

void Button::Init(int x, int y, int width, int height, const std::string &text, std::function<void()> onClick)
{
    this->x = x;
    this->y = y;
    this->width = width;
    this->height = height;
    this->text = text;
    this->onClick = onClick;
    rect = {static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), static_cast<float>(height)};
}

void Button::Draw()
{
    DrawRectangleRec(rect, isButtonHovered() ? DARKGREEN : GREEN);
    DrawText(text.c_str(), x + 10, y + height / 2 - 10, 20, BLACK);
}

void Button::Update()
{
    if (isButtonClicked() && onClick)
    {
        onClick();
    }
    else if (!onClick)
    {
        std::cout << "Warning: Button '" << text << "' has no onClick action defined.\n";
    }
}

Vector2 Button::getPosition() const
{
    return {static_cast<float>(x), static_cast<float>(y)};
}

bool Button::isButtonHovered() const
{
    Vector2 mousePos = GetMousePosition();
    return CheckCollisionPointRec(mousePos, rect);
}

bool Button::isButtonClicked() const
{
    return isButtonHovered() && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

void ActionButton::Init(int x, int y, int width, int height, const std::string &text, std::function<void()> onClick, int *toAct, int *myId)
{
    Button::Init(x, y, width, height, text, onClick);
    this->toAct = toAct;
    this->myId = myId;
}

void ActionButton::Draw()
{
    if (*toAct == *myId)
    {
        Button::Draw();
    }
    else
    {
        Vector2 pos = getPosition();
        DrawRectangleRec(rect, GRAY);
        DrawText(text.c_str(), pos.x + 10, pos.y + rect.height / 2 - 10, 20, DARKGRAY);
    }
}

void CheckCallButton::Init(int x, int y, int width, int height, const std::string &text, std::function<void()> onClick, int *toAct, int *myId, int *toCall)
{
    ActionButton::Init(x, y, width, height, text, onClick, toAct, myId);
    this->toCall = toCall;
}

void CheckCallButton::Update()
{
    if (*toAct == *myId)
    {
        if (*toCall > 0)
        {
            text = "Call $" + std::to_string(*toCall);
        }
        else
        {
            text = "Check";
        }
    }
    Button::Update();
}

void RaiseAmountButton::Init(int x, int y, int width, int height, int *toAct, int *myId, int *minRaise, int *money, int *raiseAmount)
{
    ActionButton::Init(x, y, width, height, "", nullptr, toAct, myId);
    this->minRaise = minRaise;
    this->money = money;
    this->raiseAmount = raiseAmount;
    this->percentageRaised = 0.0f;
}

void RaiseAmountButton::Update()
{
    if (minRaise >= money)
    {
        return;
    }
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && isButtonHovered())
    {
        if (GetMousePosition().x < rect.x + rect.width / 2)
        {
            percentageRaised -= 0.05f;
            percentageRaised = std::max(0.0f, percentageRaised);
        }
        else
        {
            percentageRaised += 0.05f;
            percentageRaised = std::min(1.0f, percentageRaised);
        }
        *raiseAmount = *minRaise + static_cast<int>(percentageRaised * (*money - *minRaise));
    }

    Vector2 pos = getPosition();
    rect1 = {pos.x + percentageRaised * rect.width, pos.y - 30, 10, 60};
}

void RaiseAmountButton::Draw()
{
    if (minRaise >= money)
    {
        return;
    }
    ActionButton::Draw();
    DrawText(TextFormat("Raise: $%d", *raiseAmount), rect1.x, rect1.y - 20, 20, BLACK);
    DrawRectangleRec(rect1, (*toAct == *myId) ? GOLD : GRAY);
}
