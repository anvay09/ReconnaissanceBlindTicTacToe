from rbt_classes import TicTacToeBoard, InformationSet, Policy, NonTerminalHistory
from evaluate_policy_parallel import get_expected_utility_parallel
import matplotlib.pyplot as plt
import logging
import json

logging.basicConfig(format='%(levelname)s - %(asctime)s - %(message)s', datefmt='%d-%b-%y %H:%M:%S',
                    level=logging.INFO)

if __name__ == "__main__":
    true_board = TicTacToeBoard(board=['0', '0', '0', '0', '0', '0', '0', '0', '0'])
    I_1 = InformationSet(player='x', move_flag=True, board=['0', '0', '0', '0', '0', '0', '0', '0', '0'])
    I_2 = InformationSet(player='o', move_flag=False, board=['-', '-', '-', '-', '-', '-', '-', '-', '-'])
    player = 'x'

    p1_file_base = 'data_files_avg/P1_average_overall_policy_after_{}_rounds'
    p2_file_base = 'data_files_avg/P2_average_overall_policy_after_{}_rounds'
    expected_utility_list = []

    x_range = range(1, 89)

    for round in x_range:
        p1_policy_dict = json.load(open(p1_file_base.format(str(round)) + '.json', 'r'))
        p1_policy_obj = Policy(policy_dict=p1_policy_dict, player='x')

        p2_policy_dict = json.load(open(p2_file_base.format(str(round)) + '.json', 'r'))
        p2_policy_obj = Policy(policy_dict=p2_policy_dict, player='o')

        logging.info("Getting expected utility for round {}".format(round))
        expected_utility = get_expected_utility_parallel(I_1, I_2, true_board, player, p1_policy_obj, p2_policy_obj, 1, NonTerminalHistory(), player)
        logging.info('Expected Utility: {}'.format(expected_utility))
        expected_utility_list.append(expected_utility)

    plt.plot([i for i in range(1, 89)], expected_utility_list)
    # plt.xticks(range(0, len(x_range)*2, 2), x_range)
    plt.xlabel('Round')
    plt.xticks(rotation=-60)
    plt.ylabel('Expected Utility')
    plt.title('Convergence of Expected Utility')
    plt.savefig('avg_expected_utility_convergence.png')
