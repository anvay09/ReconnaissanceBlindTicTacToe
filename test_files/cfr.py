from config import num_workers
from rbt_utilties import calc_cfr_policy_given_I
from rbt_classes import InformationSet, Policy
from multiprocessing import Pool
from tqdm import tqdm
import logging
import json
import gc

if __name__ == '__main__':
    logging.info('Initializing policy objects...')

    with open('./data_files/{}.json'.format('P1_DG_policy'), 'r') as f:
        policy_dict = json.load(f)
    policy_obj_x = Policy(player='x', policy_dict=policy_dict)

    with open('./data_files/{}.json'.format('P2_iteration_9_cfr_policy'), 'r') as f:
        policy_dict = json.load(f)
    policy_obj_o = Policy(player='o', policy_dict=policy_dict)

    logging.info('Loaded policy objects...')

    P1_information_sets_file = 'data_files/P1_information_sets.txt'
    P1_information_sets = []
    with open(P1_information_sets_file, 'r') as f:
        lines = f.readlines()
        for line in lines:
            P1_information_sets.append(line.strip())

    P2_information_sets_file = 'data_files/P2_information_sets.txt'
    P2_information_sets = []
    with open(P2_information_sets_file, 'r') as f:
        lines = f.readlines()
        for line in lines:
            P2_information_sets.append(line.strip())

    prev_regret_list_x = {I_hash:[0 for _ in range(13)] for I_hash in P1_information_sets}
    prev_regret_list_o = {I_hash:[0 for _ in range(13)] for I_hash in P2_information_sets}
    
    for T in range(1,1001):
        P1_args = []
        P2_args = []
        
        logging.info('Generating arguments...')
        for I_hash in tqdm(P1_information_sets):
            I = InformationSet(player='x', move_flag=I_hash[-1]=='m', board=[*I_hash[:-1]])
            
            P1_args.append((I, policy_obj_x, policy_obj_o, T, prev_regret_list_x[I_hash], None))

        logging.info('Starting iteration {} for player 1...'.format(T))
        with Pool(num_workers) as p:
            regrets = p.starmap(calc_cfr_policy_given_I, P1_args)

        del P1_args
        gc.collect()

        for I_hash in tqdm(P2_information_sets):
            I = InformationSet(player='o', move_flag=I_hash[-1]=='m', board=[*I_hash[:-1]])
            
            P2_args.append((I, policy_obj_x, policy_obj_o, T, prev_regret_list_o[I_hash], None))

        logging.info('Starting iteration {} for player 2...'.format(T))
        with Pool(num_workers) as p:
            regrets = p.starmap(calc_cfr_policy_given_I, P2_args)

        del P2_args
        gc.collect()
            
        logging.info('Updating policy objects...')
        for arg in P1_args:
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

        for arg in P2_args:
            actions = arg[0].get_actions()
            I_hash = arg[0].get_hash()
            prev_regret_list_o[I_hash] = regrets.pop(0)
            total_regret = sum(prev_regret_list_o[I_hash])
                        
            if total_regret > 0:
                for action in actions:
                    policy_obj_o.policy_dict[I_hash][action] = prev_regret_list_o[I_hash][action] / total_regret
            else:
                for action in actions:
                    policy_obj_o.policy_dict[I_hash][action] = 1 / len(actions)

        logging.info('Completed iteration {}...'.format(T))
        logging.info('Saving policy objects...')
        with open('./data_files_new/P1_iteration_{}_cfr_policy.json'.format(T), 'w') as f:
            json.dump(policy_obj_x.policy_dict, f)
        with open('./data_files_new/P2_iteration_{}_cfr_policy.json'.format(T), 'w') as f:
            json.dump(policy_obj_o.policy_dict, f)
