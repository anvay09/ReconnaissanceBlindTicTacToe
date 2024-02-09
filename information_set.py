from TicTacToe import TicTacToeBoard
from sympy.utilities.iterables import multiset_permutations


class InformationSet(TicTacToeBoard):
    """
    Inherit data and functions from TicTacToeBoard
    """

    def __init__(self, player, board=None):
        """
        :param player: str 'x' or 'o'
        :param board: bytes object representing the information set of player
        """
        TicTacToeBoard.__init__(self, board)
        # map squares to sense moves
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
        # states do not contain '-', so create a new board array and replace '-' with '0' which indicates empty.
        board_arr = self.__arr__()
        for i in range(len(board_arr)):
            if board_arr[i] == '-':
                board_arr[i] = '0'
        board_byte = self.array_to_bytearray(board_arr)

        if num_unknown_opponent_moves == 0:
            return [TicTacToeBoard(board_byte)]
        else:
            # generate states, where for each state consider a permutation of 'num_unknown_opponent_moves' of opponent
            # marks and "num_uncertain_squares" - 'num_unknown_opponent_moves' empty squares. Use this permuatation to
            # set the values of uncertain sqaures
            output_states = []
            uncertain_ind = self.get_uncertain_squares()
            base_perm = [self.other_player()] * num_unknown_opponent_moves + ['0'] * (
                    len(uncertain_ind) - num_unknown_opponent_moves)

            perm_itr = multiset_permutations(base_perm)
            for perm in perm_itr:
                new_state = TicTacToeBoard(board=board_byte)
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
        valid_moves = []
        for i in range(len(self.board)):
            if self.board[i] == ord('0') or self.board[i] == ord('-'):
                valid_moves.append(i)
        return valid_moves

    def get_useful_senses(self):
        """
        :return: list of valid senses
        """
        valid_sense = []
        for key, value in self.sense_square_dict.items():
            for i in range(len(value)):
                if self.board[value[i]] == ord('-'):
                    valid_sense.append(key)
                    break
        return valid_sense

    def get_number_of_unknown_opponent_moves(self):
        """ Given the current information set for a player, determine the number of unknown opponents moves.
        :return: int
        """
        count_x = 0
        count_o = 0
        for i in range(len(self.board)):
            if self.board[i] == ord('x'):
                count_x += 1
            if self.board[i] == ord('o'):
                count_o += 1
        if self.player == 'x':
            return count_x - count_o
        else:
            return count_o - count_x + 1

    def get_uncertain_squares(self):
        """
        :return: list of uncertain squares (squares containing '-')
        """
        uncertain_squares = []
        for i in range(len(self.board)):
            if self.board[i] == ord('-'):
                uncertain_squares.append(i)
        return uncertain_squares

    def simulate_sense(self, action, true_board):
        """ Simulate the sense move to uncover the board and update the information set.

        :param action: sense action int-9, 10, 11 or 12
        :param true_board: current true board
        :return: None (update information set inplace)
        """
        self.reset_zeros()
        for square in self.sense_square_dict[action]:
            board_arr = self.__arr__()
            board_arr[square] = true_board[square]
            self.board = self.array_to_bytearray(board_arr)

    def reset_zeros(self):
        """ Reset the '0', known empty squares to '-' uncertain squares. This is used after a player's sense move to
        reset it's information set.

        :return: None (update information set inplace)
        """
        for i in range(len(self.board)):
            if self.board[i] == ord('0'):
                board_arr = self.__arr__()
                board_arr[i] = '-'
                self.board = self.array_to_bytearray(board_arr)


num_histories = 0


def get_information_set_from_states(states, player):
    """ Given a list of states, get the corresponding information set
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


def play(I_1, I_2, true_board, player, move_flag=True):
    """
    :param I_1:
    :param I_2:
    :param true_board:
    :param player:
    :param move_flag:
    :return:
    """
    global num_histories
    if player == 'x':
        I = I_1
    else:
        I = I_2

    actions = I.get_actions(move_flag)

    if move_flag:
        states = I.get_states()

        for action in actions:
            output_states = []
            for state in states:
                new_state = state.copy()
                valid = new_state.update_move(action, player)

                if new_state.is_over() or new_state.is_win()[0] or not valid:
                    pass  # TO-DO re-think if the following is needed num_histories += 1
                else:
                    output_states.append(new_state)

            new_true_board = true_board.copy()
            success = new_true_board.update_move(action, player)

            if success and not new_true_board.is_win()[0] and not new_true_board.is_over():
                new_I = get_information_set_from_states(output_states, player)
                new_I.reset_zeros()

                if player == 'x':
                    play(new_I, I_2, new_true_board, 'o', False)
                else:
                    play(I_1, new_I, new_true_board, 'x', False)
            else:
                num_histories += 1
                print("Histories: {}\n".format(num_histories))

    else:
        for action in actions:
            new_I = I.copy()
            new_I.simulate_sense(action, true_board)
            new_true_board = true_board.copy()

            if player == 'x':
                play(new_I, I_2, new_true_board, 'x', True)
            else:
                play(I_1, new_I, new_true_board, 'o', True)


if __name__ == "__main__":
    true_board = TicTacToeBoard(board=bytearray('ox00x0000', encoding='utf-8'))
    I_1 = InformationSet(player='x', board=bytearray('ox--x----', encoding='utf-8'))
    I_2 = InformationSet(player='o', board=bytearray('o---x----', encoding='utf-8'))
    player = 'o'
    move_flag = False

    play(I_1, I_2, true_board, player, move_flag)
