#include <cstring>
#include <vector>
#include <map>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <climits>
#include <ctime>

#define WIDTH  30
#define HEIGHT 20
#define MAX_X  (WIDTH - 1)
#define MAX_Y  (HEIGHT - 1)
#define PLAYERS 4
#define TIME_LIMIT 90
#define LOWER_TIME_LIMIT 60

using namespace std;

const char* RIGHT = "RIGHT";
const char* LEFT = "LEFT";
const char* DOWN = "DOWN";
const char* UP = "UP";

const char* const dirs[] = {RIGHT, LEFT, DOWN, UP};

const int xOffsets[] = {1, -1, 0, 0};
const int yOffsets[] = {0, 0, 1, -1};

#define CLOCKS_PER_MS (CLOCKS_PER_SEC / 1000)

long millis() {
    return clock() / CLOCKS_PER_MS;
}

class Player {
public:
    int x;
    int y;

    Player() {
        x = -1;
        y = -1;
    }

    Player(int p_x, int p_y) {
        x = p_x;
        y = p_y;
    }
};

class State {
private:
    unsigned char grid[WIDTH][HEIGHT];

public:
    int numPlayers;
    int thisPlayer;
    int maxDepth;
    int pruneMargin;
    bool pruningEnabled;
    int nodesSearched;
    bool timeLimitEnabled;
    long startTime;
    bool timeLimitReached;
    Player players[PLAYERS];
    // this is a bitmask of living players
    unsigned char alive;
    int deathCount;
    // a list of the players who have died, in chronological order of death
    int deadList[PLAYERS];

    State() {
        memset(grid, 0, WIDTH * HEIGHT * sizeof(char));
        maxDepth = 9;
        pruneMargin = 0;
        pruningEnabled = true;
        nodesSearched = 0;
        timeLimitEnabled = true;
        alive = 255;
        deathCount = 0;
        resetTimer();
    }

    inline void resetTimer() {
        startTime = millis();
        timeLimitReached = false;
    }

    inline bool isTimeLimitReached() {
        if (!timeLimitEnabled) {
            return false;
        } else if (timeLimitReached) {
            return true;
        } else if (millis() - startTime >= TIME_LIMIT) {
            timeLimitReached = true;
            return true;
        } else {
            return false;
        }
    }

    inline bool occupied(int x, int y) const {
        if (x < 0 || y < 0 || x > MAX_X || y > MAX_Y) {
            return true;
        }
        return grid[x][y] & alive;
    }

    inline void occupy(int x, int y, int player) {
        players[player].x = x;
        players[player].y = y;

        grid[x][y] |= (1 << player);
    }

    inline void unoccupy(int x, int y, int player) {
        grid[x][y] &= ~(1 << player);
    }

    inline void clear(int x, int y) {
        grid[x][y] = 0;
    }

    inline void kill(int player) {
        alive &= ~(1 << player);
        deadList[deathCount++] = player;
    }

    inline void revive(int player) {
        alive |= (1 << player);
        deathCount--;
    }

    inline bool isAlive(int player) const {
        return alive & (1 << player);
    }

    inline int livingCount() {
        return numPlayers - deathCount;
    }

    inline int x() const {
        return players[thisPlayer].x;
    }

    inline int y() const {
        return players[thisPlayer].y;
    }

    inline void readTurn(istream& is) {
        is >> numPlayers;
        is >> thisPlayer;

        for (int i = 0; i < numPlayers; i++) {
            int tailX;
            int tailY;
            int headX;
            int headY;

            is >> tailX;
            is >> tailY;
            is >> headX;
            is >> headY;

            if (players[i].x == headX && players[i].y == headY) {
                kill(i);
            } else {
                occupy(headX, headY, i);
            }
        } 
    }

    void print() {
        for (int y = -1; y <= HEIGHT; y++) {
            for (int x = -1; x <= WIDTH; x++) {
                int player = -1;
                for (int i = 0; i < numPlayers; i++) {
                    if (x == players[i].x && y == players[i].y) {
                        player = i;
                        break;
                    }
                }
                if (player >= 0) {
                    cout << player;
                } else if (occupied(x, y)) {
                    cout << '#';
                } else {
                    cout << ' ';
                }
            }
            cout << endl;
        }
    }
};

class Vor {
public:
    unsigned char player;
    unsigned char distance;
};

