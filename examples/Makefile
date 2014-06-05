LIBS = -lboost_system
LDFLAGS = -g -L.. -pthread
CXXFLAGS = -g -O2 -std=c++11 -Wall -Wextra -I../src

all: examples
examples: example1 example2 example3 example4

example1: $(OBJS) example1.o
	$(CXX) $^ -o $@  $(LDFLAGS) $(LIBS)

example2: $(OBJS) example2.o
	$(CXX) $^ -o $@  $(LDFLAGS) $(LIBS)

example3: $(OBJS) example3.o
	$(CXX) $^ -o $@  $(LDFLAGS) $(LIBS)

example4: $(OBJS) example4.o
	$(CXX) $^ -o $@  $(LDFLAGS) $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
	
clean:
	rm -f $(OBJS) example1.o example1 example2.o example2 example3.o example3 example4.o example4

.PHONY: all examples clean
