#pragma once
#include "raylib.h"
#include <string>

static constexpr int VIRTUAL_WIDTH = 1600;
static constexpr int VIRTUAL_HEIGHT = 900;

static Vector2 GetVirtualMousePosition()
{
    Vector2 mouse = GetMousePosition();

    float scaleX = (float)VIRTUAL_WIDTH / (float)GetScreenWidth();
    float scaleY = (float)VIRTUAL_HEIGHT / (float)GetScreenHeight();

    return {mouse.x * scaleX, mouse.y * scaleY};
}

static void DrawTextCentered(Rectangle rect, const std::string &text, float fontSize, Font font, Color color = BLACK)
{
    Vector2 textSize = MeasureTextEx(font, text.c_str(), fontSize, 1.0f);
    Vector2 position = {
        rect.x + (rect.width - textSize.x) / 2.0f,
        rect.y + (rect.height - textSize.y) / 2.0f};

    DrawTextEx(font, text.c_str(), position, fontSize, 1.0f, color);
}
