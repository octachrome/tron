#include <cstring>
#include <vector>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <climits>
#include <time.h>

#define WIDTH  30
#define HEIGHT 20
#define MAX_X  (WIDTH - 1)
#define MAX_Y  (HEIGHT - 1)
#define PLAYERS 4
#define TIME_LIMIT 95

using namespace std;

const char* RIGHT = "RIGHT";
const char* LEFT = "LEFT";
const char* DOWN = "DOWN";
const char* UP = "UP";
const char* GULP = "GULP";

const char* const dirs[] = {RIGHT, LEFT, DOWN, UP};

const int xOffsets[] = {1, -1, 0, 0};
const int yOffsets[] = {0, 0, 1, -1};

long millis() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
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
        maxDepth = 8;
        pruneMargin = 0;
        pruningEnabled = false;
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

    inline bool isDoor(int x, int y) const {
        return (occupied(x - 1, y) && occupied(x + 1, y)) || (occupied(x, y - 1) && occupied(x, y + 1));
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
        for (int i = 0; i < deathCount; i++) {
            if (deadList[i] == player) {
                // already dead
                return;
            }
        }
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
                // Player is already dead
                kill(i);
            } else {
                occupy(tailX, tailY, i);
                occupy(headX, headY, i);
            }
        } 

        resetTimer();
        nodesSearched = 0;
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
                    cerr << player;
                } else if (occupied(x, y)) {
                    cerr << '#';
                } else {
                    cerr << ' ';
                }
            }
            cerr << endl;
        }
    }
};

class Vor {
public:
    unsigned char player;
    unsigned char distance;
    unsigned short room;
};

class Coord {
public:
    unsigned char x;
    unsigned char y;
};

class Room {
public:
    unsigned short size;
    unsigned short neighbourCount;
    unsigned short neighbours[4];
    bool visited;
};

class Voronoi {
private:
    int sizes[PLAYERS];
    int regions[PLAYERS];
    Vor grid[WIDTH][HEIGHT];
    Coord openNodes[10000];
    int nodeCount;
    Room rooms[10000];
    int roomCount;

    void clear() {
        memset(grid, 255, sizeof(grid));
        nodeCount = 0;
        roomCount = 0;
    }

    inline void addNode(int x, int y) {
        openNodes[nodeCount].x = x;
        openNodes[nodeCount].y = y;
        nodeCount++;
    }

    inline int addRoom() {
        int id = roomCount++;
        Room& room = rooms[id];
        room.size = 0;
        room.neighbourCount = 0;
        room.visited = false;
        return id;
    }

    inline void makeNeighbours(int roomId1, int roomId2) {
        makeNeighboursOneWay(roomId1, roomId2);
        makeNeighboursOneWay(roomId2, roomId1);
    }

    inline void makeNeighboursOneWay(int fromId, int toId) {
        Room& from = rooms[fromId];
        for (int i = 0; i < from.neighbourCount; i++) {
            if (from.neighbours[i] == toId) {
                return;
            }
        }
        from.neighbours[from.neighbourCount++] = toId;
    }

