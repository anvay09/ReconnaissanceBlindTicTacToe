from multiprocessing import Pool
from python_files.config import num_workers
from python_files.rbt_utilties import *
from python_files.rbt_classes import *
from python_files.evaluate_policy_parallel import get_expected_utility_parallel
from tqdm import tqdm
import argparse
import logging
import json

logging.basicConfig(format='%(levelname)s - %(asctime)s - %(message)s', datefmt='%d-%b-%y %H:%M:%S',
                    level=logging.INFO)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--player', type=str, default='x', help='Player to calculate policy for')
    parser.add_argument('--policy', type=str, help='Other player\'s policy file')
    parser.add_argument('--T', type=int, default=1, help='Number of iterations to run')
    args = parser.parse_args()

    player = args.player
    policy_file = args.policy
    final_policy_name = policy_file.split('/')[-1]
    num_iterations = args.T

    policy_file_x = 'data/Iterative_1/average/P1_average_overall_policy_after_100_rounds.json'
    policy_file_o = 'data/Iterative_1/average/P2_average_overall_policy_after_100_rounds.json'
    
    P1_information_sets_file = 'data/P1_information_sets.txt'
    P2_information_sets_file = 'data/P2_information_sets.txt'
    information_sets = []
    regret_set = {}

    if player == 'x':
        with open(P1_information_sets_file, 'r') as f:
            lines = f.readlines()
            for line in lines:
                I_hash = line.strip()
                information_sets.append(I_hash)
                regret_set[I_hash] = [0]*13

        p1_policy_dict = json.load(open(policy_file_x, 'r'))
        policy_obj_x = Policy(policy_dict=p1_policy_dict, player='x')
        p2_policy_dict = json.load(open(policy_file, 'r'))
        policy_obj_o = Policy(policy_dict=p2_policy_dict, player='o')

    else:
        with open(P2_information_sets_file, 'r') as f:
            lines = f.readlines()
            for line in lines:
                I_hash = line.strip()
                information_sets.append(I_hash)
                regret_set[I_hash] = [0]*13
                
        p1_policy_dict = json.load(open(policy_file, 'r'))
        policy_obj_x = Policy(policy_dict=p1_policy_dict, player='x')
        p2_policy_dict = json.load(open(policy_file_o, 'r'))
        policy_obj_o = Policy(policy_dict=p2_policy_dict, player='o')

    expu_true_board = TicTacToeBoard(board=['0', '0', '0', '0', '0', '0', '0', '0', '0'])
    expu_I_1 = InformationSet(player='x', move_flag=True, board=['0', '0', '0', '0', '0', '0', '0', '0', '0'])
    expu_I_2 = InformationSet(player='o', move_flag=False, board=['-', '-', '-', '-', '-', '-', '-', '-', '-'])
    expu_player = 'x'

    for T in range(1, num_iterations+1):
        args = []
        
        for I_hash in information_sets:
            I = InformationSet(player, I_hash[-1]=='m', board=[*I_hash[:-1]])
            args.append((I, policy_obj_x, policy_obj_o, T, regret_set[I_hash]))

        expected_utility = get_expected_utility_parallel(expu_I_1, expu_I_2, expu_true_board, expu_player, policy_obj_x, policy_obj_o, 1, NonTerminalHistory(), expu_player)
        logging.info('Expected utility: {}'.format(expected_utility))
        logging.info('Starting iteration {}...'.format(T))
        with Pool(num_workers) as p:
            regrets = p.starmap(calc_cfr_policy_given_I, tqdm(args, total=len(args)))
 
        logging.info('Updating policy objects...')

        for arg in args:
            actions = arg[0].get_actions()
            I_hash = arg[0].get_hash()
            regret_set[I_hash] = regrets.pop(0)
            total_regret = sum(regret_set[I_hash])

            if total_regret > 0:
                for action in actions:
                    if player == 'x':
                        policy_obj_x.policy_dict[I_hash][action] = regret_set[I_hash][action] / total_regret
                    else:
                        policy_obj_o.policy_dict[I_hash][action] = regret_set[I_hash][action] / total_regret
            else:
                for action in actions:
                    if player == 'x':
                        policy_obj_x.policy_dict[I_hash][action] = 1 / len(actions)
                    else:
                        policy_obj_o.policy_dict[I_hash][action] = 1 / len(actions)

        logging.info('Completed iteration {}...'.format(T))

    if player == 'x':
        with open('data/best_response/best_response_against_' + final_policy_name, 'w') as f:
            json.dump(policy_obj_x.policy_dict, f)
    else:
        with open('data/best_response/best_response_against_' + final_policy_name, 'w') as f:
            json.dump(policy_obj_o.policy_dict, f)
        
