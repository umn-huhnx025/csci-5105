CXX = g++
CC = g++
CPPFLAGS += -MP -MD
CXXFLAGS += -g -std=c++11
LDFLAGS += -pthread -lcrypto

PROG = tracker peer test

LINK = $(CXX) $+ -o $@ $(LDFLAGS)

all: $(PROG)

tracker: tracker_main.o tracker.o

peer: peer_main.o peer.o
	$(LINK)

test: test.o tracker.o peer.o
	$(LINK)

.PHONY: clean
clean:
	@$(RM) -f *.o *.d $(PROG)

-include $(SRC:.cpp=.d)
