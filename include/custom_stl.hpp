#ifndef CUSTOM_STL_HPP
#define CUSTOM_STL_HPP

#include <cstddef>
#include <string>


template<typename T>
class Vector {
    T* data;
    size_t capacity;
    size_t sz;
public:
    Vector() : data(nullptr), capacity(0), sz(0) {}
    Vector(size_t n) : data(new T[n]), capacity(n), sz(n) {
        for(size_t i=0; i<sz; ++i) data[i] = T();
    }
    Vector(const Vector& other) {
        sz = other.sz;
        capacity = other.capacity;
        data = new T[capacity];
        for(size_t i=0; i<sz; ++i) data[i] = other.data[i];
    }
    Vector& operator=(const Vector& other) {
        if(this != &other) {
            delete[] data;
            sz = other.sz;
            capacity = other.capacity;
            data = new T[capacity];
            for(size_t i=0; i<sz; ++i) data[i] = other.data[i];
        }
        return *this;
    }
    ~Vector() { delete[] data; }
    
    void push_back(const T& val) {
        if(sz == capacity) {
            capacity = (capacity == 0) ? 8 : capacity * 2;
            T* newData = new T[capacity];
            for(size_t i=0; i<sz; ++i) newData[i] = data[i];
            delete[] data;
            data = newData;
        }
        data[sz++] = val;
    }
    
    T& operator[](size_t i) { return data[i]; }
    const T& operator[](size_t i) const { return data[i]; }
    size_t size() const { return sz; }
    bool empty() const { return sz == 0; }
    T* begin() { return data; }
    T* end() { return data + sz; }
    const T* begin() const { return data; }
    const T* end() const { return data + sz; }
    void clear() { sz = 0; }
};

template<typename K, typename V>
struct Pair {
    K first;
    V second;
    Pair() : first(), second() {}
    Pair(K k, V v) : first(k), second(v) {}
};

template<typename K, typename V>
class HashMap {
    struct Node {
        K key;
        V value;
        Node* next;
        Node(K k, V v) : key(k), value(v), next(nullptr) {}
    };
    Node** buckets;
    size_t bucket_count;
    size_t sz;

    size_t hash(int k) const { return k % bucket_count; }
    size_t hash(const std::string& k) const {
        size_t h = 0;
        for(unsigned char c : k) h = 31 * h + c;
        return h % bucket_count;
    }

public:
    HashMap(size_t buckets = 50021) : bucket_count(buckets), sz(0) {
        this->buckets = new Node*[bucket_count];
        for(size_t i=0; i<bucket_count; ++i) this->buckets[i] = nullptr;
    }
    ~HashMap() {
        for(size_t i=0; i<bucket_count; ++i) {
            Node* curr = buckets[i];
            while(curr) {
                Node* next = curr->next;
                delete curr;
                curr = next;
            }
        }
        delete[] buckets;
    }
    
    V& operator[](const K& key) {
        size_t h = hash(key);
        Node* curr = buckets[h];
        while(curr) {
            if(curr->key == key) return curr->value;
            curr = curr->next;
        }
        Node* newNode = new Node(key, V());
        newNode->next = buckets[h];
        buckets[h] = newNode;
        sz++;
        return newNode->value;
    }
    
    V* find(const K& key) const {
        size_t h = hash(key);
        Node* curr = buckets[h];
        while(curr) {
            if(curr->key == key) return &curr->value;
            curr = curr->next;
        }
        return nullptr;
    }

    template<typename Func>
    void forEach(Func f) const {
        for(size_t i=0; i<bucket_count; ++i) {
            Node* curr = buckets[i];
            while(curr) {
                f(curr->key, curr->value);
                curr = curr->next;
            }
        }
    }
    
    size_t size() const { return sz; }
};

inline Vector<std::string> split_string(const std::string& str, char delimiter) {
    Vector<std::string> tokens;
    std::string token;
    for (char c : str) {
        if (c == delimiter) {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
        } else {
            token += c;
        }
    }
    if (!token.empty()) tokens.push_back(token);
    return tokens;
}

#endif
