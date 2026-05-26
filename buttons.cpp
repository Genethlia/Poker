#include "buttons.h"

void Button::Init(int x, int y, int width, int height, const std::string &text, std::function<void()> onClick, int colorScheme)
{
    this->x = x;
    this->y = y;
    this->width = width;
    this->height = height;
    this->text = text;
    this->onClick = onClick;
    this->colorScheme = colorScheme;
    rect = {static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), static_cast<float>(height)};
}

void Button::Draw(int textSize)
{
    Color bgColor = isButtonHovered() ? buttonColorSchemes[colorScheme].first : buttonColorSchemes[colorScheme].second;
    DrawRectangleRounded(rect, 0.5f, 16, bgColor);
    float textWidth = MeasureText(text.c_str(), textSize);
    float textX = x + (width - textWidth) / 2;
    float textY = y + height / 2 - textSize / 2;
    DrawText(text.c_str(), textX, textY, textSize, BLACK);
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

float Button::getWidth() const
{
    return this->width;
}

float Button::getHeight() const
{
    return this->height;
}

bool Button::isButtonHovered() const
{
    Vector2 mousePos = GetVirtualMousePosition();
    return CheckCollisionPointRec(mousePos, rect);
}

bool Button::isButtonClicked() const
{
    return isButtonHovered() && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

void ActionButton::Init(int x, int y, int width, int height, const std::string &text, std::function<void()> onClick, int *toAct, int *myId, int colorScheme)
{
    Button::Init(x, y, width, height, text, onClick, colorScheme);
    this->toAct = toAct;
    this->myId = myId;
}

void ActionButton::Draw()
{
    int textSize = 25;
    if (*toAct == *myId)
    {
        textSize = 30;
        Button::Draw(textSize);
    }
    else
    {
        Vector2 pos = getPosition();
        float textWidth = MeasureText(text.c_str(), textSize);
        float textX = pos.x + (getWidth() - textWidth) / 2;
        float textY = pos.y + getHeight() / 2 - textSize / 2;
        DrawRectangleRounded(rect, 0.5f, 16, GRAY);
        DrawText(text.c_str(), textX, textY, textSize, DARKGRAY);
    }
}

void ActionButton::Update()
{
    if (*toAct == *myId)
    {
        Button::Update();
    }
}

void CheckCallButton::Init(int x, int y, int width, int height, const std::string &text, std::function<void()> onClick, int *toAct, int *myId, int *toCall)
{
    ActionButton::Init(x, y, width, height, text, onClick, toAct, myId, 2);
    this->toCall = toCall;
}

void CheckCallButton::Update()
{
    if (*toAct == *myId)
    {
        if (*toCall > 0)
        {
            text = "Call $" + std::to_string(*toCall);
            colorScheme = 2;
        }
        else
        {
            text = "Check";
            colorScheme = 1;
        }
        Button::Update();
    }
}

void RaiseSliderButton::Init(int x, int y, int width, int height, PokerClient::ClientState *currentState, int *raiseAmount, bool *buttonInteractionFlag, quickBetButtonPressed *quick)
{
    ActionButton::Init(x, y, width, height, "", nullptr, &currentState->toAct, &currentState->myId, 0);
    this->currentState = currentState;
    this->raiseAmount = raiseAmount;
    this->buttonInteractionFlag = buttonInteractionFlag;
    this->quick = quick;
    this->percentageRaised = 0.0f;
    barRect = {static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), static_cast<float>(height)};
}

void RaiseSliderButton::Update()
{
    if (currentState->toAct != currentState->myId)
    {
        return;
    }

    int myId = currentState->myId;

    if (!currentState->playerMoney.count(myId))
        return;

    if (!currentState->betThisRound.count(myId))
        return;

    int money = currentState->playerMoney[myId];
    int currentBet = currentState->currentBet;
    int minRaise = currentState->minRaise;
    int betThisRound = currentState->betThisRound[myId];

    int minTotalRaise = minRaise + currentBet;
    int maxTotalRaise = money + betThisRound;

    Vector2 pos = getPosition();
    barRect = {pos.x, pos.y, getWidth(), getHeight()};

    if (!*buttonInteractionFlag)
    {
        percentageRaised = 0.0f;
        *raiseAmount = minTotalRaise;
        *buttonInteractionFlag = true;
    }

    if (*quick != quickBetButtonPressed::None)
    {
        switch (*quick)
        {
        case quickBetButtonPressed::Min:
            *raiseAmount = minTotalRaise;
            percentageRaised = 0.0f;
            break;
        case quickBetButtonPressed::Pot:
        {
            *raiseAmount = std::min(currentState->potAmount + currentBet, maxTotalRaise);
            int range = maxTotalRaise - minTotalRaise;
            int betForPercentage = *raiseAmount - minTotalRaise;
            percentageRaised = static_cast<float>(betForPercentage) / range;
            break;
        }
        case quickBetButtonPressed::AllIn:
            *raiseAmount = maxTotalRaise;
            percentageRaised = 1.0f;
            break;
        default:
            break;
        }
        *quick = quickBetButtonPressed::None;
    }

    if (maxTotalRaise <= currentBet)
        return;

    if (minTotalRaise > maxTotalRaise)
    {
        *raiseAmount = maxTotalRaise;
        percentageRaised = 1.0f;
    }

    else if (isButtonPressed())
    {

        float mouseX = GetVirtualMousePosition().x;
        percentageRaised = (mouseX - barRect.x) / barRect.width;
        percentageRaised = std::clamp(percentageRaised, 0.0f, 1.0f);

        int range = maxTotalRaise - minTotalRaise;
        *raiseAmount = minTotalRaise + static_cast<int>(percentageRaised * range);
    }

    smallRect = {barRect.x + percentageRaised * barRect.width - 5, barRect.y, 50, 50};
}

void RaiseSliderButton::Draw()
{
    if (*toAct != *myId)
    {
        return;
    }

    Rectangle barRectDraw = barRect;
    barRectDraw.width += 45;
    Rectangle betRect = {barRect.x - 120, barRect.y, 100, barRect.height};
    DrawRectangleRounded(betRect, 0.5f, 16, BLACK);
    DrawRectangleRoundedLines(betRect, 0.5f, 16, GRAY);
    int textWidth = MeasureText(TextFormat("$%d", *raiseAmount), 30);
    DrawText(TextFormat("$%d", *raiseAmount), barRect.x - 120 + betRect.width / 2 - (textWidth) / 2, barRect.y + 15, 30, WHITE);
    DrawRectangleRec(barRectDraw, LIGHTGRAY);
    DrawRectangleRec(smallRect, GOLD);
}

bool RaiseSliderButton::isButtonPressed() const
{
    bool isButtonHovered = CheckCollisionPointRec(GetVirtualMousePosition(), barRect) || CheckCollisionPointRec(GetVirtualMousePosition(), smallRect);
    return isButtonHovered && IsMouseButtonDown(MOUSE_LEFT_BUTTON);
}

void RaiseAmountButton::Draw()
{
    if (*toAct == *myId)
    {
        Button::Draw(30);
    }
}