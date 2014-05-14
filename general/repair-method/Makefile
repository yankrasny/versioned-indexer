SOURCES = $(wildcard ../Repair-Partitioning/util/*.cpp) $(wildcard ../Repair-Partitioning/repair-algorithm/*.cpp) $(wildcard ../Repair-Partitioning/indexed-heap/*.cpp) $(wildcard ../Repair-Partitioning/partitioning/*.cpp) $(wildcard ../Repair-Partitioning/prototype/*.cpp)

1:
	g++ diff2_repair.cc libhash.a $(SOURCES) -o diff2_repair -g -std=c++0x

1opt:
	g++ diff2_repair.cc libhash.a $(SOURCES) -o diff2_repair -O3 -std=c++0x

1prof:
	g++ diff2_repair.cc libhash.a $(SOURCES) -o diff2_repair -pg -std=c++0x


2:
	g++ convert_repair.cc -o convert_repair -g -std=c++0x

2opt:
	g++ convert_repair.cc -o convert_repair -O3 -std=c++0x

2prof:
	g++ convert_repair.cc -o convert_repair -pg -std=c++0x


3:
	g++ index_repair.cc libhash.a $(SOURCES) -o index_repair -g -std=c++0x

3opt:
	g++ index_repair.cc libhash.a $(SOURCES) -o index_repair -O3 -std=c++0x

3prof:
	g++ index_repair.cc libhash.a $(SOURCES) -o index_repair -pg -std=c++0x