    int calculateRegionSize(Room& room) {
        if (room.visited) {
            return 0;
        }
        room.visited = true;
        int maxNeighbourSize = 0;
        for (int i = 0; i < room.neighbourCount; i++) {
            Room& neighbour = getNeighbour(room, i);
            if (&neighbour != &room) {
                int size = calculateRegionSize(neighbour);
                if (size > maxNeighbourSize) {
                    maxNeighbourSize = size;
                }
            }
        }
        room.visited = false;
        return room.size + maxNeighbourSize;
    }

public:
    void calculate(const State& state) {
        clear();

        for (int i = 0; i < state.numPlayers; i++) {
            // Ensure that player id = room id, no matter how many players are alive
            int room = addRoom();
            if (state.isAlive(i)) {
                const Player& player = state.players[i];
                grid[player.x][player.y].player = i;
                grid[player.x][player.y].distance = 0;
                grid[player.x][player.y].room = room;
                addNode(player.x, player.y);
            }
            regions[i] = i;
        }

        for (int i = 0; i < nodeCount; i++) {
            int x = openNodes[i].x;
            int y = openNodes[i].y;
            Vor& vor = grid[x][y];
            if (vor.player == 254) {
                continue;
            }

            for (int j = 0; j < 4; j++) {
                int xx = x + xOffsets[j];
                int yy = y + yOffsets[j];
                if (!state.occupied(xx, yy)) {
                    Vor& neighbour = grid[xx][yy];
                    if (neighbour.player == 255) {
                        neighbour.player = vor.player;
                        neighbour.distance = vor.distance + 1;
                        if (state.isDoor(xx, yy)) {
                            neighbour.room = addRoom();
                            makeNeighbours(vor.room, neighbour.room);
                        } else {
                            neighbour.room = vor.room;
                        }
                        addNode(xx, yy);
                        Room& room = rooms[neighbour.room];
                        room.size++;
                    } else if (vor.room != neighbour.room) {
                        if (neighbour.player == vor.player) {
                            makeNeighbours(vor.room, neighbour.room);
                        } else if (neighbour.player != 254) {
                            // Join the regions
                            regions[neighbour.player] = regions[vor.player];

                            if (neighbour.distance == vor.distance + 1) {
                                // This is a shared boundary: remove it from the other player's territory
                                rooms[neighbour.room].size--;
                                // This cell is no man's land
                                neighbour.player = 254;
                            }
                        }
                    }
                }
            }
        }

        for (int i = 0; i < state.numPlayers; i++) {
            if (state.isAlive(i)) {
                Room& room = startingRoom(i);
                sizes[i] = calculateRegionSize(room);
            } else {
                sizes[i] = 0;
            }
        }
    }

    inline int playerRegionSize(int player) const {
        return sizes[player];
    }

    inline const Vor& get(int x, int y) const {
        return grid[x][y];
    }

    void print() const {
        cerr << hex;
        for (int y = 0; y <= MAX_Y; y++) {
            for (int x = 0; x <= MAX_X; x++) {
                Vor vor = get(x, y);
                cerr << setw(3) << (int(vor.room) > 16 ? 16 : int(vor.room));
            }
            cerr << endl;
        }
        cerr << dec;
    }

    inline int regionForPlayer(int player) const {
        return regions[player];
    }

    inline Room& startingRoom(int player) {
        return rooms[player];
    }

    inline Room& getNeighbour(const Room& room, int index) {
        int roomId = room.neighbours[index];
        return rooms[roomId];
    }
};

class Scores {
public:
    int scores[PLAYERS];
    int regions[PLAYERS];
    unsigned int losers;
    const char* move;
#ifdef TRON_TRACE
    char moves[30];
#endif

    inline Scores() {
        losers = 0;
        move = "";
    }

    Scores(int score0, int score1) {
        scores[0] = score0;
        scores[1] = score1;
        losers = 0;        
    }

    inline void clearFlags() {
        losers = 0;
    }

    inline void setLoser(int player) {
        losers |= (1 << player);
    }

    inline bool isLoser(int player) {
        return losers & (1 << player);
    }

    inline void print() const {
        for (int i = 0; i < PLAYERS; i++) {
            if (i != 0) {
                cerr << " / ";
            }
            cerr << scores[i];
        }
        cerr << " / " << move;
#ifdef TRON_TRACE
        cerr << " / " << moves;
#endif
        cerr << endl;
    }
};

class Bounds {
public:
    int bounds[PLAYERS];
#ifdef TRON_TRACE
    char moves[PLAYERS][30];
#endif

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

void calculateScores(Scores& scores, Voronoi& voronoi, State& state) {
    int occupants[PLAYERS];
    int totalSize[PLAYERS];
    int playersByRegion[PLAYERS];
    int maxSize = -1;

    voronoi.calculate(state);

    scores.clearFlags();

    for (int i = 0; i < state.numPlayers; i++) {
        scores.regions[i] = voronoi.regionForPlayer(i);
    }

    // for each region
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
        if (aliveCount > 0) {
            for (int i = 0; i < state.numPlayers; i++) {
                if (!dead[i]) {
                    scores.scores[i] += (1000 / aliveCount);
                }
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
                    scores.setLoser(i);
                }
            }
        }
    }
}

typedef void (*ScoreCalculator)(Scores& scores, Bounds& bounds, State& state, int turn, void* scoreCalculator, void* data);

