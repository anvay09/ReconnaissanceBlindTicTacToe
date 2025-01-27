import matplotlib.pyplot as plt
import argparse
import logging
import numpy as np
import math

logging.basicConfig(format='%(levelname)s - %(asctime)s - %(message)s', datefmt='%d-%b-%y %H:%M:%S',
                    level=logging.INFO)


def parse_commandline_args():
    """Parse command line arguments"""
    logging.info("Fetching command line arguments")
    parser = argparse.ArgumentParser()
    parser.add_argument('--game', type=str, required=True, help='Khun,Leduc, RBT')
    parser.add_argument('--player', type=str, required=True, help='x/o')
    parser.add_argument('--numexperiments', type=int, required=True, help='number of experiments')
    parser.add_argument('--numiterations', type=int, required=True, help='number of iterations')
    parser.add_argument('--logfreq', type=int, required=True, help='frequency of logs')
    parser.add_argument('--omitrange', type=int, required=False,
                        help='If need to ignore first few entries in a log (only applicable to our algorithm sue to '
                             'pull each arm once)')
    parser.add_argument('--cours', type=int, required=False,
                        help='Optional argument for our algorithm')
    parser.add_argument('--epsmccfr', type=float, required=False,
                        help='Optional argument for mccfr or its variants')
    parser.add_argument('--cuct', type=int, required=False,
                        help='Optional argument if using UCT or its variants')
    parser.add_argument('--epsuct', type=float, required=False,
                        help='Optional argument if using CT or its variants')
    parser.add_argument('--nuct', type=float, required=False,
                        help='Optional argument if using UCT smooth')
    parser.add_argument('--duct', type=float, required=False,
                        help='Optional argument if using UCT smooth')
    parser.add_argument('--logfiles', type=str, required=True, help='List of log file names')
    parser.add_argument('--algorithms', type=str, required=True, help='List of algorithms')
    parser.add_argument('--yaxisupper', type=float, required=True, help='Upper limit of y-axis')
    arguments = parser.parse_args()
    return arguments


def read_exploitability_log(file_name):
    """Read data files for plotting"""
    logging.info("Reading log file {}".format(file_name))
    with open(file_name) as f:
        lines = f.readlines()
    iterations = []
    exploitabilities = []
    for line in lines:
        iteration, exploitability = line.split()
        iterations.append(int(iteration))
        exploitabilities.append(float(exploitability))
    return iterations, exploitabilities


def clean_data(player, file_name, num_experiments, num_iterations, step_size=1000, omit_range=1):
    """Format data as required."""
    x = [i for i in range(0, num_iterations, step_size)]
    y = [0.0 for i in range(0, num_iterations, step_size)]
    y_std_list = [[0.0 for m in range(0, num_experiments)] for i in range(0, num_iterations, step_size)]
    y_err = [0.0 for j in range(0, num_iterations, step_size)]

    for i in range(1, num_experiments+1):
        iterations, exploitabilities = read_exploitability_log(file_name + f"_{i}.txt")
        iterations = iterations[:num_iterations // step_size]
        exploitabilities = exploitabilities[:num_iterations // step_size]

        y_curr = [-1.0 for i in range(0, num_iterations+1, step_size)]

        for j in range(len(iterations)):
            x_index = iterations[j] // step_size

            if player == 'x':
                y_curr[x_index-1] = exploitabilities[j]
            else:
                y_curr[x_index-1] = -1 * exploitabilities[j]

        for j in range(len(y)):
            y[j] += y_curr[j]
            y_std_list[j][i - 1] = y_curr[j]

    for j in range(len(y)):
        y_err[j] = np.std(y_std_list[j]) / math.sqrt(num_experiments)

    x = x[omit_range:]
    y = y[omit_range:]
    y_err = y_err[omit_range:]
    y = [y[i] / num_experiments for i in range(len(y))]
    return x, y, y_err


if __name__ == "__main__":
    args = parse_commandline_args()
    line_styles = ['solid', 'dashed', 'dotted', 'dashdot', (0, (3, 1, 1, 1, 1, 1)), 'dashed', 'dotted', 'dashdot', 'solid', 'dashed']
    colors = ['blue', 'red', 'green', 'orange', 'purple', 'brown', 'pink', 'olive', 'black', 'magenta']
    markers = ['', 'o', 'x']
    a = 0
    b = 0
    c = 0
    logfiles_str = args.logfiles
    logfiles = logfiles_str.split(',')
    algorithms_str = args.algorithms
    algorithms = algorithms_str.split(',')

    # horizontal line
    plt.figure(figsize=(6,6))
    plt.grid(True, which='both', linestyle='-.', linewidth=0.3)
    low = -args.yaxisupper/20
    high = args.yaxisupper
    num_ticks = 5
    plt.yticks(np.arange(0, high, (high) / num_ticks), fontsize = 17)
    plt.xticks(fontsize = 17)
    plt.ticklabel_format(style='sci', axis='both', scilimits=(0,0))
    plt.ylim(low, high)
    plt.xlabel('Number of samples')
    plt.ylabel('Exploitability')
    plt.title(args.game + ", Player " + args.player)

    for logfile in logfiles:
        if args.omitrange is None:
            x, y, y_err = clean_data(args.player, logfile, args.numexperiments, args.numiterations,
                                     args.logfreq)
        else:
            x, y, y_err = clean_data(args.player, logfile, args.numexperiments, args.numiterations,
                                     args.logfreq, args.omitrange)

        plt.plot(x, y, '-', linewidth=1, color=colors[a], linestyle=line_styles[b],
                 label=algorithms[a], marker=markers[c])
        plt.fill_between(x, np.array(y) + np.array(y_err), np.array(y) - np.array(y_err), edgecolor=colors[a],
                         facecolor=colors[a], alpha=0.1)
        
        # save x, y, y_err to file in the format x y y_err, up to 4 decimal places

        log_name = logfile.split('/')[-1]
        with open(f"logs/{log_name}_data.txt", 'w') as f:
            for i in range(len(x)):
                f.write(f"{x[i]} ${y[i]:.4f} \pm {y_err[i]:.4f}$\n")

        a += 1
        b += 1
        if b > len(line_styles):
            b = 0
            c += 1
    plt.legend(fontsize = 17)
    plot_path = ("plots/game={game}_player={player}_numexperiments={numexperiments}_numiterations={numiterations}"
                 "_logfreq={logfreq}_cours={cours}_epsmccfr={epsmccfr}_cuct={cuct}_epsuct={epsuct}_nuct={nuct}"
                 "_duct={duct}".format(game=args.game, player=args.player, numexperiments=args.numexperiments,
                                       numiterations=args.numiterations, logfreq=args.logfreq, cours=args.cours,
                                       epsmccfr=args.epsmccfr, cuct=args.cuct, epsuct=args.epsuct, nuct=args.nuct,
                                       duct=args.duct))
    
    plt.savefig("{}.pdf".format(plot_path))
