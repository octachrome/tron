#include "tron.cc"

int main() {
    State state;
    Regions regions;

    state.numPlayers = 2;
    state.thisPlayer = 0;
    state.maxDepth = 25;

    state.occupy(26, 18, 0);
    state.occupy(16, 1, 1);

    maxScore(regions, state, 0, (void*) minimax);
    return 0;
}
