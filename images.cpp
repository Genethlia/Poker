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

    int x = 54, y = 60;

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
        break;
    case 2:
        suitData = hearts_png;
        suitDataLen = hearts_png_len;
        break;
    case 3:
        suitData = clubs_png;
        suitDataLen = clubs_png_len;
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
    ImageResize(&small, x / 2.5, y / 2.5);
    filiTexture = LoadTextureFromImage(small);
    UnloadImage(small);

    UnloadImage(image);
}
void Images::LoadHiddenCard()
{
    if (hiddenCardTexture.id != 0)
    {
        UnloadTexture(hiddenCardTexture);
    }
    Image card = LoadImageFromMemory(".png", card_png, card_png_len);
    if (card.data != nullptr)
    {
        ImageResize(&card, 72, 120);
        hiddenCardTexture = LoadTextureFromImage(card);
        UnloadImage(card);
    }
}

void Images::LoadChatTexture()
{
    if (chatTexture.id != 0)
    {
        UnloadTexture(chatTexture);
    }
    Image chat = LoadImageFromMemory(".png", chat_png, chat_png_len);
    if (chat.data != nullptr)
    {
        ImageResize(&chat, 40, 40);
        chatTexture = LoadTextureFromImage(chat);
        UnloadImage(chat);
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
    if (hiddenCardTexture.id != 0)
    {
        UnloadTexture(hiddenCardTexture);
        hiddenCardTexture = Texture2D{};
    }
    if (chatTexture.id != 0)
    {
        UnloadTexture(chatTexture);
        chatTexture = Texture2D{};
    }
}

Images::~Images()
{
    UnloadAll();
}