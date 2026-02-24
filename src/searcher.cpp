#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cmath>
#include <chrono>
#include <algorithm>
#include "../include/tokenizer.hpp"
#include "../include/custom_stl.hpp"

using InvertedIndex = HashMap<std::string, Vector<Pair<int, int>>>;
using DocMap = HashMap<int, std::string>;

struct SearchResult {
    int doc_id;
    double score;
};

Vector<std::string> split(const std::string& s, char delimiter) {
    return split_string(s, delimiter);
}

void load_index(const std::string& filename, InvertedIndex& index) {
    std::ifstream infile(filename);
    if (!infile.is_open()) return;
    
    std::string line;
    while (std::getline(infile, line)) {
        size_t colon_pos = line.find(':');
        if (colon_pos == std::string::npos) continue;
        
        std::string term = line.substr(0, colon_pos);
        std::string postings_str = line.substr(colon_pos + 1);
        
        Vector<std::string> pairs = split(postings_str, ';');
        for(size_t i=0; i<pairs.size(); ++i) {
            Vector<std::string> kv = split(pairs[i], ',');
            if(kv.size() == 2) {
                int doc_id = std::stoi(kv[0]);
                int count = std::stoi(kv[1]);
                index[term].push_back(Pair<int, int>(doc_id, count));
            }
        }
    }
}

void load_docs(const std::string& filename, DocMap& docs) {
    std::ifstream infile(filename);
    if (!infile.is_open()) return;
    
    std::string line;
    while (std::getline(infile, line)) {
        size_t pipe_pos = line.find('|');
        if (pipe_pos == std::string::npos) continue;
        
        int id = std::stoi(line.substr(0, pipe_pos));
        std::string url = line.substr(pipe_pos + 1);
        docs[id] = url;
    }
}

void my_quicksort(Vector<SearchResult>& arr, int low, int high) {
    if (low < high) {
        double pivot = arr[high].score;
        int i = (low - 1);
        for (int j = low; j <= high - 1; j++) {
            if (arr[j].score > pivot) {
                i++;
                SearchResult temp = arr[i];
                arr[i] = arr[j];
                arr[j] = temp;
            }
        }
        SearchResult temp = arr[i + 1];
        arr[i + 1] = arr[high];
        arr[high] = temp;
        int pi = i + 1;

        my_quicksort(arr, low, pi - 1);
        my_quicksort(arr, pi + 1, high);
    }
}

Vector<int> intersect_lists(const Vector<int>& list1, const Vector<int>& list2) {
    Vector<int> result;
    size_t i = 0, j = 0;
    while(i < list1.size() && j < list2.size()) {
        if(list1[i] < list2[j]) i++;
        else if(list2[j] < list1[i]) j++;
        else {
            result.push_back(list1[i]);
            i++; j++;
        }
    }
    return result;
}

Vector<int> union_lists(const Vector<int>& list1, const Vector<int>& list2) {
    Vector<int> result;
    size_t i = 0, j = 0;
    while(i < list1.size() && j < list2.size()) {
        if(list1[i] < list2[j]) {
            result.push_back(list1[i]);
            i++;
        } else if(list2[j] < list1[i]) {
            result.push_back(list2[j]);
            j++;
        } else {
            result.push_back(list1[i]);
            i++; j++;
        }
    }
    while(i < list1.size()) result.push_back(list1[i++]);
    while(j < list2.size()) result.push_back(list2[j++]);
    return result;
}

