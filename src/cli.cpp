#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <fstream>
#include <filesystem>

int run_command(const std::string& cmd) {
    std::cout << "[CMD] " << cmd << std::endl;
    return std::system(cmd.c_str());
}

void do_dump() {
    std::cout << "Creating execution dump..." << std::endl;
    std::ofstream query_file("dump_queries.txt");
    query_file << "россия & сша" << std::endl;
    query_file << "путин | медведев" << std::endl;
    query_file << "экономика" << std::endl;
    query_file << "exit" << std::endl;
    query_file.close();

    std::string cmd = "./bin/searcher < dump_queries.txt > dump_output.txt";
    int ret = run_command(cmd);
    
    if (ret == 0) {
        std::cout << "Dump created in 'dump_output.txt'" << std::endl;
    } else {
        std::cerr << "Failed to create dump." << std::endl;
    }
    
    std::remove("dump_queries.txt");
}

void do_pack() {
    std::cout << "Packing project..." << std::endl;
    std::string cmd = "zip -r solution.zip . -x \"*.git*\" \"*.o\" \"bin/*\" \"cli\" \"main\" \"*.zip\" \"data/*\" \"dump_output.txt\" \"archive/*\"";
    int ret = run_command(cmd);
    
    if (ret == 0) {
        std::cout << "Project packed into 'solution.zip'" << std::endl;
    } else {
        std::cerr << "Failed to pack project. Ensure 'zip' is installed." << std::endl;
    }
}

void do_send() {
    std::cout << "--- Email Instructions ---" << std::endl;
    std::cout << "To send the solution, please use the provided Python script or a mail client." << std::endl;
    std::cout << "Recommended command (if configured):" << std::endl;
    std::cout << "  python3 cli.py send --to recipient@example.com --file solution.zip" << std::endl;
    std::cout << "--------------------------" << std::endl;
}

void print_usage() {
    std::cout << "Usage: ./cli <command>" << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  dump    Run search engine and save output" << std::endl;
    std::cout << "  pack    Zip the project files" << std::endl;
    std::cout << "  send    Show sending instructions" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    std::string command = argv[1];

    if (command == "dump") {
        do_dump();
    } else if (command == "pack") {
        do_pack();
    } else if (command == "send") {
        do_send();
    } else {
        std::cerr << "Unknown command: " << command << std::endl;
        print_usage();
        return 1;
    }

    return 0;
}
