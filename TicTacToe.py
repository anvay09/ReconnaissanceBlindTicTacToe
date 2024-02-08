class TicTacToeBoard:
    def __init__(self, board=None):
        if board is None:
            self.board = bytearray('000000000', encoding='utf-8')
        else:
            self.board = board

    def __str__(self):
        return self.board.decode(encoding='utf-8')

    def __arr__(self):
        return [*self.__str__()]

    # overload [] operator
    def __getitem__(self, key):
        return self.__arr__()[key]

    # assign to [] operator
    def __setitem__(self, key, value):
        board_arr = self.__arr__()
        board_arr[key] = value
        self.board = self.array_to_bytearray(board_arr)

    def __eq__(self, other):
        return self.board == other.board

    def copy(self):
        return TicTacToeBoard(self.board)

    def string_to_bytearray(self, board_str):
        return board_str.encode(encoding='utf-8')

    def array_to_bytearray(self, board_arr):
        return ''.join(board_arr).encode(encoding='utf-8')

    def is_win(self):  # returns a boolean (True or False) and the winner ('x or 'o' or None)
        board_arr = self.__arr__()
        for i in range(3):
            if (board_arr[3 * i] == board_arr[3 * i + 1] == board_arr[3 * i + 2] != '0'):
                return True, board_arr[3 * i]

            if (board_arr[i] == board_arr[i + 3] == board_arr[i + 6] != '0'):
                return True, board_arr[i]

        if (board_arr[0] == board_arr[4] == board_arr[8] != '0'):
            return True, board_arr[0]

        if (board_arr[2] == board_arr[4] == board_arr[6] != '0'):
            return True, board_arr[2]

        return False, None

    def is_over(self):
        for i in range(9):
            if self.board[i] == ord('0'):
                return False

    def is_draw(self):
        if not self.is_win() and self.is_over():
            return True
        return False

    def is_valid_move(self, square):
        if square < 0 or square > 8:
            return False
        else:
            return self.board[square] == ord('0')

    def update_move(self, square, player):
        if self.is_valid_move(square):
            board_arr = self.__arr__()
            board_arr[square] = player
            self.board = self.array_to_bytearray(board_arr)
            return True
        return False

    def print_board(self):
        print("+---+---+---+")
        board = self.__arr__()
        for i in range(3):
            print("|", end="")
            for j in range(3):
                if board[3 * i + j] == 'x':
                    print(" x ", end="|")
                elif board[3 * i + j] == 'o':
                    print(" o ", end="|")
                else:
                    print(" 0 ", end="|")
            print("\n+---+---+---+")
