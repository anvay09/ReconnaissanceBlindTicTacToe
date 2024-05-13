import argparse
from python_files.rbt_classes import *
from python_files.rbt_utilties import *
import json

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--policy', type=str, help='Path to the policy file')
    parser.add_argument('--information_sets', type=str, help='Path to the information set file')
    parser.add_argument('--player', type=str, help='Player')

    args = parser.parse_args()

    information_sets = []
    
    
    with open(args.information_sets, 'r') as f:
        lines = f.readlines()
        for line in lines:
            I_hash = line.strip()
            information_sets.append(I_hash)

    policy_dict = json.load(open(args.policy, 'r'))
    policy_obj = Policy(policy_dict=policy_dict, player=args.player)

    for I_hash in information_sets:
        prob_dict = policy_obj.policy_dict[I_hash]

        # normalise the probabilities
        total = sum(prob_dict.values())

        if total == 0:
            for action in prob_dict:
                prob_dict[action] = 1/len(prob_dict)
        else:
            for action in prob_dict:
                prob_dict[action] /= total
                if prob_dict[action] < 1e-6:
                    prob_dict[action] = 0

            # renormalise the probabilities
            total = sum(prob_dict.values())

            for action in prob_dict:
                prob_dict[action] /= total
                
    # save the policy
    with open(args.policy[:-5] + '_normalised.json', 'w') as f:
        json.dump(policy_obj.policy_dict, f)