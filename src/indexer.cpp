#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <clocale>
#include "../include/tokenizer.hpp"
#include "../include/custom_stl.hpp"

using InvertedIndex = HashMap<std::string, Vector<Pair<int, int>>>;
using DocMap = HashMap<int, std::string>;

void save_index(const InvertedIndex& index, const std::string& filename) {
    std::ofstream outfile(filename);
    if (!outfile.is_open()) {
        std::cerr << "Error opening output file: " << filename << std::endl;
        return;
    }
    
    index.forEach([&outfile](const std::string& term, const Vector<Pair<int, int>>& postings) {
        outfile << term << ":";
        for(size_t i=0; i<postings.size(); ++i) {
            outfile << postings[i].first << "," << postings[i].second << ";";
        }
        outfile << "\n";
    });
    outfile.close();
}

void save_docs(const DocMap& docs, const std::string& filename) {
    std::ofstream outfile(filename);
    if (!outfile.is_open()) return;
    
    docs.forEach([&outfile](const int& id, const std::string& url) {
        outfile << id << "|" << url << "\n";
    });
    outfile.close();
}

int main(int argc, char* argv[]) {
    std::setlocale(LC_ALL, "");
    std::string corpus_file = "data/corpus.txt";
    if (argc > 1) corpus_file = argv[1];

    InvertedIndex index;
    DocMap doc_map;
    HashMap<int, int> doc_lengths;

    std::ifstream file(corpus_file);
    if (!file.is_open()) {
        std::cerr << "Error opening corpus file: " << corpus_file << std::endl;
        return 1;
    }

    std::string urls_file = "data/urls.txt";
    std::ifstream ufile(urls_file);
    Vector<std::string> urls;
    if (ufile.is_open()) {
        std::string url;
        while (std::getline(ufile, url)) {
            urls.push_back(url);
        }
    }

    std::string line;
    int doc_id = 0;
    
    std::cout << "Building index..." << std::endl;
    auto start_time = std::chrono::high_resolution_clock::now();

    while (std::getline(file, line)) {
        if (doc_id < urls.size()) {
            doc_map[doc_id] = urls[doc_id];
        } else {
            doc_map[doc_id] = "Doc #" + std::to_string(doc_id);
        }

        Vector<std::string> tokens;
        tokenize_to_container(line, tokens);
        
        doc_lengths[doc_id] = tokens.size(); 
        
        HashMap<std::string, int> term_counts;
        for (size_t i = 0; i < tokens.size(); ++i) {
            term_counts[tokens[i]]++;
        }

        Vector<std::string> unique_tokens;
        for(size_t i=0; i<tokens.size(); ++i) {
            bool found = false;
            for(size_t j=0; j<unique_tokens.size(); ++j) {
                if(unique_tokens[j] == tokens[i]) {
                    found = true;
                    break;
                }
            }
            if(!found) unique_tokens.push_back(tokens[i]);
        }

        for(size_t i=0; i<unique_tokens.size(); ++i) {
            std::string term = unique_tokens[i];
            int count = term_counts[term];
            index[term].push_back(Pair<int, int>(doc_id, count));
        }

        doc_id++;
        if (doc_id % 1000 == 0) {
            std::cout << "Processed " << doc_id << " documents\r" << std::flush;
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;
    std::cout << "\nIndex built in " << elapsed.count() << " seconds." << std::endl;
    std::cout << "Total documents: " << doc_id << std::endl;
    std::cout << "Total unique terms: " << index.size() << std::endl;

    std::cout << "Saving index to 'data/index_data.txt'..." << std::endl;
    save_index(index, "data/index_data.txt");
    save_docs(doc_map, "data/docs_map.txt");
    std::cout << "Done." << std::endl;

    return 0;
}
