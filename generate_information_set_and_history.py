from rbt_classes import TicTacToeBoard
from rbt_classes import InformationSet
from multiprocessing import Pool


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


def play(I_1, I_2, true_board, player):
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
    I_2_set = set((I_2.get_hash(),))

    if player == 'x':
        I = I_1
    else:
        I = I_2

    actions = I.get_actions()

    if I.move_flag:
        for action in actions:
            new_true_board = true_board.copy()
            success = new_true_board.update_move(action, player)

            if success and not new_true_board.is_win()[0] and not new_true_board.is_over():
                new_I = I.copy()
                new_I.update_move(action, player)
                new_I.reset_zeros()

                if player == 'x':
                    num_histories_future, future_I_1_set, future_I_2_set = play(new_I, I_2, new_true_board, 'o')
                    num_histories += num_histories_future
                    I_1_set = I_1_set.union(future_I_1_set)
                    I_2_set = I_2_set.union(future_I_2_set)
                else:
                    num_histories_future, future_I_1_set, future_I_2_set = play(I_1, new_I, new_true_board, 'x')
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
                num_histories_future, future_I_1_set, future_I_2_set = play(new_I, I_2, new_true_board, 'x')
                num_histories += num_histories_future
                I_1_set = I_1_set.union(future_I_1_set)
                I_2_set = I_2_set.union(future_I_2_set)
            else:
                num_histories_future, future_I_1_set, future_I_2_set = play(I_1, new_I, new_true_board, 'o')
                num_histories += num_histories_future
                I_1_set = I_1_set.union(future_I_1_set)
                I_2_set = I_2_set.union(future_I_2_set)

    return num_histories, I_1_set, I_2_set


def parallel_play(I_1, I_2, true_board, player):
    Total_histories = 0
    I_1_vars = []
    I_2_vars = []
    true_board_vars = []
    P1_information_sets = set((I_1.get_hash(),))
    P2_information_sets = set((I_2.get_hash(),))

    if player == 'x':
        I = I_1
    else:
        I = I_2

    actions = I.get_actions()

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
        obj_list = pool.starmap(play, [(I_1_vars[i], I_2_vars[i], true_board_vars[i], player) for i in
                                       range(len(I_1_vars))])

    for item in obj_list:
        Total_histories += item[0]
        P1_information_sets = P1_information_sets.union(item[1])
        P2_information_sets = P2_information_sets.union(item[2])

    print('Total Histories: ', Total_histories)
    
    with open('P1_information_sets.txt', 'w') as f:
        for item in P1_information_sets:
            f.write("%s\n" % item)

    with open('P2_information_sets.txt', 'w') as f:
        for item in P2_information_sets:
            f.write("%s\n" % item)

    return


if __name__ == "__main__":
    true_board = TicTacToeBoard(board=['0', '0', '0', '0', '0', '0', '0', '0', '0'])
    I_1 = InformationSet(player='x', move_flag=True, board=['0', '0', '0', '0', '0', '0', '0', '0', '0'])
    I_2 = InformationSet(player='o', move_flag=False, board=['-', '-', '-', '-', '-', '-', '-', '-', '-'])
    player = 'x'
    move_flag = True
    # true_board = TicTacToeBoard(board=['0', '0', '0', '0', 'x', '0', '0', '0', '0'])
    # I_1 = InformationSet(player='x', move_flag=False, board=['-', '-', '-', '-', 'x', '-', '-', '-', '-'])
    # I_2 = InformationSet(player='o', move_flag=True, board=['-', '-', '-', '0', 'x', '-', '0', '0', '-'])
    # player = 'o'
    # move_flag = False

    parallel_play(I_1, I_2, true_board, player)
