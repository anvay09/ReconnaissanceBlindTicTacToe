from rbt_classes import InformationSet, NonTerminalHistory, TerminalHistory, TicTacToeBoard
from sympy.utilities.iterables import multiset_permutations, combinations_with_replacement
from multiprocessing import Pool
import logging
from config import num_workers

logging.basicConfig(format='%(levelname)s - %(asctime)s - %(message)s', datefmt='%d-%b-%y %H:%M:%S',
                    level=logging.INFO)


def toggle_player(player):
    return 'x' if player == 'o' else 'o'


def is_valid_history(H, end_I, policy_obj_x=None, policy_obj_o=None):
    I_1 = InformationSet(player='x', move_flag=True, board=['0', '0', '0', '0', '0', '0', '0', '0', '0'])
    I_2 = InformationSet(player='o', move_flag=False, board=['-', '-', '-', '-', '-', '-', '-', '-', '-'])
    true_board = TicTacToeBoard(['0', '0', '0', '0', '0', '0', '0', '0', '0'])
    player = 'x'

    for idx in range(len(H)):
        action = H[idx]
        if player == 'x':
            I = I_1.copy()
        else:
            I = I_2.copy()

        actions = I.get_actions()

        if I.move_flag:
            success = true_board.update_move(action, player)
            if not success or action not in actions:
                return False
            else:
                if not player == end_I.player:
                    if player == 'x':
                        if policy_obj_x is not None:
                            if policy_obj_x.policy_dict[I_1.get_hash()][action] == 0:
                                return False
                    else:
                        if policy_obj_o is not None:
                            if policy_obj_o.policy_dict[I_2.get_hash()][action] == 0:
                                return False

                I.update_move(action, player)
                I.reset_zeros()
                if player == 'x':
                    I_1 = I.copy()
                else:
                    I_2 = I.copy()

                player = toggle_player(player)
        else:
            if action not in actions:
                return False
            else:
                if not player == end_I.player:
                    if player == 'x':
                        if policy_obj_x is not None:
                            if policy_obj_x.policy_dict[I_1.get_hash()][action] == 0:
                                return False
                    else:
                        if policy_obj_o is not None:
                            if policy_obj_o.policy_dict[I_2.get_hash()][action] == 0:
                                return False

                I.simulate_sense(action, true_board)
                if player == 'x':
                    I_1 = I.copy()
                else:
                    I_2 = I.copy()


    if end_I.player == 'x':
        return I_1 == end_I
    else:
        return I_2 == end_I


def get_histories_given_I(I, policy_obj_x=None, policy_obj_o=None):
    if I.get_hash() == "000000000m":
        return [[]]
    states = I.get_states()
    histories = []
    sense_actions = list(I.sense_square_dict.keys())
    
    for state in states:
        p1_moves = [idx for idx, value in enumerate(state.board) if value == 'x']
        p2_moves = [idx for idx, value in enumerate(state.board) if value == 'o']

        p1_permutations = list(multiset_permutations(p1_moves))
        p2_permutations = list(multiset_permutations(p2_moves))

        if I.move_flag:
            num_sense_actions = len(p1_moves) + len(p2_moves)
        else:
            num_sense_actions = len(p1_moves) + len(p2_moves) - 1

        sense_combinations = list(combinations_with_replacement(sense_actions, num_sense_actions))

        sense_combinations_permuted = []
        for comb in sense_combinations:
            perms = list(multiset_permutations(comb))
            sense_combinations_permuted = sense_combinations_permuted + perms

        for p1 in p1_permutations:
            for p2 in p2_permutations:
                for s in sense_combinations_permuted:
                    history = []
                    player = 'x'
                    idx_1 = 0
                    idx_2 = 0
                    
                    if p1 == []:
                        history.append([])
                    elif s == []:
                        history.append(p1[idx_1])
                    else:
                        for idx in range(len(s)):
                            if player == 'x':
                                history.append(p1[idx_1])
                                history.append(s[idx])
                                idx_1 += 1
                                player = toggle_player(player)
                            else:
                                history.append(p2[idx_2])
                                history.append(s[idx])
                                idx_2 += 1
                                player = toggle_player(player)

                        if idx_1 < len(p1):
                            history.append(p1[idx_1])
                        elif idx_2 < len(p2):
                            history.append(p2[idx_2])

                    histories.append(history)

    valid_histories = []
    for h in histories:
        if is_valid_history(h, I, policy_obj_x, policy_obj_o):
            valid_histories.append(h)

    logging.info('Calculated {} valid histories for {}...'.format(len(valid_histories), I.get_hash()))

    return valid_histories


