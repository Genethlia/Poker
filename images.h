#pragma once
#include <raylib.h>

class Images
{
public:
    Images() = default; // Constructor that loads images based on rank
    ~Images();

    void UnloadAll();
    void LoadSuit(int rank);
    void LoadHiddenCard();
    void LoadChatTexture();

    Texture filiTexture{}, bigfiliTexture{}; // Textures for small and big suit images
    Texture hiddenCardTexture{};
    Texture chatTexture{};
};
