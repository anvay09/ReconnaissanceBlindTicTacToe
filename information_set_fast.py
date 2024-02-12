from TicTacToe_fast import TicTacToeBoard
from sympy.utilities.iterables import multiset_permutations
from multiprocessing import Pool

class InformationSet(TicTacToeBoard):
    """
    Inherit data and functions from TicTacToeBoard
    """

    def __init__(self, player, board=None):
        """
        :param player: x, o
        :param board: bytes object indicating the information set of player
        """
        TicTacToeBoard.__init__(self, board)
        self.sense_square_dict = {9: [0, 1, 3, 4], 10: [1, 2, 4, 5], 11: [3, 4, 6, 7], 12: [4, 5, 7, 8]}
        self.player = player

    def other_player(self):
        if self.player == 'x':
            return 'o'
        else:
            return 'x'
        
    def copy(self):
        return InformationSet(self.player, self.board)

    def get_states(self):
        """ Get the states part of the information set

        :return: list of TicTacToeBoard objects
        """
        num_unknown_opponent_moves = self.get_number_of_unknown_opponent_moves()

        board_copy = self.board.copy()
        for i in range(len(board_copy)):
            if board_copy[i] == '-':
                board_copy[i] = '0'
        
        if num_unknown_opponent_moves == 0:
            return [TicTacToeBoard(board=board_copy)]
        else:
            output_states = []
            uncertain_ind = self.get_uncertain_squares()
            base_perm = [self.other_player()] * num_unknown_opponent_moves + ['0'] * (
                    len(uncertain_ind) - num_unknown_opponent_moves)
            
            perm_itr = multiset_permutations(base_perm)
            for perm in perm_itr:
                new_state = TicTacToeBoard(board=board_copy)
                for j in range(len(perm)):
                    new_state[uncertain_ind[j]] = perm[j]

                output_states.append(new_state)

            return output_states

    def get_actions(self, move_flag=True):
        """
        Get the valid actions that can be taken from the current information set. These include the squares which the
        player knows is empty ('0') and the uncertain squares ('-')
        :param move_flag: indicates whether the current action is move or sense
        :return: list of valid actions that can be taken from the current information set
        """
        if move_flag:
            return self.get_valid_moves()
        else:
            return self.get_useful_senses()

    def get_valid_moves(self):
        """
        :return: list of valid actions
        """
        w = self.win_exists()
        if w != -1:
            return [w]
        
        valid_moves = []
        for i in range(len(self.board)):
            if self.board[i] == '0' or self.board[i] == '-':
                valid_moves.append(i)
        return valid_moves

    def get_useful_senses(self):
        """
        :return: list of valid senses
        """
        valid_sense = []
        for key, value in self.sense_square_dict.items():
            for i in range(len(value)):
                if self.board[value[i]] == '-':
                    valid_sense.append(key)
                    break
        return valid_sense

    def get_number_of_unknown_opponent_moves(self):
        """
        :return: int
        """
        count_x = 0
        count_o = 0
        for i in range(len(self.board)):
            if self.board[i] == 'x':
                count_x += 1
            if self.board[i] == 'o':
                count_o += 1
        if self.player == 'x':
            return count_x - count_o
        else:
            return count_o - count_x + 1

    def get_uncertain_squares(self):
        """
        :return: list of uncertain squares
        """
        uncertain_squares = []
        for i in range(len(self.board)):
            if self.board[i] == '-':
                uncertain_squares.append(i)
        return uncertain_squares

    def simulate_sense(self, action, true_board):
        self.reset_zeros()
        for square in self.sense_square_dict[action]:
            self.board[square] = true_board[square]

    def reset_zeros(self):
        for i in range(len(self.board)):
            if self.board[i] == '0':
                self.board[i] = '-'

    def is_valid_move(self, square):
        if square < 0 or square > 8:
            return False
        else:
            return self.board[square] == '0' or self.board[square] == '-'
    
    def update_move(self, square, player):
        if self.is_valid_move(square):
            self.board[square] = player
            return True
        return False

    def is_win_for_player(self):  # returns a boolean (True or False)
        for i in range(3):
            if (self.board[3 * i] == self.board[3 * i + 1] == self.board[3 * i + 2] == self.player):
                return True

            if (self.board[i] == self.board[i + 3] == self.board[i + 6] == self.player):
                return True

        if (self.board[0] == self.board[4] == self.board[8] == self.player):
            return True

        if (self.board[2] == self.board[4] == self.board[6] == self.player):
            return True

        return False

    def win_exists(self):
        """
        :return: int, action that leads to a win
        """
        zeroes = []
        for i in range(len(self.board)):
            if self.board[i] == '0':
                zeroes.append(i)
        
        for zero in zeroes:
            new_I_board = self.board.copy()
            new_I_board[zero] = self.player
            if InformationSet(self.player, new_I_board).is_win_for_player():
                return zero
            
        return -1


