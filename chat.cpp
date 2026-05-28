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

void Chat::Init(Images *chatImages, PokerClient *client, Font *mainFont)
{
    this->chatImages = chatImages;
    this->client = client;
    this->mainFont = mainFont;

    countOfNewMessagesRec = {70, 20, 40, 40};
    chatOpenRec = Rectangle{20, 20, (float)chatImages->chatTexture.width, (float)chatImages->chatTexture.height};
    chatCloseRec = Rectangle{380, 20, (float)chatImages->xTexture.width, (float)chatImages->xTexture.height};
    panel = {20, 20, 380, 250};
    scrolbarRec = {380, 40, 20, 230};
    inputBox = {30, 230, 340, 30};
}

void Chat::Update()
{
    if (!client || !chatImages)
        return;
    auto newMessages = client->getAndClearChatMessages();
    if (!newMessages.empty())
    {
        if (!chatOpen)
        {
            hasNewMessage = true;
            countOfNewMessages += newMessages.size();
        }
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
    bool hoveringChatButton = CheckCollisionPointRec(GetVirtualMousePosition(), chatOpenRec);
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
    bool hoveringChatButton = CheckCollisionPointRec(GetVirtualMousePosition(), chatCloseRec);
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

            DrawRectangleRec(countOfNewMessagesRec, RED);
            DrawTextCentered(countOfNewMessagesRec, to_string(countOfNewMessages).c_str(), 30, *mainFont, WHITE);
        }
    }
    else
    {
        DrawRectangleRec(panel, Fade(BLACK, 0.6f));
        DrawRectangleRec(scrolbarRec, Fade(GRAY, 0.6f));
        DrawRectangleRec(inputBox, Fade(WHITE, 0.6f));
        DrawTexture(chatImages->xTexture, 400 - (float)chatImages->xTexture.width, 20, WHITE);
    }
}
