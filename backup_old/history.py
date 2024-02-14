from information_set_fast import InformationSet
from TicTacToe_fast import TicTacToeBoard


class History:
    """
    Parent class for RBT histories
    """
    def __init__(self, history=None):
        """
        :param history: list of actions
        """
        if self.history is None:
            self.history = []
        else:
            self.history = history

    @staticmethod
    def other_player(player):
        if player == 'x':
            return 'o'
        else:
            return 'x'

    def get_board(self):
        """ Get board for the current history.

        :return: TicTacToe object, bool (True, False), str (player 'x', 'o')
        If all update moves are played on empty squares return TicTacToe object, False, None
        Else return TicTacToe object, True, player who played on the non-empty square
        """
        true_board = TicTacToeBoard()
        curr_player = 'x'
        for action in self.history:
            if action < 9:
                if not true_board.update_move(action, curr_player):
                    return true_board, True, curr_player

                curr_player = self.other_player(curr_player)
        return true_board, False, None


class TerminalHistory(History):
    """
    Child class for terminal histories
    """
    def __init__(self, history=None):
        """
        :param history: list of actions
        """
        History.__init__(history)
        self.reward = {'x': 0, 'o': 0}

    def set_reward(self):
        """
        Set rewards for players based on the terminal history.
        :return: None
        """
        true_board, overlapping_move_flag, overlapping_move_player = self.get_board()

        if overlapping_move_flag:
            self.reward[overlapping_move_player] = -1
            self.reward[self.other_player(overlapping_move_player)] = 1
        else:
            flag, player = true_board.is_win()
            if flag:
                self.reward[player] = 1
                self.reward[self.other_player(player)] = -1


class NonTerminalHistory(History):
    """
    Child class for non terminal histories
    """
    def __init__(self, history=None):
        """
        :param history: list of actions
        """
        History.__init__(history)

    def get_information_sets(self):
        """ Get information set of both players for the current history

        :return: Two information set objects
        """
        I_1 = InformationSet(player='x')
        I_2 = InformationSet(player='o')
        true_board = TicTacToeBoard()
        curr_player = 'x'
        for action in self.history:
            if action < 9:
                if curr_player == 'x':
                    I_1.update_move(action, curr_player)
                    I_1.reset_zeros()
                else:
                    I_2.update_move(action, curr_player)
                    I_2.reset_zeros()
                true_board.update_move(action, curr_player)
                curr_player = self.other_player(curr_player)
            else:
                if curr_player == 'x':
                    I_1.simulate_sense(action, true_board)
                else:
                    I_2.simulate_sense(action, true_board)
        return I_1, I_2


def get_probability(h, h_dash, policy):
    # TODO
    pass
