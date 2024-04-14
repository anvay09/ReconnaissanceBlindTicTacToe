from multiprocessing import Pool
from config import num_workers
from tqdm import tqdm
import logging
import os

logging.basicConfig(format='%(levelname)s - %(asctime)s - %(message)s', datefmt='%d-%b-%y %H:%M:%S',
                    level=logging.INFO)

def calc_cfr_policy_given_I_cpp(policy_file_x, policy_file_o, I_hash, player, T, next_policy_file):
    os.system('./cfr ' + policy_file_x + ' ' + policy_file_o + ' ' + I_hash + ' ' + player + ' ' + str(T) + ' ' + next_policy_file)


if __name__ == '__main__':
    logging.info('Initializing policy objects...')
    policy_file_x = 'data/P1_uniform_policy.json'
    policy_file_o = 'data/P2_uniform_policy.json'

    P1_information_sets_file = 'data/P1_information_sets.txt'
    P2_information_sets_file = 'data/P2_information_sets.txt'
    P1_information_sets = []
    P2_information_sets = []
   
    with open(P1_information_sets_file, 'r') as f:
        lines = f.readlines()
        for line in lines:
            P1_information_sets.append(line.strip())

    with open(P2_information_sets_file, 'r') as f:
        lines = f.readlines()
        for line in lines:
            P2_information_sets.append(line.strip())

    for T in range(1,1001):
        P1_args = []
        P2_args = []
        next_policy_file_x = 'data/P1_iteration_{}_cfr_policy_cpp.json'.format(T)
        next_policy_file_o = 'data/P2_iteration_{}_cfr_policy_cpp.json'.format(T)
        
        for I_hash in P1_information_sets:
            P1_args.append((policy_file_x, policy_file_o, I_hash, 'x', T, next_policy_file_x))

        for I_hash in P2_information_sets:
            P2_args.append((policy_file_x, policy_file_o, I_hash, 'o', T, next_policy_file_o))

        logging.info('Starting iteration {} for player 1...'.format(T))
        with Pool(num_workers) as p:
            p.starmap(calc_cfr_policy_given_I_cpp, tqdm(P1_args, total=len(P1_args)))

        logging.info('Starting iteration {} for player 2...'.format(T))
        with Pool(num_workers) as p:
            p.starmap(calc_cfr_policy_given_I_cpp, tqdm(P2_args, total=len(P2_args)))
 
        policy_file_x = next_policy_file_x
        policy_file_o = next_policy_file_o
        logging.info('Completed iteration {}...'.format(T))

        
