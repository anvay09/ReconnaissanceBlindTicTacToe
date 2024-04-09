from rbt_classes import TicTacToeBoard, InformationSet, Policy, NonTerminalHistory
from multiprocessing import Pool
import json
import logging
from config import num_workers
import argparse
from tqdm import tqdm
import copy
import time

logging.basicConfig(format='%(levelname)s - %(asctime)s - %(message)s', datefmt='%d-%b-%y %H:%M:%S',
                    level=logging.INFO)


def valid_histories_play(I_1, I_2, true_board, player, current_history, end_I,
                         played_actions, policy_obj_x=None, policy_obj_o=None):
    """

    :param I_1:
    :param I_2:
    :param true_board:
    :param player:
    :param policy_obj_x:
    :param policy_obj_o:
    :param current_history:
    :return:
    """
    valid_histories_list = []

    if player == 'x':
        I = I_1
        if end_I.player == 'x':
            actions = I.get_actions_given_policy(policy_obj_x)
            if I.move_flag:
                actions = [action for action in played_actions if action in actions]
        else:
            actions = I.get_actions()
    else:
        I = I_2
        if end_I.player == 'o':
            actions = I.get_actions_given_policy(policy_obj_o)
            if I.move_flag:
                actions = [action for action in played_actions if action in actions]
        else:
            actions = I.get_actions()

    if I.move_flag:
        for action in actions:
            new_true_board = true_board.copy()
            success = new_true_board.update_move(action, player)

            new_history = current_history.copy()
            new_history.history.append(action)

            if success and not new_true_board.is_win()[0] and not new_true_board.is_over():
                new_I = I.copy()
                new_I.update_move(action, player)
                new_I.reset_zeros()

                if player == 'x':
                    if end_I.player == 'x':
                        valid_histories_list.extend(
                            valid_histories_play(new_I, I_2, new_true_board, 'o', new_history, end_I,
                                                 played_actions, policy_obj_x, policy_obj_o))
                    else:
                        if I_2 == end_I:
                            valid_histories_list.append(new_history.history)
                        else:
                            valid_histories_list.extend(
                                valid_histories_play(new_I, I_2, new_true_board, 'o', new_history, end_I,
                                                     played_actions, policy_obj_x, policy_obj_o))
                else:
                    if end_I.player == 'o':
                        valid_histories_list.extend(
                            valid_histories_play(I_1, new_I, new_true_board, 'x', new_history, end_I,
                                                 played_actions, policy_obj_x, policy_obj_o))
                    else:
                        if I_1 == end_I:
                            valid_histories_list.append(new_history.history)
                        else:
                            valid_histories_list.extend(
                                valid_histories_play(I_1, new_I, new_true_board, 'x', new_history, end_I,
                                                     played_actions, policy_obj_x, policy_obj_o))

    else:
        for action in actions:
            new_I = I.copy()
            new_I.simulate_sense(action, true_board)
            new_true_board = true_board.copy()

            new_history = current_history.copy()
            new_history.history.append(action)

            if player == 'x':
                if end_I.player == 'x':
                    if not new_I == end_I:
                        valid_histories_list.extend(
                            valid_histories_play(new_I, I_2, new_true_board, 'x', new_history, end_I,
                                                 played_actions, policy_obj_x, policy_obj_o))
                    else:
                        valid_histories_list.append(new_history.history)
                else:
                    valid_histories_list.extend(
                        valid_histories_play(new_I, I_2, new_true_board, 'x', new_history, end_I,
                                             played_actions, policy_obj_x, policy_obj_o))
            else:
                if end_I.player == 'o':
                    if not new_I == end_I:
                        valid_histories_list.extend(
                            valid_histories_play(I_1, new_I, new_true_board, 'o', new_history, end_I,
                                                 played_actions, policy_obj_x, policy_obj_o))
                    else:
                        valid_histories_list.append(new_history.history)
                else:
                    valid_histories_list.extend(
                        valid_histories_play(I_1, new_I, new_true_board, 'o', new_history, end_I,
                                             played_actions, policy_obj_x, policy_obj_o))

    return valid_histories_list


def upgraded_get_histories_given_I(I, policy_obj_x=None, policy_obj_o=None):
    if I.get_hash() == "000000000m":
        return [[]]

    I_1 = InformationSet(player='x', move_flag=True, board=['0', '0', '0', '0', '0', '0', '0', '0', '0'])
    I_2 = InformationSet(player='o', move_flag=False, board=['-', '-', '-', '-', '-', '-', '-', '-', '-'])
    true_board = TicTacToeBoard(['0', '0', '0', '0', '0', '0', '0', '0', '0'])
    player = 'x'
    played_actions = I.get_played_actions()

    histories = valid_histories_play(I_1, I_2, true_board, player, NonTerminalHistory([]),
                                     I, played_actions, policy_obj_x, policy_obj_o)
    logging.info('Calculated {} valid histories for {}...'.format(len(histories), I.get_hash()))
    return histories


