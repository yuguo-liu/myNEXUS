#include "utils.h"


string generateRandomString(size_t length) {
    const string chars =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    static mt19937 rng(static_cast<unsigned long>(time(nullptr)));
    uniform_int_distribution<> dist(0, chars.size() - 1);

    string result;
    result.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        result += chars[dist(rng)];
    }

    return result;
}


vector<string> split(const string &s, const string &delimiter) {
    vector<string> tokens;
    size_t start = 0;
    size_t end;
    while ((end = s.find(delimiter, start)) != string::npos) {
        tokens.push_back(s.substr(start, end - start));
        start = end + delimiter.length();
    }
    tokens.push_back(s.substr(start));
    return tokens;
}