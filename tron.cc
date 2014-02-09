#include <cstring>
#include <vector>
#include <map>
#include <iostream>
#include <ctime>
#include <algorithm>

#define WIDTH  30
#define HEIGHT 20
#define MAX_X  (WIDTH - 1)
#define MAX_Y  (HEIGHT - 1)
#define PLAYERS 4

using namespace std;

const char* RIGHT = "RIGHT";
const char* LEFT = "LEFT";
const char* DOWN = "DOWN";
const char* UP = "UP";

const char* const dirs[] = {RIGHT, LEFT, DOWN, UP};

const int xOffsets[] = {1, -1, 0, 0};
const int yOffsets[] = {0, 0, 1, -1};

class Player {
public:
    int x;
    int y;

    Player() {}

    Player(int p_x, int p_y) {
        x = p_x;
        y = p_y;
    }
};

class State {
private:
    int grid[WIDTH][HEIGHT];

public:
    int numPlayers;
    int thisPlayer;
    int maxDepth;

    Player players[PLAYERS];

    State() {
        memset(grid, 0, WIDTH * HEIGHT * sizeof(int));
        maxDepth = 10;
    }

    bool occupied(int x, int y) const {
        if (x < 0 || y < 0 || x > MAX_X || y > MAX_Y) {
            return true;
        }
        return grid[x][y] != 0;
    }

    void occupy(int x, int y, int player) {
        players[player].x = x;
        players[player].y = y;

        grid[x][y] = 1;
    }

    void clear(int x, int y) {
        grid[x][y] = 0;
    }

    int x() {
        return players[thisPlayer].x;
    }

    int y() {
        return players[thisPlayer].y;
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
    Vor grid[WIDTH][HEIGHT];
    Coord openNodes[WIDTH * HEIGHT];
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
    void calculate(State& state) {
        clear();

        for (int i = 0; i < state.numPlayers; i++) {
            Player* player = state.players + i;
            grid[player->x][player->y].player = i;
            grid[player->x][player->y].distance = 0;
            addNode(player->x, player->y);
        }

        for (int i = 0; i < listEnd; i++) {
            int x = openNodes[i].x;
            int y = openNodes[i].y;
            Vor* vor = &grid[x][y];

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
                    } else if (neighbour->player != vor->player) {
                        if (neighbour->distance == vor->distance + 1) {
                            // This is a shared boundary: remove it from the other player's territory
                            if (neighbour->player != 254) {
                                sizes[neighbour->player]--;
                                neighbour->player = 254;
                            }
                        }
                    }
                }
            }
        }
    }

    int playerRegionSize(int player) {
        return sizes[player];
    }
};

class Scores {
public:
    int scores[PLAYERS];
    int sizes[PLAYERS];
    const char* move;
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

typedef Scores (*ScoreCalculator)(Regions& regions, State& state, int turn, void* scoreCalculator);

Scores maxScore(Regions& regions, State& state, int turn, void* sc) {
    ScoreCalculator scoreCalculator = (ScoreCalculator) sc;

    Scores bestScores;
    int player = (state.thisPlayer + turn) % state.numPlayers;
    bestScores.scores[player] = -1;

    int origX = state.players[player].x;
    int origY = state.players[player].y;

    for (int i = 0; i < 4; i++) {
        int x = origX + xOffsets[i];
        int y = origY + yOffsets[i];
        if (!state.occupied(x, y)) {
            state.occupy(x, y, player);
            Scores scores = scoreCalculator(regions, state, turn + 1, (void*) scoreCalculator);
            state.clear(x, y);
            state.occupy(origX, origY, player); // restore player position
            scores.move = dirs[i];
            if (scores.scores[player] > bestScores.scores[player]) {
                bestScores = scores;
            }
        }
    }

    if (bestScores.scores[player] == -1) {
        // All moves are illegal, so pass
        Scores scores = scoreCalculator(regions, state, turn + 1, (void*) scoreCalculator);
        scores.move = 0;
        return scores;
    } else {
        return bestScores;
    }
}

Scores minimax(Regions& regions, State& state, int turn, void* sc) {
    if (turn >= state.maxDepth) {
        return calculateScores(regions, state);
    } else {
        return maxScore(regions, state, turn + 1, sc);
    }
}

State state;
Regions regions;

void readTurn() {
    cin >> state.numPlayers;
    cin >> state.thisPlayer;

    for (int i = 0; i < state.numPlayers; i++) {
        int tailX;
        int tailY;
        int headX;
        int headY;

        cin >> tailX;
        cin >> tailY;
        cin >> headX;
        cin >> headY;

        state.occupy(headX, headY, i);
    } 
}

int sizeOf(int x, int y) {
    Region* region = regions.regionAt(x, y);
    return region ? region->size : 0;
}

const char* findLargestRegion(int x, int y) {
    int sizes[4];

    sizes[0] = sizeOf(x + 1, y);
    sizes[1] = sizeOf(x - 1, y);
    sizes[2] = sizeOf(x, y + 1);
    sizes[3] = sizeOf(x, y - 1);

    int max = sizes[0];
    int maxIdx = 0;
    for (int i = 1; i < 4; i++) {
        if (sizes[i] > max) {
            max = sizes[i];
            maxIdx = i;
        }
    }

    return dirs[maxIdx];
}

#define CLOCKS_PER_MS (CLOCKS_PER_SEC / 1000)

void run() {
    Scores scores;

    while (1) {
        readTurn();

        for (int i = 0; i < state.numPlayers; i++) {
            cerr << state.players[i].x << "," << state.players[i].y << endl;
        }

        clock_t start = clock();
        scores = maxScore(regions, state, 0, (void*) minimax);
        clock_t elapsed = clock() - start;
        cerr << (elapsed / CLOCKS_PER_MS) << endl;

        cout << scores.move << endl;
    }
}

#if !defined(TRON_TESTS) && !defined(TRON_PROF)
int main() {
    run();
    return 0;
}
#endif
