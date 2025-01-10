import matplotlib.pyplot as plt

def read_exploitability_log(file_name):
    with open(file_name) as f:
        lines = f.readlines()
    iterations = []
    exploitabilities = []
    for line in lines:
        iteration, exploitability = line.split()
        iterations.append(int(iteration))
        exploitabilities.append(float(exploitability))
    return iterations, exploitabilities

def clean_data(file_name, num_experiments, num_iterations, step_size = 1000, omit_range = 1, multiplier = 1, interpolation = False):
    x = [step_size * i for i in range(0, num_iterations+1)]
    y = [0.0 for i in range(0, num_iterations+1)]

    for i in range(1, num_experiments + 1):
        iterations, exploitabilities = read_exploitability_log(file_name + f"_{i}.txt")
        y_curr = [-1.0 for i in range(0, num_iterations+1)]
        for j in range(len(iterations)):
            x_index = iterations[j] // step_size
            y_curr[x_index] = multiplier * exploitabilities[j]

        if interpolation:
            for j in range(1,num_iterations):
                if y_curr[j] == -1.0:
                    y_curr[j] = (y_curr[j - 1] + y_curr[j + 1])/2.0

            if y_curr[-1] == -1.0:
                y_curr[-1] = y_curr[-2]
            if y_curr[0] == -1.0:
                y_curr[0] = y_curr[1]

        for j in range(0, num_iterations+1):
            y[j] += y_curr[j]

    x = x[omit_range:num_iterations + 1]
    y = y[omit_range:num_iterations + 1]
    y = [y[i] / num_experiments for i in range(len(y))]
    return x, y

game = 'L'
player = 'x'
multiplier = 1
num_iterations = 100
step_size = 10000
omit_range = 2
LUCB_num_experiments = 100
MCCFR_num_experiments = 100
On_path_flipping_num_experiments = 100
C = 16
eps = 0.1
x, y = clean_data(f"data/{game}_poker_" + player + "_C=16_LUCB_exploitability_log", LUCB_num_experiments, num_iterations, step_size, omit_range, multiplier, interpolation = False)
x_mccfr, y_mccfr = clean_data(f"data/{game}_poker_" + player + "_MCCFR_OS_exploitability_log", MCCFR_num_experiments, num_iterations, step_size, omit_range, multiplier, interpolation = False)
x_opf, y_opf = clean_data(f"data/{game}poker_eps_constant=0.100000_" + player + "onpath_flipping_exploitability_log", On_path_flipping_num_experiments, num_iterations, step_size, omit_range, multiplier, interpolation = False)

plt.plot(x, y, '-', linewidth=1, color='blue', label='Our Algorithm for player ' + player + ', C = ' + str(C))
plt.plot(x_mccfr, y_mccfr, '-.', linewidth=1, color='red', label='MCCFR Outcome Sampling for player ' + player + ', eps =' + str(eps))
plt.plot(x_opf, y_opf, ':', linewidth=1, color='green', label='On-Path Flipping for player ' + player + ', eps =' + str(eps))

# horizontal line
plt.axhline(y=0, color='black', linestyle='--', linewidth=0.4)
plt.yticks([0.01, 0.05, 0.1, 0.15])
plt.xlabel('Number of samples')
plt.ylabel('Exploitability')
plt.title('Leduc Poker')
plt.legend()
plt.savefig(f'Leduc_{player}_all_3_new.pdf')
plt.show()
