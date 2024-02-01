
#@title TicTacToe Board Functions

class TicTacToeBoard:
    def __init__(self, board = None):
        if board is None:
            self.board = create_board()
            # self.history = history
        else:
            self.board = board
            # self.history = history

    def __str__(self):
        return self.board.decode(encoding = 'utf-8')
    
    def __arr__(self):
        return convert_to_array(self.board)
    
    # def add_to_history(self, move):
    #     self.history += str(move)

    def make_move(self, move, turn):
        self.board = convert_to_bytearray(make_move(self.__arr__(), move, turn))
        # self.add_to_history(move)
        return self.board
    
    def is_win(self):
        return is_win(self.__arr__())
    
    def is_draw(self):
        return is_draw(self.__arr__())
    
    def is_over(self):
        return is_over(self.__arr__())
    
    def copy(self):
        return TicTacToeBoard(self.board)
    
    def win_exists(self, turn):
        return win_exists(self.__arr__(), turn)
    
    def opponent_win_exists(self, turn):
        return opponent_win_exists(self.__arr__(), turn)
    
    def has_perfect_information(self):
        return has_perfect_information(self.__arr__())
    
    # overload [] operator
    def __getitem__(self, key):
        return self.__arr__()[key]
    
    # assign to [] operator
    def __setitem__(self, key, value):
        arr = self.__arr__()
        arr[key] = value
        self.board = convert_to_bytearray(arr)

    def __eq__(self, other):
        return self.board == other.board

#---------------------------------------------#    

def convert_to_bytearray(board):
    board_str = ''
    for i in range(9):
        board_str += board[i]

    board_str = board_str.encode(encoding = 'utf-8')
    return board_str

def convert_to_array(board):
    board_str = board.decode(encoding = 'utf-8')

    board = []
    for c in board_str:

        board.append(c)

    return board

def create_board():
    # board = ['-'] * 9
    board = bytearray('---------', encoding = 'utf-8')
    return board

def toggle_turn(turn):
    if turn == 'x':
        return 'o'
    elif turn == 'o':
        return 'x'

def print_board(board):
    board = convert_to_array(board)
    print("+---+---+---+")
    for i in range(3):
        print("|", end="")
        for j in range(3):
            if board[3 * i + j] == 'x':
                print(" x ", end="|")
            elif board[3 * i + j] == 'o':
                print(" o ", end="|")
            elif board[3 * i + j] == '-':
                print(" - ", end="|")
            else:
                print(" 0 ", end="|")
        print("\n+---+---+---+")

def make_move(board, move, turn): # expects decoded board
    if ((board[move] != '-') and
        (board[move] != '0')):
        return None
    board[move] = turn

    return board

def is_win(board): # expects decoded board
    for i in range(3):
        if ((board[3 * i] == board[3 * i + 1] == board[3 * i + 2] != '0') and
            (board[3 * i] == board[3 * i + 1] == board[3 * i + 2] != '-')):
            return True, board[3 * i]

        if ((board[i] == board[i + 3] == board[i + 6] != '0') and
            (board[i] == board[i + 3] == board[i + 6] != '-')):
            return True, board[i]

    if ((board[0] == board[4] == board[8] != '0') and
        (board[0] == board[4] == board[8] != '-')):
        return True, board[0]

    if ((board[2] == board[4] == board[6] != '0') and
        (board[2] == board[4] == board[6] != '-')):
        return True, board[2]

    return False, None

def is_draw(board): # expects decoded board
    win, _ = is_win(board)

    if win:
        return False
    for i in range(9):
        if board[i] == '0':
            return False
        elif board[i] == '-':
            return False
    return True

def is_over(board): # expects decoded board
    for i in range(9):
        if board[i] == '0':
            return False
        elif board[i] == '-':
            return False
    return True

def copy(board): # expects decoded board
    new_board = board.copy()
    return new_board

def win_exists(board, turn): # expects decoded board
    if (board[0] == '0' and board[1] == turn and board[2] == turn):
        return True, 0
    elif (board[0] == turn and board[1] == '0' and board[2] == turn):
        return True, 1
    elif (board[0] == turn and board[1] == turn and board[2] == '0'):
        return True, 2
    elif (board[3] == '0' and board[4] == turn and board[5] == turn):
        return True, 3
    elif (board[3] == turn and board[4] == '0' and board[5] == turn):
        return True, 4
    elif (board[3] == turn and board[4] == turn and board[5] == '0'):
        return True, 5
    elif (board[6] == '0' and board[7] == turn and board[8] == turn):
        return True, 6
    elif (board[6] == turn and board[7] == '0' and board[8] == turn):
        return True, 7
    elif (board[6] == turn and board[7] == turn and board[8] == '0'):
        return True, 8
    elif (board[0] == '0' and board[3] == turn and board[6] == turn):
        return True, 0
    elif (board[0] == turn and board[3] == '0' and board[6] == turn):
        return True, 3
    elif (board[0] == turn and board[3] == turn and board[6] == '0'):
        return True, 6
    elif (board[1] == '0' and board[4] == turn and board[7] == turn):
        return True, 1
    elif (board[1] == turn and board[4] == '0' and board[7] == turn):
        return True, 4
    elif (board[1] == turn and board[4] == turn and board[7] == '0'):
        return True, 7
    elif (board[2] == '0' and board[5] == turn and board[8] == turn):
        return True, 2
    elif (board[2] == turn and board[5] == '0' and board[8] == turn):
        return True, 5
    elif (board[2] == turn and board[5] == turn and board[8] == '0'):
        return True, 8
    elif (board[0] == '0' and board[4] == turn and board[8] == turn):
        return True, 0
    elif (board[0] == turn and board[4] == '0' and board[8] == turn):
        return True, 4
    elif (board[0] == turn and board[4] == turn and board[8] == '0'):
        return True, 8
    elif (board[2] == '0' and board[4] == turn and board[6] == turn):
        return True, 0
    elif (board[2] == turn and board[4] == '0' and board[6] == turn):
        return True, 4
    elif (board[2] == turn and board[4] == turn and board[6] == '0'):
        return True, 6
    else:
        return False, -1

def get_true_board(player_board, other_player_board): # expects decoded board
    true_board = create_board()
    true_board = convert_to_array(true_board)

    for square in range(9):
        if player_board[square] == other_player_board[square]:
            if player_board[square] == '0':
                true_board[square] = '-'
            else:
                true_board[square] = player_board[square]
        elif player_board[square] == 'x' or other_player_board[square] == 'x':
            true_board[square] = 'x'
        elif player_board[square] == 'o' or other_player_board[square] == 'o':
            true_board[square] = 'o'
        else:
            true_board[square] = '-'

    return true_board

def opponent_win_exists(board, turn): # expects decoded board
    w_e, sq = win_exists(board, toggle_turn(turn))
    if w_e:
        return w_e, sq
    else:
        return False, -1

def has_perfect_information(board, depth): # expects decoded board
    play_count = 0
    board = copy(board)

    for i in range(9):
        if board[i] == 'x' or board[i] == 'o':
            play_count += 1
        else:
            board[i] = '0'
    
    if play_count == depth:
        return True, board
    else:
        return False, None
    

    # 478,240,896 at depth 5