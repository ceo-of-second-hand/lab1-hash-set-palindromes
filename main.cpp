// Лабораторна робота, варіант 2: множина рядків + пошук паліндромів.
// Запуск: lab [-t] [input [output]]   (-t — вивести час роботи)

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "string_set.h"

// читає наступне слово; повертає довжину або -1 в кінці файлу
static int readToken(FILE* in, char* buf, int cap) {
    int c = std::getc(in);
    while (c == ' ' || c == '\n' || c == '\r' || c == '\t') c = std::getc(in);
    if (c == EOF) return -1;
    int n = 0;
    while (c != EOF && c != ' ' && c != '\n' && c != '\r' && c != '\t') {
        if (n < cap) buf[n] = (char)c;
        ++n;
        c = std::getc(in);
    }
    return n;
}

static bool validWord(const char* w, int len) {
    if (len < 1 || len > StringSet::MAX_LEN) return false;
    for (int i = 0; i < len; ++i)
        if (w[i] < 'a' || w[i] > 'z') return false;
    return true;
}

int main(int argc, char** argv) {
    bool timing = false;
    std::vector<const char*> files;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-t") == 0)
            timing = true;
        else
            files.push_back(argv[i]);
    }

    FILE* in = stdin;
    FILE* out = stdout;
    if (files.size() >= 1 && !(in = std::fopen(files[0], "rb"))) {
        std::fprintf(stderr, "cannot open %s\n", files[0]);
        return 1;
    }
    if (files.size() >= 2 && !(out = std::fopen(files[1], "wb"))) {
        std::fprintf(stderr, "cannot open %s\n", files[1]);
        return 1;
    }
    static char inBuf[1 << 16], outBuf[1 << 16];
    std::setvbuf(in, inBuf, _IOFBF, sizeof inBuf);
    std::setvbuf(out, outBuf, _IOFBF, sizeof outBuf);

    auto start = std::chrono::steady_clock::now();

    StringSet set;
    char op[8], word[64];
    long long lineNo = 0;
    for (;;) {
        int opLen = readToken(in, op, sizeof op);
        if (opLen == -1) break;
        ++lineNo;
        if (opLen == 1 && op[0] == '#') break;  // кінець вхідних даних

        int len = readToken(in, word, sizeof word);
        if (opLen != 1 || len == -1 || !validWord(word, len)) {
            std::fprintf(stderr, "line %lld: invalid operation, skipped\n", lineNo);
            continue;
        }
        switch (op[0]) {
            case '+': set.insert(word, len); break;
            case '-': set.erase(word, len); break;
            case '?': std::fputs(set.contains(word, len) ? "yes\n" : "no\n", out); break;
            default: std::fprintf(stderr, "line %lld: unknown operation, skipped\n", lineNo);
        }
    }

    // паліндроми за абеткою
    std::vector<std::string> pals = set.palindromes();
    std::sort(pals.begin(), pals.end());
    std::fprintf(out, "palindromes: %zu\n", pals.size());
    for (const std::string& s : pals) {
        std::fputs(s.c_str(), out);
        std::fputc('\n', out);
    }
    std::fflush(out);

    if (timing) {
        double ms = std::chrono::duration<double, std::milli>(
                        std::chrono::steady_clock::now() - start).count();
        std::fprintf(stderr, "time_ms=%.2f size=%zu capacity=%zu\n", ms, set.size(),
                     set.capacity());
    }
    if (in != stdin) std::fclose(in);
    if (out != stdout) std::fclose(out);
    return 0;
}
