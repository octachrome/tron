#include <cstring>
#include <iostream>
#include <iomanip>
#include <climits>
#include <time.h>

#define WIDTH  30
#define HEIGHT 20
#define MAX_X  (WIDTH - 1)
#define MAX_Y  (HEIGHT - 1)
#define PLAYERS 4
#define TIME_LIMIT 80
#define MAX_NEIGHBOURS 32

//#define WORST_CASE_TESTING
#define NO_MANS_LAND
#define SHARED_ROOM_PENALTY 9 / 10

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
    unsigned char grid[WIDTH+2][HEIGHT+2];

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
        memset(grid, 0, (WIDTH + 2) * (HEIGHT + 2) * sizeof(char));
        for (int x = 0; x < WIDTH + 2; x++) {
            grid[x][0] = grid[x][HEIGHT + 1] = 255;
        }
        for (int y = 0; y < HEIGHT + 2; y++) {
            grid[0][y] = grid[WIDTH + 1][y] = 255;
        }
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

    inline int getMaxDepth() {
        if (isTimeLimitReached()) {
            return 1;
        } else {
            return maxDepth;
        }
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

    // Starting from (x,y), move towards xOffset OR yOffset, and find out whether we pass through a door
    inline bool isDoor(int x, int y, int xOffset, int yOffset) const {
        if (xOffset) {
            bool above = occupied(x, y - 1) || occupied(x + xOffset, y - 1);
            bool below = occupied(x, y + 1) || occupied(x + xOffset, y + 1);
            return above && below;
        }
        if (yOffset) {
            bool left = occupied(x - 1, y) || occupied(x - 1, y + yOffset);
            bool right = occupied(x + 1, y) || occupied(x + 1, y + yOffset);
            return left && right;
        }
        cerr << "Illegal arguments to isDoor" << endl;
        return false;
    }

    inline bool occupied(int x, int y) const {
        return grid[x + 1][y + 1] & alive;
    }

    inline void occupy(int x, int y, int player) {
        players[player].x = x;
        players[player].y = y;

        grid[x + 1][y + 1] |= (1 << player);
    }

    inline void unoccupy(int x, int y, int player) {
        grid[x + 1][y + 1] &= ~(1 << player);
    }

    inline void clear(int x, int y) {
        grid[x + 1][y + 1] = 0;
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

            if (headX < 0 || (players[i].x == headX && players[i].y == headY)) {
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
        for (int y = 0; y < HEIGHT; y++) {
            cerr << "\"";
            for (int x = 0; x < WIDTH; x++) {
                int player = -1;
                for (int i = 0; i < numPlayers; i++) {
                    if (x == players[i].x && y == players[i].y) {
                        player = i;
                        break;
                    }
                }
                if (player >= 0) {
                    cerr << char('A' + player);
                } else {
                    unsigned char b = grid[x + 1][y + 1];
                    if (b == 0) {
                        cerr << ' ';
                    } else {
                        int p = 0;
                        while (b >>= 1) p++;
                        cerr << p;
                    }
                }
            }
            cerr << "\\n\"" << endl;
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
    short size;
    short neighbourCount;
    short neighbours[MAX_NEIGHBOURS];
    bool shared;
    bool visited;
};

class Voronoi {
private:
    int sizes[PLAYERS];
    int regions[PLAYERS];
    Vor grid[WIDTH][HEIGHT];
    Coord openNodes[10000];
    Room rooms[10000];
    int roomCount;
    short equivalences[600];

    void clear() {
        memset(grid, 255, sizeof(grid));
        memset(equivalences, -1, sizeof(equivalences));
        roomCount = 0;
    }

    #define addNode(xx, yy) {            \
        openNodes[nodeCount].x = xx;     \
        openNodes[nodeCount].y = yy;     \
        nodeCount++;                     \
    }

    inline int addRoom() {
        int id = roomCount++;
        Room& room = rooms[id];
        room.size = 0;
        room.neighbourCount = 0;
        room.shared = false;
        room.visited = false;
        return id;
    }

    inline int trueId(int id) {
        do {
            register int equiv = equivalences[id];
            if (equiv == -1) {
                return id;
            }
            id = equiv;
        } while (true);
    }

    inline Room& room(int id) {
        return rooms[trueId(id)];
    }

    inline void makeNeighbours(int fromId, int toId) {
        Room& from = room(fromId);
        if (from.neighbourCount >= MAX_NEIGHBOURS) {
            cerr << "Neighbour limit reached" << endl;
            return;
        }
#ifdef TRON_TRACE
        if (room(toId).size < 0) {
            cerr << "Making neigbour with dead room" << endl;
        }
#endif
        from.neighbours[from.neighbourCount++] = toId;
    }

    inline void combineRooms(int id1, int id2) {
        if (id1 == id2) {
            cerr << "Room cannot be combined with itself" << endl;
            return;
        }
        int combinedId, oldId;
        if (id1 < id2) {
            combinedId = id1;
            oldId = id2;
        } else {
            combinedId = id2;
            oldId = id1;
        }
#ifdef TRON_TRACE
        if (trueId(oldId) == trueId(combinedId)) {
            cerr << "Room " << oldId << " already combined with " << combinedId << endl;
            return;
        }
#endif

        Room& combined = room(combinedId);
        Room& old = room(oldId);

        combined.size += old.size;
        old.size = -1;

        combined.shared = combined.shared || old.shared;

        // Delete any neighbour relationship between new room and old, by swapping for the last neighbour in the list
        for (int i = 0; i < combined.neighbourCount; i++) {
            if (combined.neighbours[i] == oldId) {
                combined.neighbours[i] = combined.neighbours[combined.neighbourCount - 1];
                combined.neighbourCount--;
                i--;
            }
        }
        // Follow the old neighbour relationships, fix the inverse relationships, and add as a neighbour of the new room
        for (int i = 0; i < old.neighbourCount; i++) {
            int neighbourId = old.neighbours[i];
            makeNeighbours(combinedId, neighbourId);
        }

        old.neighbourCount = 0;
        equivalences[oldId] = combinedId;
    }

    int calculateRegionSize(Room& room) {
        if (room.visited) {
            return 0;
        }
        if (room.size < 0) {
            cerr << "Dead room while calculating region size" << endl;
        }
        room.visited = true;
        int maxNeighbourSize = 0;
        bool sharedNeighbour = false;
        for (int i = 0; i < room.neighbourCount; i++) {
            Room& neighbour = getNeighbour(room, i);
            if (&neighbour != &room) {
                int size = calculateRegionSize(neighbour);
                if (size > maxNeighbourSize) {
                    maxNeighbourSize = size;
                }
            }
            if (neighbour.shared) {
                sharedNeighbour = true;
            }
        }
        room.visited = false;
        int size = room.size;
#ifdef SHARED_ROOM_PENALTY
        if (sharedNeighbour) {
            size = size * SHARED_ROOM_PENALTY;
        }
#endif
        return size + maxNeighbourSize;
    }

public:
    void calculate(const State& state, int turn = 0) {
        clear();
        int nodeCount = 0;

        int numPlayers = state.numPlayers;
        for (int i = 0; i < numPlayers; i++) {
            // Ensure that player id = room id, no matter how many players are alive
            addRoom();
            // Calculate in turn order, so the player with the first turn gets the edge on boundary territory
            int playerNum = (i + turn) % state.numPlayers;
            if (state.isAlive(playerNum)) {
                const Player& player = state.players[playerNum];
                int px = player.x;
                int py = player.y;
                Vor& v = grid[px][py];
                v.player = playerNum;
                v.distance = 0;
                v.room = playerNum;
                addNode(px, py);
            }
            regions[playerNum] = playerNum;
        }

        for (int i = 0; i < nodeCount; i++) {
            int x = openNodes[i].x;
            int y = openNodes[i].y;
            Vor& vor = grid[x][y];
            if (vor.player == 254) {
                continue;
            }

            for (int j = 0; j < 4; j++) {
                int xOffset = xOffsets[j];
                int yOffset = yOffsets[j];
                int xx = x + xOffset;
                int yy = y + yOffset;
                if (!state.occupied(xx, yy)) {
                    int vorRoom = trueId(vor.room);
                    Vor& neighbour = grid[xx][yy];
                    int neighbourPlayer = neighbour.player;
                    if (neighbourPlayer == 255) {
                        neighbour.player = vor.player;
                        neighbour.distance = vor.distance + 1;
                        int neighbourRoom;
                        if (state.isDoor(x, y, xOffset, yOffset)) {
                            neighbourRoom = addRoom();
                            makeNeighbours(vorRoom, neighbourRoom);
                        } else {
                            neighbourRoom = vorRoom;
                        }
                        addNode(xx, yy);
                        neighbour.room = neighbourRoom;
                        // neighbourRoom is definitely not dead
                        rooms[neighbourRoom].size++;
                    } else {
                        int neighbourRoom = trueId(neighbour.room);
                        if (vorRoom != neighbourRoom) {
                            if (neighbourPlayer == vor.player) {
                                if (state.isDoor(x, y, xOffset, yOffset)) {
                                    makeNeighbours(vorRoom, neighbourRoom);
                                } else {
                                    combineRooms(vorRoom, neighbourRoom);
                                }
                            } else {
                                // Join the regions (buggy, because it might assign p0.region = 1, then later p1.region = 0)
                                // regions[neighbourPlayer] = regions[vor.player];
#ifdef NO_MANS_LAND
                                if (neighbourPlayer != 254) {
                                    // Join the regions
                                    regions[neighbourPlayer] = regions[vor.player];

                                    if (neighbour.distance == vor.distance + 1) {
                                        // This is a shared boundary: remove it from the other player's territory
                                        // neighbourRoom is definitely not dead
                                        rooms[neighbourRoom].size--;
                                        // This cell is no man's land
                                        neighbour.player = 254;
                                    }
                                }
#endif
                                // Penalise both rooms
                                // neighbourRoom and vorRoom are definitely not dead
                                rooms[neighbourRoom].shared = true;
                                rooms[vorRoom].shared = true;
                            }
                        }
                    }
                }
            }
        }

        for (int i = 0; i < numPlayers; i++) {
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
                cerr << setw(3) << (int(vor.player) > 255 ? 255 : int(vor.player));
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

    inline Room& getNeighbour(const Room& r, int index) {
        int roomId = r.neighbours[index];
        return room(roomId);
    }
};

class Scores {
public:
    int scores[PLAYERS];
    int ranks[PLAYERS];
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

void calculateScores(Scores& scores, Voronoi& voronoi, State& state, int turn) {
    voronoi.calculate(state, turn);

    // for (int i = 0; i < state.numPlayers; i++) {
    //     scores.regions[i] = voronoi.regionForPlayer(i);
    // }

    for (int i = 0; i < state.numPlayers; i++) {
        scores.scores[i] = voronoi.playerRegionSize(i);
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

    // calculate ranks
    for (int i = 0; i < state.numPlayers; i++) {
        scores.ranks[i] = 0;
        for (int j = 0; j < state.numPlayers; j++) {
            if (i != j && scores.scores[j] < scores.scores[i]) {
                scores.ranks[i]++;
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
#ifdef TRON_TRACEX
            cerr << "Score " << scores.scores[i] << " breaks bound " << bounds.bounds[i] << " for player " << i
                << " from " << bounds.moves[i] << endl;
            scores.print();
#endif
            return true;
        }
    }
    return false;
}

// The given score will be chosen over the best score so far if it reduces *our* rank - this is an "avoid worst case" strategy
inline bool worsensOurRank(Scores& scores, Scores& bestScores, int player, int thisPlayer) {
    if (player == thisPlayer) {
        return false;
    }
    return scores.ranks[thisPlayer] < bestScores.ranks[thisPlayer];
}

// The given score will be chosen over the best score so far if it improves the player's score (preserving rank)
inline bool improvesTheirScore(Scores& scores, Scores& bestScores, int player) {
    return scores.ranks[player] == bestScores.ranks[player] && scores.scores[player] > bestScores.scores[player];
}

// The given score will be chosen over the best score so far if it improves the player's rank
inline bool improvesTheirRank(Scores& scores, Scores& bestScores, int player) {
    return scores.ranks[player] > bestScores.ranks[player];
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
    bestScores.ranks[player] = 0;

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
#ifdef TRON_TRACEX
                cerr << "Pruned at " << scores.moves << endl;
                cerr << "          ";
                for (int j = 0; j < turn; j++) {
                    cerr << " ";
                }
                cerr << "^" << endl;
#endif
                return;
            }
#ifdef TRON_TRACE
            for (int k = 0; k < turn; k++) cerr << "  ";
            cerr << "player " << player << ": ";
            scores.print();
#endif
            if (improvesTheirRank(scores, bestScores, player)
#ifdef WORST_CASE_TESTING
                    || worsensOurRank(scores, bestScores, player, state.thisPlayer)
#endif
                    || improvesTheirScore(scores, bestScores, player)) {
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
#ifdef TRON_TRACE
        for (int k = 0; k < turn; k++) cerr << "  ";
        cerr << "player " << player << " chose ";
        scores.print();
#endif
    }
}

void voronoiRecursive(Scores& scores, Bounds& bounds, State& state, int turn, void* sc, void* data) {
    if (turn >= state.getMaxDepth()) {
        calculateScores(scores, *((Voronoi*)data), state, turn);
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

        cout << scores.move << endl;
    }
}

#if !defined(TRON_TESTS) && !defined(TRON_PROF)
int main() {
    run();
    return 0;
}
#endif
