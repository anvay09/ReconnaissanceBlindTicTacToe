from rbt_classes import InformationSet, NonTerminalHistory, TerminalHistory, TicTacToeBoard
from sympy.utilities.iterables import multiset_permutations, combinations_with_replacement
from multiprocessing import Pool

num_workers = 4


def toggle_player(player):
    return 'x' if player == 'o' else 'o'


def is_valid_history(H, end_I):
    I_1 = InformationSet(player='x', board=['0', '0', '0', '0', '0', '0', '0', '0', '0'])
    I_2 = InformationSet(player='o', board=['-', '-', '-', '-', '-', '-', '-', '-', '-'])
    true_board = TicTacToeBoard(['0', '0', '0', '0', '0', '0', '0', '0', '0'])
    player = 'x'
    move_flag = True

    for idx in range(len(H)):
        action = H[idx]
        if player == 'x':
            I = I_1.copy()
        else:
            I = I_2.copy()

        actions = I.get_actions(move_flag)
        if move_flag:
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

        move_flag = not move_flag

    if end_I.player == 'x':
        return I_1 == end_I
    else:
        return I_2 == end_I


def get_histories_given_I(I):
    states = I.get_states()
    histories = []
    sense_actions = list(I.sense_square_dict.keys())

    for state in states:
        p1_moves = [idx for idx, value in enumerate(state.board) if value == 'x']
        p2_moves = [idx for idx, value in enumerate(state.board) if value == 'o']

        p1_permutations = list(multiset_permutations(p1_moves))
        p2_permutations = list(multiset_permutations(p2_moves))
        
        if I.is_curr_action_move():
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

                    histories.append(history)

    args = [(h, I) for h in histories]
    with Pool(num_workers) as p:
        valid_histories = p.starmap(is_valid_history, args)

    valid_histories = [h for h, valid in zip(histories, valid_histories) if valid]
    return valid_histories


def play(I_1, I_2, true_board, player, policy_obj_x, policy_obj_o, probability, current_history, initial_player,
         move_flag=True):
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
    :param move_flag:
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

    if move_flag:
        for action in actions:
            new_true_board = true_board.copy()
            success = new_true_board.update_move(action, player)
            # TODO update this line after policy class is updated
            probability *= policy_obj.policy_dict[I.get_hash()][action]
            new_history = current_history.copy()
            new_history.history.append(action)
            if success and not new_true_board.is_win()[0] and not new_true_board.is_over():
                new_I = I.copy()
                new_I.update_move(action, player)
                new_I.reset_zeros()

                if player == 'x':
                    expected_utility_h += play(new_I, I_2, new_true_board, 'o', policy_obj_x, policy_obj_o, probability,
                                               new_history, initial_player, False)
                else:
                    expected_utility_h += play(I_1, new_I, new_true_board, 'x', policy_obj_x, policy_obj_o, probability,
                                               new_history, initial_player, False)
            else:
                terminal_history = TerminalHistory(new_history.history.copy())
                terminal_history.set_reward()
                expected_utility_h += probability * terminal_history.reward[initial_player]

    else:
        for action in actions:
            new_I = I.copy()
            new_I.simulate_sense(action, true_board)
            new_true_board = true_board.copy()
            # TODO update this line after policy class is updated
            probability *= policy_obj.policy_dict[I.get_hash()][action]
            new_history = current_history.copy()
            new_history.history.append(action)

            if player == 'x':
                expected_utility_h += play(new_I, I_2, new_true_board, 'x', policy_obj_x, policy_obj_o, probability,
                                           new_history, initial_player, True)
            else:
                expected_utility_h += play(I_1, new_I, new_true_board, 'o', policy_obj_x, policy_obj_o, probability,
                                           new_history, initial_player, True)

    return expected_utility_h


def get_prob_h_given_policy(I_1, I_2, true_board, player, next_action, policy_obj_x, policy_obj_o, probability,
                            history_obj,
                            move_flag=True):
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
    :param move_flag:
    :return:
    """
    if player == 'x':
        I = I_1
        policy_obj = policy_obj_x
    else:
        I = I_2
        policy_obj = policy_obj_o

    if move_flag:
        new_true_board = true_board.copy()
        success = new_true_board.update_move(next_action, player)
        # TODO update this line after policy class is updated
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
                                                          policy_obj_x, policy_obj_o, probability, history_obj, False)
                else:
                    probability = get_prob_h_given_policy(I_1, new_I, new_true_board, 'x', new_next_action,
                                                          policy_obj_x, policy_obj_o, probability, history_obj, False)

    else:
        new_I = I.copy()
        new_I.simulate_sense(next_action, true_board)
        new_true_board = true_board.copy()
        # TODO update this line after policy class is updated
        probability *= policy_obj.policy_dict[I.get_hash()][next_action]
        history_obj.track_traversal_index += 1
        if history_obj.track_traversal_index < len(history_obj.history):
            new_next_action = history_obj.history[history_obj.track_traversal_index]

            if player == 'x':
                probability = get_prob_h_given_policy(new_I, I_2, new_true_board, 'x', new_next_action, policy_obj_x,
                                                      policy_obj_o, probability, history_obj, True)
            else:
                probability = get_prob_h_given_policy(I_1, new_I, new_true_board, 'o', new_next_action, policy_obj_x,
                                                      policy_obj_o, probability, history_obj, True)

    return probability


def get_counter_factual_utility(information_set, policy_obj_x, policy_obj_o):
    starting_histories = get_histories_given_I(information_set)
    utility = 0
    for h in starting_histories:
        h_object = NonTerminalHistory(h)
        curr_I_1, curr_I_2 = h_object.get_information_sets()
        true_board, overlapping_move_flag, overlapping_move_player = h_object.get_board()
        expected_utiltiy_h = play(curr_I_1, curr_I_2, true_board, information_set.player, policy_obj_x, policy_obj_o, 1,
                                  h_object.copy(), information_set.player, information_set.is_curr_action_move())
        if not curr_I_1.get_hash() == '000000000':
            probabiltiy_reaching_h = get_prob_h_given_policy(
                InformationSet(player='x', board=['0', '0', '0', '0', '0', '0', '0', '0', '0']),
                InformationSet(player='o', board=['-', '-', '-', '-', '-', '-', '-', '-', '-']),
                TicTacToeBoard(board=['0', '0', '0', '0', '0', '0', '0', '0', '0']), 'x', h[0], policy_obj_x, policy_obj_o,
                1, h_object, True)
        else:
            probabiltiy_reaching_h = 1
        utility += expected_utiltiy_h * probabiltiy_reaching_h
    return utility
