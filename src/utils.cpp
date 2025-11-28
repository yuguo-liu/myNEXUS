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


string base64_encode(const unsigned char* buffer, size_t length) {
    BIO *bio, *b64;
    BUF_MEM *bufferPtr;

    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    BIO_write(bio, buffer, length);
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bufferPtr);

    string result(bufferPtr->data, bufferPtr->length);
    BIO_free_all(bio);
    return result;
}


string matrix_to_string(const vector<vector<double>>& mat) {
    uint32_t rows = mat.size();
    uint32_t cols = rows ? mat[0].size() : 0;

    vector<unsigned char> raw;
    raw.reserve(8 + rows * cols * 8);

    // insert the rows and cols
    raw.insert(raw.end(), reinterpret_cast<unsigned char*>(&rows),
               reinterpret_cast<unsigned char*>(&rows) + 4);
    raw.insert(raw.end(), reinterpret_cast<unsigned char*>(&cols),
               reinterpret_cast<unsigned char*>(&cols) + 4);

    // insert the matrix
    for (const auto& row : mat) {
        for (double v : row) {
            unsigned char b[8];
            memcpy(b, &v, 8);
            raw.insert(raw.end(), b, b + 8);
        }
    }

    // Base64 encoding
    return base64_encode(raw.data(), raw.size());
}


vector<unsigned char> base64_decode(const string& input) {
    BIO *bio, *b64;
    int decodeLen = input.length();
    vector<unsigned char> buffer(decodeLen);

    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_new_mem_buf(input.data(), input.length());
    bio = BIO_push(b64, bio);

    int length = BIO_read(bio, buffer.data(), decodeLen);
    buffer.resize(length);
    BIO_free_all(bio);
    return buffer;
}


vector<vector<double>> string_to_matrix(const string& s) {
    auto raw = base64_decode(s);

    uint32_t rows, cols;

    memcpy(&rows, raw.data(), 4);
    memcpy(&cols, raw.data() + 4, 4);

    vector<vector<double>> mat(rows, vector<double>(cols));

    const unsigned char* p = raw.data() + 8;

    for (uint32_t i = 0; i < rows; i++) {
        for (uint32_t j = 0; j < cols; j++) {
            memcpy(&mat[i][j], p, 8);
            p += 8;
        }
    }

    return mat;
}
