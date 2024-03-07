from rbt_classes import InformationSet, Policy
from rbt_utilties import calc_cfr_policy_given_I
import logging
import json

if __name__ == '__main__':
    logging.info('Initializing policy objects...')

    with open('./data_files/{}.json'.format('P1_DG_policy'), 'r') as f:
        policy_dict = json.load(f)
    policy_obj_x = Policy(player='x', policy_dict=policy_dict)

    with open('./data_files/{}.json'.format('P2_iteration_10_cfr_policy'), 'r') as f:
        policy_dict = json.load(f)
    policy_obj_o = Policy(player='o', policy_dict=policy_dict)

    logging.info('Loaded policy objects...')

    P2_reachable_information_sets_file = 'data_files/reachable_P2_information_sets.txt'
    P2_reachable_information_sets = set()
    with open(P2_reachable_information_sets_file, 'r') as f:
        lines = f.readlines()
        for line in lines:
            P2_reachable_information_sets.add(line.strip())

    P1_reachable_information_sets_file = 'data_files/reachable_P1_information_sets.txt'
    P1_reachable_information_sets = set()
    with open(P1_reachable_information_sets_file, 'r') as f:
        lines = f.readlines()
        for line in lines:
            P1_reachable_information_sets.add(line.strip())

    prev_regret_list_x = {I_hash:[0 for _ in range(13)] for I_hash in P1_reachable_information_sets}
    prev_regret_list_o = {I_hash:[0 for _ in range(13)] for I_hash in P2_reachable_information_sets}
    
    processed_I_count_x = 0
    processed_I_count_o = 0
    next_itr_policy_obj_x = policy_obj_x.copy()
    next_itr_policy_obj_o = policy_obj_o.copy()

    with open('data_files/p1_valid_histories_for_reachable_I.json', 'r') as f:
        p1_valid_histories_for_I = json.load(f)

    with open('data_files/p2_valid_histories_for_reachable_I.json', 'r') as f:
        p2_valid_histories_for_I = json.load(f)

    for T in range(1,25):
        for I_hash in P1_reachable_information_sets:
            I = InformationSet(player='x', move_flag=I_hash[-1]=='m', board=[*I_hash[:-1]])

            next_itr_policy_obj_x, next_itr_policy_obj_o, prev_regret_list_o[I_hash] = calc_cfr_policy_given_I(I, policy_obj_x, 
                                                                                                               policy_obj_o, T, 
                                                                                                               prev_regret_list_o[I_hash], 
                                                                                                               next_itr_policy_obj_x, 
                                                                                                               next_itr_policy_obj_o, 
                                                                                                               p1_valid_histories_for_I[I_hash])
            
            logging.info('Updated policy for player {}, information set {}:'.format(I.player, I_hash))
            logging.info('{}'.format(next_itr_policy_obj_x.policy_dict[I_hash]))
            processed_I_count_x += 1
            logging.info('Processed {} information sets in iteration {}...'.format(processed_I_count_x, T))

        for I_hash in P2_reachable_information_sets:
            I = InformationSet(player='o', move_flag=I_hash[-1]=='m', board=[*I_hash[:-1]])

            next_itr_policy_obj_x, next_itr_policy_obj_o, prev_regret_list_o[I_hash] = calc_cfr_policy_given_I(I, policy_obj_x, 
                                                                                                               policy_obj_o, T, 
                                                                                                               prev_regret_list_o[I_hash], 
                                                                                                               next_itr_policy_obj_x, 
                                                                                                               next_itr_policy_obj_o, 
                                                                                                               p2_valid_histories_for_I[I_hash])
            
            logging.info('Updated policy for player {}, information set {}:'.format(I.player, I_hash))
            logging.info('{}'.format(next_itr_policy_obj_o.policy_dict[I_hash]))
            processed_I_count_o += 1
            logging.info('Processed {} information sets in iteration {}...'.format(processed_I_count_o, T))
        
        processed_I_count_x = 0
        processed_I_count_o = 0
        logging.info('Completed iteration {}...'.format(T))
        policy_obj_x = next_itr_policy_obj_x.copy()
        policy_obj_o = next_itr_policy_obj_o.copy()

        logging.info('Saving policy objects...')
        with open('./data_files/P2_iteration_{}_cfr_policy.json'.format(T+10), 'w') as f:
            json.dump(policy_obj_o.policy_dict, f)

        with open('./data_files/P1_iteration_{}_cfr_policy.json'.format(T), 'w') as f:
            json.dump(policy_obj_x.policy_dict, f)
