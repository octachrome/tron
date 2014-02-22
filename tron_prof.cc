#include <algorithm>
#include "tron.cc"

Scores timedSearch(State& s, bool pruningEnabled) {
    State state = s;
    state.pruningEnabled = pruningEnabled;

    Voronoi voronoi;
    Bounds bounds;

    clock_t start = millis();
    Scores scores;
    minimax(scores, bounds, state, 0, (void*) voronoiRecursive, &voronoi);
    clock_t elapsed = millis() - start;
    cerr << state.nodesSearched << " in " << elapsed << "ms" << endl;
    return scores;
}

void randomlyPopulate(State& state) {
    int count = 5 + rand() % 5;
    for (int i = 0; i < count; i++) {
        int x = rand() % MAX_X;
        int y = rand() % MAX_Y;
        int size = rand() % 10;
        if (rand() % 2 == 0) {
            for (int yy = y; yy <= MAX_Y && yy - y <= size; yy++) {
                state.occupy(x, yy, 0);
            }
        } else {
            for (int xx = x; xx <= MAX_X && xx - x <= size; xx++) {
                state.occupy(xx, y, 0);
            }
        }
    }
}

int main() {
    State state;
    state.numPlayers = 2;
    state.thisPlayer = 0;
    state.maxDepth = 13;
    state.pruningEnabled = false;
    state.timeLimitEnabled = false;

    srand(time(0));
    randomlyPopulate(state);

    state.occupy(26, 18, 0);
    state.occupy(16, 1, 1);
    state.print();

    timedSearch(state, true);
}
