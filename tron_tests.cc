#include "tron.cc"

#include "gtest/gtest.h"

TEST(Regions, ShouldFindSingleRegionWhenUnoccupied) {
    State state;
    Regions regions;
    regions.findRegions(state);

    ASSERT_EQ(1, regions.count()) << "Expected one region";

    Region* region1 = regions.regionAt(0, 0);
    Region* region2 = regions.regionAt(MAX_X, MAX_Y);

    ASSERT_TRUE(region1 == region2) << "Expected same region";
    ASSERT_EQ(WIDTH * HEIGHT, region1->size) << "Expected region to cover whole grid";
}

TEST(Regions, ShouldFindTwoRegionsWhenDividedHorizontally) {
    State state;
    for (int x = 0; x <= MAX_X; x++) {
        state.occupy(x, 10, 1);
    }
    Regions regions;
    regions.findRegions(state);

    ASSERT_EQ(2, regions.count()) << "Expected two regions";

    Region* region1 = regions.regionAt(0, 0);
    Region* region2 = regions.regionAt(MAX_X, MAX_Y);

    ASSERT_TRUE(region1 != region2) << "Expected different regions";
    ASSERT_EQ(WIDTH * 10, region1->size) << "Expected region1 to cover top half of grid";
    ASSERT_EQ(WIDTH * (HEIGHT - 11), region2->size) << "Expected region2 to cover bottom half of grid";
}

TEST(Regions, ShouldFindOneRegionWhenDividedHorizontallyWithAHole) {
    State state;
    for (int x = 0; x <= MAX_X; x++) {
        if (x != 15) {
            state.occupy(x, 10, 1);
        }
    }
    Regions regions;
    regions.findRegions(state);

    ASSERT_EQ(1, regions.count()) << "Expected one region";
}

TEST(Regions, ShouldFindOneRegionWhenDividedHorizontallyWithAHoleAndTopWallIsJagged) {
    State state;
    for (int x = 0; x <= MAX_X; x++) {
        if (x != 15) {
            state.occupy(x, 10, 1);
        }
        if (x % 2 == 0) {
            state.occupy(x, 0, 1);
        }
    }
    Regions regions;
    regions.findRegions(state);

    ASSERT_EQ(1, regions.count()) << "Expected one region";
}

TEST(Regions, ShouldFindTwoRegionsWhenDividedVertically) {
    State state;
    for (int y = 0; y <= MAX_Y; y++) {
        state.occupy(10, y, 1);
    }
    Regions regions;
    regions.findRegions(state);

    ASSERT_EQ(2, regions.count()) << "Expected two regions";
}

TEST(Regions, ShouldFindTwoRegionsWhenDividedVerticallyWithAHole) {
    State state;
    for (int y = 0; y <= MAX_Y; y++) {
        if (y != 10) {
            state.occupy(10, y, 1);
        }
    }
    Regions regions;
    regions.findRegions(state);

    ASSERT_EQ(1, regions.count()) << "Expected one region";
}

TEST(Regions, ShouldFindFourRegionsWhenDividedIntoQuarters) {
    State state;
    for (int x = 0; x <= MAX_X; x++) {
        state.occupy(x, 10, 1);
    }
    for (int y = 0; y <= MAX_Y; y++) {
        state.occupy(10, y, 1);
    }

    Regions regions;
    regions.findRegions(state);

    ASSERT_EQ(4, regions.count()) << "Expected four regions";

    Region* region1 = regions.regionAt(0, 0);
    Region* region2 = regions.regionAt(0, MAX_Y);
    Region* region3 = regions.regionAt(MAX_X, 0);
    Region* region4 = regions.regionAt(MAX_X, MAX_Y);

    ASSERT_TRUE(region1 != region2);
    ASSERT_TRUE(region1 != region3);
    ASSERT_TRUE(region1 != region4);
    ASSERT_TRUE(region2 != region3);
    ASSERT_TRUE(region2 != region4);
    ASSERT_TRUE(region3 != region4);

    ASSERT_EQ(10 * 10, region1->size);
    ASSERT_EQ((HEIGHT - 11) * 10, region2->size);
    ASSERT_EQ(10 * (WIDTH - 11), region3->size);
    ASSERT_EQ((HEIGHT - 11) * (WIDTH - 11), region4->size);
}

