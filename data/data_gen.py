import numpy as np
import matplotlib.pyplot as plt


# Define the GELU function
def gelu(x):
    return x * 0.5 * (1 + np.tanh(np.sqrt(2 / np.pi) * (x + 0.044715 * x**3)))


def layernorm(x):
    return np.sqrt(len(x)) * x / np.sqrt(np.sum(x**2))


def softmax(x):
    return np.exp(x) / np.sum(np.exp(x))

def argmax(x):
    max_idx = np.argmax(x)
    return np.eye(len(x))[max_idx]


# Matrix Multiplication
def generate_matrices(k, m, n):
    """ generate random matrices pair in the shape of k x m and m x n
    
    Args:
        k (int): the number of row of first matrix
        m (int): the number of colmun of first matrix (row of second matrix)
        n (int): the number of colmun of second matrix
    
    Returns:
        the randomly generated matrices
    """
    np.random.seed(42)
    matrix_a = np.random.uniform(-1, 1, (k, m))
    matrix_b = np.random.uniform(-1, 1, (m, n))

    return matrix_a, matrix_b

# Matrix Multiplication
def generate_matrix(k, m):
    """ generate random matrices pair in the shape of k x m
    
    Args:
        k (int): the number of row of first matrix
        m (int): the number of colmun of first matrix 
    
    Returns:
        the randomly generated matrix
    """
    np.random.seed(44)
    matrix_a = np.random.uniform(-1, 1, (k, m))

    return matrix_a


def save_matrix_to_txt(matrix, filename):
    np.savetxt(filename, matrix, fmt="%.4f", delimiter=" ")


def multiply_matrices(matrix1, matrix2):
    return np.dot(matrix1, matrix2)


k, m, n = 4096, 768, 64

matrix_a, matrix_b = generate_matrices(k, m, n)
matrix_rnd = generate_matrix(k, m)
matrix_res = multiply_matrices(matrix_a, matrix_b)


save_matrix_to_txt(matrix_a, f"input/matrix_client_input_k_{k}_m_{m}.mtx")
save_matrix_to_txt(matrix_b, f"input/matrix_server_input_m_{m}_n_{n}.mtx")
save_matrix_to_txt(matrix_rnd, f"input/matrix_random_input_k_{k}_m_{m}.mtx")
save_matrix_to_txt(matrix_res, f"calibration/matrix_output_k_{k}_n_{n}.mtx")
