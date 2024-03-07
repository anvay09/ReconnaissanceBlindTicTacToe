from rbt_classes import InformationSet, Policy
from rbt_utilties import calc_cfr_policy_given_I
import logging
import json
from multiprocessing import Pool
from config import num_workers

if __name__ == '__main__':
    logging.info('Initializing policy objects...')

    with open('./data_files/{}.json'.format('P1_DG_policy'), 'r') as f:
        policy_dict = json.load(f)
    policy_obj_x = Policy(player='x', policy_dict=policy_dict)

    with open('./data_files/{}.json'.format('P2_iteration_9_cfr_policy'), 'r') as f:
        policy_dict = json.load(f)
    policy_obj_o = Policy(player='o', policy_dict=policy_dict)

    logging.info('Loaded policy objects...')

    P1_reachable_information_sets_file = 'data_files/reachable_P1_information_sets.txt'
    P1_reachable_information_sets = set()
    with open(P1_reachable_information_sets_file, 'r') as f:
        lines = f.readlines()
        for line in lines:
            P1_reachable_information_sets.add(line.strip())

    prev_regret_list_x = {I_hash:[0 for _ in range(13)] for I_hash in P1_reachable_information_sets}
 
    # with open('data_files/p2_valid_histories_for_reachable_I.json', 'r') as f:
    #     p2_valid_histories_for_I = json.load(f)

    for T in range(1,10):
        args = []
        for I_hash in P1_reachable_information_sets:
            I = InformationSet(player='x', move_flag=I_hash[-1]=='m', board=[*I_hash[:-1]])
            args.append((I, policy_obj_x, policy_obj_o, T, prev_regret_list_x[I_hash], None))
        logging.info('Starting iteration {}...'.format(T))
        with Pool(num_workers) as p:
            regrets = p.starmap(calc_cfr_policy_given_I, args)
            
        logging.info('Updating policy objects...')
        for arg in args:
            actions = arg[0].get_actions()
            I_hash = arg[0].get_hash()
            prev_regret_list_x[I_hash] = regrets.pop(0)
            total_regret = sum(prev_regret_list_x[I_hash])
                        
            if total_regret > 0:
                for action in actions:
                    policy_obj_x.policy_dict[I_hash][action] = prev_regret_list_x[I_hash][action] / total_regret
            else:
                for action in actions:
                    policy_obj_x.policy_dict[I_hash][action] = 1 / len(actions)

        logging.info('Completed iteration {}...'.format(T))
        logging.info('Saving policy objects...')
        with open('./data_files/P1_iteration_{}_cfr_policy.json'.format(T), 'w') as f:
            json.dump(policy_obj_x.policy_dict, f)