class Coord {
public:
    unsigned char x;
    unsigned char y;
};

class Voronoi {
private:
    int sizes[PLAYERS];
    int regions[PLAYERS];
    Vor grid[WIDTH][HEIGHT];
    Coord openNodes[10000];
    int listEnd;

    void clear() {
        memset(grid, 255, WIDTH * HEIGHT * sizeof(Vor));
        memset(sizes, 0, PLAYERS * sizeof(int));
        listEnd = 0;
    }

    inline void addNode(int x, int y) {
        openNodes[listEnd].x = x;
        openNodes[listEnd].y = y;
        listEnd++;
    }

public:
    void calculate(const State& state) {
        clear();

        for (int i = 0; i < state.numPlayers; i++) {
            if (state.isAlive(i)) {
                const Player* player = state.players + i;
                grid[player->x][player->y].player = i;
                grid[player->x][player->y].distance = 0;
                addNode(player->x, player->y);
            }
            regions[i] = i;
        }

        for (int i = 0; i < listEnd; i++) {
            int x = openNodes[i].x;
            int y = openNodes[i].y;
            Vor* vor = &grid[x][y];
            if (vor->player == 254) {
                continue;
            }

            for (int j = 0; j < 4; j++) {
                int xx = x + xOffsets[j];
                int yy = y + yOffsets[j];
                if (!state.occupied(xx, yy)) {
                    Vor* neighbour = &grid[xx][yy];
                    if (neighbour->player == 255) {
                        neighbour->player = vor->player;
                        neighbour->distance = vor->distance + 1;
                        addNode(xx, yy);
                        sizes[vor->player]++;
                    } else if (neighbour->player != vor->player && neighbour->player != 254) {
                        // Join the regions
                        regions[neighbour->player] = regions[vor->player];

                        if (neighbour->distance == vor->distance + 1) {
                            // This is a shared boundary: remove it from the other player's territory
                            sizes[neighbour->player]--;
                            neighbour->player = 254;
                        }
                    }
                }
            }
        }
    }

    int playerRegionSize(int player) {
        return sizes[player];
    }

    Vor& get(int x, int y) {
        return grid[x][y];
    }

    void print() {
        cout << hex;
        for (int y = 0; y <= MAX_Y; y++) {
            for (int x = 0; x <= MAX_X; x++) {
                Vor vor = get(x, y);
                cout << setw(3) << int(vor.player);
            }
            cout << endl;
        }
        cout << dec;
    }

    int regionForPlayer(int player) {
        return regions[player];
    }
};

class Scores {
public:
    int scores[PLAYERS];
    const char* move;
};

class Bounds {
public:
    int bounds[PLAYERS];

    Bounds() {
        for (int i = 0; i < PLAYERS; i++) {
            bounds[i] = INT_MIN;
        }
    }
};

class Size {
public:
    int player;
    int size;

    Size(int p_player, int p_size) {
        player = p_player;
        size = p_size;
    }
};

Scores calculateScores(Voronoi& voronoi, State& state) {
    int occupants[PLAYERS];
    int totalSize[PLAYERS];
    int maxSize = -1;
    Scores scores;

    voronoi.calculate(state);

    for (int i = 0; i < state.numPlayers; i++) {
        occupants[i] = 0;
        totalSize[i] = 0;
    }
    for (int i = 0; i < state.numPlayers; i++) {
        scores.scores[i] = voronoi.playerRegionSize(i);

        int region = voronoi.regionForPlayer(i);
        occupants[region]++;

        if (state.isAlive(i)) {
            totalSize[region] += voronoi.playerRegionSize(i);
        } else {
            totalSize[region] = -1;
        }

        if (totalSize[region] > maxSize) {
            maxSize = totalSize[region];
        }
    }

    // penalise dead people. revive everyone and go through the deaths in order.
    bool dead[PLAYERS] = {false, false, false, false};
    int aliveCount = state.numPlayers;
    for (int j = 0; j < state.deathCount; j++) {
        aliveCount--;
        int player = state.deadList[j];
        scores.scores[player] -= 1000;
        dead[player] = true;

        // give the points to the players who were still alive at that point
        for (int i = 0; i < state.numPlayers; i++) {
            if (!dead[i]) {
                scores.scores[i] += (1000 / aliveCount);
            }
        }
    }

    // if more than one person is still alive, but one of them has clearly won, award a bonus to the winner
    if (aliveCount >= 2) {
        int bonus = -1;
        for (int i = 0; i < state.numPlayers; i++) {
            int region = voronoi.regionForPlayer(i);
            if (occupants[region] == 1 && totalSize[i] == maxSize) {
                // Bonus!
                bonus = i;
                break;
            }
        }
        if (bonus >= 0) {
            scores.scores[bonus] += 1000;
            for (int i = 0; i < state.numPlayers; i++) {
                if (i != bonus && !dead[i]) {
                    scores.scores[i] -= 1000 / (aliveCount - 1);
                }
            }
        }
    }
    return scores;
}

