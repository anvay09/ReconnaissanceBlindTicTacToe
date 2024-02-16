from rbt_classes import InformationSet, Policy
from rbt_utilties import calc_cfr_policy_given_I
import logging

if __name__ == '__main__':
    I = InformationSet(player='o', board=['o', '-', '-', '-', 'x', 'o', 'o', '-', 'x'])
    player = 'o'
    T = 0
    prev_regret_list = [0 for i in range(13)]

    logging.info('Initializing policy objects...')

    policy_obj_x = Policy(player='x', policy_file='./../data_files/{}.yml'.format('P1_information_sets_unifrom'))
    policy_obj_o = Policy(player='o', policy_file='./../data_files/{}.yml'.format('P2_information_sets_unifrom'))

    logging.info('Loaded policy for player {}, information set {}:'.format(player, I.get_hash()))
    if player == 'x':
        print(policy_obj_x.policy_dict[I.get_hash()])
    else:
        print(policy_obj_o.policy_dict[I.get_hash()])

    calc_cfr_policy_given_I(I, policy_obj_x, policy_obj_o, T, prev_regret_list)

    logging.info('Updated policy for player {}, information set {}:'.format(player, I.get_hash()))
    if player == 'x':
        print(policy_obj_x.policy_dict[I.get_hash()])
    else:
        print(policy_obj_o.policy_dict[I.get_hash()])