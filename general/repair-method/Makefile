SOURCES = $(wildcard ../Repair-Partitioning/util/*.cpp) $(wildcard ../Repair-Partitioning/repair-algorithm/*.cpp) $(wildcard ../Repair-Partitioning/indexed-heap/*.cpp) $(wildcard ../Repair-Partitioning/partitioning/*.cpp) $(wildcard ../Repair-Partitioning/prototype/*.cpp)

all:
	g++ diff2_repair.cc libhash.a $(SOURCES) -o diff2_repair -g -std=c++0x

optimized:
	g++ diff2_repair.cc libhash.a $(SOURCES) -o diff2_repair -O3 -std=c++0x

profile:
	g++ diff2_repair.cc libhash.a $(SOURCES) -o diff2_repair -pg -std=c++0x
