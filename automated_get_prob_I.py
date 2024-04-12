from rbt_classes import InformationSet, Policy
from multiprocessing import Pool
import json
import logging
from config import num_workers
import argparse
from tqdm import tqdm
import copy
import os

logging.basicConfig(format='%(levelname)s - %(asctime)s - %(message)s', datefmt='%d-%b-%y %H:%M:%S',
                    level=logging.INFO)


def get_probability_of_reaching_I_cpp(I, policy_file_x, policy_file_o, cfr_player, cfr_round):
    I_hash = I.gethash()
    os.system("./automated_get_prob_I" + " " + policy_file_x + " " + policy_file_o + " " + I_hash + " " + str(cfr_player) + str(cfr_round))
    outfile = "prob_I" + I_hash + "_round_" + str(cfr_round) + ".txt"
    with open(outfile, 'r') as f:
        prob_I = f.read()
    os.system("rm " + outfile)
    logging.info('Prob for I {} player {} round {} : {}'.format(I_hash, cfr_player, cfr_round, prob_I))
    return prob_I 


def parse_commandline_args():
    """Parse command line arguments"""
    parser = argparse.ArgumentParser()
    parser.add_argument('--CurrentPlayer', type=str, required=True)
    parser.add_argument('--PolicyFileX', type=str, required=True)
    parser.add_argument('--PolicyFileO', type=str, required=True)
    parser.add_argument('--Round', type=str, required=True)
    parser.add_argument('--BasePath', type=str, required=True)
    arguments = parser.parse_args()
    return arguments.CurrentPlayer, arguments.PolicyFileX, arguments.PolicyFileO, int(
        arguments.Round), arguments.BasePath

    
if __name__ == "__main__":
    # %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% create reachable IS file %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    cfr_player, policy_file_x, policy_file_o, cfr_round, base_path = parse_commandline_args()

    if cfr_player == 'o':
        IS_file_player = 'data/P2_information_sets.json'
    else:
        IS_file_player = 'data/P1_information_sets.json'

    p1_policy_dict = json.load(open(policy_file_x, 'r'))
    policy_obj_x = Policy(policy_dict=p1_policy_dict, player='x')
    p2_policy_dict = json.load(open(policy_file_o, 'r'))
    policy_obj_o = Policy(policy_dict=p2_policy_dict, player='o')

    # %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% cfr run %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    I_set = json.load(open(IS_file_player, 'r'))
    player_information_sets = list(I_set.keys())
    args = []

    logging.info('Generating arguments...')
    for I_hash in player_information_sets:
        I = InformationSet(player=cfr_player, move_flag=I_hash[-1] == 'm', board=[*I_hash[:-1]])
        starting_histories = None
        if cfr_player == 'x':
            args.append((I, policy_file_x, policy_file_o, cfr_player, cfr_round))
        else:
            args.append((I, policy_file_x, policy_obj_o, cfr_player, cfr_round))

    logging.info('get_probability_of_reaching_I parallel run {}...'.format(cfr_round))
    with Pool(num_workers) as p:
        probs = p.starmap(get_probability_of_reaching_I_cpp, tqdm(args, total=len(args)))

    avg_policy_x = copy.deepcopy(policy_obj_x.policy_dict)
    avg_policy_o = copy.deepcopy(policy_obj_o.policy_dict)
    prob_dict = {}
    for key, val in avg_policy_x.items():
        for k, v in val.items():
            avg_policy_x[key][k] = 0

    for key, val in avg_policy_o.items():
        for k, v in val.items():
            avg_policy_o[key][k] = 0

    logging.info('Computing avg policy...')
    for arg in args:
        actions = arg[0].get_actions()
        I_hash = arg[0].get_hash()
        prob_I = probs.pop(0)

        for action in actions:
            if cfr_player == 'x':
                avg_policy_x[I_hash][action] = prob_I * policy_obj_x.policy_dict[I_hash][action]
            else:
                avg_policy_o[I_hash][action] = prob_I * policy_obj_o.policy_dict[I_hash][action]
            prob_dict[I_hash] = prob_I

    logging.info('Completed computing avg policy {}...'.format(cfr_round))
    logging.info('Saving policy objects...')

    if cfr_player == 'x':
        with open('./{}/average/P1_avg_policy_after_round_{}.json'.format(base_path, cfr_round), 'w') as f:
            json.dump(avg_policy_x, f)
        with open('./{}/prob_reaching/P1_prob_reaching_round_{}.json'.format(base_path, cfr_round), 'w') as f:
            json.dump(prob_dict, f)
    else:
        with open('./{}/average/P2_avg_policy_after_round_{}.json'.format(base_path, cfr_round), 'w') as f:
            json.dump(avg_policy_o, f)
        with open('./{}/prob_reaching/P2_prob_reaching_round_{}.json'.format(base_path, cfr_round), 'w') as f:
            json.dump(prob_dict, f)
