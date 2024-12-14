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

def clean_data(file_name, num_experiments, num_iterations, interpolation = False):
    x = [1000 * i for i in range(0, num_iterations)]
    y = [-1.0 for i in range(0, num_iterations)]

    for i in range(1, num_experiments + 1):
        iterations, exploitabilities = read_exploitability_log(file_name + f"_{i}.txt")
        y_curr = [-1.0 for i in range(0, num_iterations)]
        for j in range(len(iterations)):
            x_index = iterations[j]
            x_index = iterations[j] // 1000
            y_curr[x_index] = exploitabilities[j]

        if interpolation:
            for j in range(1,num_iterations - 1):
                if y_curr[j] == -1.0:
                    y_curr[j] = (y_curr[j - 1] + y_curr[j + 1])/2.0

            if y_curr[-1] == -1.0:
                y_curr[-1] = y_curr[-2]
            if y_curr[0] == -1.0:
                y_curr[0] = y_curr[1]

        for j in range(num_iterations):
            y[j] += y_curr[j]

    x = x[12:num_iterations - 11]
    y = y[12:num_iterations - 11]
    y = [y[i] / num_experiments for i in range(len(y))]
    return x, y

def clean_data_mccfr(file_name, num_experiments):
    x_mccfr = [1000 * i for i in range(0, 412)]
    y_mccfr = [0.0 for i in range(0, 412)]

    for i in range(1, num_experiments + 1):
        iterations, exploitabilities = read_exploitability_log(file_name + f"_{i}.txt")
        for j in range(411):
            x_index = iterations[j]
            x_index = iterations[j] // 1000
            y_mccfr[x_index] += exploitabilities[j]

    x_mccfr = x_mccfr[12:401]
    y_mccfr = y_mccfr[12:401]
    y_mccfr = [y_mccfr[i] / num_experiments for i in range(len(y_mccfr))]
    return x_mccfr, y_mccfr

x, y = clean_data("data/exploitability_log", 10, 412, interpolation = True)
x_a, y_a = clean_data("data/LUCB_average_exploitability_log", 100, 412, interpolation = True)
x_uniform, y_uniform = clean_data("data/LUCB_uniform_exploitability_log", 100, 412, interpolation = True)
x_mccfr, y_mccfr = clean_data_mccfr("data/mccfr_exploitability_log", 47)
x_mccfr_dampened, y_mccfr_dampened = clean_data_mccfr("data/mccfr_dampen_eps_exploitability_log", 100)

# plt.plot(x_uniform, y_uniform, marker='', linewidth=1, color='blue', linestyle = '--', label='Exploitability of LUCB with uniform exploration against number of samples, averaged over 100 experiments')
plt.plot(x, y, marker='', linewidth=1, color='black', label='LUCB, averaged over 100 experiments')
plt.plot(x_mccfr, y_mccfr, marker='', linewidth=1, color='red', label='MCCFR, averaged over 47 experiments')
plt.plot(x_mccfr_dampened, y_mccfr_dampened, marker='', linewidth=1, color='green', label='MCCFR with dampened epsilon, averaged over 100 experiments')
# plt.plot(x_a, y_a, marker='', linewidth=1, color='blue', label='Exploitability of LUCB with average policy against number of samples, averaged over 96 experiments')

plt.yticks([0.01, 0.05, 0.1, 0.2, 0.3, 0.5])
plt.xlabel('Number of samples')
plt.ylabel('Exploitability')
plt.legend()
plt.show()

