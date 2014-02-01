#include <cstring>
#include <vector>
#include <iostream>

#define WIDTH  30
#define HEIGHT 20
#define MAX_X  (WIDTH - 1)
#define MAX_Y  (HEIGHT - 1)

using namespace std;

class State {
public:
    bool occupied(int x, int y) const {
        return false;
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
        for (vector<Group*>::iterator i = oldRegion->groups.begin(); i != oldRegion->groups.end(); i++) {
            (*i)->region = newRegion;
        }
        newRegion->groups.insert(newRegion->groups.end(), oldRegion->groups.begin(), oldRegion->groups.end());
        newRegion->size += oldRegion->size;

        // var idx = this._regions.indexOf(oldRegion);
        // this._regions.splice(idx, 1);
        regionCount--;
    }
};

#ifndef TRON_TESTS
int main() {
    run();
}
#endif
