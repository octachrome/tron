//#define TRON_TRACE
#include "tron.cc"

#include "gtest/gtest.h"
#include <fstream>

Scores minimax(Bounds& parentBounds, State& state, int turn, void* sc, void* data) {
    Scores scores;
    minimax(scores, parentBounds, state, turn, sc, data);
    return scores;
}

Scores calculateScores(Voronoi& voronoi, State& state) {
    Scores scores;
    calculateScores(scores, voronoi, state);
    return scores;
}

Scores scoreCalculator_MaximiseScore(Bounds& bounds, State& state, int turn, void* dummy, void* data) {
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

    Scores scores = minimax(bounds, state, 0, (void*) scoreCalculator_MaximiseScore, 0);
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

class ScoreCalculatorMock {
private:
    int expectedCalls;
    int calls;
    Scores scores[100];

public:
    ScoreCalculatorMock() {
        expectedCalls = 0;
        calls = 0;
    }

    void expectCall(Scores s) {
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

Scores mockScoreCalculator(Bounds& bounds, State& state, int turn, void* dummy, void* data) {
    ScoreCalculatorMock* mock = (ScoreCalculatorMock*) data;
    return mock->call();
}

TEST(Bounding, ShouldPruneWhenBoundExceededByFirstChild) {
    State state;
    state.numPlayers = 2;
    state.thisPlayer = 0;
    state.pruningEnabled = true;

    state.occupy(5, 5, 0);
    state.occupy(15, 15, 1);

    Scores s(40, 100);
    s.regions[0] = 0;
    s.regions[0] = 0;

    ScoreCalculatorMock mock;
    mock.expectCall(s);

    Bounds bounds;
    bounds.bounds[0] = 50;

    Scores scores = minimax(bounds, state, 1, (void*) mockScoreCalculator, &mock);
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

Scores scoreCalculator_PSDWNLM(Bounds& bounds, State& state, int turn, void* dummy, void* data) {
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

    minimax(bounds, state, 0, (void*) scoreCalculator_PSDWNLM, &results);

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

    minimax(bounds, state, 0, (void*) scoreCalculator_PSDWNLM, &results);

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
            state.numPlayers = i + 1;
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

TEST(State, ReadTurnShouldDetectDeath) {
    istringstream is(
        "2 0\n"
        "0 0 0 0\n"
        "4 4 4 4\n"
        "2 0\n"
        "0 0 0 1\n"
        "4 4 4 4\n");

    State state;
    state.readTurn(is);

    ASSERT_TRUE(state.isAlive(0));
    ASSERT_TRUE(state.isAlive(1));

    state.readTurn(is);

    ASSERT_TRUE(state.isAlive(0));
    ASSERT_FALSE(state.isAlive(1));
}

TEST(Minimax, ScoreBasedOnWhoDiesFirst) {
    State state;
    state.numPlayers = 3;
    state.thisPlayer = 0;
    state.maxDepth = 5;
    state.timeLimitEnabled = false;

    readBoard(state,
        "000..111..222\n"
        "0A0..1B1..2C2\n"
        "000..1.1..2.2\n"
        ".....111..2.2\n"
        "..........222\n");

    Voronoi voronoi;
    Bounds bounds;

    Scores scores = minimax(bounds, state, 0, (void*) voronoiRecursive, &voronoi);

    ASSERT_EQ(-1000, scores.scores[0]) << "First player in third place";
    ASSERT_EQ(-500, scores.scores[1]) << "Second player in second placce";
    ASSERT_EQ(1501, scores.scores[2]) << "Third player in first place";
}

TEST(Minimax, ScoreFairlyWithOneDeathAndAClearWinner) {
    State state;
    state.numPlayers = 3;
    state.thisPlayer = 0;
    state.maxDepth = 1;
    state.timeLimitEnabled = false;

    readBoard(state,
        "000..111..222\n"
        "0A0..1B1..2C2\n"
        "000..1.1..2.2\n"
        ".....111..2.2\n"
        "..........222\n");

    Voronoi voronoi;
    Bounds bounds;

    Scores scores = minimax(bounds, state, 0, (void*) voronoiRecursive, &voronoi);

    ASSERT_EQ(-1000, scores.scores[0]) << "First player in third place";
    ASSERT_EQ(-499, scores.scores[1]) << "Second player in second placce";
    ASSERT_EQ(1502, scores.scores[2]) << "Third player in first place";
}

TEST(Minimax, BadDecision1) {
    State state;
    state.numPlayers = 4;
    state.thisPlayer = 0;
    state.maxDepth = 8;
    state.pruneMargin = 1;
    state.timeLimitEnabled = false;

    readBoard(state,
        "....*....*....*....*....*....*\n"
        "....*....*.11.*....*....*....*\n"
        "..2222...*.1..*....*....*....*\n"
        "....*2...*.1..*....*....*....*\n"
        "....*22..*.1..*....*....*....*\n"
        "....*.2..*.11.*....*....*....*\n"
        "....*.2222..1.*....*....*..3D*\n"
        "....*....2..11*....*....*..33*\n"
        "....*....2...11.3333333333333*\n"
        "....*....2....1133.*....*....*\n"
        "....*....22..111.3.*....*0000A\n"
        "....*....*2..1*..3.*....*0000*\n"
        "....*....*2..1*..3.*...000000*\n"
        "....*....*2..1*..333...00000.*\n"
        "....*....*2..1*....33333*00000\n"
        "....*....*2..1*....*....*00000\n"
        "....*....*2..1*....*....*....*\n"
        "....*....*2.11*....*....*....*\n"
        "....*....C2211B....*....*....*\n"
        "....*....22211*....*....*....*\n");

    Voronoi voronoi;
    Bounds bounds;

    state.pruningEnabled = false;
    Scores scores1 = minimax(bounds, state, 0, (void*) voronoiRecursive, &voronoi);

    state.pruningEnabled = true;
    Scores scores2 = minimax(bounds, state, 0, (void*) voronoiRecursive, &voronoi);

    ASSERT_EQ(scores1.scores[0], scores2.scores[0]) << "Expected same result for p0 with and without pruning";
    ASSERT_EQ(scores1.scores[1], scores2.scores[1]) << "Expected same result for p0 with and without pruning";
    ASSERT_EQ(scores1.scores[2], scores2.scores[2]) << "Expected same result for p0 with and without pruning";
    ASSERT_EQ(scores1.scores[3], scores2.scores[3]) << "Expected same result for p0 with and without pruning";
}

void playTurn(State& state, const char* input) {
    istringstream is(input);
    state.readTurn(is);
}

TEST(State, FirstPlayersFullTrailShouldBeOccupied) {
    State state;
    state.numPlayers = 2;
    state.thisPlayer = 1;

    playTurn(state,
        "2 1\n"
        "0 0 0 1\n"
        "5 5 5 5\n");

    ASSERT_TRUE(state.occupied(0, 0)) << "Expected p0 to occupy 0,0";
    ASSERT_TRUE(state.occupied(0, 0)) << "Expected p0 to occupy 0,1";
    ASSERT_TRUE(state.occupied(0, 0)) << "Expected p1 to occupy 5,5";
}

TEST(Minimax, BadDecision2) {
    State state;
    state.numPlayers = 4;
    state.thisPlayer = 0;
    state.maxDepth = 8;
    state.timeLimitEnabled = false;

    readBoard(state,
        "....*....*....*....*....*....*\n"
        "....*....3333.*....*....*....*\n"
        "..3333..33333.*....*....*...2*\n"
        "..3333..333222*....*....*...2*\n"
        "..3333.3333222*....*222.*...2*\n"
        "..333..333322.*....*222.....2*\n"
        "..333..333322.*....*.2222...2*\n"
        "..33...3..322.*....*.2222...2*\n"
        ".A333.33..32222........22...2*\n"
        ".0333.3...322.2....*...22..22*\n"
        ".0333D3...3.222....*...22.22..\n"
        ".033333..*32222222222222222..*\n"
        ".0.3333..*3C2222222222.......*\n"
        ".003*.3..*332222222222......00\n"
        ".003333..*..000000000000000000\n"
        ".00330000000000000000000000000\n"
        ".00330...*....*....*....*....*\n"
        ".003300000000000...*....*....*\n"
        ".003300000000000...*....*....*\n"
        ".00000........*....*....*....*\n");

    Voronoi voronoi;
    Bounds bounds;

    // player 1 is dead already
    state.players[1].x = 0;
    state.players[1].y = 0;

    playTurn(state,
        "4 0\n"
        "12 14 1 7\n"
        "0 0 0 0\n"
        "28 2 11 12\n"
        "11 13 5 9\n");

    state.pruningEnabled = false;
    Scores scores1 = minimax(bounds, state, 0, (void*) voronoiRecursive, &voronoi);

    state.pruningEnabled = true;
    Scores scores2 = minimax(bounds, state, 0, (void*) voronoiRecursive, &voronoi);

    ASSERT_EQ(scores1.scores[0], scores2.scores[0]) << "Expected same result for p0 with and without pruning";
    ASSERT_EQ(scores1.scores[1], scores2.scores[1]) << "Expected same result for p0 with and without pruning";
    ASSERT_EQ(scores1.scores[2], scores2.scores[2]) << "Expected same result for p0 with and without pruning";
    ASSERT_EQ(scores1.scores[3], scores2.scores[3]) << "Expected same result for p0 with and without pruning";
}

TEST(Minimax, BadDecision4) {
    State state;
    state.numPlayers = 4;
    state.thisPlayer = 2;
    state.maxDepth = 8;
    state.timeLimitEnabled = false;
    state.pruningEnabled = false;

    // Game #983770: should have killed p3
    readBoard(state,
        "....1...0000000000000000000000\n"
        "....11110000000000000000000000\n"
        "....11.111111111111111..*...00\n"
        "....11.11111B.*....*.1333...00\n"
        "....111..11111111..*.130000.00\n"
        "....1.11111111111..*.130*...00\n"
        "....11111111111111.*.130000000\n"
        "....2222222222222111113.*.0000\n"
        "....*....*....*22333333.*.000*\n"
        "....*....*....2233.*....*.000*\n"
        "22222222222222233..*....*.0A.*\n"
        "2.33333333333333...*....*....*\n"
        "22333333333333333333333333...*\n"
        ".22222222222222222222222*3...*\n"
        "....*....*....*....*...233...*\n"
        "....*....*....*....*...233...*\n"
        "....*....*....*....*...223...*\n"
        "....*....*....*....*.22223...*\n"
        "....*....*..C2222222223333...*\n"
        "....*....*...D333333333.*....*\n");

    Voronoi voronoi;
    Bounds bounds;

    Scores scores = minimax(bounds, state, 0, (void*) voronoiRecursive, &voronoi);
    ASSERT_EQ(scores.move, DOWN) << "Expected p2 to kill";
}

TEST(Minimax, KillingSamePlayerRepeatedlyShouldDoNothing) {
    State state;
    state.numPlayers = 3;
    state.thisPlayer = 0;

    state.kill(1);
    state.kill(1);
    state.kill(1);
    state.kill(1);

    ASSERT_EQ(1, state.deathCount);
    ASSERT_EQ(1, state.deadList[0]);
}

TEST(Scoring, ShouldDetectOpponentsInSameRegion) {
    State state;
    state.numPlayers = 3;
    state.thisPlayer = 0;

    readBoard(state,
        "....*1...*....*..0.*....*....*\n"
        "....*1...*....*..0.*....*....*\n"
        "....*B...*....*..0.*....*....*\n"
        "....*....*....*..0.*....*....*\n"
        "....*....*....*..0.*....*....*\n"
        "....*....*....*..0.*....*....*\n"
        "....*....*....*..0.*....*....*\n"
        "....*....*....*..0.*....*....*\n"
        "....*....*....*..0.*....*....*\n"
        "....*....*....*..0.*....*....*\n"
        "....*....*....*..0.*....*....*\n"
        "....*....*....*..0.*....*....*\n"
        "....*....*....*..0.*....*....*\n"
        "....*....*....*..0.*....*....*\n"
        "....*....*....*..0.*....*....*\n"
        "....*....*....*..0.*....*....*\n"
        "....*....*....*..0.*....*....*\n"
        "....*C...*....*..0.*....*....*\n"
        "....*2...*....*..0.*....*....*\n"
        "....*2...*....*..000A...*....*\n");

    Voronoi voronoi;

    Scores scores = calculateScores(voronoi, state);
    // ASSERT_TRUE(scores.areOpponents(1, 2)) << "Players 1 and 2 are opponents";
    // ASSERT_TRUE(scores.areOpponents(2, 1)) << "Players 2 and 1 are opponents";
    // ASSERT_FALSE(scores.areOpponents(0, 1)) << "Players 0 and 1 are not opponents";
    // ASSERT_FALSE(scores.areOpponents(1, 0)) << "Players 1 and 0 are not opponents";
    // ASSERT_FALSE(scores.areOpponents(0, 2)) << "Players 0 and 2 are not opponents";
    // ASSERT_FALSE(scores.areOpponents(2, 0)) << "Players 2 and 0 are not opponents";
}

TEST(Scoring, PlayerWhoGainsLargestRegionIsEveryonesOpponent) {
    State state;
    state.numPlayers = 3;
    state.thisPlayer = 0;

    readBoard(state,
        "....*....*....*..0.*....*....*\n"
        "....*....*....*..0.*....*....*\n"
        "....*....*....*..0.*....*....*\n"
        "....*....*....*..0.*....*....*\n"
        "....*...B*....*..0.*....*....*\n"
        "....*...1*....*..0.*....*....*\n"
        "....*...1111111110.*....*....*\n"
        "111111111111111110.*....*....*\n"
        "....*....*....*..0.*....*....*\n"
        "....*....*....*..0.*....*....*\n"
        "....*....*....*..0.*....*....*\n"
        "....*....*....*..0.*....*....*\n"
        "....*....*....*..0.*....*....*\n"
        "....*....*....*..0.*....*....*\n"
        "....*....*....*..0.*....*....*\n"
        "....*....*....*..0.*....*....*\n"
        "....*....*....*..0.*....*....*\n"
        "....*C...*....*..0.*....*....*\n"
        "....*2...*....*..0.*....*....*\n"
        "....*2...*....*..000A...*....*\n");

    Voronoi voronoi;

    Scores scores = calculateScores(voronoi, state);
    // ASSERT_FALSE(scores.areOpponents(1, 2)) << "Players 1 and 2 are not opponents";
    // ASSERT_FALSE(scores.areOpponents(2, 1)) << "Players 2 and 1 are not opponents";
    // ASSERT_TRUE(scores.areOpponents(0, 1)) << "Players 0 and 1 are opponents";
}

class GameSim {
public:
    long maxTime;
    State states[PLAYERS];
    Scores scores;
    Voronoi voronoi;
    Bounds bounds;
    int x[PLAYERS];
    int y[PLAYERS];
    int numPlayers;
    int turn;

    GameSim(int* sx, int* sy, int p_numPlayers, bool pruningEnabled) {
        maxTime = -1;
        numPlayers = p_numPlayers;
        turn = 0;

        for (int i = 0; i < numPlayers; i++) {
            x[i] = sx[i];
            y[i] = sy[i];
            states[i].timeLimitEnabled = false;
            states[i].pruningEnabled = pruningEnabled;
            states[i].pruneMargin = 1;
            states[i].maxDepth = 8;
        }
    }

    void playTurn() {
        int i = turn % numPlayers;
        cerr << "Turn " << i << endl;
        stringstream ss;
        ss << numPlayers << " " << i << endl;
        for (int k = 0; k < numPlayers; k++) {
            ss << "0 0 " << x[k] << " " << y[k] << endl;
        }
        long start = millis();
        states[i].readTurn(ss);
        minimax(scores, bounds, states[i], 0, (void*) voronoiRecursive, &voronoi);
        long duration = millis() - start;
        if (duration > maxTime) {
            maxTime = duration;
        }
        if (scores.move[0] == 'L') {
            x[i]--;
        } else if (scores.move[0] == 'R') {
            x[i]++;
        } else if (scores.move[0] == 'U') {
            y[i]--;
        } else if (scores.move[0] == 'D') {
            y[i]++;
        } else {
            cerr << "Illegal move " << scores.move << " for player " << i << endl;
        }
        turn++;
    }
};

TEST(Pruning, DISABLED_PlaySelf) {
    int numPlayers = 3;
    int moves = 100;

    int sx[PLAYERS];
    int sy[PLAYERS];

    // srand(time(0));

    for (int i = 0; i < numPlayers; i++) {
        sx[i] = rand() % WIDTH;
        sy[i] = rand() % HEIGHT;
    }

    GameSim pruningOff(sx, sy, numPlayers, false);
    GameSim pruningOn(sx, sy, numPlayers, true);

    for (int j = 0; j < 100; j++) {
        pruningOff.playTurn();
        pruningOn.playTurn();

        if (strcmp(pruningOff.scores.move, pruningOn.scores.move) != 0) {
            cerr << "Difference at turn " << j << ", p" << (j % numPlayers) << endl;
            pruningOff.scores.print();
            pruningOn.scores.print();
            break;
        }
    }

    cerr << "Max move time: " << pruningOff.maxTime << "ms" << endl;
    cerr << "Max move time: " << pruningOn.maxTime << "ms" << endl;

    // pruningOff.states[0].print();
    // pruningOn.states[0].print();
}

TEST(Scoring, ShouldSetLosersWhenSingleOccupantOfLargestRegion) {
    State state;
    readBoard(state,
        "....*....*....*..A.*....*....*\n"
        "000000000000000000000000000000\n"
        "....*....*....*....*....*....*\n"
        "....*....*....*..B.*....*....*\n"
        "111111111111111111111111111111\n"
        "....*....*....*....*....*....*\n"
        "....*....*....*..C.*....*....*\n"
        "....*....*....*....*....*....*\n");

    Voronoi voronoi;

    Scores scores = calculateScores(voronoi, state);

    ASSERT_TRUE(scores.isLoser(0)) << "Expected player 0 to lose";
    ASSERT_TRUE(scores.isLoser(1)) << "Expected player 1 to lose";
    ASSERT_FALSE(scores.isLoser(2)) << "Expected player 2 not to lose";
}

TEST(Scoring, ShouldNotSetLosersWhenNoSingleOccupantOfLargestRegion) {
    State state;
    readBoard(state,
        "....*....*....*..A.*....*....*\n"
        "....*....*....*....*....*....*\n"
        "000000000000000000000000000000\n"
        "....*....*....*....*....*....*\n"
        "....*....*....*..B.*....*....*\n"
        "....*....*....*....*....*....*\n"
        "....*....*....*..C.*....*....*\n"
        "....*....*....*....*....*....*\n");

    Voronoi voronoi;

    Scores scores = calculateScores(voronoi, state);

    ASSERT_FALSE(scores.isLoser(0)) << "Expected player 0 not to lose";
    ASSERT_FALSE(scores.isLoser(1)) << "Expected player 1 not to lose";
    ASSERT_FALSE(scores.isLoser(2)) << "Expected player 2 not to lose";
}

TEST(Bounding, ShouldNotPruneWhenBothAreLosers) {
    State state;
    state.numPlayers = 3;
    state.pruningEnabled = true;

    // Player 2 wins by occupying region 2 alone
    Scores scores;
    scores.scores[0] = -100;
    scores.scores[1] = -200;
    scores.scores[2] = 300;
    scores.regions[0] = 0;
    scores.regions[1] = 0;
    scores.regions[2] = 1;

    scores.setLoser(0);
    scores.setLoser(1);

    ScoreCalculatorMock mock;
    mock.expectCall(scores);

    Bounds bounds;
    bounds.bounds[0] = 50;
    bounds.bounds[1] = 50;
    bounds.bounds[2] = 50;

    ASSERT_FALSE(checkBounds(bounds, scores, state, 0)) << "Expected player 1's bound not to cause pruning: "
        "a better move for player 0 could cause both players to survive";
}

TEST(Scoring, ShouldCalculateRegionsForPlayers) {
    State state;
    readBoard(state,
        "....*....*....*..A.*....*....*\n"
        "....*....*....*....*....*....*\n"
        "000000000000000000000000000000\n"
        "....*....*....*....*....*....*\n"
        "....*....*....*..B.*....*....*\n"
        "....*....*....*....*....*....*\n"
        "....*....*....*..C.*....*....*\n"
        "....*....*....*....*....*....*\n");

    Voronoi voronoi;

    Scores scores = calculateScores(voronoi, state);

    ASSERT_NE(scores.regions[0], scores.regions[1]) << "Expected players 0 and 1 to be in different regions";
    ASSERT_EQ(scores.regions[1], scores.regions[2]) << "Expected players 1 and 2 to be in same region";
}

TEST(Bounding, ShouldPruneInThreeWayGameWhenBothBoundsExceeded) {
    State state;
    state.numPlayers = 3;
    state.pruningEnabled = true;

    Scores scores;
    scores.scores[0] = 200;
    scores.scores[1] = 50;
    scores.scores[2] = 50;

    scores.regions[0] = 0;
    scores.regions[1] = 0;
    scores.regions[2] = 0;

    ScoreCalculatorMock mock;
    mock.expectCall(scores);

    Bounds bounds;
    bounds.bounds[0] = INT_MIN;
    bounds.bounds[1] = 70;
    bounds.bounds[2] = 70;

    ASSERT_TRUE(checkBounds(bounds, scores, state, 0)) << "Expected player 1's and player 2's bound to cause pruning";
}

TEST(Bounding, ShouldNotPruneInThreeWayGameWhenOnlyOneBoundExceeded) {
    State state;
    state.numPlayers = 3;
    state.pruningEnabled = true;

    Scores scores;
    scores.scores[0] = 200;
    scores.scores[1] = 50;
    scores.scores[2] = 50;

    scores.regions[0] = 0;
    scores.regions[1] = 0;
    scores.regions[2] = 0;

    ScoreCalculatorMock mock;
    mock.expectCall(scores);

    Bounds bounds;
    bounds.bounds[0] = INT_MIN;
    bounds.bounds[1] = 70;
    bounds.bounds[2] = 30;

    ASSERT_FALSE(checkBounds(bounds, scores, state, 0)) << "Expected player 1's bound alone not to cause pruning";
}

TEST(Bounding, ShouldNotPruneWhenInDifferentRegions) {
    State state;
    state.numPlayers = 2;
    state.thisPlayer = 0;

    state.occupy(5, 5, 0);
    state.occupy(15, 15, 1);

    Scores scores(40, 100);
    scores.regions[0] = 0;
    scores.regions[0] = 1;

    ScoreCalculatorMock mock;
    mock.expectCall(scores);

    Bounds bounds;
    bounds.bounds[0] = 50;
    bounds.bounds[1] = INT_MIN;

    ASSERT_FALSE(checkBounds(bounds, scores, state, 1)) << "Expected player 0's bound not to cause pruning";
}

TEST(Voronoi, Rooms) {
    State state;
    state.numPlayers = 2;

    readBoard(state,
        "....*.0..*....A0...*....1....*\n"
        "....*.0..*....*0...*....1....*\n"
        "....*.000*....*0...*....1....*\n"
        "....*....*....*0...*....1....*\n"
        "0000000..*....*0...*....1....*\n"
        "....0....*....*0...*....1....*\n"
        "....000000000000...*....B....*\n"
        "....*....*....*....*....*....*\n"
        "....*....*....*....*....*....*\n");

    Voronoi voronoi;
    voronoi.calculate(state);

    const Room& room = voronoi.startingRoom(0);
    ASSERT_EQ(45, room.size) << "Expected p0's starting room to have 45 cells";
    ASSERT_EQ(2, room.neighbourCount) << "Expected p0's starting room to have 2 neighbours";
}
