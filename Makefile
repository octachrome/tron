all: run_tron_tests

run_tron_tests: tron_tests
	./tron_tests

tron_tests: tron_tests.o gtest_main.a
	g++ -g -o $@ $^ -lrt

tron_tests.o : tron.cc tron_tests.cc
	g++ -c -o $@ -g -Wall -Wextra -fstack-protector-all -I$(GTEST_DIR)/include -DTRON_TESTS tron_tests.cc

tron_bot: tron.o
	g++ -g -o $@ $^ -lrt

tron_bot.o : tron.cc
	g++ -c -o $@ -g -Wall -Wextra -fstack-protector-all -I$(GTEST_DIR)/include -DTRON_TESTS tron.cc

profile: tron_prof
	./tron_prof
	# gprof tron_prof > tron_prof.out

tron_prof: tron_prof.cc tron.cc
	g++ -g -O3 -o $@ tron_prof.cc -lrt -DTRON_PROF

clean:
	-rm *.o *.a *_tests prof

GTEST_DIR = /home/chris/code/gtest-1.6.0


# Builds gtest.a and gtest_main.a.

GTEST_HEADERS = $(GTEST_DIR)/include/gtest/*.h \
                $(GTEST_DIR)/include/gtest/internal/*.h

# Usually you shouldn't tweak such internal variables, indicated by a
# trailing _.
GTEST_SRCS_ = $(GTEST_DIR)/src/*.cc $(GTEST_DIR)/src/*.h $(GTEST_HEADERS)

# For simplicity and to avoid depending on Google Test's
# implementation details, the dependencies specified below are
# conservative and not optimized.  This is fine as Google Test
# compiles fast and for ordinary users its source rarely changes.
gtest-all.o : $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest-all.cc

gtest_main.o : $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest_main.cc

gtest.a : gtest-all.o
	$(AR) $(ARFLAGS) $@ $^

gtest_main.a : gtest-all.o gtest_main.o
	$(AR) $(ARFLAGS) $@ $^
