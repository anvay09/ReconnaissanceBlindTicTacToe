from TicTacToe import TicTacToeBoard


class InformationSet(TicTacToeBoard):
    """
    Inherit data and functions from TicTacToeBoard
    """

    def __init__(self, player, board):
        """

        :param player:
        :param board:
        """
        TicTacToeBoard.__init__(self, board)
        self.sense_square_dict = {9: [0, 1, 3, 4], 10: [1, 2, 4, 5], 11: [3, 4, 6, 7], 12: [4, 5, 7, 8]}
        self.player = player

    def get_states(self):
        pass

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
