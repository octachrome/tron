#include "tron.cc"

#include "gtest/gtest.h"
#include <fstream>


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

    Voronoi voronoi;
    Bounds bounds;

    state.resetTimer();
    long start = millis();
    Scores scores = minimax(bounds, state, 0, (void*) voronoiRecursive, &voronoi);
    // Scores scores = minimax(bounds, state, 0, (void*) randRecursive, 0);
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

TEST(State, SimpleOccupy) {
    State state;
    ASSERT_FALSE(state.occupied(0, 0));
    state.occupy(0, 0, 0);
    ASSERT_TRUE(state.occupied(0, 0));
    state.unoccupy(0, 0, 0);
    ASSERT_FALSE(state.occupied(0, 0));
}

TEST(State, DoubleOccupancy) {
    State state;
    state.occupy(0, 0, 0);
    state.occupy(0, 0, 1);
    ASSERT_TRUE(state.occupied(0, 0));
    state.unoccupy(0, 0, 0);
    ASSERT_TRUE(state.occupied(0, 0));
    state.unoccupy(0, 0, 1);
    ASSERT_FALSE(state.occupied(0, 0));
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

TEST(State, RevivingDeadPlayersPreservesOtherPlayers) {
    State state;
    state.occupy(0, 0, 0);
    state.occupy(0, 0, 1);
    ASSERT_TRUE(state.occupied(0, 0));
    state.kill(1);
    ASSERT_TRUE(state.occupied(0, 0));
    state.kill(0);
    ASSERT_FALSE(state.occupied(0, 0));
    state.revive(0);
    ASSERT_TRUE(state.occupied(0, 0));
    state.unoccupy(0, 0, 0);
    ASSERT_FALSE(state.occupied(0, 0));
    state.revive(1);
    ASSERT_TRUE(state.occupied(0, 0));
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

    minimax(bounds, state, 0, (void*) calculateScores_PSDWNLM, &results);

    ASSERT_EQ(1, results.calls) << "Expected one call";
    ASSERT_EQ(1, results.turn) << "Expected player 0 to miss their turn";
    ASSERT_FALSE(results.occupied) << "Expected player 0 to be removed";
    ASSERT_TRUE(state.occupied(1, 1)) << "Expected player to be revived after the call";
}

TEST(Minimax, DeadPlayerShouldNotGetTurn) {
    Bounds bounds;
    State state;
    state.numPlayers = 2;
    state.thisPlayer = 0;

    state.occupy(1, 1, 0);
    state.occupy(5, 5, 1);

    state.kill(0);

    TestResults_PSDWNLM results;
    results.calls = 0;

    minimax(bounds, state, 0, (void*) calculateScores_PSDWNLM, &results);

    ASSERT_EQ(1, results.calls) << "Expected one call";
    ASSERT_EQ(1, results.turn) << "Expected player 0 to miss their turn";
    ASSERT_FALSE(results.occupied) << "Expected player 0 to be missing";
    ASSERT_FALSE(state.occupied(1, 1)) << "Expected player 0 to be still missing after the call";
}

void readBoard(State& state, istream& is) {
    Player players[PLAYERS];
    for (int i = 0; i < PLAYERS; i++) {
        players[i].x = -1;
    }
    string line;
    int y = 0;
    while (getline(is, line)) {
        for (unsigned x = 0; x < line.size(); x++) {
            char cell = line[x];
            int player = cell - '0';
            if (player >= 0 && player < PLAYERS) {
                state.occupy(x, y, player);
            } else {
                player = cell - 'A';
                if (player >= 0 && player < PLAYERS) {
                    players[player].x = x;
                    players[player].y = y;
                }
            }
        }
        y++;
    }
    for (unsigned i = 0; i < PLAYERS; i++) {
        if (players[i].x >= 0) {
            state.occupy(players[i].x, players[i].y, i);
        }
    }
}

void readBoard(State& state, const char* board) {
    istringstream is(board);
    readBoard(state, is);
}

TEST(Util, BoardReader) {
    State state;
    readBoard(state,
        "....*....*....*....*....*....*\n"
        "....*....*....*....*....*...0A\n"
        "....*....*....*....*C...*.000*\n"
        "....*B1111111111.2222...*00..*\n"
        "....*....*....*1.2.*...000...*\n"
        "....*....*....11.2.*.000*....*\n"
        "....*....*....1..2.*.000000000\n"
        "..1111111111..1..2.00000000000\n"
        "..1.*....*.11.1..2.*....*....*\n"
        "....33...*..111..22*....*....*\n"
        "..333D...*....*...2*....*....*\n"
        "..3333333333333333222...*....*\n"
        "....*....*....*..333222222...*\n"
        "....*....*....*....3322222...*\n"
        "....*....*....*....*3222*....*\n"
        "....*....*....*....*3..22....*\n"
        "....*....*....*....*33..2....*\n"
        "....*....*....*....*.3..*....*\n"
        "....*....*....*....*.3..*....*\n"
        "....*....*....*....*....*....*\n");

    ASSERT_TRUE(state.occupied(2, 7));
    ASSERT_FALSE(state.occupied(3, 8));
    ASSERT_EQ(5, state.players[1].x);
    ASSERT_EQ(3, state.players[1].y);
}

TEST(Minimax, DISABLED_ZeroScoreWhenEnclosed) {
    State state;
    state.numPlayers = 4;
    state.thisPlayer = 0;
    state.maxDepth = 26;
    state.timeLimitEnabled = false;
    readBoard(state,
        "....*....*....*....*....*....*\n"
        "....*....*....*..22222..*...00\n"
        "....*....*....*..2222C..*.0000\n"
        "..11111111111111.2222...*00000\n"
        ".11.*....*....*1.2.*...000.000\n"
        ".1..*....*....11.2.*.000*..A00\n"
        ".1..*....*....1..2.*.000000000\n"
        "B11111111111..1..2.00000000000\n"
        "..13333..*.11.1..2.*....*....*\n"
        ".D33333..*..111..22*....*....*\n"
        "..33333..*....*...2*....*....*\n"
        "..3333333333333333222...*....*\n"
        "....*....*....*..333222222...*\n"
        "....*....*....*....3322222...*\n"
        "....*....*....*....*3222*....*\n"
        "....*....*....*....*3..22....*\n"
        "....*....*....*....*33..2....*\n"
        "....*....*....*....*.3..*....*\n"
        "....*....*....*....*.3..*....*\n"
        "....*....*....*....*....*....*\n");

    Voronoi voronoi;
    Bounds bounds;

    Scores scores = minimax(bounds, state, 0, (void*) voronoiRecursive, &voronoi);

    cout << scores.scores[0] << endl;
    cout << scores.scores[1] << endl;
    cout << scores.scores[2] << endl;
    cout << scores.scores[3] << endl;
    cout << scores.move << endl;
}

TEST(Minimax, BothPlayersDieButTheSecondShouldScore) {
    State state;
    state.numPlayers = 2;
    state.thisPlayer = 0;
    state.maxDepth = 1;
    state.timeLimitEnabled = false;

    readBoard(state,
        "000..111\n"
        "0A0..1B1\n"
        "000..1.1\n"
        ".....111\n");

    Voronoi voronoi;
    Bounds bounds;

    Scores scores = minimax(bounds, state, 0, (void*) voronoiRecursive, &voronoi);

    ASSERT_EQ(-1000, scores.scores[0]) << "First player loses";
    ASSERT_EQ(1001, scores.scores[1]) << "Second player wins";
}

TEST(Minimax, BothPlayersDieInThreeMovesButTheSecondShouldScore) {
    State state;
    state.numPlayers = 2;
    state.thisPlayer = 0;
    state.maxDepth = 1;
    state.timeLimitEnabled = false;

    readBoard(state,
        "000..111\n"
        "0A0..1B1\n"
        "0.0..1.1\n"
        "000..1.1\n"
        ".....111\n");

    Voronoi voronoi;
    Bounds bounds;

    Scores scores = minimax(bounds, state, 0, (void*) voronoiRecursive, &voronoi);

    ASSERT_EQ(-1000, scores.scores[0]) << "First player loses";
    ASSERT_EQ(1002, scores.scores[1]) << "Second player wins";
}
