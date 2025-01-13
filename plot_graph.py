import matplotlib.pyplot as plt
import argparse
import logging

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
    parser.add_argument('logfiles', nargs='+', type=str, required=True, help='List of log file names')
    parser.add_argument('algorithms', nargs='+', type=str, required=True, help='List of algorithms')
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
    x = [step_size * i for i in range(0, num_iterations + 1)]
    y = [0.0 for i in range(0, num_iterations + 1)]

    for i in range(1, num_experiments + 1):
        iterations, exploitabilities = read_exploitability_log(file_name + f"_{i}.txt")
        y_curr = [-1.0 for i in range(0, num_iterations + 1)]
        for j in range(len(iterations)):
            x_index = iterations[j] // step_size
            if player == 'x':
                y_curr[x_index] = exploitabilities[j]
            else:
                y_curr[x_index] = -1 * exploitabilities[j]

        for j in range(0, num_iterations + 1):
            y[j] += y_curr[j]

    x = x[omit_range:num_iterations + 1]
    y = y[omit_range:num_iterations + 1]
    y = [y[i] / num_experiments for i in range(len(y))]
    return x, y


if __name__ == "__main__":
    args = parse_commandline_args()
    line_styles = ['solid', 'dashed', 'dotted', 'dashdot']
    colors = ['blue', 'red', 'green', 'orange', 'purple', 'brown', 'pink', 'olive']
    markers = ['-', 'o', 'x']
    i = 0
    j = 0
    k = 0

    for logfile in args.logfiles:
        if args.omitrange is None:
            x, y = clean_data(args.player, logfile, args.numexperiments, args.numiterations, args.logfreq)
        else:
            x, y = clean_data(args.player, logfile, args.numexperiments, args.numiterations, args.logfreq,
                              args.omitrange)

        plt.plot(x, y, '-', linewidth=1, color=colors[i], line_style=line_styles[j],
                 label='Our Algorithm for player ', marker=markers[k])
        i += 1
        j += 1
        if j > len(line_styles):
            j = 0
            k += 1

    # horizontal line
    plt.axhline(y=0, color='black', linestyle='--', linewidth=0.4)
    plt.yticks([0.01, 0.05, 0.1, 0.15])
    plt.xlabel('Number of samples')
    plt.ylabel('Exploitability')
    plt.title(args.game)
    plt.legend()
    plot_path = ("./data/plot/game={game}_player={player}_numexperiments={numexperiments}_numiterations={numiterations}"
                 "_logfreq={logfreq}_cours={cours}_epsmccfr={epsmccfr}_cuct={cuct}_epsuct={epsuct}_nuct={nuct}"
                 "_duct={duct}".format(game=args.game, player=args.player, numexperiments=args.numexperiments,
                                       numiterations=args.numiterations, logfreq=args.logfreq, cours=args.cours,
                                       epsmccfr=args.epsmccfr, cuct=args.cuct, epsuct=args.epsuct, nuct=args.nuct,
                                       duct=args.duct))
    plt.savefig("{}.pdf".format(plot_path))
    plt.show()
