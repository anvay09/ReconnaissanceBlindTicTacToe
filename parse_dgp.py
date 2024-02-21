from rbt_classes import TicTacToeBoard, InformationSet, Policy
import json

policy_file = './data_files/p1_policy.txt'
policy_dict = {}

with open(policy_file, 'r') as f:
    lines = f.readlines()
    for line in lines:
        if not line.startswith('#'):
            values = line.split()
            I = InformationSet('x', True, [*values[0]])
            moves = [int(x) for x in values[1:][::2]]
            senses = [int(x) for x in values[2:][::2]]
        
            if len(set(moves)) == 1:
                prob_dist = [0] * 9
                prob_dist[moves[0]] = 1
                policy_dict[I.get_hash()] = {i: prob_dist[i] for i in range(9)}

                new_I = I.copy()
                new_I.update_move(moves[0], 'x')
                new_I.reset_zeros()
                if new_I.is_win_for_player() or new_I.is_over():
                    print(new_I.get_hash(), 'win or draw')
                    continue

                prob_dist = [0] * 4
                for sense in senses:
                    prob_dist[sense] = 1 / len(senses)
                
                policy_dict[new_I.get_hash()] = {i + 9: prob_dist[i] for i in range(4)}
            else:
                prob = 1 / len(moves)
                prob_dist = [0] * 9
                for move in moves:
                    prob_dist[move] = prob
                policy_dict[I.get_hash()] = {i: prob_dist[i] for i in range(9)}

                for j in range(len(moves)):
                    move = moves[j]
                    sense = senses[j]

                    new_I = I.copy()
                    new_I.update_move(move, 'x')
                    new_I.reset_zeros()
                    if new_I.is_win_for_player() or new_I.is_over():
                        print(new_I.get_hash(), 'win or draw')
                        continue

                    prob_dist = [0] * 4
                    prob_dist[sense] = 1
                    policy_dict[new_I.get_hash()] = {i + 9: prob_dist[i] for i in range(4)}


with open('./data_files/P1_information_sets.txt', 'r') as f:
    lines = f.readlines()
    for line in lines:
        I_hash = line.strip()
        if I_hash not in policy_dict:
            I = InformationSet('x', I_hash[-1] == 'm', [*I_hash[:-1]])
            actions = I.get_actions()
            if I.move_flag:
                policy_dict[I_hash] = {i: 1 / len(actions) if i in actions else 0 for i in range(9)}
            else:
                policy_dict[I_hash] = {i: 1 / len(actions) if i in actions else 0 for i in range(9, 13)}
       
with open('./data_files/P1_DG_policy.json', 'w') as f:
    json.dump(policy_dict, f, indent=4)