Vector<int> execute_query(const std::string& query, InvertedIndex& index, int total_docs) {
    Vector<std::string> or_groups = split(query, '|');
    Vector<int> final_result;

    for(size_t i=0; i<or_groups.size(); ++i) {
        Vector<std::string> and_terms = split(or_groups[i], '&');
        Vector<int> group_result;
        bool first = true;

        for(size_t j=0; j<and_terms.size(); ++j) {
            std::string term = and_terms[j];
            size_t first_not_space = term.find_first_not_of(" \t");
            if (std::string::npos == first_not_space) continue;
            size_t last_not_space = term.find_last_not_of(" \t");
            term = term.substr(first_not_space, (last_not_space - first_not_space + 1));
            
            Vector<std::string> tokens;
            tokenize_to_container(term, tokens);
            if(tokens.empty()) continue;
            term = tokens[0]; 

            Vector<Pair<int, int>>* postings = index.find(term);
            Vector<int> term_docs;
            if(postings) {
                for(size_t k=0; k<postings->size(); ++k) {
                    term_docs.push_back((*postings)[k].first);
                }
            }

            if (first) {
                group_result = term_docs;
                first = false;
            } else {
                group_result = intersect_lists(group_result, term_docs);
            }
        }

        if (first) continue; 

        if (final_result.empty() && or_groups.size() == 1) {
             final_result = group_result;
        } else {
             final_result = union_lists(final_result, group_result);
        }
    }
    
    return final_result;
}

Vector<SearchResult> rank_results(const Vector<int>& docs, const std::string& query, InvertedIndex& index, int total_docs) {
    Vector<SearchResult> results;
    
    Vector<std::string> terms;
    Vector<std::string> raw_tokens;
    tokenize_to_container(query, raw_tokens);
    for(size_t i=0; i<raw_tokens.size(); ++i) terms.push_back(raw_tokens[i]);

    for(size_t i=0; i<docs.size(); ++i) {
        int doc_id = docs[i];
        double score = 0.0;
        
        for(size_t j=0; j<terms.size(); ++j) {
            std::string term = terms[j];
            Vector<Pair<int, int>>* postings = index.find(term);
            if(postings) {
                int tf = 0;
                for(size_t k=0; k<postings->size(); ++k) {
                    if((*postings)[k].first == doc_id) {
                        tf = (*postings)[k].second;
                        break;
                    }
                }
                if(tf > 0) {
                    double df = (double)postings->size();
                    double idf = std::log10((double)total_docs / (df + 1.0));
                    score += (double)tf * idf;
                }
            }
        }
        
        SearchResult res;
        res.doc_id = doc_id;
        res.score = score;
        results.push_back(res);
    }
    
    if(results.size() > 0)
        my_quicksort(results, 0, results.size() - 1);
        
    return results;
}

int main(int argc, char* argv[]) {
    std::setlocale(LC_ALL, "");
    
    std::string index_file = "data/index_data.txt";
    std::string docs_file = "data/docs_map.txt";
    
    InvertedIndex index;
    DocMap doc_map;
    
    std::cout << "Loading index..." << std::endl;
    load_index(index_file, index);
    load_docs(docs_file, doc_map);
    
    if (index.size() == 0) {
        std::cerr << "Index is empty. Run the indexer first." << std::endl;
        return 1;
    }
    
    std::cout << "Index loaded. " << index.size() << " terms, " << doc_map.size() << " docs." << std::endl;
    std::cout << "Enter query (or 'exit'):" << std::endl;
    
    std::string query;
    while (true) {
        std::cout << "\nQuery> ";
        std::getline(std::cin, query);
        
        if (query == "exit" || query.empty()) break;
        
        auto start_q = std::chrono::high_resolution_clock::now();
        Vector<int> results = execute_query(query, index, doc_map.size());
        
        Vector<SearchResult> ranked_results = rank_results(results, query, index, doc_map.size());
        
        auto end_q = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed_q = end_q - start_q;
        
        std::cout << "Found " << ranked_results.size() << " documents in " << elapsed_q.count() << " sec:" << std::endl;
        
        int limit = 10;
        for (size_t i = 0; i < (ranked_results.size() < limit ? ranked_results.size() : limit); ++i) {
            std::cout << "[" << ranked_results[i].doc_id << "] (score: " << ranked_results[i].score << ") " << doc_map[ranked_results[i].doc_id] << std::endl;
        }
        if (ranked_results.size() > limit) {
            std::cout << "... and " << (ranked_results.size() - limit) << " more." << std::endl;
        }
    }

    return 0;
}
