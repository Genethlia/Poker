#include "chat.h"

Chat::Chat()
{
    chatImages = nullptr;
    client = nullptr;

    chatOpen = false;
    typingChat = false;
    countOfNewMessages = 0;
    hasNewMessage = false;
    scrollOffset = 0;
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
    scrollbarRec = {378, 50, 12, 170};
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

        bool atBottom = scrollOffset == 0;
        for (auto &message : newMessages)
        {
            string playerName = client->nameOf(message.playerId);
            chatMessages.push_back(playerName + ": " + message.message);
        }

        if (atBottom)
            scrollOffset = 0;
    }

    UpdateButtons();

    if (chatOpen)
    {
        UpdateTyping();
        UpdateScroll();
    }
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

void Chat::UpdateTyping()
{
    Vector2 mouse = GetVirtualMousePosition();
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        typingChat = CheckCollisionPointRec(mouse, inputBox);
    }

    if (!typingChat)
        return;

    int key = GetCharPressed();

    while (key > 0)
    {
        if (key >= 32 && key <= 126)
        {
            if (input.size() <= 35)
            {
                input += static_cast<char>(key);
            }
        }
        key = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE) && !input.empty())
    {
        input.pop_back();
    }

    if (IsKeyPressed(KEY_ENTER))
    {
        if (!input.empty())
        {
            sendChat(input);
            input.clear();
        }
        typingChat = false;
    }
}

void Chat::UpdateScroll()
{
    Vector2 mouse = GetVirtualMousePosition();

    Rectangle messagesArea = {
        panel.x + 10,
        panel.y,
        panel.width - 40,
        inputBox.y - panel.y - 15};

    if (!CheckCollisionPointRec(mouse, messagesArea))
        return;

    float wheel = GetMouseWheelMove();

    if (wheel > 0)
    {
        scrollOffset++;
    }
    else if (wheel < 0)
    {
        scrollOffset--;
    }

    int fontSize = 18;
    int spacing = 6;
    int messageHeight = fontSize + spacing;

    int maxVisibleMessages = (int)(messagesArea.height / messageHeight);

    int maxScroll = std::max(0, (int)chatMessages.size() - maxVisibleMessages);

    if (scrollOffset < 0)
        scrollOffset = 0;

    if (scrollOffset > maxScroll)
        scrollOffset = maxScroll;
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
        DrawTexture(chatImages->xTexture, 400 - (float)chatImages->xTexture.width, 20, WHITE);

        DrawScrollBar();
        DrawMessages();
        DrawInput();
    }
}

void Chat::DrawMessages()
{
    int fontSize = 18;
    int spacing = 6;
    int messageHeight = fontSize + spacing;

    Rectangle messagesArea = {
        panel.x + 10,
        panel.y + 5,
        panel.width - 45,
        inputBox.y - panel.y - 15};

    BeginScissorMode(
        (int)messagesArea.x,
        (int)messagesArea.y,
        (int)messagesArea.width,
        (int)messagesArea.height);

    int maxVisibleMessages = (int)(messagesArea.height / messageHeight);

    int totalMessages = (int)chatMessages.size();

    int start = std::max(0, totalMessages - maxVisibleMessages - scrollOffset);

    int end = std::min(totalMessages, start + maxVisibleMessages);

    float x = messagesArea.x + 5;
    float y = messagesArea.y + 5;

    for (int i = start; i < end; i++)
    {
        DrawTextEx(*mainFont, chatMessages[i].c_str(), {x, y}, fontSize, 1, WHITE);
        y += messageHeight;
    }

    EndScissorMode();

    DrawRectangleLinesEx(messagesArea, 1, Fade(WHITE, 0.15f));
}

void Chat::DrawInput()
{
    Color inputColor = typingChat ? Fade(WHITE, 0.85f) : Fade(WHITE, 0.6f);

    DrawRectangleRec(inputBox, inputColor);
    DrawRectangleLinesEx(inputBox, 2, typingChat ? YELLOW : Fade(GRAY, 0.8f));

    std::string textToDraw = input;

    if (typingChat && ((int)(GetTime() * 2) % 2 == 0))
        textToDraw += "|";

    DrawTextEx(*mainFont, textToDraw.empty() && !typingChat ? "Type message..." : textToDraw.c_str(), {inputBox.x + 8, inputBox.y + 7}, 18, 1, textToDraw.empty() && !typingChat ? GRAY : BLACK);
}

void Chat::DrawScrollBar()
{
    int fontSize = 18;
    int spacing = 6;
    int messageHeight = fontSize + spacing;

    Rectangle messagesArea = {
        panel.x + 10,
        panel.y + 45,
        panel.width - 45,
        inputBox.y - panel.y - 55};

    int maxVisibleMessages = (int)(messagesArea.height / messageHeight);
    int maxScroll = std::max(0, (int)chatMessages.size() - maxVisibleMessages);

    DrawRectangleRec(scrollbarRec, Fade(GRAY, 0.35f));

    if (maxScroll > 0)
    {
        float thumbHeight = std::max(25.0f, scrollbarRec.height * ((float)maxVisibleMessages / chatMessages.size()));

        float scrollPercent = (float)scrollOffset / maxScroll;

        float thumbY = scrollbarRec.y + (scrollbarRec.height - thumbHeight) * (1.0f - scrollPercent);

        Rectangle thumb = {
            scrollbarRec.x + 4,
            thumbY,
            scrollbarRec.width - 8,
            thumbHeight};

        DrawRectangleRounded(thumb, 0.4f, 6, Fade(WHITE, 0.65f));
    }
}
