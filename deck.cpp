#include "deck.h"

Deck::Deck()
{
    srand(static_cast<unsigned>(time(nullptr)));
    cards = RandomizeDeck();
}

vector<valRank> Deck::CreateDeck()
{
    vector<valRank> tempDeck;
    int rank, value;
    for (int i = 0; i < 4; i++)
    {
        rank = i;
        for (int j = 2; j <= 14; j++)
        {
            value = j;
            tempDeck.push_back({value, rank});
        }
    }
    return tempDeck;
}

vector<valRank> Deck::RandomizeDeck()
{
    vector<valRank> tempDeck;
    tempDeck = CreateDeck();
    for (int i = tempDeck.size() - 1; i > 0; i--)
    {
        int j = rand() % (i + 1);
        swap(tempDeck[i], tempDeck[j]);
    }
    return tempDeck;
}

void Deck::Draw()
{
    for (int i = 0; i < cards.size(); i++)
    {
        cout << cards[i].value << " " << cards[i].suit << endl;
    }
}

valRank Deck::DrawCard()
{
    if (cards.empty())
    {
        cards = RandomizeDeck();
    }
    valRank temp = cards[cards.size() - 1];
    cards.pop_back();
    return temp;
}

void Deck::SaveDeck()
{

    ofstream fout("saves/deck.txt");
    if (fout.is_open())
    {
        for (auto &card : cards)
        {
            fout << card.value << " " << card.suit << endl;
        }
    }
    fout.close();
}

void Deck::LoadDeck()
{
    ifstream fin("saves/deck.txt");
    if (fin.is_open())
    {
        cards.clear();
        valRank temp;
        while (fin >> temp.value >> temp.suit)
        {
            cards.push_back(temp);
        }
    }
    fin.close();
}
