from rbt_classes import InformationSet
from rbt_utilties import get_histories_given_I
import json
from tqdm import tqdm

if __name__ == '__main__':
    P2_reachable_information_sets_file = 'data_files/reachable_P2_information_sets.txt'
    P2_reachable_information_sets = set()
    with open(P2_reachable_information_sets_file, 'r') as f:
        lines = f.readlines()
        for line in lines:
            P2_reachable_information_sets.add(line.strip())

    histories = {}

    for I_hash in tqdm(P2_reachable_information_sets):
        I = InformationSet(player='o', move_flag=I_hash[-1]=='m', board=[*I_hash[:-1]])
    
        H = get_histories_given_I(I)
        histories[I_hash] = H

    with open('data_files/valid_histories_for_reachable_I.json', 'w') as f:
        json.dump(histories, f)
        