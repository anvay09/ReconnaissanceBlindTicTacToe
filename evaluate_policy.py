from rbt_classes import TicTacToeBoard, InformationSet, Policy, NonTerminalHistory, TerminalHistory
from multiprocessing import Pool
import json
import logging

logging.basicConfig(format='%(levelname)s - %(asctime)s - %(message)s', datefmt='%d-%b-%y %H:%M:%S',
                    level=logging.INFO)

def get_information_set_from_states(states, player):
    """
    :param states: list of TicTacTie boards
    :param player: 'x' or 'o'
    :return: Information set object
    """
    I = InformationSet(player)
    for i in range(9):
        flag = 0
        temp = states[0][i]
        for state in states:
            if not state[i] == temp:
                flag = 1
                break
        if flag:
            I[i] = '-'
        else:
            I[i] = temp

    return I


def play(I_1, I_2, true_board, player, p1_policy_obj, p2_policy_obj):
    """
    :param I_1:
    :param I_2:
    :param true_board:
    :param player:
    :param move_flag:
    :return:
    """
    num_histories = 0

    I_1_set = set((I_1.get_hash(),))

    if I_2.num_self_moves() == 4:
        I_2_set = set()
    else:
        I_2_set = set((I_2.get_hash(),))

    if player == 'x':
        I = I_1
        actions = I.get_actions_given_policy(p1_policy_obj)
    else:
        I = I_2
        actions = I.get_actions_given_policy(p2_policy_obj)

    if I.move_flag:
        for action in actions:
            new_true_board = true_board.copy()
            success = new_true_board.update_move(action, player)

            if success and not new_true_board.is_win()[0] and not new_true_board.is_over():
                new_I = I.copy()
                new_I.update_move(action, player)
                new_I.reset_zeros()

                if player == 'x':
                    num_histories_future, future_I_1_set, future_I_2_set = play(new_I, I_2, new_true_board, 'o', p1_policy_obj, p2_policy_obj)
                    num_histories += num_histories_future
                    I_1_set = I_1_set.union(future_I_1_set)
                    I_2_set = I_2_set.union(future_I_2_set)
                else:
                    num_histories_future, future_I_1_set, future_I_2_set = play(I_1, new_I, new_true_board, 'x', p1_policy_obj, p2_policy_obj)
                    num_histories += num_histories_future
                    I_1_set = I_1_set.union(future_I_1_set)
                    I_2_set = I_2_set.union(future_I_2_set)
            else:
                num_histories += 1

    else:
        for action in actions:
            new_I = I.copy()
            new_I.simulate_sense(action, true_board)
            new_true_board = true_board.copy()
            
            if player == 'x':
                num_histories_future, future_I_1_set, future_I_2_set = play(new_I, I_2, new_true_board, 'x', p1_policy_obj, p2_policy_obj)
                num_histories += num_histories_future
                I_1_set = I_1_set.union(future_I_1_set)
                I_2_set = I_2_set.union(future_I_2_set)
            else:
                num_histories_future, future_I_1_set, future_I_2_set = play(I_1, new_I, new_true_board, 'o', p1_policy_obj, p2_policy_obj)
                num_histories += num_histories_future
                I_1_set = I_1_set.union(future_I_1_set)
                I_2_set = I_2_set.union(future_I_2_set)

    return num_histories, I_1_set, I_2_set


