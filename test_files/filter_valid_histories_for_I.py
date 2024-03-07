from rbt_classes import InformationSet, Policy
from rbt_utilties import get_histories_given_I
import json
from tqdm import tqdm

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

    for I_hash in tqdm(P2_reachable_information_sets):
        I = InformationSet(player='o', move_flag=I_hash[-1]=='m', board=[*I_hash[:-1]])
    
        H = get_histories_given_I(I, policy_obj_x=policy_obj_x, policy_obj_o=None)
        histories[I_hash] = H

    with open('data_files/p2_valid_histories_for_reachable_I.json', 'w') as f:
        json.dump(histories, f)
        