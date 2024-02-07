from TicTacToe import TicTacToeBoard
from sympy.utilities.iterables import multiset_permutations


class InformationSet(TicTacToeBoard):
    """
    Inherit data and functions from TicTacToeBoard
    """

    def __init__(self, player, board):
        """
        :param player: x, o
        :param board: bytes object indicating the information set of player
        """
        TicTacToeBoard.__init__(self, board)
        self.sense_square_dict = {9: [0, 1, 3, 4], 10: [1, 2, 4, 5], 11: [3, 4, 6, 7], 12: [4, 5, 7, 8]}
        self.player = player

    def get_states(self):
        """ Get the states part of the information set

        :return: list of states
        """
        num_unknown_opponent_moves = self.get_number_of_unknown_opponent_moves()

        if num_unknown_opponent_moves == 0:
            return [self.board]
        else:
            output_states = []
            uncertain_ind = self.get_uncertain_squares()
            base_perm = [self.player] * num_unknown_opponent_moves + ['-'] * (
                    len(uncertain_ind) - num_unknown_opponent_moves)

            perm_itr = multiset_permutations(base_perm)
            for perm in perm_itr:
                new_state = TicTacToeBoard(board=self.board)
                for j in perm:
                    new_state[uncertain_ind[j]] = base_perm[j]

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
        board_array = self.__arr__()
        valid_moves = []
        for i in range(len(board_array)):
            if board_array[i] == '0' or board_array[i] == '-':
                valid_moves.append(i)
        return valid_moves

    def get_useful_senses(self):
        """
        :return: list of valid senses
        """
        board_array = self.__arr__()
        valid_sense = []
        for key, value in self.sense_square_dict.items():
            for i in range(len(value)):
                if board_array[i] == '-':
                    valid_sense.append(key)
                    break
        return valid_sense

    def get_number_of_unknown_opponent_moves(self):
        """
        :return: int
        """
        count_x = 0
        count_o = 0
        board_array = self.__arr__()
        for i in range(len(board_array)):
            if board_array[i] == 'x':
                count_x += 1
            if board_array[i] == 'o':
                count_o += 1
        if self.player == 'x':
            return count_x - count_o
        else:
            return count_o - count_x

    def get_uncertain_squares(self):
        """
        :return: list of uncertain squares
        """
        board_array = self.__arr__()
        uncertain_squares = []
        for i in range(len(board_array)):
            if board_array[i] == '-':
                uncertain_squares.append(i)
        return uncertain_squares


num_histories = 0

def play(I_1, I_2, true_board, player, move_flag=True): 
    global num_histories
    if player == 'x':
        I = I_1
    else:
        I = I_2
    
    actions = I.get_actions(move_flag)
    states = I.get_states()

    if move_flag:
        for action in actions:
            output_states = []
            for state in states:
                new_state = state.copy().update_move(action, player)
                if new_state.is_over():
                    num_histories += 1
                else:
                    output_states.append(new_state)

            # I = get_intersection(output_states)
            # true_board = true_board.copy().update_move(action, player)
                    
            if player == 'x':
                play(I, I_2, true_board, 'o', False)
            else:
                play(I_1, I, true_board, 'x', False)
            
    else:
        for action in actions:
            # I = simulate_sense(I, action, true_board)
            # true_board = true_board.copy()

            if player == 'x':
                play(I, I_2, true_board, 'o', True)
            else:
                play(I_1, I, true_board, 'x', True)
        