#pragma once
#include <vector>
#include <unordered_map>
#include <string>
#include <raylib.h>
using Colors = std::pair<Color, Color>;
inline std::vector<Colors> buttonColorSchemes = {
    Colors{{168, 24, 24, 255}, {193, 18, 31, 255}},  // Red
    Colors{{72, 202, 228, 255}, {0, 180, 216, 255}}, // LightBlue
    Colors{{0, 114, 0, 255}, {0, 128, 0, 255}},      // Green
    Colors{{255, 183, 0, 255}, {255, 170, 0, 255}}   // Gold
};

inline unordered_map<string, Color> all_Colors = {
    {"woodColor", {60, 30, 10, 255}},
    {"tableRed", {200, 33, 42, 255}},
    {"background", {82, 36, 0, 255}}};

inline Color dark_Gold = {255, 189, 0, 230};