 # the compiler: gcc for C program, define as g++ for C++
CC = g++

# compiler flags:
#  -std=c++11 for C++ 11 support
#  -Wall turns on most, but not all, compiler warnings
CFLAGS  = -g -std=c++11 -Wall

# worker pflags
# -lcrypt for linking crypt
# -lpthread for linking pthread
WFLAGS = -lcrypt -lpthread

# the build target executable:
SERVER = server
USER = user
WORKER = worker

all: $(SERVER) $(USER) $(WORKER)

$(SERVER): $(SERVER).cpp
	$(CC) $(CFLAGS) -o $(SERVER) $(SERVER).cpp

$(USER): $(USER).cpp
	$(CC) $(CFLAGS) -o $(USER) $(USER).cpp

$(WORKER): $(WORKER).cpp
	$(CC) $(CFLAGS) -o $(WORKER) $(WORKER).cpp $(WFLAGS) 

clean:
	$(RM) $(SERVER)
	$(RM) $(USER)
	$(RM) $(WORKER)