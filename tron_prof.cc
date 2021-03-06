#include <algorithm>
#include <fstream>
#include "tron.cc"

long timedSearch(State& s, bool pruningEnabled) {
    State state = s;
    state.pruningEnabled = pruningEnabled;

    Voronoi voronoi;
    Bounds bounds;

    Scores scores;
    minimax(scores, bounds, state, 0, (void*) voronoiRecursive, &voronoi);
    return state.nodesSearched;
}

void randomlyPopulate(State& state) {
    int count = 10 + rand() % 5;
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

int main(int argc, char* argv[]) {
    ofstream os("timing.log");
    os << "Nodes,Time,NodesPer100ms" << endl;

    int passes = 10;
    int loops = 1000;
    if (argc > 1) {
        passes = 1;
        loops = 50;
    }

    for (int j = 0; j < passes; j++) {
        srand(199);
        // srand(time(0));

        State state;
        state.numPlayers = 2;
        state.thisPlayer = 0;
        state.maxDepth = 8;
        state.pruningEnabled = false;
        state.timeLimitEnabled = false;

        randomlyPopulate(state);
        state.occupy(26, 18, 0);
        state.occupy(16, 1, 1);

        long nodes = 0;
        clock_t start = millis();

        for (int i = 0; i < loops; i++) {
            nodes += timedSearch(state, true);
        }

        clock_t elapsed = millis() - start;
        os << nodes << "," << elapsed << "," << 100.0 * nodes / elapsed << endl;
        cerr << "Pass " << j << " complete" << endl;
    }

    os.close();
}