inline bool checkBounds(Bounds& bounds, Scores& scores, State& state, int player) {
    if (!state.pruningEnabled) {
        return false;
    }
    int region = scores.regions[player];
    for (int i = 0; i < state.numPlayers; i++) {
        // Is bound exceeded?
        if (i != player && scores.scores[i] + state.pruneMargin <= bounds.bounds[i]
            // Is this player's score connected to mine (negative correlation)?
            && scores.regions[i] == region
            // Is this player's score connected to mine (positive correlation)?
            && !(scores.isLoser(i) && scores.isLoser(player))) {
#ifdef TRON_TRACE
            cerr << "Score " << scores.scores[i] << " breaks bound " << bounds.bounds[i] << " for player " << i
                << " from " << bounds.moves[i] << endl;
            scores.print();
#endif
            return true;
        }
    }
    return false;
}

void minimax(Scores& scores, Bounds& parentBounds, State& state, int turn, void* sc, void* data) {
    state.nodesSearched++;
    Bounds bounds = parentBounds;
    ScoreCalculator scoreCalculator = (ScoreCalculator) sc;

    int player = (state.thisPlayer + turn) % state.numPlayers;

    // Skip dead players, and fast forward to scoring when only one player left alive
    if (!state.isAlive(player) || state.livingCount() == 1) {
#ifdef TRON_TRACE
        scores.moves[turn] = 'X';
        scores.moves[turn + 1] = 0;
#endif
        scoreCalculator(scores, bounds, state, turn + 1, (void*) scoreCalculator, data);
        return;
    }

    Scores bestScores;
    bestScores.scores[player] = INT_MIN;

    int origX = state.players[player].x;
    int origY = state.players[player].y;

    for (int i = 0; i < 4; i++) {
        int x = origX + xOffsets[i];
        int y = origY + yOffsets[i];
        if (!state.occupied(x, y)) {
#ifdef TRON_TRACE
            scores.moves[turn] = dirs[i][0];
            scores.moves[turn + 1] = 0;
#endif
            state.occupy(x, y, player);
            scoreCalculator(scores, bounds, state, turn + 1, (void*) scoreCalculator, data);
            state.unoccupy(x, y, player);
            state.occupy(origX, origY, player); // restore player position
            scores.move = dirs[i];
            if (checkBounds(bounds, scores, state, player)) {
#ifdef TRON_TRACE
                cerr << "Pruned at " << scores.moves << endl;
                cerr << "          ";
                for (int j = 0; j < turn; j++) {
                    cerr << " ";
                }
                cerr << "^" << endl;
#endif
                return;
            }
            if (scores.scores[player] > bestScores.scores[player]) {
                bestScores = scores;
                if (state.pruningEnabled) {
                    bounds.bounds[player] = scores.scores[player];
#ifdef TRON_TRACE
                    strcpy(bounds.moves[player], scores.moves);
#endif
                }
            }
        }
    }

    if (bestScores.scores[player] == INT_MIN) {
        // All moves are illegal - player dies and turn passes to the next player
#ifdef TRON_TRACE
        scores.moves[turn] = 'G';
        scores.moves[turn + 1] = 0;
#endif
        state.kill(player);
        scoreCalculator(scores, bounds, state, turn + 1, (void*) scoreCalculator, data);
        state.revive(player);
        scores.move = GULP;
    } else {
        scores = bestScores;
    }
}

void voronoiRecursive(Scores& scores, Bounds& bounds, State& state, int turn, void* sc, void* data) {
    if (turn >= state.maxDepth || state.isTimeLimitReached()) {
        calculateScores(scores, *((Voronoi*)data), state);
    } else {
        minimax(scores, bounds, state, turn, sc, data);
    }
}

void run() {
    State state;
    Scores scores;
    Voronoi voronoi;
    Bounds bounds;

    while (1) {
        state.readTurn(cin);

        // for (int i = 0; i < state.numPlayers; i++) {
        //     cerr << state.players[i].x << "," << state.players[i].y << endl;
        // }

        long start = millis();
        minimax(scores, bounds, state, 0, (void*) voronoiRecursive, &voronoi);
        long elapsed = millis() - start;
        cerr << elapsed << "ms";
        if (state.isTimeLimitReached()) {
            cerr << " (timeout)";
        }
        cerr << endl;

        for (int i = 0; i < state.numPlayers; i++) {
            cerr << scores.scores[i];
            if (i == state.thisPlayer) {
                cerr << "*";
            }
            cerr << endl;
        }
        cerr << state.maxDepth << " plies" << endl;
        cerr << state.nodesSearched << " nodes" << endl;

        if (state.timeLimitEnabled) {
            if (state.isTimeLimitReached()) {
                state.maxDepth--;
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
