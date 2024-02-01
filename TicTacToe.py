class TicTacToeBoard:
    def __init__(self, board = None):
        if board is None:
            self.board = bytearray('000000000', encoding = 'utf-8')
        else:
            self.board = board

    def __str__(self):
        return self.board.decode(encoding = 'utf-8')
    
    def __arr__(self):
        return [*self.__str__()]
    
    # overload [] operator
    def __getitem__(self, key):
        return self.__arr__()[key]
    
    # assign to [] operator
    def __setitem__(self, key, value):
        arr_board = self.__arr__()
        arr_board[key] = value
        self.board = self.array_to_bytearray(arr_board)

    def __eq__(self, other):
        return self.board == other.board
    
    def copy(self):
        return TicTacToeBoard(self.board)
    
    def string_to_bytearray(self, str_board):
        return str_board.encode(encoding = 'utf-8')
    
    def array_to_bytearray(self, arr_board):
        return ''.join(arr_board).encode(encoding = 'utf-8')
    
    def is_win(self): # returns a boolean (True or False) and the winner ('x or 'o' or None)
        arr_board = self.__arr__()
        for i in range(3):
            if (arr_board[3 * i] == arr_board[3 * i + 1] == arr_board[3 * i + 2] != '0'):
                return True, arr_board[3 * i]

            if (arr_board[i] == arr_board[i + 3] == arr_board[i + 6] != '0'):
                return True, arr_board[i]

        if (arr_board[0] == arr_board[4] == arr_board[8] != '0'):
            return True, arr_board[0]

        if (arr_board[2] == arr_board[4] == arr_board[6] != '0'):
            return True, arr_board[2]

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
        
    def update_move(self, square, turn):
        if self.is_valid_move(square):
            arr_board = self.__arr__()
            arr_board[square] = turn
            self.board = self.array_to_bytearray(arr_board)
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