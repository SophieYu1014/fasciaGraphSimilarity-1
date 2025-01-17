CXX=g++
CXXFLAGS=-Wall -std=gnu++0x -fopenmp -O3
CXXDEBUGFLAGS=-Wall -std=gnu++0x -fopenmp -g

all:
	${CXX} ${CXXFLAGS} fascia.cpp -o fascia
	${CXX} ${CXXFLAGS} generate.cpp -o generateGraphs

debug:
	${CXX} ${CXXDEBUGFLAGS} fascia.cpp -o fascia

clean:
	rm -f fascia fascia.exe
