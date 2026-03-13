#pragma once
#include <raylib.h>

class Images
{
public:
    Images() = default; // Constructor that loads images based on rank
    ~Images();

    void UnloadAll();
    void LoadSuit(int rank);
    void LoadMatHiddenCardAndHome();

    Texture filiTexture{}, bigfiliTexture{}; // Textures for small and big suit images
    Texture matTexture{};                    // Table mat texture
    Texture hiddenCardTexture{};
};
