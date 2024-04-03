from rbt_classes import TicTacToeBoard, InformationSet, Policy, NonTerminalHistory
from evaluate_policy import get_expected_utility
import matplotlib.pyplot as plt
import logging
import json
from multiprocessing import Pool
from config import num_workers

logging.basicConfig(format='%(levelname)s - %(asctime)s - %(message)s', datefmt='%d-%b-%y %H:%M:%S',
                    level=logging.INFO)


def parallel_run(I_1, I_2, true_board, player, p1_policy_obj, p2_policy_obj, History, round):
    logging.info("Getting expected utility for round {}, player {}...".format(round, 'o'))
    expected_utility = get_expected_utility(I_1, I_2, true_board, player, p1_policy_obj, p2_policy_obj, 1,
                                                     History, player)
    print('Expected Utility: ', expected_utility)
    return expected_utility


if __name__ == "__main__":
    true_board = TicTacToeBoard(board=['0', '0', '0', '0', '0', '0', '0', '0', '0'])
    I_1 = InformationSet(player='x', move_flag=True, board=['0', '0', '0', '0', '0', '0', '0', '0', '0'])
    I_2 = InformationSet(player='o', move_flag=False, board=['-', '-', '-', '-', '-', '-', '-', '-', '-'])
    player = 'x'

    p1_file_base = 'data_files_avg/P1_average_overall_policy_after_{}_rounds'
    p2_file_base = 'data_files_avg/P2_average_overall_policy_after_{}_rounds'
    expected_utility_list = []

    x_range = range(1, 31)
    args = []
    for round in x_range:
        p1_policy_dict = json.load(open(p1_file_base.format(str(round)) + '.json', 'r'))
        p1_policy_obj = Policy(policy_dict=p1_policy_dict, player='x')

        p2_policy_dict = json.load(open(p2_file_base.format(str(round)) + '.json', 'r'))
        p2_policy_obj = Policy(policy_dict=p2_policy_dict, player='o')
        args.append((I_1, I_2, true_board, player, p1_policy_obj, p2_policy_obj,
                                                     NonTerminalHistory(), round))

    with Pool(num_workers) as pool:
        dict_list = pool.starmap(parallel_run, args)

    plt.plot(dict_list)
    plt.xticks(range(0, len(x_range)*2, 2), x_range)
    plt.xlabel('Round')
    plt.ylabel('Expected Utility')
    plt.title('Convergence of Expected Utility')
    plt.savefig('avg_expected_utility_convergence.png')
