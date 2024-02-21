from rbt_classes import InformationSet, NonTerminalHistory, TerminalHistory, TicTacToeBoard
from sympy.utilities.iterables import multiset_permutations, combinations_with_replacement
from multiprocessing import Pool
import logging
from config import num_workers

logging.basicConfig(format='%(levelname)s - %(asctime)s - %(message)s', datefmt='%d-%b-%y %H:%M:%S',
                    level=logging.INFO)


def toggle_player(player):
    return 'x' if player == 'o' else 'o'


def is_valid_history(H, end_I):
    I_1 = InformationSet(player='x', move_flag=True, board=['0', '0', '0', '0', '0', '0', '0', '0', '0'])
    I_2 = InformationSet(player='o', move_flag=False, board=['-', '-', '-', '-', '-', '-', '-', '-', '-'])
    true_board = TicTacToeBoard(['0', '0', '0', '0', '0', '0', '0', '0', '0'])
    player = 'x'
    # move_flag = True

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
                I.simulate_sense(action, true_board)
                if player == 'x':
                    I_1 = I.copy()
                else:
                    I_2 = I.copy()

        # move_flag = not move_flag

    if end_I.player == 'x':
        return I_1 == end_I
    else:
        return I_2 == end_I


def get_histories_given_I(I):
    if I.get_hash() == "000000000m":
        return [[]]
    states = I.get_states()
    histories = []
    sense_actions = list(I.sense_square_dict.keys())
    logging.info('Calculating h for {}...'.format(I.get_hash()))

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
                    # print(p1, p2, s) 
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

    logging.info('Filtering valid histories {}...'.format(I.get_hash()))
    args = [(h, I) for h in histories]
    with Pool(num_workers) as p:
        valid_histories = p.starmap(is_valid_history, args)

    valid_histories = [h for h, valid in zip(histories, valid_histories) if valid]
    logging.info('Filtered {} valid histories for {}...'.format(len(valid_histories), I.get_hash()))
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
            # TODO update this line after policy class is updated
            # probability *= policy_obj.policy_dict[I.get_hash()][action]
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
                # expected_utility_h += probability * terminal_history.reward[initial_player]
                expected_utility_h += probability_new * terminal_history.reward[initial_player]

    else:
        for action in actions:
            new_I = I.copy()
            new_I.simulate_sense(action, true_board)
            new_true_board = true_board.copy()
            # TODO update this line after policy class is updated
            # probability *= policy_obj.policy_dict[I.get_hash()][action]
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
        true_board, overlapping_move_flag, overlapping_move_player = h_object.get_board()
        if prob_reaching_h_list[count] > 0:
            expected_utility_h = play(curr_I_1, curr_I_2, true_board, I.player, policy_obj_x, policy_obj_o, 1,
                                      h_object.copy(), I.player)

            if not curr_I_1.get_hash() == '000000000m':
                # probability_reaching_h = get_prob_h_given_policy(
                #     InformationSet(player='x', move_flag=True, board=['0', '0', '0', '0', '0', '0', '0', '0', '0']),
                #     InformationSet(player='o', move_flag=False, board=['-', '-', '-', '-', '-', '-', '-', '-', '-']),
                #     TicTacToeBoard(board=['0', '0', '0', '0', '0', '0', '0', '0', '0']), 'x', h[0], policy_obj_x,
                #     policy_obj_o, 1, h_object)
                probability_reaching_h = prob_reaching_h_list[count]
            else:
                probability_reaching_h = 1
            utility += expected_utility_h * probability_reaching_h
        count += 1
    return utility


def get_counter_factual_utility_parallel_new(I, policy_obj_x, policy_obj_o, starting_histories, initial_player):
    utility = 0
    play_args = []
    get_prob_h_given_policy_args = []
    prob_reaching_h_list = []
    positive_histories = []
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

        if prob_reaching_h > 0:
            positive_histories.append(h)
            prob_reaching_h_list.append(prob_reaching_h)
            curr_I_1, curr_I_2 = h_object.get_information_sets()
            true_board, _, _ = h_object.get_board()
            play_args.append((curr_I_1, curr_I_2, true_board, I.player, policy_obj_x, policy_obj_o, 1, h_object, I.player))

    with Pool(num_workers) as p:
        expected_utilities = p.starmap(play, play_args)
    for h, expected_utility_h, prob_reaching_h in zip(positive_histories, expected_utilities, prob_reaching_h_list):
        utility += expected_utility_h * prob_reaching_h

    return utility, prob_reaching_h_list_all


