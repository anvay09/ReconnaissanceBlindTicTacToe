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


# LUCB data
num_experiments = 100
x = [1000 * i for i in range(0, 412)]
y = [-1.0 for i in range(0, 412)]

for i in range(1, num_experiments + 1):
    iterations, exploitabilities = read_exploitability_log(f"data/exploitability_log_{i}.txt")
    y_curr = [-1.0 for i in range(0, 412)]
    for j in range(len(iterations)):
        x_index = iterations[j]
        x_index = iterations[j] // 1000
        y_curr[x_index] = exploitabilities[j]

    for j in range(1,411):
        if y_curr[j] == -1.0:
            y_curr[j] = (y_curr[j - 1] + y_curr[j + 1])/2.0

    if y_curr[-1] == -1.0:
        y_curr[-1] = y_curr[-2]
    if y_curr[0] == -1.0:
        y_curr[0] = y_curr[1]

    for j in range(412):
        y[j] += y_curr[j]

    
x = x[12:401]
y = y[12:401]
y = [y[i] / num_experiments for i in range(len(y))]

# LUCB uniform data
num_experiments = 88
x_uniform = [1000 * i for i in range(0, 412)]
y_uniform = [-1.0 for i in range(0, 412)]

for i in range(1, num_experiments + 1):
    iterations, exploitabilities = read_exploitability_log(f"data/LUCB_uniform_exploitability_log_{i}.txt")
    y_curr = [-1.0 for i in range(0, 412)]
    for j in range(len(iterations)):
        x_index = iterations[j]
        x_index = iterations[j] // 1000
        y_curr[x_index] = exploitabilities[j]
        
    for j in range(1,411):
        if y_curr[j] == -1.0:
            y_curr[j] = (y_curr[j - 1] + y_curr[j + 1])/2.0

    if y_curr[-1] == -1.0:
        y_curr[-1] = y_curr[-2]
    if y_curr[0] == -1.0:
        y_curr[0] = y_curr[1]

    for j in range(412):
        y_uniform[j] += y_curr[j]

x_uniform = x_uniform[12:401]
y_uniform = y_uniform[12:401]
y_uniform = [y_uniform[i] / num_experiments for i in range(len(y_uniform))]

# mccfr data
num_experiments = 4
x_mcfr = [1000 * i for i in range(0, 412)]
y_mcfr = [0.0 for i in range(0, 412)]

for i in range(1, num_experiments + 1):
    iterations, exploitabilities = read_exploitability_log(f"data/mccfr_exploitability_log_{i}.txt")
    for j in range(411):
        x_index = iterations[j]
        x_index = iterations[j] // 1000
        y_mcfr[x_index] += exploitabilities[j]

x_mcfr = x_mcfr[12:401]
y_mcfr = y_mcfr[12:401]
y_mcfr = [y_mcfr[i] / num_experiments for i in range(len(y_mcfr))]


# mccfr damnpened data
num_experiments = 15
x_mcfr_dampened = [1000 * i for i in range(0, 412)]
y_mcfr_dampened = [0.0 for i in range(0, 412)]

for i in range(1, num_experiments + 1):
    iterations, exploitabilities = read_exploitability_log(f"data/mccfr_dampen_eps_exploitability_log_{i}.txt")
    for j in range(411):
        x_index = iterations[j]
        x_index = iterations[j] // 1000
        y_mcfr_dampened[x_index] += exploitabilities[j]

x_mcfr_dampened = x_mcfr_dampened[12:401]
y_mcfr_dampened = y_mcfr_dampened[12:401]
y_mcfr_dampened = [y_mcfr_dampened[i] / num_experiments for i in range(len(y_mcfr_dampened))]


# plot LUCB uniform data, yellow color
plt.plot(x_uniform, y_uniform, marker='', linewidth=1, color='blue', linestyle = '--', label='Exploitability of LUCB with uniform exploration against number of samples, averaged over 88 experiments')
# plot x vs y, join points with lines, no markers, line width 1, color blue, label "Exploitability of LUCB BR against number of samples, averaged over 5 experiments"
plt.plot(x, y, marker='', linewidth=1, color='black', label='Exploitability of LUCB BR against number of samples, averaged over 100 experiments')
# plot mccfr data, red color
plt.plot(x_mcfr, y_mcfr, marker='', linewidth=1, color='red', label='Exploitability of MCCFR against number of samples, averaged over 4 experiments')
# plot mccfr dampened data, green color
plt.plot(x_mcfr_dampened, y_mcfr_dampened, marker='', linewidth=1, color='green', label='Exploitability of MCCFR with dampened epsilon against number of samples, averaged over 15 experiments')

plt.yticks([0.01, 0.05, 0.1, 0.2, 0.3, 0.5])
plt.xlabel('Number of samples')
plt.ylabel('Exploitability')
plt.legend()
plt.show()