TEST(Regions, DivideGridIntoCheckerboard) {
    State state;
    for (int x = 0; x <= MAX_X; x++) {
        for (int y = 0; y <= MAX_Y; y++) {
            if (x % 2 == y % 2) {
                state.occupy(x, y, 1);
            }
        }
    }

    Regions regions;
    regions.findRegions(state);

    ASSERT_EQ(WIDTH * HEIGHT / 2, regions.count()) << "Expected half of all squares to be regions";

    Region* region1 = regions.regionAt(1, 0);
    ASSERT_EQ(1, region1->size);
}

class MockRegions {
private:
    Region* regions[MAX_X][MAX_Y];
    int next;

public:
    MockRegions() {
        memset(regions, 0, MAX_X * MAX_Y * sizeof(Region*));
        next = 1;
    }

    void mockRegionAt(int x, int y, int size) {
        Region* region = new Region(1);
        region->size = size;
        region->id = next++;
        regions[x][y] = region;
    }

    void findRegions(const State& state) {}

    Region* regionAt(int x, int y) const {
        return regions[x][y];
    }
};

TEST(Scoring, ShouldScoreFourPlayersCorrectly) {
    State state;
    state.players[0] = Player(1, 1);
    state.players[1] = Player(3, 3);
    state.players[2] = Player(5, 5);
    state.players[3] = Player(7, 7);
    state.numPlayers = 4;

    MockRegions regions;
    regions.mockRegionAt(0, 1, 42);
    regions.mockRegionAt(4, 3, 51);
    regions.mockRegionAt(5, 6, 75);
    regions.mockRegionAt(7, 6, 66);

    Scores scores = calculateScores(regions, state);
    ASSERT_EQ(42 - 75, scores.scores[0]) << "Player in 4th place should have correct score";
    ASSERT_EQ(51 - 75, scores.scores[1]) << "Player in 3rd place should have correct score";
    ASSERT_EQ(75 - 75, scores.scores[2]) << "Player in 1st place should have correct score";
    ASSERT_EQ(66 - 75, scores.scores[3]) << "Player in 2nd place should have correct score";
}

TEST(Scoring, ShouldScoreLargestAdjacentRegion) {
    State state;
    state.numPlayers = 1;

    for (int x = 0; x <= MAX_X; x++) {
        state.occupy(x, 10, 0);
    }
    // Our player is at the intercection of two regions
    state.occupy(10, 10, 0);

    Regions regions;
    regions.findRegions(state);

    vector<Size> sizes;
    calculateSizes(regions, state, sizes);
    ASSERT_EQ(WIDTH * 10, sizes[0].size) << "Should return size of largest region";
}

TEST(Scoring, ShouldReduceScoreWhenRegionIsShared) {
    State state;
    state.players[0] = Player(1, 1);
    state.players[1] = Player(1, 3);
    state.numPlayers = 2;

    MockRegions regions;
    // this region is adjacent to both players
    regions.mockRegionAt(1, 2, 20);
    // this region is smaller, but only accessible to one player
    regions.mockRegionAt(1, 4, 11);

    vector<Size> sizes;
    calculateSizes(regions, state, sizes);
    ASSERT_EQ(10, sizes[0].size) << "Size of shared region should be halved";
    ASSERT_EQ(11, sizes[1].size) << "Player 1 should pick the smaller region, because its size is not halved";
}

Scores calculateScores_MaximiseScore(Bounds& bounds, State& state, int turn, void* dummy, void* data) {
    Scores scores;
    scores.scores[1] = 1;
    if (state.players[0].x == 11) {
        scores.scores[0] = 3;
    } else if (state.players[0].x == 9) {
        scores.scores[0] = 4;
    } else if (state.players[0].y == 11) {
        scores.scores[0] = 5;
    } else if (state.players[0].y == 9) {
        scores.scores[0] = 2;
    }
    return scores;
}

TEST(Scoring, MaximiseScore) {
    State state;
    state.numPlayers = 2;
    state.thisPlayer = 0;
    state.players[0].x = 10;
    state.players[0].y = 10;

    Bounds bounds;

    Scores scores = minimax(bounds, state, 0, (void*) calculateScores_MaximiseScore, 0);
    ASSERT_EQ(DOWN, scores.move);
}

TEST(Minimax, Regions) {
    State state;
    state.numPlayers = 2;
    state.thisPlayer = 0;
    state.players[0].x = 10;
    state.players[0].y = 10;
    state.players[1].x = 20;
    state.players[1].y = 20;

    Regions regions;
    Bounds bounds;

    minimax(bounds, state, 0, (void*) regionsRecursive, &regions);
}