def get_prob_h_given_policy(I_1, I_2, true_board, player, next_action, policy_obj_x, policy_obj_o, probability,
                            history_obj, initial_player):
    """

    :param I_1:
    :param I_2:
    :param true_board:
    :param player:
    :param next_action:
    :param policy_obj_x:
    :param policy_obj_o:
    :param probability:
    :param history_obj:
    :return:
    """
    if player == 'x':
        I = I_1
        policy_obj = policy_obj_x
    else:
        I = I_2
        policy_obj = policy_obj_o

    if I.move_flag:
        new_true_board = true_board.copy()
        success = new_true_board.update_move(next_action, player)
        # TODO update this line after policy class is updated
        if I.player == initial_player:
            probability *= policy_obj.policy_dict[I.get_hash()][next_action]
        history_obj.track_traversal_index += 1
        if history_obj.track_traversal_index < len(history_obj.history):
            new_next_action = history_obj.history[history_obj.track_traversal_index]
            if success and not new_true_board.is_win()[0] and not new_true_board.is_over():
                new_I = I.copy()
                new_I.update_move(next_action, player)
                new_I.reset_zeros()

                if player == 'x':
                    probability = get_prob_h_given_policy(new_I, I_2, new_true_board, 'o', new_next_action,
                                                          policy_obj_x, policy_obj_o, probability, history_obj,
                                                          initial_player)
                else:
                    probability = get_prob_h_given_policy(I_1, new_I, new_true_board, 'x', new_next_action,
                                                          policy_obj_x, policy_obj_o, probability, history_obj,
                                                          initial_player)

    else:
        new_I = I.copy()
        new_I.simulate_sense(next_action, true_board)
        new_true_board = true_board.copy()

        # TODO update this line after policy class is updated
        if I.player == initial_player:
            probability *= policy_obj.policy_dict[I.get_hash()][next_action]
        history_obj.track_traversal_index += 1
        if history_obj.track_traversal_index < len(history_obj.history):
            new_next_action = history_obj.history[history_obj.track_traversal_index]

            if player == 'x':
                probability = get_prob_h_given_policy(new_I, I_2, new_true_board, 'x', new_next_action, policy_obj_x,
                                                      policy_obj_o, probability, history_obj, initial_player)
            else:
                probability = get_prob_h_given_policy(I_1, new_I, new_true_board, 'o', new_next_action, policy_obj_x,
                                                      policy_obj_o, probability, history_obj, initial_player)

    return probability


def get_prob_h_given_policy_wrapper(I_1, I_2, true_board, player, next_action, policy_obj_x, policy_obj_o, probability,
                                    history_obj, curr_I_1, initial_player):
    if curr_I_1.get_hash() == '000000000m':
        return 1
    else:
        return get_prob_h_given_policy(I_1, I_2, true_board, player, next_action, policy_obj_x, policy_obj_o,
                                       probability, history_obj, initial_player)


def get_probability_of_reaching_all_h(I, policy_obj_x, policy_obj_o, starting_histories, initial_player):
    prob_reaching_h_list_all = []
    for h in starting_histories:
        # h = h.decode(action_bit_encoding)
        h_object = NonTerminalHistory(h)

        if not I.get_hash() == "000000000m":
            temp_args = (
                InformationSet(player='x', move_flag=True, board=['0', '0', '0', '0', '0', '0', '0', '0', '0']),
                InformationSet(player='o', move_flag=False, board=['-', '-', '-', '-', '-', '-', '-', '-', '-']),
                TicTacToeBoard(board=['0', '0', '0', '0', '0', '0', '0', '0', '0']),
                'x', h[0], policy_obj_x, policy_obj_o, 1, h_object, I, initial_player)

            prob_reaching_h = get_prob_h_given_policy_wrapper(*temp_args)
        else:
            prob_reaching_h = 1

        prob_reaching_h_list_all.append(prob_reaching_h)

    return prob_reaching_h_list_all


