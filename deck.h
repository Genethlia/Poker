#pragma once
#include "visual.hpp"
using namespace std;

class Deck
{
private:
    vector<valRank> cards; // The deck of cards
public:
    Deck();
    vector<valRank> CreateDeck();    // Creates a standard deck of 52 cards
    vector<valRank> RandomizeDeck(); // Shuffles the deck
    void Draw();                     // Just for testing
    valRank DrawCard();              // Draws a card from the deck
    void SaveDeck();
    void LoadDeck();
};
