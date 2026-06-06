#pragma once
#include <utility>
#include <string>
#include <vector>
#include <random>
#include <iostream>
#include <fstream>
#include <array>
#include <algorithm>
#include <unordered_map>
#include "poker_networking.hpp"

using namespace std;

// Struct to hold card value and rank

static inline int valueOf(valRank card) { return card.value; }
static inline int suitOf(valRank card) { return card.suit; }

using Score = array<int, 6>;

static bool scoreLess(const Score &a, const Score &b)
{
    return a < b;
}

static int straightHighFromValues(vector<int> r)
{
    sort(r.begin(), r.end());
    r.erase(unique(r.begin(), r.end()), r.end());

    if ((int)r.size() != 5)
        return 0;

    if (r == vector<int>{2, 3, 4, 5, 14})
        return 5; // A-2-3-4-5 straight

    for (int i = 0; i < 4; i++)
    {
        if (r[i + 1] != r[i] + 1)
            return 0;
    }
    return r[4];
}

static Score score5(const array<valRank, 5> &hand)
{
    vector<int> value;
    value.reserve(5);
    vector<int> suit;
    suit.reserve(5);

    for (auto &card : hand)
    {
        value.push_back(valueOf(card));
        suit.push_back(suitOf(card));
    }

    bool flush = all_of(suit.begin() + 1, suit.end(), [&](int r)
                        { return r == suit[0]; });

    int straightHigh = straightHighFromValues(value);
    bool straight = (straightHigh != 0);

    unordered_map<int, int> valueCount;
    for (int v : value)
        valueCount[v]++;

    vector<pair<int, int>> groups;
    for (auto &[v, c] : valueCount)
        groups.push_back({c, v});

    sort(groups.begin(), groups.end(), [](auto a, auto b)
         {
             if (a.first != b.first)
                 return a.first > b.first; // Sort by count descending
             return a.second > b.second;   // Then by value descending
         });

    vector<int> valueSorted = value;
    sort(valueSorted.begin(), valueSorted.end(), greater<int>());

    if (straight && flush && straightHigh == 14)
        return Score{10, straightHigh, 0, 0, 0, 0}; // Royal Flush
    if (straight && flush)
        return Score{9, straightHigh, 0, 0, 0, 0}; // Straight Flush
    if (groups.size() == 2 && groups[0].first == 4)
        return Score{8, groups[0].second, groups[1].second, 0, 0, 0}; // Four of a Kind
    if (groups.size() == 2 && groups[0].first == 3 && groups[1].first == 2)
        return Score{7, groups[0].second, groups[1].second, 0, 0, 0}; // Full House
    if (flush)
        return Score{6, valueSorted[0], valueSorted[1], valueSorted[2], valueSorted[3], valueSorted[4]}; // Flush
    if (straight)
        return Score{5, straightHigh, 0, 0, 0, 0}; // Straight
    if (groups.size() == 3 && groups[0].first == 3)
    {
        int trip = groups[0].second;
        vector<int> kickers;
        for (auto &g : groups)
            if (g.first == 1)
                kickers.push_back(g.second);
        sort(kickers.begin(), kickers.end(), greater<int>());
        return Score{4, trip, kickers[0], kickers[1], 0, 0}; // Three of a Kind
    }
    if (groups.size() == 3 && groups[0].first == 2 && groups[1].first == 2)
    {
        int highPair = max(groups[0].second, groups[1].second);
        int lowPair = min(groups[0].second, groups[1].second);
        int kicker = 0;
        for (auto &g : groups)
            if (g.first == 1)
                kicker = g.second;
        return Score{3, highPair, lowPair, kicker, 0, 0}; // Two Pair
    }
    if (groups.size() == 4 && groups[0].first == 2)
    {
        int pair = groups[0].second;
        vector<int> kickers;
        for (auto &g : groups)
            if (g.first == 1)
                kickers.push_back(g.second);
        sort(kickers.begin(), kickers.end(), greater<int>());
        return Score{2, pair, kickers[0], kickers[1], kickers[2], 0}; // One Pair
    }

    return Score{1, valueSorted[0], valueSorted[1], valueSorted[2], valueSorted[3], valueSorted[4]}; // High Card
}

static Score bestof7(const array<valRank, 7> &hand)
{
    Score bestScore{0, 0, 0, 0, 0, 0};

    for (int a = 0; a < 7; a++)
    {
        for (int b = a + 1; b < 7; b++)
        {
            array<valRank, 5> combo;
            int idx = 0;
            for (int i = 0; i < 7; i++)
            {
                if (i == a || i == b)
                    continue;

                combo[idx++] = hand[i];
            }
            Score score = score5(combo);
            if (scoreLess(bestScore, score))
                bestScore = score;
        }
    }
    return bestScore;
}

