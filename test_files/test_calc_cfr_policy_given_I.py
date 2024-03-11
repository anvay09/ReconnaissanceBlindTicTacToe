from config import num_workers, action_bit_encoding
from rbt_utilties import calc_cfr_policy_given_I
from rbt_classes import InformationSet, Policy
from multiprocessing import Pool
from bitarray import bitarray
from tqdm import tqdm
import logging
import json

if __name__ == '__main__':
    logging.info('Initializing policy objects...')

    with open('./data_files/{}.json'.format('P1_iteration_9_cfr_policy'), 'r') as f:
        policy_dict = json.load(f)
    policy_obj_x = Policy(player='x', policy_dict=policy_dict)

    with open('./data_files/{}.json'.format('P2_iteration_9_cfr_policy'), 'r') as f:
        policy_dict = json.load(f)
    policy_obj_o = Policy(player='o', policy_dict=policy_dict)

    logging.info('Loaded policy objects...')

    P2_reachable_information_sets_file = 'data_files/reachable_P2_information_sets.txt'
    P2_reachable_information_sets = []
    with open(P2_reachable_information_sets_file, 'r') as f:
        lines = f.readlines()
        for line in lines:
            P2_reachable_information_sets.append(line.strip())
     
    prev_regret_list_o = {I_hash:[0 for _ in range(13)] for I_hash in P2_reachable_information_sets}
    
    logging.info('Loading valid histories...')
    with open('data_files/p2_valid_histories_for_reachable_I.json', 'r') as f:
        p2_valid_histories_for_I = json.load(f)     

    print(len(p2_valid_histories_for_I.keys()))
    for T in range(1,32):
        args = []
        
        # logging.info('Converting histories to bit arrays and generating arguments...')
        logging.info('Generating arguments...')
        for I_hash in tqdm(P2_reachable_information_sets):
            I = InformationSet(player='o', move_flag=I_hash[-1]=='m', board=[*I_hash[:-1]])
            # starting_histories = copy.deepcopy(p1_valid_histories_for_I[I_hash])
            starting_histories = p2_valid_histories_for_I[I_hash]
            
            # for i in range(len(starting_histories)):
            #     b = bitarray()
            #     b.encode(action_bit_encoding, starting_histories[i])
            #     starting_histories[i] = b

            args.append((I, policy_obj_x, policy_obj_o, T, prev_regret_list_o[I_hash], starting_histories))

        # del p1_valid_histories_for_I
        # gc.collect()
        logging.info('Starting iteration {}...'.format(T+9))
        with Pool(num_workers) as p:
            regrets = p.starmap(calc_cfr_policy_given_I, args)
            
        logging.info('Updating policy objects...')
        for arg in args:
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

        logging.info('Completed iteration {}...'.format(T+9))
        logging.info('Saving policy objects...')
        with open('./data_files/P2_iteration_{}_cfr_policy.json'.format(T+9), 'w') as f:
            json.dump(policy_obj_o.policy_dict, f)
