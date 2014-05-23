SRCS = redisclient.cpp redisvalue.cpp redisparser.cpp
HDRS = redisclient.h redisvalie.h redisparser.h
OBJS = $(SRCS:.cpp=.o)
LIBS = -pthread -lboost_system
LDFLAGS = -g 
CXXFLAGS = -g -O2 -std=c++0x -Wall -Wextra

all: examples
examples: example1 example2 example3 example4 parsertest

example1: $(OBJS) example1.o
	g++ $^ -o $@  $(LDFLAGS) $(LIBS)

example2: $(OBJS) example2.o
	g++ $^ -o $@  $(LDFLAGS) $(LIBS)

example3: $(OBJS) example3.o
	g++ $^ -o $@  $(LDFLAGS) $(LIBS)

example4: $(OBJS) example4.o
	g++ $^ -o $@  $(LDFLAGS) $(LIBS)

parsertest: $(OBJS) parsertest.o
	g++ $^ -o $@  $(LDFLAGS) $(LIBS) -lboost_unit_test_framework

test: parsertest
	./parsertest

.cpp.o:
	g++ $(CXXFLAGS) -c $< -o $@
	
clean:
	rm -f $(OBJS) example1.o example1 example2.o example2 example3.o example3 example4.o example4 parsertest.o parsertest

.PHONY: all examples clean
