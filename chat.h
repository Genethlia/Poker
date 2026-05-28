#pragma once
#include "client_in_client.hpp"
#include "raylib_helpers.h"
class Chat
{
public:
    Chat();
    ~Chat() = default;
    void sendChat(const std::string &chat);
    void Init(Images *chatImages, PokerClient *client, Font *mainFont);
    void Update();
    void UpdateButtons();
    void UpdateChatButton();
    void UpdateXButton();
    void Draw();

private:
    Images *chatImages;
    PokerClient *client;
    Font *mainFont;

    bool chatOpen;
    bool typingChat;
    bool hasNewMessage;
    int countOfNewMessages;
    std::string input;
    vector<std::string> chatMessages;

    Rectangle countOfNewMessagesRec;
    Rectangle chatOpenRec;
    Rectangle chatCloseRec;
    Rectangle panel;
    Rectangle scrolbarRec;
    Rectangle inputBox;
};