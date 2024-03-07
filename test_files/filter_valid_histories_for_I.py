from rbt_classes import InformationSet, Policy
from rbt_utilties import get_histories_given_I
import json
import logging
from multiprocessing import Pool
from config import num_workers

if __name__ == '__main__':
    with open('./data_files/{}.json'.format('P2_iteration_9_cfr_policy'), 'r') as f:
        policy_dict = json.load(f)
    policy_obj_o = Policy(player='o', policy_dict=policy_dict)

    P1_reachable_information_sets_file = 'data_files/reachable_P1_information_sets.txt'
    P1_reachable_information_sets = set()
    with open(P1_reachable_information_sets_file, 'r') as f:
        lines = f.readlines()
        for line in lines:
            P1_reachable_information_sets.add(line.strip())

    histories = {}
    args = []

    for I_hash in P1_reachable_information_sets:
        I = InformationSet(player='x', move_flag=I_hash[-1]=='m', board=[*I_hash[:-1]])
        args.append((I, policy_obj_o, None))
        
    # args_chunk_1 = args[:len(args)//2]
    # args_chunk_2 = args[len(args)//2:]

    logging.info('Filtering valid histories for P1 information sets...')
    with Pool(num_workers) as p:
        H = p.starmap(get_histories_given_I, args)
    
    logging.info('Saving valid histories for P1 information sets...')
    
    for idx in range(len(args)):
        I_hash = args[idx][0].get_hash()
        histories[I_hash] = H[idx]

    # logging.info('Filtering valid histories for P2 information sets, chunk 2...')
    # with Pool(num_workers) as p:
    #     H = p.starmap(get_histories_given_I, args_chunk_2)
    
    # logging.info('Saving valid histories for P2 information sets, chunk 2...')

    # for idx in range(len(args_chunk_2)):
    #     I_hash = args_chunk_2[idx][0].get_hash()
    #     histories[I_hash] = H[idx]
        
    with open('data_files/p1_valid_histories_for_reachable_I.json', 'w') as f:
        json.dump(histories, f)
        