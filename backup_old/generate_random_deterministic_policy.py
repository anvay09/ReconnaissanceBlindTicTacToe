from subgame_files.subgame_rbt_classes import InformationSet, Policy
from subgame_files.subgame_rbt_utilities import calc_cfr_policy_given_I, get_root_sets
import logging
import json
import random


def generate_json_from_txt(file_name):
    with open('./data_files/{}.txt'.format(file_name), 'r') as f:
        arr = f.read().splitlines()

    dict_is = dict()
    for item in arr:
        dict_is["{}".format(item)] = 0

    with open('./data_files/{}.json'.format(file_name), 'w') as outfile:
        json.dump(dict_is, outfile, indent=4)


def write_uniform_policy_json_from_information_set_json(information_set_file, player, output_file):
    with open(information_set_file, 'r') as file:
        policy_dict = json.load(file)

        for key, _ in policy_dict.items():
            board = [*key]
            move_flag = board[-1] == 'm'
            board = board[:-1]

            information_set_obj = InformationSet(player=player, move_flag=move_flag, board=board)
            valid_actions = information_set_obj.get_actions()

            if not move_flag:
                random_sense = random.sample(valid_actions, 1)[0]
                policy_dict[key] = {}
                for i in range(9, 13):
                    if i == random_sense:
                        policy_dict[key][i] = 1
                    else:
                        policy_dict[key][i] = 0

            else:
                policy_dict[key] = {}
                random_move = random.sample(valid_actions, 1)[0]
                for i in range(0, 9):
                    if i == random_move:
                        policy_dict[key][i] = 1
                    else:
                        policy_dict[key][i] = 0

        with open(output_file, 'w') as outfile:
            json.dump(policy_dict, outfile, indent=4)


if __name__ == '__main__':
    generate_json_from_txt('P1_information_sets')
    generate_json_from_txt('P2_information_sets')
    write_uniform_policy_json_from_information_set_json('./data_files/P1_information_sets.json', 'x',
                                                        './data_files/P1_deterministic_policy.json')
    write_uniform_policy_json_from_information_set_json('./data_files/P2_information_sets.json', 'o',
                                                        './data_files/P2_deterministic_policy.json')
