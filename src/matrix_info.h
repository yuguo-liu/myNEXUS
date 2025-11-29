#ifndef _MATRIX_INFO_H_
#define _MATRIX_INFO_H_

/**
 * MatrixInfo is a structure to denote the row/col information of two matrice
 */
typedef struct MatrixInfo
{
    int idx;
    int first_row;
    int first_col;
    int second_row;
    int second_col;

    MatrixInfo(int i, int k, int m, int n) {
        idx = i;
        first_row = k;
        first_col = m;
        second_row = m;
        second_col = n;
    }
} MatrixInfo;

#endif