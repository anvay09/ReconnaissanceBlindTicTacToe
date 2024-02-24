from rbt_classes import InformationSet, Policy
from rbt_utilties import calc_cfr_policy_given_I
import logging
import json

if __name__ == '__main__':
    logging.info('Initializing policy objects...')

    with open('./data_files/{}.json'.format('P1_DG_policy'), 'r') as f:
        policy_dict = json.load(f)
    policy_obj_x = Policy(player='x', policy_dict=policy_dict)

    with open('./data_files/{}.json'.format('P2_uniform_policy'), 'r') as f:
        policy_dict = json.load(f)
    policy_obj_o = Policy(player='o', policy_dict=policy_dict)

    logging.info('Loaded policy objects...')

    P2_reachable_information_sets_file = 'data_files/reachable_P2_information_sets.txt'
    P2_reachable_information_sets = set()
    with open(P2_reachable_information_sets_file, 'r') as f:
        lines = f.readlines()
        for line in lines:
            P2_reachable_information_sets.add(line.strip())

    prev_regret_list_o = {I_hash:[0 for _ in range(13)] for I_hash in P2_reachable_information_sets}
    processed_I_count = 0
    next_itr_policy_obj_x = policy_obj_x.copy()
    next_itr_policy_obj_o = policy_obj_o.copy()
            
    for T in range(1,5):
        for I_hash in P2_reachable_information_sets:
            I = InformationSet(player='o', move_flag=I_hash[-1]=='m', board=[*I_hash[:-1]])

            next_itr_policy_obj_x, next_itr_policy_obj_o, prev_regret_list_o[I_hash] = calc_cfr_policy_given_I(I, policy_obj_x, 
                                                                                                               policy_obj_o, T, 
                                                                                                               prev_regret_list_o[I_hash], 
                                                                                                               next_itr_policy_obj_x, next_itr_policy_obj_o)
            
            logging.info('Updated policy for player {}, information set {}:'.format(I.player, I_hash))
            logging.info('{}'.format(next_itr_policy_obj_o.policy_dict[I_hash]))
            processed_I_count += 1
            logging.info('Processed {} information sets in iteration {}...'.format(processed_I_count, T))
        
        processed_I_count = 0
        logging.info('Completed iteration {}...'.format(T))
        policy_obj_x = next_itr_policy_obj_x.copy()
        policy_obj_o = next_itr_policy_obj_o.copy()

        logging.info('Saving policy objects...')
        with open('./data_files/P2_iteration_{}_cfr_policy.json'.format(T), 'w') as f:
            json.dump(policy_obj_o.policy_dict, f)