#include "tron.cc"

Scores randRecursive(Bounds& bounds, State& state, int turn, void* sc, void* data) {
    if (turn >= state.maxDepth) {
        Scores scores;
        for (int i = 0; i < state.numPlayers; i++) {
            scores.scores[i] = rand() % 500;
            scores.move = DOWN;
        }
        return scores;
    } else {
        return minimax(bounds, state, turn, sc, data);
    }
}

Scores timedSearch(State& s, bool pruningEnabled) {
    State state = s;
    state.pruningEnabled = pruningEnabled;

    Regions regions;
    Voronoi voronoi;
    Bounds bounds;

    clock_t start = clock();
    // Scores scores = minimax(bounds, state, 0, (void*) voronoiRecursive, &voronoi);
    Scores scores = minimax(bounds, state, 0, (void*) randRecursive, 0);
    // Scores scores = minimax(bounds, state, 0, (void*) regionsRecursive, &regions);
    clock_t elapsed = clock() - start;
    cerr << state.nodesSearched << " in " << (elapsed / CLOCKS_PER_MS) << "ms" << endl;
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
    state.pruneMargin = 5;
    state.maxDepth = 27;

    srand(time(0));
    randomlyPopulate(state);

    state.occupy(26, 18, 0);
    state.occupy(16, 1, 1);
    state.print();

    timedSearch(state, true);
}
