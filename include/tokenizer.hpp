#ifndef TOKENIZER_HPP
#define TOKENIZER_HPP

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <clocale>
#include <cwctype>
#include <cstdlib>

struct TokenStats {
    long long total_tokens = 0;
    long long total_length = 0;
    std::map<std::string, int> frequency;
};

inline std::wstring utf8_to_wstring(const std::string& str) {
    if (str.empty()) return std::wstring();
    size_t len = std::mbstowcs(nullptr, str.c_str(), 0);
    if (len == (size_t)-1) return std::wstring();
    std::wstring wstr(len, 0);
    std::mbstowcs(&wstr[0], str.c_str(), len);
    return wstr;
}

inline std::string wstring_to_utf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    size_t len = std::wcstombs(nullptr, wstr.c_str(), 0);
    if (len == (size_t)-1) return std::string();
    std::string str(len, 0);
    std::wcstombs(&str[0], wstr.c_str(), len);
    return str;
}

class RussianStemmer {
private:
    const std::wstring vowels = L"аеиоуыэюяё";

    bool isVowel(wchar_t c) {
        return vowels.find(c) != std::wstring::npos;
    }

    size_t findRV(const std::wstring& word) {
        for (size_t i = 0; i < word.length(); ++i) {
            if (isVowel(word[i])) {
                return i + 1;
            }
        }
        return std::wstring::npos;
    }

    bool endsWith(const std::wstring& word, const std::wstring& suffix) {
        if (word.length() < suffix.length()) return false;
        return word.compare(word.length() - suffix.length(), suffix.length(), suffix) == 0;
    }

    bool removeEnd(std::wstring& word, const std::wstring& suffix, size_t rv) {
        if (endsWith(word, suffix)) {
            if (word.length() - suffix.length() >= rv) {
                word.resize(word.length() - suffix.length());
                return true;
            }
        }
        return false;
    }

    bool removeAny(std::wstring& word, const std::vector<std::wstring>& suffixes, size_t rv) {
        for (const auto& suffix : suffixes) {
            if (removeEnd(word, suffix, rv)) return true;
        }
        return false;
    }

public:
    void stem(std::wstring& word) {
        size_t rv = findRV(word);
        if (rv == std::wstring::npos) return;

        static const std::vector<std::wstring> perfGerund1 = {L"вши", L"вшись", L"в"}; // preceded by a, я
        static const std::vector<std::wstring> perfGerund2 = {L"ив", L"ивши", L"ившись", L"ыв", L"ывши", L"ывшись"};

        bool step1Success = false;
        
        for (const auto& suffix : perfGerund1) {
            if (endsWith(word, suffix)) {
                size_t len = word.length() - suffix.length();
                if (len >= rv && len > 0) {
                    wchar_t prev = word[len - 1];
                    if (prev == L'а' || prev == L'я') {
                        word.resize(len);
                        step1Success = true;
                        break;
                    }
                }
            }
        }
        
        if (!step1Success) {
            if (removeAny(word, perfGerund2, rv)) step1Success = true;
        }

        if (!step1Success) {
            static const std::vector<std::wstring> reflexive = {L"ся", L"сь"};
            removeAny(word, reflexive, rv);

            static const std::vector<std::wstring> adjective = {
                L"ее", L"ие", L"ые", L"ое", L"ими", L"ыми", L"ей", L"ий", L"ый", L"ой", 
                L"ем", L"им", L"ым", L"ом", L"его", L"ого", L"ему", L"ому", L"их", L"ых", 
                L"ую", L"юю", L"ая", L"яя", L"ою", L"ею"
            };
            if (!removeAny(word, adjective, rv)) {
                static const std::vector<std::wstring> verb1 = {
                    L"ла", L"на", L"ете", L"йте", L"ли", L"й", L"л", L"ем", L"н", L"ло", L"но", 
                    L"ет", L"ют", L"ны", L"ть", L"ешь", L"нно"
                }; 
                static const std::vector<std::wstring> verb2 = {
                    L"ила", L"ыла", L"ена", L"ейте", L"уйте", L"ите", L"или", L"ыли", L"ей", 
                    L"уй", L"ил", L"ыл", L"им", L"ым", L"ен", L"ило", L"ыло", L"ено", L"ят", 
                    L"ует", L"уют", L"ит", L"ыт", L"ены", L"ить", L"ыть", L"ишь", L"ую", L"ю"
                };

                bool verbRemoved = false;
                for (const auto& suffix : verb1) {
                    if (endsWith(word, suffix)) {
                        size_t len = word.length() - suffix.length();
                        if (len >= rv && len > 0) {
                            wchar_t prev = word[len - 1];
                            if (prev == L'а' || prev == L'я') {
                                word.resize(len);
                                verbRemoved = true;
                                break;
                            }
                        }
                    }
                }
                if (!verbRemoved) {
                    if (removeAny(word, verb2, rv)) verbRemoved = true;
                }

                if (!verbRemoved) {
                    static const std::vector<std::wstring> noun = {
                        L"а", L"ев", L"ов", L"ие", L"ье", L"е", L"иями", L"ями", L"ами", L"еи", 
                        L"ии", L"и", L"ией", L"ей", L"ой", L"ий", L"й", L"иям", L"ям", L"ием", 
                        L"ем", L"ам", L"ом", L"о", L"у", L"ах", L"иях", L"ях", L"ы", L"ь", L"ию", 
                        L"ью", L"ю", L"ия", L"ья", L"я"
                    };
                    removeAny(word, noun, rv);
                }
            }
        }

        removeEnd(word, L"и", rv);

        static const std::vector<std::wstring> derivational = {L"ост", L"ость"};
        removeAny(word, derivational, rv);

        static const std::vector<std::wstring> superlative = {L"ейше", L"ейш"};
        removeAny(word, superlative, rv);

        if (endsWith(word, L"нн")) {
            if (word.length() - 2 >= rv) {
                word.pop_back();
            }
        }
        
        if (endsWith(word, L"ь")) {
             if (word.length() - 1 >= rv) {
                word.pop_back();
            }
        }
    }
};

