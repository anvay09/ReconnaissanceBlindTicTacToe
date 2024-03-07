from rbt_classes import InformationSet, Policy
from rbt_utilties import get_histories_given_I
import json
import logging
from tqdm import tqdm
from multiprocessing import Pool
from config import num_workers


if __name__ == '__main__':
    with open('./data_files/{}.json'.format('P1_DG_policy'), 'r') as f:
        policy_dict = json.load(f)
    policy_obj_x = Policy(player='x', policy_dict=policy_dict)

    P2_reachable_information_sets_file = 'data_files/reachable_P2_information_sets.txt'
    P2_reachable_information_sets = set()
    with open(P2_reachable_information_sets_file, 'r') as f:
        lines = f.readlines()
        for line in lines:
            P2_reachable_information_sets.add(line.strip())

    histories = {}
    args = []

    for I_hash in tqdm(P2_reachable_information_sets):
        I = InformationSet(player='o', move_flag=I_hash[-1]=='m', board=[*I_hash[:-1]])
        args.append((I, policy_obj_x, None))
        
    logging.info('Filtering valid histories for P2 information sets...')
    with Pool(num_workers) as p:
        H = p.starmap(get_histories_given_I, args)
    
    logging.info('Saving valid histories for P2 information sets...')
    
    for idx in range(len(args)):
        I_hash = args[idx][0].get_hash()
        histories[I_hash] = H[idx]
        
    with open('data_files/p2_valid_histories_for_reachable_I.json', 'w') as f:
        json.dump(histories, f)
        