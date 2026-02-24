CXX = g++
#
BIN_DIR = bin
#
SRC_DIR = src
#
CXXFLAGS = -I include -std=c++17 -O3

.PHONY: all clean run indexer searcher main cli export

all: indexer searcher main cli

export:
	python3 export_corpus.py

indexer:
	mkdir -p $(BIN_DIR)
	$(CXX) $(SRC_DIR)/indexer.cpp -o $(BIN_DIR)/indexer $(CXXFLAGS)

searcher:
	mkdir -p $(BIN_DIR)
	$(CXX) $(SRC_DIR)/searcher.cpp -o $(BIN_DIR)/searcher $(CXXFLAGS)

main:
	$(CXX) $(SRC_DIR)/main.cpp -o main $(CXXFLAGS)

cli:
	mkdir -p $(BIN_DIR)
	$(CXX) $(SRC_DIR)/cli.cpp -o $(BIN_DIR)/cli $(CXXFLAGS)

clean:
	rm -f $(BIN_DIR)/* main dump_output.txt solution.zip data/index_data.txt data/docs_map.txt

run: all
	./main index
	./main search
