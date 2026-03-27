#include "buttons.h"

Button::Button(int x, int y, int width, int height, const std::string &text, std::function<void()> onClick)
    : x(x), y(y), width(width), height(height), text(text), onClick(onClick)
{
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

bool Button::isButtonHovered() const
{
    Vector2 mousePos = GetMousePosition();
    return CheckCollisionPointRec(mousePos, rect);
}

bool Button::isButtonClicked() const
{
    return isButtonHovered() && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}