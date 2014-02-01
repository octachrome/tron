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
