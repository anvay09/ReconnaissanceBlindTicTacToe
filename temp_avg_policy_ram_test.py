from rbt_classes import NonTerminalHistory, InformationSet, TicTacToeBoard, Policy
from tqdm import tqdm
import argparse
import json
import logging
from multiprocessing import Pool
from config import num_workers


def calc_average_policy(policy_obj_list, I_hash, initial_player):
    average_policy = {}
    logging.info('Computing average policy for {}...'.format(I_hash))
    average_policy[I_hash] = {}
    I = InformationSet(player=initial_player, move_flag=I_hash[-1] == 'm', board=[*I_hash[:-1]])
    actions = I.get_actions()

    for action in actions:
        numerator = 0
        denominator = 0

        for p in range(len(policy_obj_list)):
            numerator += policy_obj_list[p].policy_dict[I_hash][action]
            for act in actions:
                denominator += policy_obj_list[p].policy_dict[I_hash][act]

        if denominator == 0:
            average_policy[I_hash][action] = 0
        else:
            average_policy[I_hash][action] = numerator / denominator

    logging.info('Average policy for {} computed to be {}...'.format(I_hash, average_policy[I_hash]))
    return average_policy


def get_average_policy(policy_obj_list, I_list, initial_player):
    """
    :param policy_obj_list:
    :param I_list:
    :return:
    """

    average_policy = {}

    with Pool(num_workers) as pool:
        dict_list = pool.starmap(calc_average_policy, [(policy_obj_list, I_hash, initial_player) for I_hash in I_list])

    for item in dict_list:
        average_policy.update(item)

    return average_policy


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--CurrentPlayer', type=str, required=True)
    parser.add_argument('--PolicyFileBase', type=str, required=True)
    parser.add_argument('--NumRounds', type=str, required=True)
    parser.add_argument('--ISetFile', type=str, required=True)
    arguments = parser.parse_args()

    policy_obj_list = []

    for i in range(1, int(arguments.NumRounds) + 1):
        with open(arguments.PolicyFileBase + str(i) + '.json', 'r') as f:
            policy_obj_list.append(Policy(policy_dict=json.load(f), player=arguments.CurrentPlayer))

    with open(arguments.ISetFile, 'r') as f:
        I_list = f.read().splitlines()

    average_policy = get_average_policy(policy_obj_list, I_list, arguments.CurrentPlayer)

    if arguments.CurrentPlayer == 'x':
        outfile_name = './data_files_avg/P1_average_overall_policy_after_{}_rounds.json'.format(arguments.NumRounds)
    else:
        outfile_name = './data_files_avg/P2_average_overall_policy_after_{}_rounds.json'.format(arguments.NumRounds)

    with open(outfile_name, 'w') as f:
        json.dump(average_policy, f)
