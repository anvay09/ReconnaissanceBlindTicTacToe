from rbt_classes import InformationSet, Policy
from rbt_utilties import get_histories_given_I
import json
from tqdm import tqdm

if __name__ == '__main__':
    with open('./data_files/{}.json'.format('p2_iteration_10_cfr_policy.json'), 'r') as f:
        policy_dict = json.load(f)
    policy_obj_o = Policy(player='o', policy_dict=policy_dict)

    P1_reachable_information_sets_file = 'data_files/reachable_P1_information_sets.txt'
    P1_reachable_information_sets = set()
    with open(P1_reachable_information_sets_file, 'r') as f:
        lines = f.readlines()
        for line in lines:
            P1_reachable_information_sets.add(line.strip())

    histories = {}

    for I_hash in tqdm(P1_reachable_information_sets):
        I = InformationSet(player='x', move_flag=I_hash[-1]=='m', board=[*I_hash[:-1]])
    
        H = get_histories_given_I(I, policy_obj_x=None, policy_obj_o=policy_obj_o)
        histories[I_hash] = H

    with open('data_files/p1_valid_histories_for_reachable_I.json', 'w') as f:
        json.dump(histories, f)
        