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

player = 'x'
LUCB_num_experiments = 14
MCCFR_num_experiments = 10
C = 16
x, y = clean_data("data/x_C=16_LUCB_exploitability_log", LUCB_num_experiments, 50, 10000, 2, 1, interpolation = False)
x_mccfr, y_mccfr = clean_data("data/x_MCCFR_OS_exploitability_log", MCCFR_num_experiments, 50, 10000, 1, 1, interpolation = False)

plt.plot(x, y, marker='', linewidth=1, color='blue', label='LUCB for player ' + player + ', C = ' + str(C) + ', against number of samples, averaged over ' + str(LUCB_num_experiments) + ' experiments')
plt.plot(x_mccfr, y_mccfr, marker='', linewidth=1, color='red', label='MCCFR Outcome sampling for player ' + player + ', averaged over ' + str(MCCFR_num_experiments) + ' experiments')

# horizontal line
plt.axhline(y=0, color='black', linestyle='--', linewidth=0.4)
plt.yticks([0.01, 0.05, 0.1, 0.2, 0.3, 0.4, 0.5])
plt.xlabel('Number of samples')
plt.ylabel('Exploitability')
plt.legend()
plt.show()

