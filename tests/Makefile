LIBS = -pthread -lboost_system -lboost_unit_test_framework
LDFLAGS = -g 
CXXFLAGS = -g -Wall -Wextra -I../src

all: parsertest
parsertest: parsertest.o redisparser.o redisvalue.o
	$(CXX) $^ -o $@  $(LDFLAGS) $(LIBS) -lboost_unit_test_framework
redisparser.o: ../src/redisclient/impl/redisparser.cpp ../src/redisclient/redisvalue.h
	$(CXX) $(CXXFLAGS) -c $< -o $@
redisvalue.o: ../src/redisclient/impl/redisvalue.cpp ../src/redisclient/redisvalue.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: parsertest
	./parsertest

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@
	
clean:
	rm -f $(OBJS) parsertest.o parsertest redisparser.o redisvalue.o

.PHONY: all examples clean
