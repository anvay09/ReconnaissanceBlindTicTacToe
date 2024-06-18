from rbt_classes import TicTacToeBoard, InformationSet, Policy
from multiprocessing import Pool
import json
import logging
from config import num_workers
from rbt_utilties import get_histories_given_I, upgraded_get_histories_given_I
from rbt_utilties import calc_cfr_policy_given_I
import argparse
from tqdm import tqdm
from compute_average_policy import get_probability_of_reaching_I
import copy


logging.basicConfig(format='%(levelname)s - %(asctime)s - %(message)s', datefmt='%d-%b-%y %H:%M:%S',
                    level=logging.INFO)


def parse_commandline_args():
    """Parse command line arguments"""
    parser = argparse.ArgumentParser()
    parser.add_argument('--CurrentPlayer', type=str, required=True)
    parser.add_argument('--PolicyFileX', type=str, required=True)
    parser.add_argument('--PolicyFileO', type=str, required=True)
    parser.add_argument('--Round', type=str, required=True)
    arguments = parser.parse_args()
    return arguments.CurrentPlayer, arguments.PolicyFileX, arguments.PolicyFileO, int(arguments.Round)


if __name__ == "__main__":
    # %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% create reachable IS file %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    true_board = TicTacToeBoard(board=['0', '0', '0', '0', '0', '0', '0', '0', '0'])
    I_1 = InformationSet(player='x', move_flag=True, board=['0', '0', '0', '0', '0', '0', '0', '0', '0'])
    I_2 = InformationSet(player='o', move_flag=False, board=['-', '-', '-', '-', '-', '-', '-', '-', '-'])
    player = 'x'
    cfr_player, policy_file_x, policy_file_o, cfr_round = parse_commandline_args()

    if cfr_player == 'o':
        reachable_IS_file_player = 'data_files/reachable_P2_information_sets_round_{}.txt'.format(cfr_round)
    else:
        reachable_IS_file_player = 'data_files/reachable_P1_information_sets_round_{}.txt'.format(cfr_round)

    p1_policy_dict = json.load(open(policy_file_x, 'r'))
    policy_obj_x = Policy(policy_dict=p1_policy_dict, player='x')
    p2_policy_dict = json.load(open(policy_file_o, 'r'))
    policy_obj_o = Policy(policy_dict=p2_policy_dict, player='o')

    # %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% cfr run %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    player_reachable_information_sets = []
    with open(reachable_IS_file_player, 'r') as f:
        lines = f.readlines()
        for line in lines:
            player_reachable_information_sets.append(line.strip())
    args = []

    logging.info('Generating arguments...')
    for I_hash in player_reachable_information_sets:
        I = InformationSet(player=cfr_player, move_flag=I_hash[-1] == 'm', board=[*I_hash[:-1]])
        # starting_histories = histories[I_hash]
        starting_histories = None
        if cfr_player == 'x':
            args.append((I, policy_obj_x, None, cfr_player))
        else:
            args.append((I, None, policy_obj_o, cfr_player))

    logging.info('get_probability_of_reaching_I parallel run {}...'.format(cfr_round))
    with Pool(num_workers) as p:
        probs = p.starmap(get_probability_of_reaching_I, tqdm(args, total=len(args)))

    avg_policy_x = copy.deepcopy(policy_obj_x.policy_dict)
    avg_policy_o = copy.deepcopy(policy_obj_o.policy_dict)
    for key, val in avg_policy_x.items():
        for k, v in val.items():
            avg_policy_x[key][k] = 0

    for key, val in avg_policy_o.items():
        for k, v in val.items():
            avg_policy_o[key][k] = 0

    if cfr_player == 'x':
        prev_regret_list_player = json.load(
            open('./data_files/P1_prev_regret_list_round_{}.json'.format(cfr_round), 'r'))
    else:
        prev_regret_list_player = json.load(
            open('./data_files/P2_prev_regret_list_round_{}.json'.format(cfr_round), 'r'))

    logging.info('Computing avg policy...')
    for arg in args:
        actions = arg[0].get_actions()
        I_hash = arg[0].get_hash()
        total_regret = sum(prev_regret_list_player[I_hash])

        prob_I = probs.pop(0)

        if total_regret > 0:
            for action in actions:
                if cfr_player == 'x':
                    avg_policy_x[I_hash][action] = prob_I * prev_regret_list_player[I_hash][
                                                                   action] / total_regret
                else:
                    avg_policy_o[I_hash][action] = prob_I * prev_regret_list_player[I_hash][
                                                                   action] / total_regret
        else:
            for action in actions:
                if cfr_player == 'x':
                    avg_policy_x[I_hash][action] = prob_I * 1 / len(actions)
                else:
                    avg_policy_o[I_hash][action] = prob_I * 1 / len(actions)

    logging.info('Completed computing avg policy {}...'.format(cfr_round))
    logging.info('Saving policy objects...')

    if cfr_player == 'x':
        with open('./data_files_avg/P1_avg_policy_after_round_{}.json'.format(cfr_round), 'w') as f:
            json.dump(avg_policy_x, f)
    else:
        with open('./data_files_avg/P2_avg_policy_after_round_{}.json'.format(cfr_round), 'w') as f:
            json.dump(avg_policy_o, f)
