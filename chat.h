#pragma once
#include "client_in_client.hpp"
#include "raylib_helpers.h"
class Chat
{
public:
    Chat();
    ~Chat() = default;
    void sendChat(const string &chat);
    void Init(Images *chatImages, PokerClient *client);
    void Update();
    void UpdateButtons();
    void UpdateChatButton();
    void UpdateXButton();
    void Draw();

private:
    Images *chatImages;
    PokerClient *client;
    bool chatOpen;
    bool typingChat;
    bool hasNewMessage;
    int countOfNewMessages;
    vector<string> chatMessages;
};