void simulateMove(State& state, int player, const char* move) {
    Player p = state.players[player];
    if (move == LEFT) {
        state.occupy(p.x - 1, p.y, player);
    } else if (move == RIGHT) {
        state.occupy(p.x + 1, p.y, player);
    } else if (move == UP) {
        state.occupy(p.x, p.y - 1, player);
    } else if (move == DOWN) {
        state.occupy(p.x, p.y + 1, player);
    }
}

TEST(Minimax, DISABLED_RunLotsOfMoves) {
    State state;
    state.numPlayers = 2;

    state.occupy(26, 18, 0);
    state.occupy(16, 1, 1);

    Scores scores;
    Regions regions;
    Bounds bounds;

    for (int i = 0; i < 189; i++) {
        // printState(state);

        state.thisPlayer = i % 2;
        if (i == 188) {
            state.maxDepth = 1;
        }
        scores = minimax(bounds, state, 0, (void*) regionsRecursive, &regions);

        // cout << state.thisPlayer << " " << scores.move << endl;

        //cout << state.players[0].x << "," <<  state.players[0].y << ": " << scores.sizes[0] << "," << scores.scores[0] << endl;
        //cout << state.players[1].x << "," <<  state.players[1].y << ": " << scores.sizes[1] << "," << scores.scores[1] << endl;

        simulateMove(state, state.thisPlayer, scores.move);

        // cout << endl;
    }
}

TEST(Minimax, DISABLED_RunLotsOfMovesVoronoi) {
    State state;
    state.numPlayers = 2;

    state.occupy(26, 18, 0);
    state.occupy(16, 1, 1);

    Scores scores;
    Voronoi voronoi;
    Bounds bounds;

    for (int i = 0; i < 189; i++) {
        // printState(state);

        state.thisPlayer = i % 2;
        if (i == 188) {
            state.maxDepth = 1;
        }
        scores = minimax(bounds, state, 0, (void*) voronoiRecursive, &voronoi);

        // cout << state.thisPlayer << " " << scores.move << endl;

        //cout << state.players[0].x << "," <<  state.players[0].y << ": " << scores.sizes[0] << "," << scores.scores[0] << endl;
        //cout << state.players[1].x << "," <<  state.players[1].y << ": " << scores.sizes[1] << "," << scores.scores[1] << endl;

        simulateMove(state, state.thisPlayer, scores.move);

        // cout << endl;
    }
}

TEST(Voronoi, EmptyBoardEqualRegions) {
    State state;
    state.numPlayers = 2;
    state.occupy(5, 10, 0);
    state.occupy(MAX_X - 5, 10, 1);

    Voronoi voronoi;
    voronoi.calculate(state);

    ASSERT_EQ(WIDTH * HEIGHT / 2 - 1, voronoi.playerRegionSize(0));
    ASSERT_EQ(WIDTH * HEIGHT / 2 - 1, voronoi.playerRegionSize(1));
    ASSERT_TRUE(voronoi.regionForPlayer(0) == voronoi.regionForPlayer(1));
}

TEST(Voronoi, UnevenlyFilledBoard) {
    State state;
    state.numPlayers = 2;
    state.occupy(4, 4, 0);
    for (int i = 2; i <= 27; i++) {
        state.occupy(i, 12, 1);
    }
    state.occupy(27, 13, 1);
    state.occupy(27, 14, 1);
    state.occupy(27, 15, 1);
    state.occupy(26, 15, 1);
    state.occupy(25, 15, 1);

    Voronoi voronoi;
    voronoi.calculate(state);

    ASSERT_EQ(306, voronoi.playerRegionSize(0));
    ASSERT_EQ(243, voronoi.playerRegionSize(1));
}

TEST(Scoring, DISABLED_Timing) {
    State state;
    state.numPlayers = 2;
    state.occupy(4, 4, 0);
    for (int i = 2; i <= 27; i++) {
        state.occupy(i, 12, 1);
    }
    state.occupy(27, 13, 1);
    state.occupy(27, 14, 1);
    state.occupy(27, 15, 1);
    state.occupy(26, 15, 1);
    state.occupy(25, 15, 1);

    Voronoi voronoi;
    long start = millis();
    for (int i = 0; i < 10000; i++) {
        voronoi.calculate(state);
    }
    long elapsed = millis() - start;
    cerr << elapsed << endl; // 470ms

    Regions regions;
    start = millis();
    for (int i = 0; i < 10000; i++) {
        regions.findRegions(state);
    }
    elapsed = millis() - start;
    cerr << elapsed << endl; // 170ms
}