def get_counter_factual_utility_parallel(I, policy_obj_x, policy_obj_o, starting_histories, initial_player):
    utility = 0
    play_args = []
    get_prob_h_given_policy_args = []

    for h in starting_histories:
        h_object = NonTerminalHistory(h)
        curr_I_1, curr_I_2 = h_object.get_information_sets()
        true_board, _, _ = h_object.get_board()

        play_args.append((curr_I_1, curr_I_2, true_board, I.player, policy_obj_x, policy_obj_o, 1, h_object, I.player))
        if not I.get_hash() == "000000000m":
            get_prob_h_given_policy_args.append((
                InformationSet(player='x', move_flag=True, board=['0', '0', '0', '0', '0', '0', '0', '0', '0']),
                InformationSet(player='o', move_flag=False, board=['-', '-', '-', '-', '-', '-', '-', '-', '-']),
                TicTacToeBoard(board=['0', '0', '0', '0', '0', '0', '0', '0', '0']),
                'x', h[0], policy_obj_x, policy_obj_o, 1, h_object, I, initial_player))
        else:
            get_prob_h_given_policy_args.append([])

    with Pool(num_workers) as p:
        expected_utilities = p.starmap(play, play_args)
    prob_reaching_h_list = []
    for h, expected_utility_h, args in zip(starting_histories, expected_utilities, get_prob_h_given_policy_args):
        prob_reaching_h = get_prob_h_given_policy_wrapper(*args)
        prob_reaching_h_list.append(prob_reaching_h)
        utility += expected_utility_h * prob_reaching_h
    return utility, prob_reaching_h_list


def calc_regret_given_I_and_action(I, action, policy_obj_x, policy_obj_o, T, prev_regret, starting_histories, util,
                                   prob_reaching_h_list):
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

    logging.info('Calculating cf-utility-a for {}, {}...'.format(I.get_hash(), action))
    util_a = get_counter_factual_utility(I, new_policy_obj_x, new_policy_obj_o, starting_histories,
                                         prob_reaching_h_list)
    logging.info('Calculated cf-utility-a = {} for action {}...'.format(util_a, action))

    if T == 0:
        regret_T = util_a - util
    else:
        regret_T = (1 / T) * ((T - 1) * prev_regret + util_a - util)

    final_regret_T = max(0, regret_T)
    return final_regret_T


def calc_cfr_policy_given_I(I, policy_obj_x, policy_obj_o, T, prev_regret_list):
    actions = I.get_actions()
    regret_list = [0 for _ in range(13)]

    starting_histories = get_histories_given_I(I)

    logging.info('Calculating cf-utility for {}...'.format(I.get_hash()))
    util, prob_reaching_h_list = get_counter_factual_utility_parallel_new(I, policy_obj_x, policy_obj_o, starting_histories,
                                                                      I.player)
    logging.info('Calculated cf-utility = {}...'.format(util))

    logging.info('Calculating regret for {}...'.format(I.get_hash()))

    args = [(
        I, action, policy_obj_x, policy_obj_o, T, prev_regret_list[action], starting_histories,
        util, prob_reaching_h_list) for action in actions]

    with Pool(len(actions)) as p:
        regrets = p.starmap(calc_regret_given_I_and_action, args)

    for action, regret in zip(actions, regrets):
        regret_list[action] = regret
        logging.info('Calculated regret for {}, {} = {}...'.format(I.get_hash(), action, regret))

    total_regret_I = sum(regret_list)

    if I.player == 'x':
        policy = policy_obj_x
    else:
        policy = policy_obj_o
    if total_regret_I > 0:
        for action in actions:
            policy.policy_dict[I.get_hash()][action] = regret_list[action] / total_regret_I
    else:
        for action in actions:
            policy.policy_dict[I.get_hash()][action] = 1 / len(actions)

    logging.info('Updated policy for {} is {}'.format(I.get_hash(), policy.policy_dict[I.get_hash()]))
    return policy_obj_x, policy_obj_o, regret_list