inline void tokenize(const std::string& text, TokenStats& stats) {

    std::wstring wtext = utf8_to_wstring(text);
    std::wstring current_token;
    RussianStemmer stemmer;

    for (wchar_t c : wtext) {
        if (std::iswalpha(c)) {
            current_token += std::towlower(c);
        } else if (c == L'-' && !current_token.empty()) {
            if (!current_token.empty()) {
                stemmer.stem(current_token);
                std::string token_utf8 = wstring_to_utf8(current_token);
                
                stats.total_tokens++;
                stats.total_length += token_utf8.length();
                stats.frequency[token_utf8]++;
                current_token.clear();
            }
        } else {
            if (!current_token.empty()) {
                stemmer.stem(current_token);
                std::string token_utf8 = wstring_to_utf8(current_token);
                
                stats.total_tokens++;
                stats.total_length += token_utf8.length();
                stats.frequency[token_utf8]++;
                current_token.clear();
            }
        }
    }
    
    if (!current_token.empty()) {
        stemmer.stem(current_token);
        std::string token_utf8 = wstring_to_utf8(current_token);
        
        stats.total_tokens++;
        stats.total_length += token_utf8.length();
        stats.frequency[token_utf8]++;
    }
}

inline std::vector<std::string> tokenize_to_vector(const std::string& text) {
    std::vector<std::string> tokens;
    std::wstring wtext = utf8_to_wstring(text);
    std::wstring current_token;
    RussianStemmer stemmer;

    auto process_token = [&](std::wstring& token) {
        if (!token.empty()) {
            stemmer.stem(token);
            tokens.push_back(wstring_to_utf8(token));
            token.clear();
        }
    };

    for (wchar_t c : wtext) {
        if (std::iswalpha(c)) {
            current_token += std::towlower(c);
        } else if (c == L'-' && !current_token.empty()) {
             process_token(current_token);
        } else {
             process_token(current_token);
        }
    }
    process_token(current_token);
    return tokens;
}

template <typename Container>
void tokenize_to_container(const std::string& text, Container& container) {
    std::vector<std::string> tokens = tokenize_to_vector(text);
    for (const auto& token : tokens) {
        container.push_back(token);
    }
}

#endif
