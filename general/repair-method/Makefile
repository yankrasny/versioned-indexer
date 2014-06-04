SOURCES = $(wildcard ../Repair-Partitioning/util/*.cpp) $(wildcard ../Repair-Partitioning/repair-algorithm/*.cpp) $(wildcard ../Repair-Partitioning/indexed-heap/*.cpp) $(wildcard ../Repair-Partitioning/partitioning/*.cpp) $(wildcard ../Repair-Partitioning/prototype/*.cpp)
SOURCES_2 = $(wildcard tradeoff-heap/*.cpp)

all: 0 1 2 3

allopt: 0 1opt 2opt 3opt

0:
	g++ choose_random_n.cc -o choose -g -std=c++0x

1:
	g++ diff2_repair.cc libhash.a $(SOURCES) -o diff2_repair -g -std=c++0x

1opt:
	g++ diff2_repair.cc libhash.a $(SOURCES) -o diff2_repair -O3 -std=c++0x

1prof:
	g++ diff2_repair.cc libhash.a $(SOURCES) -o diff2_repair -pg -std=c++0x


2:
	g++ global_opt.cc $(SOURCES_2) -o global_opt -g -std=c++0x

2opt:
	g++ global_opt.cc $(SOURCES_2) -o global_opt -O3 -std=c++0x

2prof:
	g++ global_opt.cc $(SOURCES_2) -o global_opt -pg -std=c++0x


3:
	g++ index_repair.cc libhash.a $(SOURCES) -o index_repair -g -std=c++0x

3opt:
	g++ index_repair.cc libhash.a $(SOURCES) -o index_repair -O3 -std=c++0x

3prof:
	g++ index_repair.cc libhash.a $(SOURCES) -o index_repair -pg -std=c++0x

4:
	g++ get_doc_offsets.cc -o get_doc_offsets -g -std=c++0x

4opt:
	g++ get_doc_offsets.cc -o get_doc_offsets -O3 -std=c++0x