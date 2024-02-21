from rbt_classes import InformationSet, Policy
from rbt_utilties import calc_cfr_policy_given_I
import logging
import json

if __name__ == '__main__':
    # I = InformationSet(player='x', move_flag = True, board=['0', '0', '0', '0', '0', '0', '0', '0', '0'])
    # I = InformationSet(player='x', move_flag=True, board=['-', '0', '0', 'x', 'x', 'o', 'o', '-', 'x'])
    # I = InformationSet(player='o', move_flag = False, board=['o', '-', '-', '-', 'x', 'o', 'o', '-', 'x'])

    # TODO: Fix Bug for root node, and for player 'o' first move
    # I = InformationSet(player='x', move_flag=True, board=['o', '0', '-', '0', 'x', '-', '-', '-', '-'])
    # I = InformationSet(player='o', move_flag=False, board=['o', '-', '-', '-', 'x', 'o', 'o', '-', 'x'])

    logging.info('Initializing policy objects...')

    with open('./data_files/{}.json'.format('P1_DG_policy'), 'r') as f:
        policy_dict = json.load(f)
    policy_obj_x = Policy(player='x', policy_dict=policy_dict)

    with open('./data_files/{}.json'.format('P2_uniform_policy'), 'r') as f:
        policy_dict = json.load(f)
    policy_obj_o = Policy(player='o', policy_dict=policy_dict)

    logging.info('Loaded policy objects...')

    # if I.player == 'x':
    #     print(policy_obj_x.policy_dict[I.get_hash()])
    # else:
    #     print(policy_obj_o.policy_dict[I.get_hash()])

    P2_reachable_information_sets_file = 'data_files/reachable_P2_information_sets.txt'
    P2_reachable_information_sets = set()
    with open(P2_reachable_information_sets_file, 'r') as f:
        lines = f.readlines()
        for line in lines:
            P2_reachable_information_sets.add(line.strip())

    # prev_regret_list_x = [0 for _ in range(13)]
    prev_regret_list_o = {I_hash:[0 for _ in range(13)] for I_hash in P2_reachable_information_sets}
    processed_I_count = 0
            
    for T in range(1,5):
        # for I_hash in P2_reachable_information_sets:
        # for I_hash in ['x-oox-x0-m']:
        for I_hash in ['xo-oox---m']:
            I = InformationSet(player='o', move_flag=I_hash[-1]=='m', board=[*I_hash[:-1]])

            policy_obj_x, policy_obj_o, prev_regret_list_o[I_hash] = calc_cfr_policy_given_I(I, policy_obj_x, policy_obj_o, T,
                                                                             prev_regret_list_o[I_hash])
            
            logging.info('Updated policy for player {}, information set {}:'.format(I.player, I_hash))
            logging.info('{}'.format(policy_obj_o.policy_dict[I_hash]))
            processed_I_count += 1
            logging.info('Processed {} information sets in iteration {}...'.format(processed_I_count, T))
        
        processed_I_count = 0
        logging.info('Completed iteration {}...'.format(T))

        logging.info('Saving policy objects...')
        with open('./data_files/{}.json'.format('P2_updated_policy'), 'w') as f:
            json.dump(policy_obj_o.policy_dict, f)
    