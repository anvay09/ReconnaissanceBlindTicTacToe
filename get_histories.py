from information_set_fast import InformationSet
from TicTacToe_fast import TicTacToeBoard
from sympy.utilities.iterables import multiset_permutations, combinations_with_replacement

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
        

def get_histories(I, move_flag):
    states = I.get_states()
    histories = []
    sense_actions = list(I.sense_square_dict.keys())

    for state in states:
        p1_moves = [idx for idx, value in enumerate(state.board) if value == 'x']
        p2_moves = [idx for idx, value in enumerate(state.board) if value == 'o']
        
        p1_permutations = list(multiset_permutations(p1_moves))
        p2_permutations = list(multiset_permutations(p2_moves))

        if move_flag:
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
                        
                    if is_valid_history(history, I):
                        histories.append(history)

    return histories

if __name__ == '__main__':
    I = InformationSet(player='x', board=['-', '0', '0', 'x', 'x', 'o', 'o', '-', 'x'])
    move_flag = True
    H = get_histories(I, move_flag)
    print(len(H))