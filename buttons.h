#pragma once
#include <string>
#include <functional>
#include <raylib.h>
#include <iostream>

class Button
{
public:
    Button(int x, int y, int width, int height, const std::string &text, std::function<void()> onClick);
    void Draw();
    void Update();

private:
    int x, y, width, height;
    Rectangle rect;
    std::string text;
    bool isButtonHovered() const;
    bool isButtonClicked() const;

    std::function<void()> onClick;
};