def parallel_play(I_1, I_2, true_board, player, p1_policy_obj, p2_policy_obj):
    Total_histories = 0
    I_1_vars = []
    I_2_vars = []
    true_board_vars = []
    P1_information_sets = set((I_1.get_hash(),))
    P2_information_sets = set((I_2.get_hash(),))

    if player == 'x':
        I = I_1
        actions = I.get_actions_given_policy(p1_policy_obj)
    else:
        I = I_2
        actions = I.get_actions_given_policy(p2_policy_obj)

    for action in actions:
        new_true_board = true_board.copy()
        success = new_true_board.update_move(action, player)

        if success and not new_true_board.is_win()[0] and not new_true_board.is_over():
            new_I = I.copy()
            new_I.update_move(action, player)
            new_I.reset_zeros()

            if player == 'x':
                I_1_vars.append(new_I)
                I_2_vars.append(I_2.copy())
            else:
                I_1_vars.append(I_1.copy())
                I_2_vars.append(new_I)

            true_board_vars.append(new_true_board)
        else:
            Total_histories += 1

    if player == 'x':
        player = 'o'
    else:
        player = 'x'

    with Pool(len(I_1_vars)) as pool:
        obj_list = pool.starmap(play, [(I_1_vars[i], I_2_vars[i], true_board_vars[i], player, p1_policy_obj, p2_policy_obj) for i in
                                       range(len(I_1_vars))])

    for item in obj_list:
        Total_histories += item[0]
        P1_information_sets = P1_information_sets.union(item[1])
        P2_information_sets = P2_information_sets.union(item[2])

    print('Total Histories: ', Total_histories)
    print('P1 Information Sets: ', len(P1_information_sets))
    # with open('data_files/reachable_P1_information_sets.txt', 'w') as f:
    #    for item in P1_information_sets:
    #        f.write(item + '\n')

    print('P2 Information Sets: ', len(P2_information_sets))
    # with open('data_files/reachable_P2_information_sets.txt', 'w') as f:
    #    for item in P2_information_sets:
    #        f.write(item + '\n')
    
    return


terminal_histories = []

def get_expected_utility(I_1, I_2, true_board, player, policy_obj_x, policy_obj_o, probability, current_history, initial_player):
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
                    expected_utility_h += get_expected_utility(new_I, I_2, new_true_board, 'o', policy_obj_x, policy_obj_o, probability_new,
                                               new_history, initial_player)
                else:
                    expected_utility_h += get_expected_utility(I_1, new_I, new_true_board, 'x', policy_obj_x, policy_obj_o, probability_new,
                                               new_history, initial_player)
            else:
                terminal_history = TerminalHistory(new_history.history.copy())
                terminal_history.set_reward()
                
                terminal_histories.append((terminal_history, probability_new, terminal_history.reward[initial_player]))
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
                expected_utility_h += get_expected_utility(new_I, I_2, new_true_board, 'x', policy_obj_x, policy_obj_o, probability_new,
                                           new_history, initial_player)
            else:
                expected_utility_h += get_expected_utility(I_1, new_I, new_true_board, 'o', policy_obj_x, policy_obj_o, probability_new,
                                           new_history, initial_player)

    return expected_utility_h


if __name__ == "__main__":
    true_board = TicTacToeBoard(board=['0', '0', '0', '0', '0', '0', '0', '0', '0'])
    I_1 = InformationSet(player='x', move_flag=True, board=['0', '0', '0', '0', '0', '0', '0', '0', '0'])
    I_2 = InformationSet(player='o', move_flag=False, board=['-', '-', '-', '-', '-', '-', '-', '-', '-'])
    player = 'x'

    p1_policy_dict = json.load(open('data_files/P1_DG_policy.json', 'r'))
    p1_policy_obj = Policy(policy_dict=p1_policy_dict, player='x')

    # p2_policy_dict = json.load(open('data_files/P2_iteration_2_cfr_policy.json', 'r'))
    p2_policy_dict = json.load(open('data_files/P2_uniform_policy.json', 'r'))
    p2_policy_obj = Policy(policy_dict=p2_policy_dict, player='o')

    # p2_random_policy_dict = json.load(open('data_files/P2_uniform_policy.json', 'r'))
    # p2_random_policy_obj = Policy(policy_dict=p2_random_policy_dict, player='o')

    parallel_play(I_1, I_2, true_board, player, p1_policy_obj, p2_policy_obj)

    expected_utility = get_expected_utility(I_1, I_2, true_board, player, p1_policy_obj, p2_policy_obj, 1, NonTerminalHistory(), player)
    print('Expected Utility: ', expected_utility)

    terminal_histories.sort(key=lambda x: x[2], reverse=True)

    with open('data_files/games.txt', 'w') as f:
        for item in terminal_histories:
            f.write('History: {}, Probability: {}, Reward: {}\n'.format(item[0].history, item[1], item[2]))
