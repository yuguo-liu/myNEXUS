#pragma once

#include <seal/seal.h>

#include <vector>

#include "ckks_evaluator.h"
class MMEvaluatorOpt {
private:
    CKKSEvaluator *ckks = nullptr;

    void enc_compress_ciphertext(vector<double> &vec, Ciphertext &ct);
    void multiply_power_of_x(Ciphertext &encrypted, Ciphertext &destination, int index);
    vector<Ciphertext> expand_ciphertext(const Ciphertext &encrypted, uint32_t m, GaloisKeys &galkey, vector<uint32_t> &galois_elts);

public:
    MMEvaluatorOpt(CKKSEvaluator &ckks) {
        this->ckks = &ckks;
    }

    void matrix_mul(vector<vector<double>> &x, vector<vector<double>> &y, vector<Ciphertext> &res);
    void matrix_encrypt(vector<vector<double>> &x, vector<Ciphertext> &res);
    void matrix_encode(vector<vector<double>> &x, vector<Plaintext> &res);

    void matrix_mul_in_plain(vector<vector<double>> &x, vector<vector<double>> &y, vector<vector<double>> &res);
    void matrix_add_in_plain(vector<vector<double>> &x, vector<vector<double>> &y, vector<vector<double>> &res);
    void matrix_sub_in_plain(vector<vector<double>> &x, vector<vector<double>> &y, vector<vector<double>> &res);

    std::vector<std::vector<double>> readMatrix(const std::string &filename, int rows, int cols);
    std::vector<std::vector<double>> transposeMatrix(const std::vector<std::vector<double>> &matrix);
};
