from rbt_classes import TicTacToeBoard, InformationSet, Policy, NonTerminalHistory
from evaluate_policy_parallel import get_expected_utility
import matplotlib.pyplot as plt
import logging
import json
from multiprocessing import Pool
from config import num_workers
import argparse

logging.basicConfig(format='%(levelname)s - %(asctime)s - %(message)s', datefmt='%d-%b-%y %H:%M:%S',
                    level=logging.INFO)


def parallel_run(I_1, I_2, true_board, player, p1_policy_obj, p2_policy_obj, History, round):
    logging.info("Getting expected utility for round {}...".format(round))
    expected_utility = get_expected_utility(I_1, I_2, true_board, player, p1_policy_obj, p2_policy_obj, 1,
                                            History, player)
    logging.info('Expected Utility:{}'.format(expected_utility))
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
    arguments = parser.parse_args()
    p1_file_base = arguments.PolicyFileXBase
    p2_file_base = arguments.PolicyFileOBase
    expected_utility_list = []
    num = int(arguments.Rounds)
    x_range = range(1, num)
    args = []
    for round in x_range:
        p1_policy_dict = json.load(open(p1_file_base.format(str(round)) + '.json', 'r'))
        p1_policy_obj = Policy(policy_dict=p1_policy_dict, player='x')

        p2_policy_dict = json.load(open(p2_file_base.format(str(round)) + '.json', 'r'))
        p2_policy_obj = Policy(policy_dict=p2_policy_dict, player='o')
        args.append((I_1, I_2, true_board, player, p1_policy_obj, p2_policy_obj, NonTerminalHistory(), round))

    with Pool(num_workers) as pool:
        exp_util_list = pool.starmap(parallel_run, args)

    plt.plot([i for i in range(1, num)], exp_util_list)
    plt.xlabel('Round')
    plt.xticks(rotation=-60)
    plt.ylabel('Expected Utility')
    plt.title('Convergence of Expected Utility')

    plt.savefig('./{}/expected_util/plots/{}.pdf'.format(arguments.BasePath, arguments.OutputFileName))
    with open('./{}/expected_util/{}.json'.format(arguments.BasePath, arguments.OutputFileName), 'w') as f:
        json.dump({"0": exp_util_list}, f)
