CXX	:= g++
CXXFLAGS	:= -Wall -std=c++1z -g

CC	= gcc
CCFLAGS = -g

BIN	:= bin
SRC := src
INCLUDE := -I kse_dim -I src -I c_library_v2/standard
LIB	:= .
LIBRARIES	:= -lusb-1.0  -lpthread -L.

OBJS	= kse_ubuntu.o dim.o client.o
SERVER_OBJS	= kse_ubuntu.o dim.o serial_port.o server.o

all: mavlink_dim_client mavlink_dim_server

mavlink_dim_client: $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LIBRARIES)

mavlink_dim_server: $(SERVER_OBJS)
	$(CXX) -o $@ $(SERVER_OBJS) $(LIBRARIES)

%.o: kse_dim/%.c
	$(CC) -c $(CCFLAGS) $(INCLUDE) -L$(LIB) $< $(LIBRARIES)

%.o: src/%.cpp
	$(CXX) -c $(CXXFLAGS) $(INCLUDE) -L$(LIB) $< $(LIBRARIES)

git_submodule:
	git submodule update --init --recursive

clean:
	 rm -rf *.o *.a mavlink_dim mavlink_dim_server

format:
	astyle --options=./astylerc --preserve-date src/*.cpp

# https://stackoverflow.com/questions/26578200/is-there-a-way-to-further-shorten-and-generalize-this-makefile/26579143#26579143
