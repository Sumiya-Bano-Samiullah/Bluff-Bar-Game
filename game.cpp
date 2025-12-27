#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <limits>    // for numeric_limits
#include <exception> // for std::exception
#include <unordered_map>
#include <random>

using namespace std;

/* 
   TEMPLATE: Deck<T>
   Allows any card type (string, int, structs, etc.)
    */
   
template<typename T>
class Deck {
private:
    vector<T> cards;

public:
    Deck() {
        reset();
    }

    void reset() {
        cards.clear();

        // Fixed card distribution (still using string type)
        for (int i = 0; i < 6; ++i) cards.push_back("Sun");
        for (int i = 0; i < 6; ++i) cards.push_back("Star");
        for (int i = 0; i < 6; ++i) cards.push_back("Moon");
        for (int i = 0; i < 2; ++i) cards.push_back("Magic");

        static std::mt19937 rng((unsigned)time(nullptr));
        std::shuffle(cards.begin(), cards.end(), rng);
    }

    vector<T> deal(int n) {
        vector<T> hand;
        for (int i = 0; i < n && !cards.empty(); ++i) {
            hand.push_back(cards.back());
            cards.pop_back();
        }
        return hand;
    }
};

/* 
   TEMPLATE: Player<T>
   Player now stores a hand of ANY card type
    */
template<typename T>
class Player {
private:
    string name;
    vector<T> hand;
    bool alive;

public:
    Player(const string& n) : name(n), alive(true) {}

    string getName() const { return name; }
    bool isAlive() const { return alive; }
    void setAlive(bool status) { alive = status; }

    void setHand(const vector<T>& newHand) {
        hand = newHand;
    }

    const vector<T>& getHand() const {
        return hand;
    }

    void removeCardAt(int idx) {
        if (idx >= 0 && idx < (int)hand.size())
            hand.erase(hand.begin() + idx);
    }

    // Play up to n cards from the back (existing behavior)
    vector<T> playCards(int n) {
        vector<T> played;
        for (int i = 0; i < n && !hand.empty(); ++i) {
            played.push_back(hand.back());
            hand.pop_back();
        }
        return played;
    }

    void showHand() const {
        cout << name << ": ";
        for (const T& c : hand)
            cout << c << " ";
        cout << endl;
    }
};

/* 
   Game Class (NOT template – uses Player<string> & Deck<string>)
    */
class Game {
private:
    vector<Player<string>> players;
    int currentPlayerIndex;
    Deck<string> deck;
    unordered_map<string, int> surviveCount;  // Tracks number of survivals after questioning

    // store last played cards for each player (hidden until questioning)
    vector<vector<string>> lastPlayedByIndex;

    static string randomFocusCard() {
        static vector<string> cards = {"Sun", "Moon", "Star"};
        return cards[rand() % cards.size()];
    }

    // Treat empty played vector as incorrect/unknown (so forced questioning is meaningful)
    static bool playIsCorrect(const vector<string>& played, const string& focus) {
        if (played.empty()) return false;
        for (const string& c : played) {
            if (c != focus && c != "Magic") {
                return false;
            }
        }
        return true;
    }

    // Safe next alive player WITH cards. Returns -1 if none found.
    int getNextAlivePlayer(int start) {
        int n = players.size();
        if (n == 0) return -1;
        for (int i = 1; i <= n; ++i) {
            int idx = (start + i) % n;
            if (players[idx].isAlive() && !players[idx].getHand().empty())
                return idx;
        }
        return -1; // none available
    }

    int countAlivePlayers() {
        return count_if(players.begin(), players.end(),
                        [](const Player<string>& p){ return p.isAlive(); });
    }

    int countAliveWithCards() {
        return count_if(players.begin(), players.end(),
                        [](const Player<string>& p){ return p.isAlive() && !p.getHand().empty(); });
    }

    void dealCardsToAlive(int cardsPerPlayer) {
        for (auto& p : players) {
            if (p.isAlive()) {
                p.setHand(deck.deal(cardsPerPlayer));
            }
        }
    }

    // Show human's hand only (we keep human visibility A)
    void showHumanHand() {
        for (const auto& p : players) {
            if (p.getName() == "Human" && p.isAlive()) {
                cout << "--- Your Hand ---\n";
                p.showHand();
                cout << endl;
                return;
            }
        }
    }

    int findPlayerIndex(const string& name) {
        for (int i = 0; i < (int)players.size(); ++i)
            if (players[i].getName() == name)
                return i;
        return -1;
    }

