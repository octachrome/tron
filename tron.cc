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
    bool alive;

    Player() {
        alive = true;
    }

    Player(int p_x, int p_y) {
        x = p_x;
        y = p_y;
        alive = true;
    }
};

class State {
private:
    char grid[WIDTH][HEIGHT];

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

    State() {
        memset(grid, -1, WIDTH * HEIGHT * sizeof(char));
        maxDepth = 9;
        pruneMargin = 0;
        pruningEnabled = true;
        nodesSearched = 0;
        timeLimitEnabled = true;
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
        int player = grid[x][y];
        return player != -1 && players[player].alive;
    }

    inline void occupy(int x, int y, int player) {
        players[player].x = x;
        players[player].y = y;

        grid[x][y] = player;
    }

    inline void kill(int player) {
        players[player].alive = false;
    }

    inline void revive(int player) {
        players[player].alive = true;
    }

    inline void clear(int x, int y) {
        grid[x][y] = -1;
    }

    inline int x() {
        return players[thisPlayer].x;
    }

    inline int y() {
        return players[thisPlayer].y;
    }

    inline void readTurn() {
        cin >> numPlayers;
        cin >> thisPlayer;

        for (int i = 0; i < numPlayers; i++) {
            int tailX;
            int tailY;
            int headX;
            int headY;

            cin >> tailX;
            cin >> tailY;
            cin >> headX;
            cin >> headY;

            occupy(headX, headY, i);
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

class Region;

class Group {
public:
    Region* region;
};

class Region {
public:
    int id;
    vector<Group*> groups;
    int size;

    Region(int p_id) {
        size = 0;
        id = p_id;
    }
};

class Regions {
private:
    Group* map[WIDTH][HEIGHT];
    int regionCount;
    vector<Region*> regions;
    int nextId;

public:
    void findRegions(const State &state) {
        init();

        for (int x = 0; x <= MAX_X; x++) {
            Group* group = 0;

            for (int y = 0; y <= MAX_Y; y++) {
                if (state.occupied(x, y)) {
                    group = 0;
                } else {
                    if (group == 0) {
                        group = groupAt(x - 1, y);
                        if (group == 0) {
                            group = createGroupAndRegion();
                        }
                    }

                    setGroupAt(x, y, group);
                    group->region->size++;

                    Region* neighbour = regionAt(x - 1, y);
                    if (neighbour && neighbour != group->region) {
                        combine(neighbour, group->region);
                    }
                }
            }
        }
    }

    inline int count() const {
        return regionCount;
    }

    inline Region* regionAt(int x, int y) const {
        Group* group = groupAt(x, y);
        return group ? group->region : 0;
    }

private:
    void init() {
        for (vector<Region*>::iterator i = regions.begin(); i < regions.end(); ++i) {
            for (vector<Group*>::iterator j = (*i)->groups.begin(); j < (*i)->groups.end(); ++j) {
                delete *j;
            }
            delete *i;
        }
        regions.clear();

        memset(map, 0, WIDTH * HEIGHT * sizeof(Region*));
        regionCount = 0;
        nextId = 1;
    }

    inline Group* createGroupAndRegion() {
        Group* group = new Group();
        Region* region = new Region(nextId++);
        region->groups.push_back(group);
        group->region = region;

        regions.push_back(region);
        regionCount++;
        return group;
    }

    inline Group* groupAt(int x, int y) const {
        if (x < 0 || y < 0 || x > MAX_X || y > MAX_Y) {
            return 0;
        }
        return map[x][y];
    }

    inline void setGroupAt(int x, int y, Group* group) {
        map[x][y] = group;
    }

    void combine(Region* oldRegion, Region* newRegion) {
        for (vector<Group*>::iterator i = oldRegion->groups.begin(); i != oldRegion->groups.end(); ++i) {
            (*i)->region = newRegion;
        }
        newRegion->groups.insert(newRegion->groups.end(), oldRegion->groups.begin(), oldRegion->groups.end());
        newRegion->size += oldRegion->size;

        oldRegion->groups.clear();

        // var idx = this._regions.indexOf(oldRegion);
        // this._regions.splice(idx, 1);
        regionCount--;
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
            const Player* player = state.players + i;
            grid[player->x][player->y].player = i;
            grid[player->x][player->y].distance = 0;
            addNode(player->x, player->y);
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
    int sizes[PLAYERS];
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

bool compareSizes(const Size& size1, const Size& size2) {
    return size1.size > size2.size;
}

template<class RegionsLike>
void calculateSizes(RegionsLike& regions, const State& state, vector<Size>& sizes) {
    map<int, int> occupants;

    for (int i = 0; i < state.numPlayers; i++) {
        int x = state.players[i].x;
        int y = state.players[i].y;
        for (int j = 0; j < 4; j++) {
            Region *region = regions.regionAt(x + xOffsets[j], y + yOffsets[j]);
            if (region != 0) {
                if (occupants.find(region->id) == occupants.end()) {
                    occupants[region->id] = 1;
                } else {
                    occupants[region->id]++;
                }
            }
        }
    }

    for (int i = 0; i < state.numPlayers; i++) {
        int x = state.players[i].x;
        int y = state.players[i].y;
        int maxSize = 0;
        for (int j = 0; j < 4; j++) {
            Region *region = regions.regionAt(x + xOffsets[j], y + yOffsets[j]);
            if (region != 0) {
                int size = region->size / occupants[region->id];
                if (size > maxSize) {
                    maxSize = size;
                }
            }
        }
        sizes.push_back(Size(i, maxSize));
    }
}

// TODO: adjust region sizes for subregions which can be entered but not left
template<class RegionsLike>
Scores calculateScores(RegionsLike& regions, const State& state) {
    Scores scores;
    vector<Size> sizes;

    regions.findRegions(state);
    calculateSizes(regions, state, sizes);

    sort(sizes.begin(), sizes.end(), compareSizes);
    int leaderSize = sizes[0].size;

    for (int i = 0; i < state.numPlayers; i++) {
        int player = sizes[i].player;
        scores.scores[player] = sizes[i].size - leaderSize;
        scores.sizes[player] = sizes[i].size;
    }
    return scores;
};

Scores calculateScores(Voronoi& voronoi, const State& state) {
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
        totalSize[region] += voronoi.playerRegionSize(i);

        if (totalSize[region] > maxSize) {
            maxSize = totalSize[region];
        }
    }
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
            if (i != bonus) {
                scores.scores[i] -= 1000 / (state.numPlayers - 1);
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

    Scores bestScores;
    int player = (state.thisPlayer + turn) % state.numPlayers;
    bestScores.scores[player] = INT_MIN;

    int origX = state.players[player].x;
    int origY = state.players[player].y;

    for (int i = 0; i < 4; i++) {
        int x = origX + xOffsets[i];
        int y = origY + yOffsets[i];
        if (!state.occupied(x, y)) {
            state.occupy(x, y, player);
            Scores scores = scoreCalculator(bounds, state, turn + 1, (void*) scoreCalculator, data);
            state.clear(x, y);
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
        // All moves are illegal, so pass
        Scores scores = scoreCalculator(bounds, state, turn + 1, (void*) scoreCalculator, data);
        scores.move = 0;
        return scores;
    } else {
        return bestScores;
    }
}

Scores regionsRecursive(Bounds& bounds, State& state, int turn, void* sc, void* data) {
    if (turn >= state.maxDepth || state.isTimeLimitReached()) {
        return calculateScores(*((Regions*)data), state);
    } else {
        return minimax(bounds, state, turn, sc, data);
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
        state.readTurn();

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
