#include <cstring>
#include <vector>
#include <iostream>

#define WIDTH  30
#define HEIGHT 20
#define MAX_X  (WIDTH - 1)
#define MAX_Y  (HEIGHT - 1)

using namespace std;

class State {
private:
    int grid[WIDTH][HEIGHT];

public:
    State() {
        memset(grid, 0, WIDTH * HEIGHT * sizeof(int));
    }

    bool occupied(int x, int y) const {
        return grid[x][y] != 0;
    }

    void occupy(int x, int y, int value) {
        grid[x][y] = value;
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
        memset(map, 0, WIDTH * HEIGHT * sizeof(Region*));
        regions.clear();
        regionCount = 0;
        nextId = 1;

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

    int count() const {
        return regionCount;
    }

    Region* regionAt(int x, int y) const {
        Group* group = groupAt(x, y);
        return group ? group->region : 0;
    }

private:
    Group* createGroupAndRegion() {
        Group* group = new Group();
        Region* region = new Region(nextId++);
        region->groups.push_back(group);
        group->region = region;

        regions.push_back(region);
        regionCount++;
        return group;
    }

    Group* groupAt(int x, int y) const {
        if (x < 0 || y < 0 || x > MAX_X || y > MAX_Y) {
            return 0;
        }
        return map[x][y];
    }

    void setGroupAt(int x, int y, Group* group) {
        map[x][y] = group;
    }

    void combine(Region* oldRegion, Region* newRegion) {
        for (vector<Group*>::iterator i = oldRegion->groups.begin(); i != oldRegion->groups.end(); ++i) {
            (*i)->region = newRegion;
        }
        newRegion->groups.insert(newRegion->groups.end(), oldRegion->groups.begin(), oldRegion->groups.end());
        newRegion->size += oldRegion->size;

        // var idx = this._regions.indexOf(oldRegion);
        // this._regions.splice(idx, 1);
        regionCount--;
    }
};

int totalPlayers;
int thisPlayer;
int x, y;

State state;

void readTurn() {
    cin >> totalPlayers;
    cin >> thisPlayer;

    for (int i = 0; i < totalPlayers; i++) {
        int tailX;
        int tailY;
        int headX;
        int headY;

        cin >> tailX;
        cin >> tailY;
        cin >> headX;
        cin >> headY;

        state.occupy(headX, headY, 1);
        if (i == thisPlayer) {
            x = headX;
            y = headY;
        }
    } 
}

void run() {
    while (1) {
        readTurn();
        // cerr << x << "," << y << endl;
        cout << "LEFT" << endl;
    }
}

#ifndef TRON_TESTS
int main() {
    run();
    return 0;
}
#endif