TEST(Voronoi, ShouldFindTwoRegionsWhenDividedHorizontally) {
    State state;
    state.numPlayers = 2;
    for (int x = 0; x <= MAX_X; x++) {
        state.occupy(x, 10, 1);
    }

    state.occupy(0, 0, 0);
    state.occupy(MAX_X, MAX_Y, 1);

    Voronoi voronoi;
    voronoi.calculate(state);

    ASSERT_FALSE(voronoi.regionForPlayer(0) == voronoi.regionForPlayer(1)) << "Expected separate regions";

    ASSERT_EQ(WIDTH * 10 - 1, voronoi.playerRegionSize(0)) << "Expected region1 to cover top half of grid";
    ASSERT_EQ(WIDTH * (HEIGHT - 11) - 1, voronoi.playerRegionSize(1)) << "Expected region2 to cover bottom half of grid";
}

TEST(Scoring, ConnectedRegionsScoreOnVoronoi) {
    State state;
    state.numPlayers = 2;
    state.occupy(4, 4, 0);
    for (int i = 2; i <= 27; i++) {
        state.occupy(i, 12, 1);
    }
    state.occupy(27, 13, 1);
    state.occupy(27, 14, 1);
    state.occupy(27, 15, 1);
    state.occupy(26, 15, 1);
    state.occupy(25, 15, 1);

    Voronoi voronoi;

    Scores scores = calculateScores(voronoi, state);
    ASSERT_EQ(306, scores.scores[0]);
    ASSERT_EQ(243, scores.scores[1]);
}

TEST(Scoring, DisconnectedRegionsScoreGetBonus) {
    State state;
    state.numPlayers = 2;
    for (int x = 0; x <= MAX_X; x++) {
        state.occupy(x, 10, 1);
    }

    state.occupy(0, 0, 0);
    state.occupy(MAX_X, MAX_Y, 1);

    Voronoi voronoi;

    Scores scores = calculateScores(voronoi, state);

    ASSERT_EQ(299 + 1000, scores.scores[0]);
    ASSERT_EQ(269 - 1000, scores.scores[1]);
}

TEST(Scoring, ThreeWayBonus) {
    State state;
    state.numPlayers = 3;
    for (int x = 0; x <= MAX_X; x++) {
        state.occupy(x, 10, 1);
    }
    for (int y = 0; y < 10; y++) {
        state.occupy(20, y, 1);
    }

    state.occupy(MAX_X, 0, 0);
    state.occupy(0, 0, 1);
    state.occupy(MAX_X, MAX_Y, 2);

    Voronoi voronoi;

    Scores scores = calculateScores(voronoi, state);

    ASSERT_EQ(89 - 500, scores.scores[0]);
    ASSERT_EQ(199 - 500, scores.scores[1]);
    ASSERT_EQ(269 + 1000, scores.scores[2]);
}

TEST(Scoring, CutOffOnAllSides) {
    State state;
    state.numPlayers = 2;

    // Player 0 is in the centre of a 3x3 block
    for (int x = 0; x < 3; x++) {
        for (int y = 0; y < 3; y++) {
            state.occupy(x, y, 0);
        }
    }
    state.occupy(1, 1, 0);

    state.occupy(MAX_X, MAX_Y, 1);

    Voronoi voronoi;

    Scores scores = calculateScores(voronoi, state);

    ASSERT_EQ(-1000, scores.scores[0]);
    ASSERT_EQ(WIDTH * HEIGHT - 10 + 1000, scores.scores[1]);
}

TEST(Scoring, Dead) {
    State state;
    state.numPlayers = 2;

    state.occupy(0, 0, 0);
    state.occupy(MAX_X, MAX_Y, 1);

    state.kill(0);

    Voronoi voronoi;

    Scores scores = calculateScores(voronoi, state);

    ASSERT_EQ(-1000, scores.scores[0]);
    ASSERT_EQ(WIDTH * HEIGHT - 1 + 1000, scores.scores[1]);
}

class CalculateScoresMock {
private:
    int expectedCalls;
    int calls;
    Scores scores[100];

public:
    CalculateScoresMock() {
        expectedCalls = 0;
        calls = 0;
    }