typedef Scores (*ScoreCalculator)(Bounds& bounds, State& state, int turn, void* scoreCalculator, void* data);

inline bool checkBounds(Bounds& bounds, Scores& scores, State& state, int player) {
    if (!state.pruningEnabled) {
        return false;
    }
    for (int i = 0; i < state.numPlayers; i++) {
        if (i != player && scores.scores[i] + state.pruneMargin <= bounds.bounds[i]) {
            return true;
        }
    }
    return false;
}

Scores minimax(Bounds& parentBounds, State& state, int turn, void* sc, void* data) {
    state.nodesSearched++;
    Bounds bounds = parentBounds;
    ScoreCalculator scoreCalculator = (ScoreCalculator) sc;

    int player = (state.thisPlayer + turn) % state.numPlayers;

    // Skip dead players, and fast forward to scoring when only one player left alive
    if (!state.isAlive(player) || state.livingCount() == 1) {
        return scoreCalculator(bounds, state, turn + 1, (void*) scoreCalculator, data);
    }

    Scores bestScores;
    bestScores.scores[player] = INT_MIN;

    int origX = state.players[player].x;
    int origY = state.players[player].y;

    for (int i = 0; i < 4; i++) {
        int x = origX + xOffsets[i];
        int y = origY + yOffsets[i];
        if (!state.occupied(x, y)) {
            state.occupy(x, y, player);
            Scores scores = scoreCalculator(bounds, state, turn + 1, (void*) scoreCalculator, data);
            state.unoccupy(x, y, player);
            state.occupy(origX, origY, player); // restore player position
            scores.move = dirs[i];
            if (checkBounds(bounds, scores, state, player)) {
                return scores;
            }
            if (scores.scores[player] > bestScores.scores[player]) {
                bestScores = scores;
                if (state.pruningEnabled) {
                    bounds.bounds[player] = scores.scores[player];
                }
            }
        }
    }

    if (bestScores.scores[player] == INT_MIN) {
        // All moves are illegal - player dies and turn passes to the next player
        state.kill(player);
        Scores scores = scoreCalculator(bounds, state, turn + 1, (void*) scoreCalculator, data);
        state.revive(player);
        return scores;
    } else {
        return bestScores;
    }
}

Scores voronoiRecursive(Bounds& bounds, State& state, int turn, void* sc, void* data) {
    if (turn >= state.maxDepth || state.isTimeLimitReached()) {
        return calculateScores(*((Voronoi*)data), state);
    } else {
        return minimax(bounds, state, turn, sc, data);
    }
}

void run() {
    State state;
    Scores scores;
    Voronoi voronoi;
    Bounds bounds;

    while (1) {
        state.resetTimer();
        state.readTurn(cin);

        // for (int i = 0; i < state.numPlayers; i++) {
        //     cerr << state.players[i].x << "," << state.players[i].y << endl;
        // }

        long start = millis();
        scores = minimax(bounds, state, 0, (void*) voronoiRecursive, &voronoi);
        long elapsed = millis() - start;
        cerr << elapsed << "ms" << endl;

        for (int i = 0; i < state.numPlayers; i++) {
            cerr << scores.scores[i];
            if (i == state.thisPlayer) {
                cerr << "*";
            }
            cerr << endl;
        }
        cerr << state.maxDepth << " plies" << endl;

        if (state.timeLimitEnabled) {
            if (state.isTimeLimitReached()) {
                state.maxDepth--;
            } else if (millis() - state.startTime <= LOWER_TIME_LIMIT) {
                state.maxDepth++;
            }
        }

        cout << scores.move << endl;
    }
}

#if !defined(TRON_TESTS) && !defined(TRON_PROF)
int main() {
    run();
    return 0;
}
#endif
