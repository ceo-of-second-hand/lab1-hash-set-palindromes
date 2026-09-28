// Множина рядків: хеш-таблиця з ланцюгами + подвійний поліноміальний хеш
// (mod 1000000123 і mod 2^64). Паліндроми визначаються при вставці
// порівнянням прямого хешу з хешем оберненого рядка.
#pragma once

#include <cstdint>
#include <cstring>
#include <random>
#include <string>
#include <vector>

class StringSet {
public:
    static const int MAX_LEN = 15;

    explicit StringSet(uint64_t seed = 0) {
        std::mt19937_64 rng(seed ? seed : std::random_device{}() ^
                                          ((uint64_t)std::random_device{}() << 32));
        // p непарне, 26 < p < M1
        uint64_t p = 27 + rng() % (M1 - 28);
        if (p % 2 == 0) ++p;
        pw1_[0] = 1;
        pw2_[0] = 1;
        for (int i = 1; i < MAX_LEN; ++i) {
            pw1_[i] = (uint32_t)((uint64_t)pw1_[i - 1] * p % M1);
            pw2_[i] = pw2_[i - 1] * p;  // mod 2^64 через переповнення
        }
        head_.assign(INIT_CAP, -1);
    }

    bool insert(const char* s, int len) {
        Hash h = hash(s, len);
        if (find(s, len, h, nullptr) != -1) return false;
        if (size_ + 1 > head_.size()) grow();  // α <= 1

        int id;
        if (free_ != -1) {
            id = free_;
            free_ = pool_[id].next;
        } else {
            id = (int)pool_.size();
            pool_.emplace_back();
        }
        Node& nd = pool_[id];
        std::memcpy(nd.s, s, len);
        nd.s[len] = '\0';
        nd.len = (uint8_t)len;
        nd.h1 = h.h1;
        nd.h2 = h.h2;
        // прямий хеш == обернений -> паліндром (перевіряємо посимвольно)
        nd.pal = h.h1 == h.r1 && h.h2 == h.r2 && isPalindrome(s, len);

        size_t b = bucket(h.h1, h.h2);
        nd.next = head_[b];
        head_[b] = id;
        ++size_;
        return true;
    }

    bool erase(const char* s, int len) {
        Hash h = hash(s, len);
        int prev;
        int id = find(s, len, h, &prev);
        if (id == -1) return false;
        if (prev == -1)
            head_[bucket(h.h1, h.h2)] = pool_[id].next;
        else
            pool_[prev].next = pool_[id].next;
        pool_[id].next = free_;
        free_ = id;
        --size_;
        return true;
    }

    bool contains(const char* s, int len) const {
        return find(s, len, hash(s, len), nullptr) != -1;
    }

    std::vector<std::string> palindromes() const {
        std::vector<std::string> res;
        for (int first : head_)
            for (int id = first; id != -1; id = pool_[id].next)
                if (pool_[id].pal) res.emplace_back(pool_[id].s, pool_[id].len);
        return res;
    }

    size_t size() const { return size_; }
    size_t capacity() const { return head_.size(); }

    static bool isPalindrome(const char* s, int len) {
        for (int i = 0, j = len - 1; i < j; ++i, --j)
            if (s[i] != s[j]) return false;
        return true;
    }

private:
    static const uint32_t M1 = 1000000123;
    static const size_t INIT_CAP = 16;

    struct Hash {
        uint32_t h1, r1;
        uint64_t h2, r2;
    };

    struct Node {
        char s[MAX_LEN + 1];
        uint8_t len;
        bool pal;
        uint32_t h1;
        uint64_t h2;
        int next;  // -1 = кінець ланцюга
    };

    uint32_t pw1_[MAX_LEN];
    uint64_t pw2_[MAX_LEN];
    std::vector<int> head_;
    std::vector<Node> pool_;
    int free_ = -1;
    size_t size_ = 0;

    // прямий і обернений хеш за один прохід; код символу 1..26
    Hash hash(const char* s, int len) const {
        uint64_t h1 = 0, r1 = 0, h2 = 0, r2 = 0;
        for (int i = 0; i < len; ++i) {
            uint64_t a = (uint64_t)(s[i] - 'a' + 1);
            uint64_t b = (uint64_t)(s[len - 1 - i] - 'a' + 1);
            h1 += a * pw1_[i];
            r1 += b * pw1_[i];
            h2 += a * pw2_[i];
            r2 += b * pw2_[i];
        }
        return {(uint32_t)(h1 % M1), (uint32_t)(r1 % M1), h2, r2};
    }

    size_t bucket(uint32_t h1, uint64_t h2) const {
        uint64_t x = h2 ^ ((uint64_t)h1 * 0x9E3779B97F4A7C15ULL);
        x ^= x >> 32;
        return (size_t)(x & (head_.size() - 1));
    }

    int find(const char* s, int len, const Hash& h, int* prev) const {
        int pr = -1;
        for (int id = head_[bucket(h.h1, h.h2)]; id != -1; pr = id, id = pool_[id].next) {
            const Node& nd = pool_[id];
            if (nd.h2 == h.h2 && nd.h1 == h.h1 && nd.len == len &&
                std::memcmp(nd.s, s, len) == 0) {
                if (prev) *prev = pr;
                return id;
            }
        }
        return -1;
    }

    // подвоєння таблиці, хеші беремо зі збережених
    void grow() {
        std::vector<int> old;
        old.swap(head_);
        head_.assign(old.size() * 2, -1);
        for (int first : old) {
            for (int id = first; id != -1;) {
                int nx = pool_[id].next;
                size_t b = bucket(pool_[id].h1, pool_[id].h2);
                pool_[id].next = head_[b];
                head_[b] = id;
                id = nx;
            }
        }
    }
};
