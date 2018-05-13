CXX = g++
CC = g++
CPPFLAGS += -MP -MD
CXXFLAGS += -g -std=c++11
LDFLAGS += -pthread

all: client server test

client: client_main.o client.o

server: server_main.o server.o server_registry.o article_manager.o article.o

test: test_main.o test.o server.o client.o server_registry.o article_manager.o article.o

.PHONY: clean
clean:
	@$(RM) -f *.o *.d client server test

-include $(SRC:.cpp=.d)