def get_information_set_from_states(states, player):
    """
    :param states: list of TicTacTie boards
    :param player: 'x' or 'o'
    :return: Information set object
    """
    # for state in states:
    #     print("State: ", state.board)

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


def play(I_1, I_2, true_board, player, move_flag=True):
    """
    :param I_1:
    :param I_2:
    :param true_board:
    :param player:
    :param move_flag:
    :return:
    """
    num_histories = 0

    if player == 'x':
        I = I_1
    else:
        I = I_2

    actions = I.get_actions(move_flag)
    
    if move_flag:
        # states = I.get_states()
    
        for action in actions:
            # output_states = []
            # for state in states:
            #     new_state = state.copy()
            #     valid = new_state.update_move(action, player)

            #     if new_state.is_over() or new_state.is_win()[0] or not valid:
            #         pass
            #     else:
            #         output_states.append(new_state)

            new_true_board = true_board.copy()
            success = new_true_board.update_move(action, player)
            
            if success and not new_true_board.is_win()[0] and not new_true_board.is_over():    
                # new_I = get_information_set_from_states(output_states, player)
                new_I = I.copy()
                new_I.update_move(action, player)
                new_I.reset_zeros()

                if player == 'x':
                    num_histories += play(new_I, I_2, new_true_board, 'o', False)
                else:
                    num_histories += play(I_1, new_I, new_true_board, 'x', False)
            else:
                num_histories += 1
                # print("Histories: {}\n".format(num_histories))

    else:
        for action in actions:
            new_I = I.copy()
            new_I.simulate_sense(action, true_board)
            new_true_board = true_board.copy()

            if player == 'x':
                num_histories += play(new_I, I_2, new_true_board, 'x', True)
            else:
                num_histories += play(I_1, new_I, new_true_board, 'o', True)

    return num_histories



if __name__ == "__main__":
    true_board = TicTacToeBoard(board=['o','0','0','x','x','0','o','0','x'])
    I_1 = InformationSet(player='x', board=['-','-','-','x','x','-','o','-','x'])
    I_2 = InformationSet(player='o', board=['o','0','-','x','x','-','o','-','-'])
    player = 'o'
    move_flag = True

    H = play(I_1, I_2, true_board, player, move_flag)
    print('Histories: ', H)

    # I_1_vars = []
    # I_2_vars = []
    # true_board_vars = []

    # states = I_2.get_states()
    # actions = I_2.get_actions(move_flag)
    
    # for action in actions:
    #     output_states = []
    #     for state in states:
    #         new_state = state.copy()
    #         valid = new_state.update_move(action, player)

    #         if new_state.is_over() or new_state.is_win()[0] or not valid:
    #             pass
    #         else:
    #             output_states.append(new_state)

    #     new_true_board = true_board.copy()
    #     success = new_true_board.update_move(action, player)
        
    #     if success and not new_true_board.is_win()[0] and not new_true_board.is_over():    
    #         new_I = get_information_set_from_states(output_states, player)
    #         new_I.reset_zeros()

    #         I_2_vars.append(new_I)
    #         I_1_vars.append(I_1.copy())
    #         true_board_vars.append(new_true_board)
            
    # with Pool(len(I_2_vars)) as pool:
    #     obj_list = pool.starmap(play, [(I_1_vars[i], I_2_vars[i], true_board_vars[i], 'x', False) for i in range(len(I_2_vars))])

    # -----------------------------------------------------------

    # I_1_vars = [I_1.copy(), I_1.copy(), I_1.copy(), I_1.copy()]
    # I_2_vars = [I_2.copy(), I_2.copy(), I_2.copy(), I_2.copy()]
    # true_board_vars = [true_board.copy(), true_board.copy(), true_board.copy(), true_board.copy()]

    # keys = list(I_2.sense_square_dict.keys())
    # for i in range(4):
    #     I_2_vars[i].simulate_sense(keys[i], true_board)  

    # with Pool(4) as pool:
    #     obj_list = pool.starmap(play, [(I_1_vars[0], I_2_vars[0], true_board_vars[0], player, move_flag),
    #                                     (I_1_vars[1], I_2_vars[1], true_board_vars[0], player, move_flag),
    #                                     (I_1_vars[2], I_2_vars[2], true_board_vars[0], player, move_flag),
    #                                     (I_1_vars[3], I_2_vars[3], true_board_vars[0], player, move_flag)])

    # print('Histories: ', sum(obj_list))