#include "chat.h"

Chat::Chat()
{
    chatImages = nullptr;
    client = nullptr;

    chatOpen = false;
    typingChat = false;
    countOfNewMessages = 0;
    hasNewMessage = false;
}

void Chat::sendChat(const string &chat)
{
    client->sendChat(chat);
}

void Chat::Init(Images *chatImages, PokerClient *client)
{
    this->chatImages = chatImages;
    this->client = client;
}

void Chat::Update()
{
    if (!client || !chatImages)
        return;
    auto newMessages = client->getAndClearChatMessages();
    if (!newMessages.empty() && chatOpen)
    {
        hasNewMessage = true;
        countOfNewMessages += newMessages.size();
        for (auto &message : newMessages)
        {
            string playerName = client->nameOf(message.playerId);
            chatMessages.push_back(playerName + ": " + message.message);
        }
    }
    UpdateButtons();
}

void Chat::UpdateButtons()
{
    if (!chatImages)
        return;

    if (chatImages->chatTexture.id == 0 || chatImages->xTexture.id == 0)
        return;

    UpdateChatButton();
    UpdateXButton();
}

void Chat::UpdateChatButton()
{
    bool clickedChatButton = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    bool hoveringChatButton = CheckCollisionPointRec(GetVirtualMousePosition(), Rectangle{20, 20, (float)chatImages->chatTexture.width, (float)chatImages->chatTexture.height});
    if (clickedChatButton && hoveringChatButton && !chatOpen)
    {
        chatOpen = true;
        hasNewMessage = false;
        countOfNewMessages = 0;
    }
}

void Chat::UpdateXButton()
{
    bool clickedChatButton = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    bool hoveringChatButton = CheckCollisionPointRec(GetVirtualMousePosition(), Rectangle{380, 20, (float)chatImages->xTexture.width, (float)chatImages->xTexture.height});
    if (clickedChatButton && hoveringChatButton && chatOpen)
    {
        chatOpen = false;
    }
}

void Chat::Draw()
{
    if (!chatImages || !client)
        return;

    if (chatImages->chatTexture.id == 0 || chatImages->xTexture.id == 0)
        return;

    if (!chatOpen)
    {
        DrawTexture(chatImages->chatTexture, 20, 20, WHITE);
        if (countOfNewMessages > 0)
        {
            DrawRectangle(70, 20, 40, 40, RED);
            int textWidth = MeasureText(to_string(countOfNewMessages).c_str(), 30);
            DrawText(to_string(countOfNewMessages).c_str(), 70 + (40 - textWidth) / 2, 27, 30, WHITE);
        }
    }
    else
    {
        DrawRectangle(20, 20, 380, 250, Fade(BLACK, 0.6f));
        DrawRectangle(380, 40, 20, 230, Fade(GRAY, 0.6f));
        DrawTexture(chatImages->xTexture, 400 - (float)chatImages->xTexture.width, 20, WHITE);
    }
}
