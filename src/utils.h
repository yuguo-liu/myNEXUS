#pragma once
#include <iostream>
#include <vector>
#include <random>
#include <ctime>
#include <string>
#include <cstring>
#include <cstdint>
#include <sstream>
#include <iomanip>

#include <openssl/evp.h>   // Base64
#include <openssl/buffer.h>

using namespace std;

string generateRandomString(size_t length);

vector<string> split(const string &s, const string &delimiter);

string base64_encode(const unsigned char* buffer, size_t length);

string matrix_to_string(const vector<vector<double>>& mat);

vector<unsigned char> base64_decode(const string& input);

vector<vector<double>> string_to_matrix(const string& s);
