from rbt_classes import InformationSet, Policy
from rbt_utilties import calc_cfr_policy_given_I
import logging
import json

if __name__ == '__main__':
    # I = InformationSet(player='x', move_flag = True, board=['0', '0', '0', '0', '0', '0', '0', '0', '0'])
    # I = InformationSet(player='x', move_flag=True, board=['-', '0', '0', 'x', 'x', 'o', 'o', '-', 'x'])
    # I = InformationSet(player='o', move_flag = False, board=['o', '-', '-', '-', 'x', 'o', 'o', '-', 'x'])

    # TODO: Fix Bug for root node, and for player 'o' first move
    # I = InformationSet(player='x', move_flag=True, board=['o', '0', '-', '0', 'x', '-', '-', '-', '-'])
    I = InformationSet(player='x', move_flag=True, board=['-', '-', '-', 'o', '0', '-', 'x', 'o', 'x'])

    T = 1
    prev_regret_list_x = [0 for _ in range(13)]
    prev_regret_list_o = [0 for _ in range(13)]

    logging.info('Initializing policy objects...')

    with open('./data_files/{}.json'.format('P1_uniform_policy'), 'r') as f:
        policy_dict = json.load(f)
    policy_obj_x = Policy(player='x', policy_dict=policy_dict)

    with open('./data_files/{}.json'.format('P2_uniform_policy'), 'r') as f:
        policy_dict = json.load(f)
    policy_obj_o = Policy(player='o', policy_dict=policy_dict)

    logging.info('Loaded policy for player {}, information set {}:'.format(I.player, I.get_hash()))

    if I.player == 'x':
        print(policy_obj_x.policy_dict[I.get_hash()])
    else:
        print(policy_obj_o.policy_dict[I.get_hash()])

    policy_obj_x, policy_obj_o, prev_regret_list_x = calc_cfr_policy_given_I(I, policy_obj_x, policy_obj_o, 1,
                                                                             prev_regret_list_x)
    print(policy_obj_x)
    I = InformationSet(player='o', move_flag=True, board=['x', '0', '-', 'o', '0', '-', 'x', 'o', 'x'])
    policy_obj_x, policy_obj_o, prev_regret_list_o = calc_cfr_policy_given_I(I, policy_obj_x, policy_obj_o, 1,
                                                                             prev_regret_list_o)
    print(policy_obj_o)

    I = InformationSet(player='x', move_flag=True, board=['-', '-', '-', 'o', '0', '-', 'x', 'o', 'x'])
    policy_obj_x, policy_obj_o, prev_regret_list_x = calc_cfr_policy_given_I(I, policy_obj_x, policy_obj_o, 2,
                                                                             prev_regret_list_x)
    print(policy_obj_x)
