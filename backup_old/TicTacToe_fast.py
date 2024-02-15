class TicTacToeBoard:
    def __init__(self, board=None):
        if board is None:
            self.board = ['0', '0', '0', '0', '0', '0', '0', '0', '0']
        else:
            self.board = board.copy()

    # overload [] operator
    def __getitem__(self, key):
        return self.board[key]

    # assign to [] operator
    def __setitem__(self, key, value):
        self.board[key] = value

    def __eq__(self, other):
        return self.board == other.board

    def copy(self):
        return TicTacToeBoard(self.board)

    def is_win(self):  # returns a boolean (True or False) and the winner ('x or 'o' or None)
        for i in range(3):
            if (self.board[3 * i] == self.board[3 * i + 1] == self.board[3 * i + 2] != '0'):
                return True, self.board[3 * i]

            if (self.board[i] == self.board[i + 3] == self.board[i + 6] != '0'):
                return True, self.board[i]

        if (self.board[0] == self.board[4] == self.board[8] != '0'):
            return True, self.board[0]

        if (self.board[2] == self.board[4] == self.board[6] != '0'):
            return True, self.board[2]

        return False, None

    def is_over(self):
        for i in range(9):
            if self.board[i] == '0':
                return False
        return True

    def is_draw(self):
        if not self.is_win() and self.is_over():
            return True
        return False

    def is_valid_move(self, square):
        if square < 0 or square > 8:
            return False
        else:
            return self.board[square] == '0'

    def update_move(self, square, player):
        if self.is_valid_move(square):
            self.board[square] = player
            return True
        return False

    def print_board(self):
        print("+---+---+---+")
        for i in range(3):
            print("|", end="")
            for j in range(3):
                if self.board[3 * i + j] == 'x':
                    print(" x ", end="|")
                elif self.board[3 * i + j] == 'o':
                    print(" o ", end="|")
                else:
                    print(" 0 ", end="|")
            print("\n+---+---+---+")
