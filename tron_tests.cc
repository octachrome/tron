#include "tron.cc"

#include "gtest/gtest.h"

TEST(RegionTest, ShouldFindSingleRegionWhenUnoccupied) {
    State state;
    Regions regions;
    regions.findRegions(state);

    ASSERT_EQ(1, regions.count()) << "Expected one region";

    Region* region1 = regions.regionAt(0, 0);
    Region* region2 = regions.regionAt(MAX_X, MAX_Y);

    ASSERT_TRUE(region1 == region2) << "Expected same region";
    ASSERT_EQ(WIDTH * HEIGHT, region1->size) << "Expected region to cover whole grid";
}

TEST(RegionTest, ShouldFindTwoRegionsWhenDividedHorizontally) {
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

TEST(RegionTest, ShouldFindOneRegionWhenDividedHorizontallyWithAHole) {
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

TEST(RegionTest, ShouldFindOneRegionWhenDividedHorizontallyWithAHoleAndTopWallIsJagged) {
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

TEST(RegionTest, ShouldFindTwoRegionsWhenDividedVertically) {
    State state;
    for (int y = 0; y <= MAX_Y; y++) {
        state.occupy(10, y, 1);
    }
    Regions regions;
    regions.findRegions(state);

    ASSERT_EQ(2, regions.count()) << "Expected two regions";
}

TEST(RegionTest, ShouldFindTwoRegionsWhenDividedVerticallyWithAHole) {
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

TEST(RegionTest, ShouldFindFourRegionsWhenDividedIntoQuarters) {
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

TEST(RegionTest, DivideGridIntoCheckerboard) {
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

public:
    MockRegions() {
        memset(regions, 0, MAX_X * MAX_Y * sizeof(Region*));
    }

    void mockRegionAt(int x, int y, int size) {
        Region* region = new Region(1);
        region->size = size;
        regions[x][y] = region;
    }

    void findRegions(const State& state) {}

    Region* regionAt(int x, int y) const {
        return regions[x][y];
    }
};

TEST(ScoreTest, ShouldScoreFourPlayersCorrectly) {
    State state;
    state.players[0] = Player(1, 1);
    state.players[1] = Player(3, 3);
    state.players[2] = Player(5, 5);
    state.players[3] = Player(7, 7);
    state.numPlayers = 4;

    MockRegions regions;
    regions.mockRegionAt(0, 1, 4);
    regions.mockRegionAt(4, 3, 5);
    regions.mockRegionAt(5, 6, 7);
    regions.mockRegionAt(7, 6, 6);

    Scores scores = calculateScores(regions, state);
    ASSERT_EQ(100, scores.scores[0]) << "Player in 4th place should score 100";
    ASSERT_EQ(200, scores.scores[1]) << "Player in 3rd place should score 200";
    ASSERT_EQ(400, scores.scores[2]) << "Player in 1st place should score 400";
    ASSERT_EQ(300, scores.scores[3]) << "Player in 2nd place should score 300";
}

TEST(ScoreTest, ShouldScoreLargestAdjacentRegion) {
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

Scores mockCalculateScores(Regions& regions, State& state, int turn, void* dummy) {
    Scores scores;
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

TEST(ScoreTest, MaximiseScore) {
    State state;
    state.numPlayers = 4;
    state.thisPlayer = 0;
    state.players[0].x = 10;
    state.players[0].y = 10;

    Regions regions;

    Scores scores = maxScore(regions, state, 0, (void*) mockCalculateScores);
    ASSERT_EQ(DOWN, scores.move);
}

TEST(ScoreTest, Minimax) {
    State state;
    state.numPlayers = 2;
    state.thisPlayer = 0;
    state.players[0].x = 10;
    state.players[0].y = 10;
    state.players[1].x = 20;
    state.players[1].y = 20;

    Regions regions;

    Scores scores = maxScore(regions, state, 0, (void*) minimax);
}