    void expectCall(int score0, int score1) {
        Scores s;
        s.scores[0] = score0;
        s.scores[1] = score1;
        scores[expectedCalls++] = s;
    }

    Scores call() {
        calls++;
        if (calls > expectedCalls) {
            ADD_FAILURE() << "Unexpected call to score calculator";
            Scores scores;
            scores.scores[0] = scores.scores[1] = -9999;
            return scores;
        } else {
            return scores[calls - 1];
        }
    }
};

Scores mockCalculateScores(Bounds& bounds, State& state, int turn, void* dummy, void* data) {
    CalculateScoresMock* mock = (CalculateScoresMock*) data;
    return mock->call();
}

TEST(Bounding, ShouldPruneWhenBoundExceededByFirstChild) {
    State state;
    state.numPlayers = 2;
    state.thisPlayer = 0;

    state.occupy(5, 5, 0);
    state.occupy(15, 15, 1);

    CalculateScoresMock mock;
    mock.expectCall(40, 100);

    Bounds bounds;
    bounds.bounds[0] = 50;

    Scores scores = minimax(bounds, state, 1, (void*) mockCalculateScores, &mock);
    ASSERT_EQ(40, scores.scores[0]);
    ASSERT_EQ(100, scores.scores[1]);
}

Scores randRecursive(Bounds& bounds, State& state, int turn, void* sc, void* data) {
    if (turn >= state.maxDepth || state.isTimeLimitReached()) {
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

    state.resetTimer();
    long start = millis();
    Scores scores = minimax(bounds, state, 0, (void*) voronoiRecursive, &voronoi);
    // Scores scores = minimax(bounds, state, 0, (void*) randRecursive, 0);
    // Scores scores = minimax(bounds, state, 0, (void*) regionsRecursive, &regions);
    long elapsed = millis() - start;
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

TEST(Bounding, DISABLED_Timing) {
    State state;
    state.numPlayers = 2;
    state.thisPlayer = 0;
    state.timeLimitEnabled = false;

    srand(time(0));
    // randomlyPopulate(state);

    state.occupy(9, 12, 0);
    state.occupy(21, 2, 1);
    state.print();

    Scores s1 = timedSearch(state, true);
    Scores s2 = timedSearch(state, false);
    cout << s1.scores[0] << endl;
    cout << s1.scores[1] << endl;
    ASSERT_EQ(s1.scores[0], s2.scores[0]) << "Pruning should not affect the search result";
    ASSERT_EQ(s1.scores[1], s2.scores[1]) << "Pruning should not affect the search result";
}

TEST(State, DeadPlayersDoNotOccupySpace) {
    State state;
    state.occupy(4, 4, 0);
    state.occupy(5, 5, 1);

    state.kill(1);

    ASSERT_FALSE(state.occupied(3, 3));
    ASSERT_TRUE(state.occupied(4, 4));
    ASSERT_FALSE(state.occupied(5, 5));

    state.revive(1);
    ASSERT_FALSE(state.occupied(3, 3));
    ASSERT_TRUE(state.occupied(4, 4));
    ASSERT_TRUE(state.occupied(5, 5));
}

struct TestResults_PSDWNLM {
    int calls;
    int turn;
    bool occupied;
};

Scores calculateScores_PSDWNLM(Bounds& bounds, State& state, int turn, void* dummy, void* data) {
    TestResults_PSDWNLM* results = (TestResults_PSDWNLM*) data;
    results->calls++;
    results->turn = turn;
    results->occupied = state.occupied(1, 1);
    return Scores();
}

TEST(Minimax, PlayerShouldDieWhenNoLegalMoves) {
    Bounds bounds;
    State state;
    state.numPlayers = 2;
    state.thisPlayer = 0;

    // Player 0 is in the centre of a 3x3 block
    for (int x = 0; x < 3; x++) {
        for (int y = 0; y < 3; y++) {
            state.occupy(x, y, 0);
        }
    }
    state.occupy(1, 1, 0);

    TestResults_PSDWNLM results;
    results.calls = 0;

    Scores scores = minimax(bounds, state, 0, (void*) calculateScores_PSDWNLM, &results);

    ASSERT_EQ(1, results.calls) << "Expected one call";
    ASSERT_FALSE(results.occupied) << "Expected player 0 to be removed";
    ASSERT_TRUE(state.occupied(1, 1)) << "Expected player to be revived after the call";
}
