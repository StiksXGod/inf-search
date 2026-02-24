#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstring>



int run_search_engine(int argc, char* argv[]); 
int run_cli(int argc, char* argv[]);

void print_usage() {
    std::cout << "Usage: ./main <mode> [args...]" << std::endl;
    std::cout << "Modes:" << std::endl;
    std::cout << "  index    Run the indexer to build the index" << std::endl;
    std::cout << "  search   Run the search engine (interactive)" << std::endl;
    std::cout << "  cli      Run CLI tools (dump, pack, send)" << std::endl;
    std::cout << "  help     Show this help" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    std::string mode = argv[1];


    std::vector<char*> new_argv;
    new_argv.push_back(argv[0]);
    for(int i = 2; i < argc; ++i) {
        new_argv.push_back(argv[i]);
    }
    new_argv.push_back(nullptr); 

    if (mode == "search") {
        std::string cmd = "./bin/searcher";
        for(int i = 2; i < argc; ++i) {
            cmd += " ";
            cmd += argv[i];
        }
        return std::system(cmd.c_str());

    } else if (mode == "index") {
        std::string cmd = "./bin/indexer";
        for(int i = 2; i < argc; ++i) {
            cmd += " ";
            cmd += argv[i];
        }
        return std::system(cmd.c_str());

    } else if (mode == "cli") {
        std::string cmd = "./bin/cli";
        for(int i = 2; i < argc; ++i) {
            cmd += " ";
            cmd += argv[i];
        }
        return std::system(cmd.c_str());
    } else {
        print_usage();
        return 1;
    }
}
