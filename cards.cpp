#include "cards.h"

Card::Card(float x, float y, valRank card, Font *font, vector<valRank> *cards, bool *soundenabled)
    : font(font), cards(cards), soundenabled(soundenabled)
{
    target = {x + 300, y};
    pos = {1100, 300};

    width = 120;
    height = 200;
    this->card = card;

    moving = true;
    facedown = true;
    secret = false;
    firstsecret = true;
    flipping = false;
    flipProgress = 0.0f;

    color[0] = {0, 0, 0, 255};
    color[1] = {255, 0, 0, 255};

    smalloffset = 0;
    switch (card.suit)
    {
    case 1:
        bigoffset = 35;
        break;
    case 2:
        bigoffset = 27;
        smalloffset = 4;
        break;
    case 3:
        bigoffset = 40;
        break;
    default:
        bigoffset = 30;
        break;
    }
}

void Card::Draw()
{
    float scaleX = cos(flipProgress * PI);
    bool showBack = (flipProgress < 0.5f);

    float displayScaleX = abs(scaleX);

    Rectangle source = {0, 0, (float)gameimages->hiddenCardTexture.width, (float)gameimages->hiddenCardTexture.height};
    Rectangle destRec = {pos.x + width * (1 - displayScaleX) / 2, pos.y, width * displayScaleX, (float)height};
    Vector2 origin = {0, 0};

    bool shouldShowBack = (secret || (facedown && showBack));

    if (shouldShowBack)
    {
        DrawTexturePro(gameimages->hiddenCardTexture, source, destRec, origin, 0, WHITE);
        return;
    }

    if (!flipping)
    {
        destRec = {pos.x, pos.y, width, height};
        displayScaleX = 1.0f;
    }

    int PointerOfcolor = GetColorOfRank(card);

    DrawRectanglePro(destRec, origin, 0, WHITE);

    float suitOffset = (1.0f - displayScaleX) * width / 3 + 5;

    DrawTextEx(*font, cardnum(card).c_str(), {pos.x + 5 + suitOffset, pos.y + 5}, 30, 2, color[PointerOfcolor]);
    DrawTextPro(*font, cardnum(card).c_str(), {pos.x + width - 5 - suitOffset, pos.y + height - 5}, {0, 0}, 180, 30, 2, color[PointerOfcolor]);

    if (suitTextures->filiTexture.id != 0)
    {
        DrawTexture(suitTextures->filiTexture, pos.x + smalloffset + suitOffset, pos.y + 30, WHITE);
        DrawTextureEx(suitTextures->filiTexture, {width + pos.x - smalloffset - suitOffset, height + pos.y - 30}, 180, 1, WHITE);
    }
    if (suitTextures->bigfiliTexture.id != 0)
    {
        DrawTexture(suitTextures->bigfiliTexture, pos.x + width / 2 - bigoffset, pos.y + height / 2 - 40, WHITE);
    }
}

void Card::Update()
{
    if (!moving && !firstsecret && !flipping)
        return;

    Vector2 dir = {target.x - pos.x, target.y - pos.y};

    float dist = sqrt(dir.x * dir.x + dir.y * dir.y);

    float speed = dist * 12 * GetFrameTime();
    if (!moving && !secret && firstsecret)
    {
        cards->push_back(card);
        firstsecret = false;
        StartFlip();
        return;
    }
    if (dist <= 0.5f)
    {
        pos = target;
        moving = false;
    }
    else
    {
        dir.x /= dist;
        dir.y /= dist;

        pos.x += dir.x * speed;
        pos.y += dir.y * speed;
    }
    if (flipping)
    {
        flipProgress += GetFrameTime() * 3;
        if (flipProgress >= 1.0f)
        {
            flipProgress = 1.0f;
            flipping = false;
            facedown = false;
        }
    }
}

bool Card::IsMoving()
{
    return moving || flipping;
}

void Card::SetFaceDown(bool v)
{
    secret = v;
}

void Card::GoImmediatelyToTarget()
{
    pos = target;
    moving = false;
}

void Card::StartFlip()
{
    flipping = true;
    flipProgress = 0.0f;
}

string Card::cardnum(valRank card)
{
    if (card.value == 14)
    {
        return "A";
    }
    if (card.value == 11)
    {
        return "J";
    }
    if (card.value == 12)
    {
        return "Q";
    }
    if (card.value == 13)
    {
        return "K";
    }

    return to_string(card.value);
}

int Card::GetColorOfRank(valRank card)
{
    if (card.suit == 1 || card.suit == 3)
    {
        return 0;
    }
    return 1;
}
