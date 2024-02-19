from subgame_rbt_classes import InformationSet, Policy
from subgame_rbt_utilities import calc_cfr_policy_given_I, get_root_sets
import logging
import json

if __name__ == '__main__':
    # root_I_1, root_I_2, _, _ = get_root_sets()
    prev_regret_list_x = [0 for _ in range(13)]
    prev_regret_list_o = [0 for _ in range(13)]
    with open('./data_files/{}.json'.format('subgame_P1_uniform_policy'), 'r') as f:
        policy_dict = json.load(f)
    policy_obj_x = Policy(player='x', policy_dict=policy_dict)
    with open('./data_files/{}.json'.format('subgame_P2_uniform_policy'), 'r') as f:
        policy_dict = json.load(f)
    policy_obj_o = Policy(player='o', policy_dict=policy_dict)
    new_policy_obj_x = None
    new_policy_obj_o = None
    for T in range(3):
        for I_hash in policy_obj_x.policy_dict.keys():
            I = InformationSet('x', I_hash[-1] == 'm', board=[*I_hash[:-1]])
            new_policy_obj_x, _, prev_regret_list_x = calc_cfr_policy_given_I(I, policy_obj_x, policy_obj_o, T,
                                                                              prev_regret_list_x)
        logging.info("Time step {} regret x list {}".format(T, prev_regret_list_x))
        for I_hash in policy_obj_o.policy_dict.keys():
            I = InformationSet('o', I_hash[-1] == 'm', board=[*I_hash[:-1]])
            _, new_policy_obj_o, prev_regret_list_o = calc_cfr_policy_given_I(I, policy_obj_x, policy_obj_o, T,
                                                                              prev_regret_list_o)
        logging.info("Time step {} regret o list {}".format(T, prev_regret_list_o))
        policy_obj_x = new_policy_obj_x
        policy_obj_o = new_policy_obj_o

    with open('./data_files/{}.json'.format("subgame_policy_x"), 'w') as outfile:
        json.dump(policy_obj_x, outfile, indent=4)
    with open('./data_files/{}.json'.format("subgame_policy_o"), 'w') as outfile:
        json.dump(policy_obj_o, outfile, indent=4)
