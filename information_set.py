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
        num_unknown_opponent_moves = self.get_number_of_unknown_moves()

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

    def get_number_of_unknown_moves(self):
        pass

    def get_uncertain_squares(self):
        pass
