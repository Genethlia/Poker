#pragma once
#include "raylib.h"

static constexpr int VIRTUAL_WIDTH = 1600;
static constexpr int VIRTUAL_HEIGHT = 900;

static Vector2 GetVirtualMousePosition()
{
    Vector2 mouse = GetMousePosition();

    float scaleX = (float)VIRTUAL_WIDTH / (float)GetScreenWidth();
    float scaleY = (float)VIRTUAL_HEIGHT / (float)GetScreenHeight();

    return {mouse.x * scaleX, mouse.y * scaleY};
}