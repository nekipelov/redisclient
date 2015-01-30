LDFLAGS = -g -pthread -lboost_system
CXXFLAGS = -g -O2 -Wall -Wextra -I../src -std=c++11

EXAMPLES = async_pubsub async_set_get async_set_get2 sync_set_get
SOURCES = $(EXAMPLES:=.cpp)
OBJS = $(SOURCES:.cpp=.o)
DEPS = $(SOURCES:.cpp=.d)

all: $(EXAMPLES) 

$(EXAMPLES): $(OBJS)
	$(CXX) $(LDFLAGS) $@.o -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $@
	
clean:
	rm -f $(OBJS) $(EXAMPLES) $(DEPS)

-include $(DEPS)

.PHONY: clean
