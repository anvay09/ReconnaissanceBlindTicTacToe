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
y = [0.0 for i in range(0, 412)]

for i in range(1, num_experiments + 1):
    iterations, exploitabilities = read_exploitability_log(f"data/exploitability_log_{i}.txt")
    for j in range(len(iterations)):
        x_index = iterations[j]
        x_index = iterations[j] // 1000
        y[x_index] += exploitabilities[j]
    
x = x[12:]
y = y[12:]

y = [y[i] / num_experiments for i in range(len(y))]

# mcfr data
num_experiments = 1
x_mcfr = [1000 * i for i in range(0, 412)]
y_mcfr = [0.0 for i in range(0, 412)]

for i in range(1, num_experiments + 1):
    iterations, exploitabilities = read_exploitability_log(f"data/mcfr_exploitability_log_{i}.txt")
    for j in range(412):
        x_index = iterations[j]
        x_index = iterations[j] // 1000
        y_mcfr[x_index] += exploitabilities[j]

x_mcfr = x_mcfr[12:]
y_mcfr = y_mcfr[12:]

y_mcfr = [y_mcfr[i] / num_experiments for i in range(len(y_mcfr))]


# plot x vs y, join points with lines, no markers, line width 1, color blue, label "Exploitability of LUCB BR against number of samples, averaged over 5 experiments"
plt.plot(x, y, marker='', linewidth=1, color='blue', label='Exploitability of LUCB BR against number of samples, averaged over 100 experiments')
# plot mccfr data, red color
plt.plot(x_mcfr, y_mcfr, marker='', linewidth=1, color='red', label='Exploitability of MCCFR against number of samples, averaged over 1 experiment')
plt.xlabel('Number of samples')
plt.ylabel('Exploitability')
plt.legend()
plt.show()