def play(I_1, I_2, true_board, player, policy_obj_x, policy_obj_o, probability, current_history, initial_player):
    """

    :param I_1:
    :param I_2:
    :param true_board:
    :param player:
    :param policy_obj_x:
    :param policy_obj_o:
    :param probability:
    :param current_history:
    :param initial_player:
    :return:
    """
    expected_utility_h = 0

    if player == 'x':
        I = I_1
        policy_obj = policy_obj_x
    else:
        I = I_2
        policy_obj = policy_obj_o

    actions = I.get_actions_given_policy(policy_obj)

    if I.move_flag:
        for action in actions:
            new_true_board = true_board.copy()
            success = new_true_board.update_move(action, player)
            
            probability_new = probability*policy_obj.policy_dict[I.get_hash()][action]
            new_history = current_history.copy()
            new_history.history.append(action)

            if success and not new_true_board.is_win()[0] and not new_true_board.is_over():
                new_I = I.copy()
                new_I.update_move(action, player)
                new_I.reset_zeros()

                if player == 'x':
                    expected_utility_h += play(new_I, I_2, new_true_board, 'o', policy_obj_x, policy_obj_o, probability_new,
                                               new_history, initial_player)
                else:
                    expected_utility_h += play(I_1, new_I, new_true_board, 'x', policy_obj_x, policy_obj_o, probability_new,
                                               new_history, initial_player)
            else:
                terminal_history = TerminalHistory(new_history.history.copy())
                terminal_history.set_reward()
                expected_utility_h += probability_new * terminal_history.reward[initial_player]

    else:
        for action in actions:
            new_I = I.copy()
            new_I.simulate_sense(action, true_board)
            new_true_board = true_board.copy()
            probability_new = probability*policy_obj.policy_dict[I.get_hash()][action]
            new_history = current_history.copy()
            new_history.history.append(action)

            if player == 'x':
                expected_utility_h += play(new_I, I_2, new_true_board, 'x', policy_obj_x, policy_obj_o, probability_new,
                                           new_history, initial_player)
            else:
                expected_utility_h += play(I_1, new_I, new_true_board, 'o', policy_obj_x, policy_obj_o, probability_new,
                                           new_history, initial_player)

    return expected_utility_h


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
        if I.player == toggle_player(initial_player):
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
        if I.player == toggle_player(initial_player):
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


def get_counter_factual_utility(I, policy_obj_x, policy_obj_o, starting_histories, prob_reaching_h_list):
    utility = 0
    count = 0
    for h in starting_histories:
        h_object = NonTerminalHistory(h)
        curr_I_1, curr_I_2 = h_object.get_information_sets()
        true_board, _, _ = h_object.get_board()
        if prob_reaching_h_list[count] > 0:
            expected_utility_h = play(curr_I_1, curr_I_2, true_board, I.player, policy_obj_x, policy_obj_o, 1,
                                      h_object.copy(), I.player)

            if not curr_I_1.get_hash() == '000000000m':
                probability_reaching_h = prob_reaching_h_list[count]
            else:
                probability_reaching_h = 1
  
            utility += expected_utility_h * probability_reaching_h
        count += 1
    return utility


def get_probability_of_reaching_all_h(I, policy_obj_x, policy_obj_o, starting_histories, initial_player):
    get_prob_h_given_policy_args = []
    prob_reaching_h_list_all = []
    for h in starting_histories:
        h_object = NonTerminalHistory(h)

        if not I.get_hash() == "000000000m":
            temp_args = (
                InformationSet(player='x', move_flag=True, board=['0', '0', '0', '0', '0', '0', '0', '0', '0']),
                InformationSet(player='o', move_flag=False, board=['-', '-', '-', '-', '-', '-', '-', '-', '-']),
                TicTacToeBoard(board=['0', '0', '0', '0', '0', '0', '0', '0', '0']),
                'x', h[0], policy_obj_x, policy_obj_o, 1, h_object, I, initial_player)
            get_prob_h_given_policy_args.append(temp_args)
        else:
            temp_args = []
            get_prob_h_given_policy_args.append(temp_args)

        prob_reaching_h = get_prob_h_given_policy_wrapper(*temp_args)
        prob_reaching_h_list_all.append(prob_reaching_h)

    return prob_reaching_h_list_all


def calc_util_a_given_I_and_action(I, action, policy_obj_x, policy_obj_o, starting_histories, prob_reaching_h_list):
    new_policy_obj_x = policy_obj_x.copy()
    new_policy_obj_o = policy_obj_o.copy()

    if I.move_flag:
        prob_dist = [1 if i == action else 0 for i in range(9)]
        if I.player == 'x':
            new_policy_obj_x.update_policy_for_given_information_set(I, prob_dist)
        else:
            new_policy_obj_o.update_policy_for_given_information_set(I, prob_dist)
    else:
        prob_dist = [1 if i == action else 0 for i in range(9, 13)]
        if I.player == 'x':
            new_policy_obj_x.update_policy_for_given_information_set(I, prob_dist)
        else:
            new_policy_obj_o.update_policy_for_given_information_set(I, prob_dist)

    util_a = get_counter_factual_utility(I, new_policy_obj_x, new_policy_obj_o, starting_histories, prob_reaching_h_list)
    # logging.info('Calculated cf-utility-a = {} for action {}...'.format(util_a, action))

    return util_a


def calc_cfr_policy_given_I(I, policy_obj_x, policy_obj_o, T, prev_regret_list, starting_histories=None):
    actions = I.get_actions()
    regret_list = [0 for _ in range(13)]

    if starting_histories is None:
        if I.player == 'x':
            starting_histories = get_histories_given_I(I, None, policy_obj_o)
        else:
            starting_histories = get_histories_given_I(I, policy_obj_x, None)

    prob_reaching_h_list = get_probability_of_reaching_all_h(I, policy_obj_x, policy_obj_o, starting_histories, I.player)
    
    args = [(I, action, policy_obj_x, policy_obj_o, starting_histories, prob_reaching_h_list) for action in actions]

    util_a_list = [0 for _ in actions]
    for idx in range(len(actions)):
        util_a_list[idx] = calc_util_a_given_I_and_action(*args[idx])

    util = 0
    for action, util_a in zip(actions, util_a_list):
        if I.player == 'x':
            util += util_a * policy_obj_x.policy_dict[I.get_hash()][action]
        else:
            util += util_a * policy_obj_o.policy_dict[I.get_hash()][action]

    for action, util_a in zip(actions, util_a_list):
        if T == 0:
            regret_T = util_a - util
        else:
            regret_T = (1 / T) * ((T - 1) * prev_regret_list[action] + util_a - util)
        
        final_regret_T = max(0, regret_T)
        regret_list[action] = final_regret_T
 
    logging.info('Calculated regret list for {}...'.format(I.get_hash()))
    return regret_list