    bool handleQuestioning(Player<string>& questioner,
                           Player<string>& playerWhoPlayed,
                           const vector<string>& played,
                           const string& focus)
    {
        // Reveal the played cards (since a question occurred)
        cout << "\nRevealing cards of " << playerWhoPlayed.getName() << ": ";
        if (played.empty()) {
            cout << "(no record of played cards)\n";
        } else {
            for (auto& c : played)
                cout << c << " ";
            cout << "\n";
        }

        bool correctPlay = playIsCorrect(played, focus);

        if (!correctPlay) {
            cout << playerWhoPlayed.getName() << " played wrongly!\n";
            cout << questioner.getName() << " was right to question!\n";

            if (surviveCount[playerWhoPlayed.getName()] >= 2) {
                cout << "Bomb exploded! " << playerWhoPlayed.getName() << " has died (3rd time bomb)!\n";
                playerWhoPlayed.setAlive(false);
                surviveCount[playerWhoPlayed.getName()] = 0;
            }
            else if (rand() % 3 == 0) {
                cout << "Bomb exploded! " << playerWhoPlayed.getName() << " has died.\n";
                playerWhoPlayed.setAlive(false);
                surviveCount[playerWhoPlayed.getName()] = 0;
            } else {
                cout << "Bomb did not explode this time! "
                     << playerWhoPlayed.getName() << " has survived.\n";
                surviveCount[playerWhoPlayed.getName()]++;
            }

            int idx = findPlayerIndex(questioner.getName());
            if (idx != -1) currentPlayerIndex = idx;
            else {
                int fallback = getNextAlivePlayer(-1);
                currentPlayerIndex = (fallback != -1 ? fallback : 0);
            }
            return true;
        }
        else {
            cout << questioner.getName() << " was wrong to question!\n";

            if (surviveCount[questioner.getName()] >= 2) {
                cout << "Bomb exploded! " << questioner.getName() << " has died (3rd time bomb)!\n";
                questioner.setAlive(false);
                surviveCount[questioner.getName()] = 0;
            }
            else if (rand() % 3 == 0) {
                cout << "Bomb exploded! " << questioner.getName() << " has died.\n";
                questioner.setAlive(false);
                surviveCount[questioner.getName()] = 0;
            } else {
                cout << "Bomb did not explode this time! "
                     << questioner.getName() << " has survived\n";
                surviveCount[questioner.getName()]++;
            }

            int idx = findPlayerIndex(questioner.getName());
            if (idx != -1) currentPlayerIndex = idx;
            else {
                int fallback = getNextAlivePlayer(-1);
                currentPlayerIndex = (fallback != -1 ? fallback : 0);
            }
            return true;
        }
    }

public:
    Game() {
        players.emplace_back("Human");
        players.emplace_back("Bot1");
        players.emplace_back("Bot2");
        players.emplace_back("Bot3");

        currentPlayerIndex = rand() % players.size();

        // Initialize surviveCount map for all players
        for (auto& p : players)
            surviveCount[p.getName()] = 0;

        // initialize last played storage
        lastPlayedByIndex.resize(players.size());
    }

