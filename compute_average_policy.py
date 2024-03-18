from rbt_utilties import get_histories_given_I
from rbt_classes import NonTerminalHistory, InformationSet, TicTacToeBoard
from tqdm import tqdm
import argparse
import json
import logging

logging.basicConfig(format='%(levelname)s - %(asctime)s - %(message)s', datefmt='%d-%b-%y %H:%M:%S',
                    level=logging.INFO)

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
    starting_histories = get_histories_given_I(I, policy_obj_x, policy_obj_o)
    prob_reaching_h_list_all = get_probability_of_reaching_all_h(I, policy_obj_x, policy_obj_o, starting_histories, initial_player)
    return sum(prob_reaching_h_list_all)


def get_average_policy(policy_obj_list, I_list, initial_player):
    """

    :param policy_obj_list:
    :param I_list:
    :return:
    """
    average_policy = {}
    for I_hash in tqdm(I_list):
        average_policy[I_hash] = {}
        I = InformationSet(player=initial_player, move_flag=I_hash[-1]=='m', board=[*I_hash[:-1]])
        actions = I.get_actions()
        
        prob_term_list = []
        for policy_obj in policy_obj_list:
            if initial_player == 'x':
                prob_term = get_probability_of_reaching_I(I, policy_obj, None, initial_player)
            else:
                prob_term = get_probability_of_reaching_I(I, None, policy_obj, initial_player)
            prob_term_list.append(prob_term)
        
        for action in actions:
            numerator = 0
            denominator = 0

            for p in range(len(policy_obj_list)):
                numerator += prob_term_list[p] * policy_obj_list[p].policy_dict[I_hash][action]
                denominator += prob_term_list[p]

            if denominator == 0:
                average_policy[I_hash][action] = 0
            else:
                average_policy[I_hash][action] = numerator / denominator

    return average_policy

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--CurrentPlayer', type=str, required=True)
    parser.add_argument('--PolicyFileBase', type=str, required=True)
    parser.add_argument('--NumRounds', type=str, required=True)
    parser.add_argument('--ISetFile', type=str, required=True)
    arguments = parser.parse_args()

    policy_obj_list = []

    for i in range(int(arguments.NumRounds)):
        with open(arguments.PolicyFileBase + str(i) + '.json', 'r') as f:
            policy_obj_list.append(json.load(f))

    with open(arguments.ISetFile, 'r') as f:
        I_list = json.load(f)

    average_policy = get_average_policy(policy_obj_list, I_list, arguments.CurrentPlayer)
    
    if arguments.CurrentPlayer == 'x':
        outfile_name = 'P1_average_policy_after_{}_rounds.json'.format(arguments.NumRounds)
    else:
        outfile_name = 'P2_average_policy_after_{}_rounds.json'.format(arguments.NumRounds)

    with open(outfile_name, 'w') as f:
        json.dump(average_policy, f)
    

