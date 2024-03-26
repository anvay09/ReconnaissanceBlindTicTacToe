from rbt_classes import TicTacToeBoard, InformationSet, Policy, NonTerminalHistory
import argparse
import logging
import json
from evaluate_policy_parallel import get_expected_utility_parallel

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--Iteration', type=str, required=True)
    arguments = parser.parse_args()
    Itr = arguments.Iteration

    p1_file = 'data_files/P1_DG_policy.json'
    p2_file = 'data_files/P2_cfr_policy_round_{}.json'.format(1)
    z = 0
    cfr_round = 0

    while cfr_round < int(Itr):
        true_board = TicTacToeBoard(board=['0', '0', '0', '0', '0', '0', '0', '0', '0'])
        I_1 = InformationSet(player='x', move_flag=True, board=['0', '0', '0', '0', '0', '0', '0', '0', '0'])
        I_2 = InformationSet(player='o', move_flag=False, board=['-', '-', '-', '-', '-', '-', '-', '-', '-'])
        player = 'x'

        logging.info("cfr round {}, policy file x {}, policy file o {}".format(cfr_round + 1, p1_file, p2_file))

        p1_policy_dict = json.load(open(p1_file, 'r'))
        p1_policy_obj = Policy(policy_dict=p1_policy_dict, player='x')

        p2_policy_dict = json.load(open(p2_file, 'r'))
        p2_policy_obj = Policy(policy_dict=p2_policy_dict, player='o')

        logging.info("Getting expected utility...")
        expected_utility, games = get_expected_utility_parallel(I_1, I_2, true_board, player, p1_policy_obj,
                                                                p2_policy_obj, 1, NonTerminalHistory(), player)
        logging.info("Expected Utility: {}".format(expected_utility))

        if z % 2 == 0:
            p1_file = 'data_files/P1_cfr_policy_round_{}.json'.format(cfr_round + 1)
            z = z + 1
        else:
            cfr_round = cfr_round + 1
            p2_file = 'data_files/P2_cfr_policy_round_{}.json'.format(cfr_round + 1)
            z = z + 1
