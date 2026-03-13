#include "images.h"
#include "assets.h"
#include <iostream>

void Images::LoadSuit(int rank)
{
    if (filiTexture.id != 0 || bigfiliTexture.id != 0)
    {
        UnloadTexture(filiTexture);
        UnloadTexture(bigfiliTexture);
    }

    int x = 60, y = 90;

    const unsigned char *suitData = nullptr;
    int suitDataLen = 0;

    // suit images
    switch (rank)
    {
    case 0:
        suitData = diamond_png;
        suitDataLen = diamond_png_len;
        break;
    case 1:
        suitData = spades_png;
        suitDataLen = spades_png_len;
        x = 70;
        break;
    case 2:
        suitData = hearts_png;
        suitDataLen = hearts_png_len;
        x = 50;
        break;
    case 3:
        suitData = clubs_png;
        suitDataLen = clubs_png_len;
        x = 80;
        break;
    default:
        std::cout << "Invalid rank: " << rank << std::endl;
        return;
    }

    Image image = LoadImageFromMemory(".png", suitData, suitDataLen);
    if (image.data == nullptr)
    {
        std::cout << "Failed to load suit image for rank " << rank << std::endl;
        return;
    }

    Image big = ImageCopy(image);
    ImageResize(&big, x, y);
    bigfiliTexture = LoadTextureFromImage(big);
    UnloadImage(big);

    Image small = ImageCopy(image);
    ImageResize(&small, x / 3, y / 3);
    filiTexture = LoadTextureFromImage(small);
    UnloadImage(small);

    UnloadImage(image);
}
void Images::LoadMatHiddenCardAndHome()
{
    if (matTexture.id != 0 || hiddenCardTexture.id != 0)
    {
        UnloadTexture(matTexture);
        UnloadTexture(hiddenCardTexture);
    }
    Image mat = LoadImageFromMemory(".jpg", mat_jpg, mat_jpg_len);
    if (mat.data != nullptr)
    {
        ImageResize(&mat, 1200, 950);
        matTexture = LoadTextureFromImage(mat);
        UnloadImage(mat);
    }

    Image card = LoadImageFromMemory(".png", card_png, card_png_len);
    if (card.data != nullptr)
    {
        ImageResize(&card, 120, 200);
        hiddenCardTexture = LoadTextureFromImage(card);
        UnloadImage(card);
    }
}

void Images::UnloadAll()
{
    if (filiTexture.id != 0)
    {
        UnloadTexture(filiTexture);
        filiTexture = Texture2D{};
    }
    if (bigfiliTexture.id != 0)
    {
        UnloadTexture(bigfiliTexture);
        bigfiliTexture = Texture2D{};
    }
    if (matTexture.id != 0)
    {
        UnloadTexture(matTexture);
        matTexture = Texture2D{};
    }
    if (hiddenCardTexture.id != 0)
    {
        UnloadTexture(hiddenCardTexture);
        hiddenCardTexture = Texture2D{};
    }
}

Images::~Images()
{
    UnloadAll();
}