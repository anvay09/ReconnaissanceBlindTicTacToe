from rbt_classes import InformationSet, Policy
from rbt_utilties import calc_cfr_policy_given_I
import logging

if __name__ == '__main__':
    # I = InformationSet(player='x', move_flag = False, board=['o', '-', '-', '-', 'x', '-', '-', '-', 'x'])
    # I = InformationSet(player='x', move_flag=True, board=['-', '0', '0', 'x', 'x', 'o', 'o', '-', 'x'])
    # I = InformationSet(player='o', move_flag = False, board=['o', '-', '-', '-', 'x', 'o', 'o', '-', 'x'])

    # TODO: Fix Bug for root node
    I = InformationSet(player='o', move_flag=True, board=['0', '0', '-', '0', 'x', '-', '-', '-', '-'])

    T = 0
    prev_regret_list = [0 for _ in range(13)]

    logging.info('Initializing policy objects...')

    policy_obj_x = Policy(player='x', policy_file='./data_files/{}.yml'.format('P1_uniform_policy'))
    policy_obj_o = Policy(player='o', policy_file='./data_files/{}.yml'.format('P2_uniform_policy'))

    logging.info('Loaded policy for player {}, information set {}:'.format(I.player, I.get_hash()))

    if I.player == 'x':
        print(policy_obj_x.policy_dict[I.get_hash()])
    else:
        print(policy_obj_o.policy_dict[I.get_hash()])

    policy_obj_x, policy_obj_o, prev_regret_list = calc_cfr_policy_given_I(I, policy_obj_x, policy_obj_o, T, prev_regret_list)
    T = 1
    policy_obj_x, policy_obj_o, prev_regret_list = calc_cfr_policy_given_I(I, policy_obj_x, policy_obj_o, T, prev_regret_list)
    T = 2
    policy_obj_x, policy_obj_o, prev_regret_list = calc_cfr_policy_given_I(I, policy_obj_x, policy_obj_o, T, prev_regret_list)