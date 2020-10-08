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

# all: mavlink_dim

# #mavlink_dim: git_submodule main.cpp
# mavlink_dim: format src/main.cpp
# #	g++ -I c_library_v2/standard -Ofast -std=c++11 -I ./ src/main.cpp -o mavlink_dim -lpthread
# 	gcc -c -fpic kse_dim/kse_ubuntu.c -o kse_ubuntu.o -lusb-1.0
# #	gcc -shared -o libkse.so kse_ubuntu.o
# #	ar rcs libkse.a kse_ubuntu.o
# 	g++ -c -I kse_dim src/dim.cpp -o dim.o -L.
# #	g++ -I c_library_v2/standard -I src dim.o src/main.cpp -o mavlink_dim -lpthread -L.

#git_submodule:
#	git submodule update --init --recursive

clean:
	 rm -rf *.o *.a mavlink_dim mavlink_dim_server

format:
	astyle --options=./astylerc --preserve-date src/server.cpp
	astyle --options=./astylerc --preserve-date src/client.cpp

udpproxy: src/udpproxy.c
	$(CC) $(CFLAGS) -o udpproxy src/udpproxy.c
	./udpproxy 10401 10402 -v

# https://stackoverflow.com/questions/26578200/is-there-a-way-to-further-shorten-and-generalize-this-makefile/26579143#26579143