inline vector<int> determine_winner(const vector<hand> &playerHand, const vector<valRank> &communityCards, int &winPower)
{
    vector<Score> bestScores(playerHand.size());
    for (size_t i = 0; i < playerHand.size(); i++)
    {
        array<valRank, 7> cards7;

        cards7[0] = playerHand[i].first;
        cards7[1] = playerHand[i].second;
        for (int j = 0; j < 5; j++)
            cards7[2 + j] = communityCards[j];

        bestScores[i] = bestof7(cards7);
    }

    Score best = bestScores[0];

    for (size_t i = 1; i < bestScores.size(); i++)
    {
        if (scoreLess(best, bestScores[i]))
        {
            best = bestScores[i];
        }
    }
    vector<int> winners;
    for (size_t i = 0; i < bestScores.size(); i++)
    {
        if (bestScores[i] == best)
        {
            winners.push_back(int(i));
        }
    }
    winPower = best[0];
    if (winners.size() == 1)
    {
        cout << "Player " << winners[0] + 1 << " wins with score: " << best[0] << endl;
    }
    else
    {
        cout << "Players ";
        for (size_t i = 0; i < winners.size(); i++)
        {
            cout << winners[i] + 1;
            if (i < winners.size() - 1)
                cout << ", ";
        }
    }
    return winners;
}

static Score scoreLessThan5(const vector<valRank> cards)
{
    if (cards.empty())
        return Score{0, 0, 0, 0, 0, 0};

    unordered_map<int, int> valueCount;

    for (auto &card : cards)
    {
        valueCount[valueOf(card)]++;
    }

    vector<pair<int, int>> groups;

    for (auto &[value, count] : valueCount)
    {
        groups.push_back({count, value});
    }

    sort(groups.begin(), groups.end(), [](auto a, auto b)
         {
             if (a.first != b.first)
                 return a.first > b.first; // bigger group first

             return a.second > b.second; // higher value first
         });

    vector<int> kickers;

    for (auto &g : groups)
    {
        if (g.first == 1)
            kickers.push_back(g.second);
    }

    sort(kickers.begin(), kickers.end(), greater<int>());

    if (groups[0].first == 4)
    {
        return Score{8, groups[0].second, 0, 0, 0, 0};
    }

    if (groups[0].first == 3)
    {
        int k1 = kickers.size() > 0 ? kickers[0] : 0;
        return Score{4, groups[0].second, k1, 0, 0, 0};
    }

    if (groups.size() >= 2 && groups[0].first == 2 && groups[1].first == 2)
    {
        int highPair = max(groups[0].second, groups[1].second);
        int lowPair = min(groups[0].second, groups[1].second);

        return Score{3, highPair, lowPair, 0, 0, 0};
    }

    if (groups[0].first == 2)
    {
        int k1 = kickers.size() > 0 ? kickers[0] : 0;
        int k2 = kickers.size() > 1 ? kickers[1] : 0;

        return Score{2, groups[0].second, k1, k2, 0, 0};
    }

    // High card
    vector<int> values;

    for (auto &card : cards)
    {
        values.push_back(valueOf(card));
    }

    sort(values.begin(), values.end(), greater<int>());

    Score score{1, 0, 0, 0, 0, 0};

    for (int i = 0; i < values.size() && i < 5; i++)
    {
        score[i + 1] = values[i];
    }

    return score;
}

static Score scoreVisibleCards(const vector<valRank> cards)
{
    if (cards.size() < 5)
    {
        return scoreLessThan5(cards);
    }

    if (cards.size() == 5)
    {
        array<valRank, 5> hand5;

        for (int i = 0; i < 5; i++)
        {
            hand5[i] = cards[i];
        }

        return score5(hand5);
    }

    Score bestScore{0, 0, 0, 0, 0, 0};

    for (int a = 0; a < cards.size() - 4; a++)
    {
        for (int b = a + 1; b < cards.size() - 3; b++)
        {
            for (int c = b + 1; c < cards.size() - 2; c++)
            {
                for (int d = c + 1; d < cards.size() - 1; d++)
                {
                    for (int e = d + 1; e < cards.size(); e++)
                    {
                        array<valRank, 5> combo = {
                            cards[a],
                            cards[b],
                            cards[c],
                            cards[d],
                            cards[e]};

                        Score score = score5(combo);

                        if (scoreLess(bestScore, score))
                        {
                            bestScore = score;
                        }
                    }
                }
            }
        }
    }

    return bestScore;
}

static std::string scoreName(const int score)
{
    switch (score)
    {
    case 10:
        return "Royal Flush";
    case 9:
        return "Straight Flush";
    case 8:
        return "Four of a Kind";
    case 7:
        return "Full House";
    case 6:
        return "Flush";
    case 5:
        return "Straight";
    case 4:
        return "Three of a Kind";
    case 3:
        return "Two Pair";
    case 2:
        return "One Pair";
    case 1:
        return "High Card";
    default:
        return "";
    }
}