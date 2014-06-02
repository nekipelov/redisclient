SRCS = src/redisclient.cpp src/redisvalue.cpp src/redisparser.cpp
HDRS = src/redisclient.h src/redisvalie.h src/redisparser.h
OBJS = $(SRCS:.cpp=.o)
LIBNAME = redisclient
LIB = lib$(LIBNAME).so
LIBS = -pthread -lboost_system
LDFLAGS = -g -shared
CXXFLAGS = -g -O2 -Wall -Wextra -fPIC -std=c++0x

all: $(LIB)
$(LIB): $(OBJS)
	$(CXX) $^ -o $@ $(LDFLAGS)
examples: $(LIB)
	+make -C examples examples
tests: test
test:
	+make -C tests test

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)
	make -C tests clean
	make -C examples clean

.PHONY: all test clean examples