    void play() {
        deck.reset();
        dealCardsToAlive(5);

        // show only human hand (A option)
        showHumanHand();

        cout << "First player: " << players[currentPlayerIndex].getName() << "\n\n";

        // main loop: use countAliveWithCards to ensure someone can act
        while (countAliveWithCards() > 1) {
            string focus = randomFocusCard();
            cout << "--- Round begins! Focus card: " << focus << " ---\n";

            bool roundOver = false;
            bool anyQuestionAsked = false;

            while (!roundOver) {
                if (countAliveWithCards() <= 1) {
                    // No one left who can play; end round safely
                    break;
                }

                Player<string>& currentPlayer = players[currentPlayerIndex];

                // Skip player if dead or no cards left
                if (!currentPlayer.isAlive() || currentPlayer.getHand().empty()) {
                    int nxt = getNextAlivePlayer(currentPlayerIndex);
                    if (nxt == -1) { roundOver = true; break; }
                    currentPlayerIndex = nxt;
                    continue;
                }

                /* 
                   HUMAN TURN with exception handling
                    */
                if (currentPlayer.getName() == "Human") {
                    cout << "Your hand:\n";
                    const auto& hand = currentPlayer.getHand();
                    for (int i = 0; i < (int)hand.size(); ++i)
                        cout << i+1 << ": " << hand[i] << "  ";

                    int n;
                    while (true) {
                        try {
                            cout << "\nHow many cards you want to play (1-3)? ";
                            cin >> n;

                            if (cin.fail()) {
                                cin.clear();
                                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                                throw runtime_error("Invalid input! Please enter an integer.");
                            }

                            if (n < 1 || n > 3) {
                                throw out_of_range("Number of cards must be between 1 and 3.");
                            }
                            break;
                        } catch (const exception& e) {
                            cout << e.what() << "\nTry again.\n";
                        }
                    }

                    vector<int> chosen;

                    while ((int)chosen.size() < n) {
                        try {
                            cout << "Enter index #" << chosen.size() + 1 << ": ";
                            int idx;
                            cin >> idx;

                            if (cin.fail()) {
                                cin.clear();
                                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                                throw runtime_error("Invalid input! Please enter an integer.");
                            }

                            idx--; // zero-based indexing

                            if (idx < 0 || idx >= (int)hand.size())
                                throw out_of_range("Index out of range.");

                            if (find(chosen.begin(), chosen.end(), idx) != chosen.end())
                                throw logic_error("Index already chosen.");

                            chosen.push_back(idx);
                        } catch (const exception& e) {
                            cout << e.what() << "\nTry again.\n";
                        }
                    }

                    sort(chosen.rbegin(), chosen.rend());

                    vector<string> played;
                    for (int idx : chosen) {
                        played.push_back(hand[idx]);
                        currentPlayer.removeCardAt(idx);
                    }
                    reverse(played.begin(), played.end());

                    // Store played secretly (indexed by player index)
                    {
                        int curIdx = findPlayerIndex(currentPlayer.getName());
                        if (curIdx != -1) lastPlayedByIndex[curIdx] = played;
                    }

                    // DO NOT reveal which cards — only show count
                    cout << "Human played " << played.size() << " card(s).\n";

                    int next = getNextAlivePlayer(currentPlayerIndex);
                    if (next == -1) { roundOver = true; break; }
                    auto& nextP = players[next];

                    if (nextP.getName().find("Bot") != string::npos) {
                        if ((rand() % 100) < 30) {  // 30% chance to question
                            cout << nextP.getName() << " decides to question!\n";
                            // Reveal player's last played cards to the questioning logic
                            int playedOwnerIdx = findPlayerIndex(currentPlayer.getName());
                            vector<string> toCheck;
                            if (playedOwnerIdx != -1) toCheck = lastPlayedByIndex[playedOwnerIdx];

                            roundOver = handleQuestioning(nextP, currentPlayer, toCheck, focus);
                            anyQuestionAsked = true;
                        } else {
                            cout << nextP.getName() << " decides NOT to question.\n";
                        }
                    }
                }

                /* 
                   BOT TURN
                    */
                else {
                    int n = rand() % 3 + 1;
                    vector<string> played = currentPlayer.playCards(n);

                    // Store secretly for later reveal if questioned
                    {
                        int curIdx = findPlayerIndex(currentPlayer.getName());
                        if (curIdx != -1) lastPlayedByIndex[curIdx] = played;
                    }

                    // DO NOT print the cards themselves — only number
                    cout << currentPlayer.getName() << " has played " << played.size() << " card(s) (hidden).\n";

                    int next = getNextAlivePlayer(currentPlayerIndex);
                    if (next == -1) { roundOver = true; break; }
                    auto& nextP = players[next];

                    if (nextP.getName() == "Human") {
                        cout << "Question previous player (y/n)? ";
                        char ch; cin >> ch;

                        if (ch == 'y' || ch == 'Y') {
                            int ownerIdx = findPlayerIndex(currentPlayer.getName());
                            vector<string> toCheck;
                            if (ownerIdx != -1) toCheck = lastPlayedByIndex[ownerIdx];

                            roundOver = handleQuestioning(nextP, currentPlayer, toCheck, focus);
                            anyQuestionAsked = true;
                        } else {
                            cout << "Human decided NOT to question.\n";
                        }
                    }
                    else {
                        if ((rand() % 100) < 30) {  // 30% chance to question
                            cout << nextP.getName() << " decides to question!\n";
                            int ownerIdx = findPlayerIndex(currentPlayer.getName());
                            vector<string> toCheck;
                            if (ownerIdx != -1) toCheck = lastPlayedByIndex[ownerIdx];

                            roundOver = handleQuestioning(nextP, currentPlayer, toCheck, focus);
                            anyQuestionAsked = true;
                        }
                        else {
                            cout << nextP.getName() << " decides NOT to question.\n";
                        }
                    }
                }

                // *** UPDATED LOGIC: forced questioning when 2 players left alive AND previous player has no cards ***
                if (!roundOver) {
                    int alivePlayers = countAlivePlayers();

                    if (!anyQuestionAsked && alivePlayers == 2) {
                        int current = currentPlayerIndex;
                        int next = getNextAlivePlayer(current);
                        if (next == -1) { roundOver = true; break; }

                        auto& questioner = players[next];
                        auto& previous = players[current];

                        // Only force question if previous player has NO cards left
                        if (previous.getHand().empty()) {
                            cout << questioner.getName() << " is forced to question!\n";

                            int prevIdx = findPlayerIndex(previous.getName());
                            vector<string> played;
                            if (prevIdx != -1) played = lastPlayedByIndex[prevIdx];

                            roundOver = handleQuestioning(questioner, previous, played, focus);
                            anyQuestionAsked = true;
                        }
                    }
                }

                if (!roundOver) {
                    int nxt = getNextAlivePlayer(currentPlayerIndex);
                    if (nxt == -1) { roundOver = true; break; }
                    currentPlayerIndex = nxt;
                }
            } // end inner round loop

            cout << "\nROUND OVER re-dealing cards.\n\n";
            deck.reset();
            dealCardsToAlive(5);

            // show only human hand (do not reveal others)
            showHumanHand();
            anyQuestionAsked = false;  // Reset for next round
        } // end outer loop

        for (auto& p : players)
            if (p.isAlive()) {
                cout << p.getName() << " wins!\n";
                break;
            }
    }
};

int main() {
    srand(time(0));
    Game game;
    game.play();
    return 0;
}
