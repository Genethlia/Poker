#include "cards.h"

Card::Card(float x, float y, valRank card, Images *suitTextures, Font *font, Images *gameimages, int rotationAngle)
    : suitTextures(suitTextures), font(font), gameimages(gameimages), rotationAngle(rotationAngle)
{
    target = {200 + x, y};
    pos = {700, 20};

    width = 108;
    height = 180;
    this->card = card;

    moving = true;
    facedown = true;
    secret = false;
    firstsecret = true;
    flipping = false;
    flipProgress = 0.0f;

    color[0] = {0, 0, 0, 255};
    color[1] = {255, 0, 0, 255};

    smalloffset = 13;
    switch (card.suit)
    {
    case 1:
        bigoffset = 35;
        smalloffset = 14;
        break;
    case 2:
        bigoffset = 27;
        smalloffset = 13;
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

    bool shouldShowBack = (secret || (facedown && showBack));

    float visibleWidth = width * displayScaleX;
    float leftX = pos.x + (width - visibleWidth) / 2.0f;

    Vector2 center = {pos.x + width / 2.0f, pos.y + height / 2.0f};

    Rectangle destRec = {center.x, center.y, visibleWidth, height};

    Vector2 origin = {visibleWidth / 2.0f, height / 2.0f};

    Rectangle source = {0, 0, (float)gameimages->hiddenCardTexture.width, (float)gameimages->hiddenCardTexture.height};

    if (shouldShowBack)
    {
        DrawTexturePro(gameimages->hiddenCardTexture, source, destRec, origin, rotationAngle, WHITE);
        return;
    }

    int PointerOfcolor = GetColorOfRank(card);
    string rankStr = cardnum(card);

    DrawRectanglePro(destRec, origin, rotationAngle, WHITE);

    float suitOffset = (1.0f - displayScaleX) * width / 3 + 5;

    Vector2 rankTopLeft = {leftX + 5 + suitOffset, pos.y + 5};
    Vector2 rankBottomRight = {leftX + visibleWidth - 5 - suitOffset, pos.y + height - 5};

    Vector2 smallSuitTopLeft = {leftX + smalloffset + suitOffset, pos.y + 48};
    Vector2 smallSuitBottomRight = {leftX + visibleWidth - smalloffset - suitOffset, pos.y + height - 48};

    Vector2 bigSuitPos = {leftX + visibleWidth / 2.0f, pos.y + height / 2.0f};

    rankTopLeft = findCenterToRotate(rankTopLeft, center, rotationAngle);
    rankBottomRight = findCenterToRotate(rankBottomRight, center, rotationAngle);

    smallSuitTopLeft = findCenterToRotate(smallSuitTopLeft, center, rotationAngle);
    smallSuitBottomRight = findCenterToRotate(smallSuitBottomRight, center, rotationAngle);

    bigSuitPos = findCenterToRotate(bigSuitPos, center, rotationAngle);

    DrawTextPro(*font, rankStr.c_str(), rankTopLeft, {0, 0}, rotationAngle, 30, 2, color[PointerOfcolor]);
    DrawTextPro(*font, rankStr.c_str(), rankBottomRight, {0, 0}, 180 + rotationAngle, 30, 2, color[PointerOfcolor]);

    if (suitTextures->filiTexture.id != 0)
    {
        Texture2D tex = suitTextures->filiTexture;
        Rectangle src = {0, 0, (float)tex.width, (float)tex.height};
        Vector2 origin = {float(tex.width) / 2.0f, float(tex.height) / 2.0f};

        Rectangle dst1 = {smallSuitTopLeft.x, smallSuitTopLeft.y, (float)tex.width, (float)tex.height};
        Rectangle dst2 = {smallSuitBottomRight.x, smallSuitBottomRight.y, (float)tex.width, (float)tex.height};

        DrawTexturePro(suitTextures->filiTexture, src, dst1, origin, rotationAngle, WHITE);
        DrawTexturePro(suitTextures->filiTexture, src, dst2, origin, 180 + rotationAngle, WHITE);
    }
    if (suitTextures->bigfiliTexture.id != 0)
    {
        Texture2D tex = suitTextures->bigfiliTexture;
        Rectangle src = {0, 0, (float)tex.width, (float)tex.height};
        Vector2 origin = {float(tex.width) / 2.0f, float(tex.height) / 2.0f};
        Rectangle dst = {bigSuitPos.x, bigSuitPos.y, (float)tex.width, (float)tex.height};

        DrawTexturePro(suitTextures->bigfiliTexture, src, dst, origin, rotationAngle, WHITE);
    }
    // DrawCircle((int)pos.x, (int)pos.y, 5, RED);        // top-left
    // DrawCircle((int)center.x, (int)center.y, 5, BLUE); // center
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

Vector2 Card::findCenterToRotate(Vector2 point, Vector2 center, float angle)
{
    float radians = angle * PI / 180.0f;
    float sinA = sinf(radians);
    float cosA = cosf(radians);

    point.x -= center.x;
    point.y -= center.y;

    float rotatedX = point.x * cosA - point.y * sinA;
    float rotatedY = point.x * sinA + point.y * cosA;

    point.x = rotatedX + center.x;
    point.y = rotatedY + center.y;

    return point;
}