def get_probability_of_reaching_I(I, policy_obj_x, policy_obj_o, initial_player):
    starting_histories = upgraded_get_histories_given_I(I, policy_obj_x, policy_obj_o)
    prob_reaching_h_list_all = get_probability_of_reaching_all_h(I, policy_obj_x, policy_obj_o, starting_histories,
                                                                 initial_player)
    return sum(prob_reaching_h_list_all)


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
    p1_policy_dict = json.load(open('data/Iterative_1/cfr_policy/P1_cfr_policy_round_2.json', 'r'))
    policy_obj_x = Policy(policy_dict=p1_policy_dict, player='x')
    p2_policy_dict = json.load(open('data/Iterative_1/cfr_policy/P2_cfr_policy_round_2.json', 'r'))
    policy_obj_o = Policy(policy_dict=p2_policy_dict, player='o')
    
    I = InformationSet(player='x', move_flag=True, board=['x', 'o', 'x', 'o', 'x', 'x', '0', 'o', '-'])

    logging.info('Getting starting histories...')
    start_time = time.time()
    # starting_histories = upgraded_get_histories_given_I(I, policy_obj_x, policy_obj_o)
    # logging.info('Starting histories: {}'.format(len(starting_histories)))
    probabiliity = get_probability_of_reaching_I(I, policy_obj_x, policy_obj_o, 'x')
    logging.info('Probability of reaching I: {}'.format(probabiliity))
    logging.info('Time taken: {}'.format(time.time() - start_time))

# if __name__ == "__main__":
#     # %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% create reachable IS file %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#     true_board = TicTacToeBoard(board=['0', '0', '0', '0', '0', '0', '0', '0', '0'])
#     I_1 = InformationSet(player='x', move_flag=True, board=['0', '0', '0', '0', '0', '0', '0', '0', '0'])
#     I_2 = InformationSet(player='o', move_flag=False, board=['-', '-', '-', '-', '-', '-', '-', '-', '-'])
#     player = 'x'
#     cfr_player, policy_file_x, policy_file_o, cfr_round, base_path = parse_commandline_args()

#     if cfr_player == 'o':
#         IS_file_player = 'data/P2_information_sets.json'
#     else:
#         IS_file_player = 'data/P1_information_sets.json'

#     p1_policy_dict = json.load(open(policy_file_x, 'r'))
#     policy_obj_x = Policy(policy_dict=p1_policy_dict, player='x')
#     p2_policy_dict = json.load(open(policy_file_o, 'r'))
#     policy_obj_o = Policy(policy_dict=p2_policy_dict, player='o')

#     # %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% cfr run %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#     I_set = json.load(open(IS_file_player, 'r'))
#     player_information_sets = list(I_set.keys())
#     args = []

#     logging.info('Generating arguments...')
#     for I_hash in player_information_sets:
#         I = InformationSet(player=cfr_player, move_flag=I_hash[-1] == 'm', board=[*I_hash[:-1]])
#         starting_histories = None
#         if cfr_player == 'x':
#             args.append((I, policy_obj_x, None, cfr_player))
#         else:
#             args.append((I, None, policy_obj_o, cfr_player))

#     logging.info('get_probability_of_reaching_I parallel run {}...'.format(cfr_round))
#     with Pool(num_workers) as p:
#         probs = p.starmap(get_probability_of_reaching_I, tqdm(args, total=len(args)))

#     avg_policy_x = copy.deepcopy(policy_obj_x.policy_dict)
#     avg_policy_o = copy.deepcopy(policy_obj_o.policy_dict)
#     prob_dict = {}
#     for key, val in avg_policy_x.items():
#         for k, v in val.items():
#             avg_policy_x[key][k] = 0

#     for key, val in avg_policy_o.items():
#         for k, v in val.items():
#             avg_policy_o[key][k] = 0

#     logging.info('Computing avg policy...')
#     for arg in args:
#         actions = arg[0].get_actions()
#         I_hash = arg[0].get_hash()
#         prob_I = probs.pop(0)

#         for action in actions:
#             if cfr_player == 'x':
#                 avg_policy_x[I_hash][action] = prob_I * policy_obj_x.policy_dict[I_hash][action]
#             else:
#                 avg_policy_o[I_hash][action] = prob_I * policy_obj_o.policy_dict[I_hash][action]
#             prob_dict[I_hash] = prob_I

#     logging.info('Completed computing avg policy {}...'.format(cfr_round))
#     logging.info('Saving policy objects...')

#     if cfr_player == 'x':
#         with open('./{}/average/P1_avg_policy_after_round_{}.json'.format(base_path, cfr_round), 'w') as f:
#             json.dump(avg_policy_x, f)
#         with open('./{}/prob_reaching/P1_prob_reaching_round_{}.json'.format(base_path, cfr_round), 'w') as f:
#             json.dump(prob_dict, f)
#     else:
#         with open('./{}/average/P2_avg_policy_after_round_{}.json'.format(base_path, cfr_round), 'w') as f:
#             json.dump(avg_policy_o, f)
#         with open('./{}/prob_reaching/P2_prob_reaching_round_{}.json'.format(base_path, cfr_round), 'w') as f:
#             json.dump(prob_dict, f)
