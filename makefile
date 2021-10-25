CXX	:= g++
CXXFLAGS	:= -Wall -std=c++1z -g

CC	= gcc
CCFLAGS = -g

BIN	:= bin
SRC := src
INCLUDE := -I kse_dim -I src -I include -I c_library_v2/standard
LIB	:= .
LIBRARIES	:= -lusb-1.0  -lpthread -L.

CLIENT_OBJS	= kse_ubuntu.o dim.o dim_client.o client.o
SERVER_OBJS	= kse_ubuntu.o dim.o dim_server.o serial_port.o server.o

all: mavlink_dim_client mavlink_dim_server

mavlink_dim_client: $(CLIENT_OBJS)
	$(CXX) -o $@ $(CLIENT_OBJS) $(LIBRARIES)

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

doxygen:
	doxygen Doxyfile
	#./node_modules/.bin/moxygen --anchors --output doc/api.md doc/xml/
	./node_modules/.bin/moxygen -h -t doc/templates/cpp --output doc/api.md doc/xml/
	# pandoc -f markdown -t org -o api.org api.md

watch_doxygen:
	watch_ian src/* - 'make doxygen && scp doc/api.md donghee@192.168.88.25:~/Dropbox/docs/log/dim-uav-gcs_code.md'

# https://stackoverflow.com/questions/26578200/is-there-a-way-to-further-shorten-and-generalize-this-makefile/26579143#26579143
