import matplotlib.pyplot as plt

# Your raw data: (n, permutations, kth_permutation, ktm_static, ktm_dynamic, chunk_1000, chunk_10000, chunk_100000)
data = [
    (2, 7.01e-07, 1.776e-06, 6.084e-06, 2.818e-06, 3.128e-06, 2.936e-06, 2.761e-06),
    (3, 1.282e-06, 5.035e-06, 0.000109714, 3.688e-06, 6.297e-06, 6.303e-06, 6.086e-06),
    (4, 2.927e-06, 2.106e-05, 7.547e-06, 5.812e-06, 2.245e-05, 2.2798e-05, 2.1719e-05),
    (5, 1.3068e-05, 0.000139349, 2.9834e-05, 2.7672e-05, 0.000141594, 0.00014985, 0.000141944),
    (6, 9.3519e-05, 0.00110984, 0.0186569, 0.017305, 0.00262899, 0.00343598, 0.000777869),
    (7, 0.000494091, 0.00790104, 0.00147931, 0.00140174, 0.00218522, 0.0150199, 0.00884868),
    (8, 0.00376286, 0.0692966, 0.0171443, 0.0135838, 0.0154931, 0.0250984, 0.0698757),
    (9, 0.0379948, 0.675036, 0.156305, 0.149438, 0.148352, 0.152756, 0.223618),
    (10, 0.297344, 5.99246, 1.29926, 1.28267, 1.30087, 1.34084, 1.45739),
    (11, 3.20421, 74.477, 18.988, 19.0699, 20.3838, 21.2485, 22.156),
]

# Split into lists for plotting
n_values = [row[0] for row in data]
permutations = [row[1] for row in data]
kth_permutation = [row[2] for row in data]
ktm_static = [row[3] for row in data]
ktm_dynamic = [row[4] for row in data]
chunk_1000 = [row[5] for row in data]
chunk_10000 = [row[6] for row in data]
chunk_100000 = [row[7] for row in data]

# Plot
plt.figure(figsize=(12, 8))

plt.plot(n_values, permutations, label='permutations', marker='o')
plt.plot(n_values, kth_permutation, label='kth_permutation', marker='o')
plt.plot(n_values, ktm_static, label='ktm_static', marker='o')
plt.plot(n_values, ktm_dynamic, label='ktm_dynamic', marker='o')
plt.plot(n_values, chunk_1000, label='chunk_1000', marker='o')
plt.plot(n_values, chunk_10000, label='chunk_10000', marker='o')
plt.plot(n_values, chunk_100000, label='chunk_100000', marker='o')

plt.xlabel('n')
plt.ylabel('Time (or whatever unit)')
plt.title('Trace Times as Function of n')
plt.yscale('log')  # optional: log scale because values vary a lot
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.show()
