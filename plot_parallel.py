from rbt_classes import TicTacToeBoard, InformationSet, Policy, NonTerminalHistory
from evaluate_policy_parallel import get_expected_utility, get_expected_utility_parallel
from multiprocessing import Pool
import matplotlib.pyplot as plt
from config import num_workers
import argparse
import logging
import numpy as np
import json
import os

logging.basicConfig(format='%(levelname)s - %(asctime)s - %(message)s', datefmt='%d-%b-%y %H:%M:%S',
                    level=logging.INFO)


def parallel_run(I_1, I_2, true_board, player, p1_policy_obj, p2_policy_obj, History, round):
    logging.info("Getting expected utility for round {}...".format(round))
    expected_utility = get_expected_utility(I_1, I_2, true_board, player, p1_policy_obj, p2_policy_obj, 1,
                                            History, player)
    logging.info('Expected Utility:{}'.format(expected_utility))
    return expected_utility

def parallel_cpp(file_path_1, file_path_2, round, save_games):
    os.system("./evaluate_policy" + " " + file_path_1 + " " + file_path_2 + " " + str(round) + " " + str(save_games))
    
    outfile = "round_" + str(round) + "_expected_utility.txt"
    with open(outfile, 'r') as f:
        expected_utility = f.read()
    os.system("rm " + outfile)
    logging.info('Expected Utility for round {} : {}'.format(expected_utility, round))
    return expected_utility

        
if __name__ == "__main__":
    true_board = TicTacToeBoard(board=['0', '0', '0', '0', '0', '0', '0', '0', '0'])
    I_1 = InformationSet(player='x', move_flag=True, board=['0', '0', '0', '0', '0', '0', '0', '0', '0'])
    I_2 = InformationSet(player='o', move_flag=False, board=['-', '-', '-', '-', '-', '-', '-', '-', '-'])
    player = 'x'

    parser = argparse.ArgumentParser()
    parser.add_argument('--PolicyFileXBase', type=str, required=True)
    parser.add_argument('--PolicyFileOBase', type=str, required=True)
    parser.add_argument('--OutputFileName', type=str, required=True)
    parser.add_argument('--Rounds', type=str, required=True)
    parser.add_argument('--BasePath', type=str, required=True)
    parser.add_argument('--StartRound', type=str, required=False, default=1)
    arguments = parser.parse_args()
    p1_file_base = arguments.PolicyFileXBase
    p2_file_base = arguments.PolicyFileOBase
    expected_utility_list = []
    start_round = int(arguments.StartRound)
    num = int(arguments.Rounds)
    x_range = range(start_round, num)
    args = []

    for round in x_range:
        p1_file = p1_file_base.format(str(round)) + '.json'
        p2_file = p2_file_base.format(str(round)) + '.json'
        args.append((p1_file, p2_file, round, 0))

    with Pool(num_workers) as pool:
        exp_util_list = pool.starmap(parallel_cpp, args)

    X = [i for i in range(start_round, num)]
    Y = [float(exp_util) for exp_util in exp_util_list]
    plt.plot(X, Y, marker='o', color='b', label='Expected Utility')
    plt.xlabel('Round')
    plt.xticks(rotation=-60)
    plt.yticks(np.arange(0, 1, 0.1))
    plt.ylabel('Expected Utility')
    plt.title('Convergence of Expected Utility')

    plt.savefig('{}/expected_util/plots/{}.pdf'.format(arguments.BasePath, arguments.OutputFileName))
    with open('{}/expected_util/{}.json'.format(arguments.BasePath, arguments.OutputFileName), 'w') as f:
        json.dump({"0": exp_util_list}, f)
