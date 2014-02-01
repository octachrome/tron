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
