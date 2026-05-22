#pragma once
#include <vector>
#include <raylib.h>
using Colors = std::pair<Color, Color>;
inline std::vector<Colors> buttonColorSchemes = {
    Colors{{168, 24, 24, 255}, {193, 18, 31, 255}},  // Red
    Colors{{72, 202, 228, 255}, {0, 180, 216, 255}}, // LightBlue
    Colors{{0, 114, 0, 255}, {0, 128, 0, 255}},      // Green
    Colors{{255, 183, 0, 255}, {255, 170, 0, 255}}   // Gold
};