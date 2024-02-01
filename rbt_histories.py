from tqdm import tqdm
from TicTacToe import *

#@title Functions

def make_legal_moves(player_board, other_player_board, depth, turn) -> list:
    output_boards = []
    output_other_player_boards = []
    terminated_games = []

    true_board = get_true_board(player_board.__arr__(), other_player_board.__arr__())

    if depth == 0 and turn == 'x':
        # squares = [0, 1, 4]
        squares = [4]
    elif player_board[4] == 'x' and turn == 'o' and depth == 1:
        squares = [0, 1]
    elif player_board[0] == 'x' and turn == 'o' and depth == 1:
        squares = [1, 2, 4, 5, 8]
    elif player_board[1] == 'x' and turn == 'o' and depth == 1:
        squares = [0, 3, 4, 6, 7]
    else:
        squares = [i for i in range(9)]

    w_e, sq1 = player_board.win_exists(turn)
    o_w_e, sq2 = player_board.opponent_win_exists(turn)
    if w_e:
        squares = [sq1]
    elif o_w_e:
        squares = [sq2]

    for i in squares:
        if player_board[i] == '0' or player_board[i] == '-':
            new_board = player_board.copy()
            new_true_board = copy(true_board)
            new_other_player_board = other_player_board.copy()

            if true_board[i] != '0' and true_board[i] != '-':
                temp_board = new_board.copy()
                temp_board.add_to_history(i)
                terminated_games.append(temp_board)
            else:
                new_board.make_move(i, turn)
                make_move(new_true_board, i, turn)

                if is_win(new_true_board)[0] or is_draw(new_true_board):
                    terminated_games.append(new_board)
                else:
                    output_boards.append(new_board)
                    output_other_player_boards.append(new_other_player_board)

    return output_boards, output_other_player_boards, terminated_games

def simulate_sense(board, other_player_board) -> list:
    sense_regions = {0: [0, 1, 3, 4],
                     1: [1, 2, 4, 5],
                     2: [3, 4, 6, 7],
                     3: [4, 5, 7, 8]}

    output_boards = []
    output_other_player_boards = []

    true_board = get_true_board(board.__arr__(), other_player_board.__arr__())

    for sense in range(4):
        sense_region = sense_regions[sense]
        if (board[sense_region[0]] == true_board[sense_region[0]] and board[sense_region[0]] != '0' and board[sense_region[0]] != '-' and
            board[sense_region[1]] == true_board[sense_region[1]] and board[sense_region[1]] != '0' and board[sense_region[0]] != '-' and
            board[sense_region[2]] == true_board[sense_region[2]] and board[sense_region[2]] != '0' and board[sense_region[0]] != '-' and
            board[sense_region[3]] == true_board[sense_region[3]] and board[sense_region[3]] != '0' and board[sense_region[0]] != '-'):
            continue
        else:
            new_board = board.copy()
            new_other_player_board = other_player_board.copy()

            for square in range(9):
                if square not in sense_region:
                    if new_board[square] == '0':
                        new_board[square] = '-'
                else:
                    if true_board[square] == '-':
                        new_board[square] = '0'
                    else:
                        new_board[square] = true_board[square]
            
            new_board.history += str(sense)

            output_boards.append(new_board)
            output_other_player_boards.append(new_other_player_board)

    return output_boards, output_other_player_boards

def play_moves(player_boards, other_player_boards, depth, turn):
    new_player_boards = []
    new_other_player_boards = []
    terminated_games = []
    
    for i in tqdm(range(len(player_boards))):
        p1, p2, t = make_legal_moves(player_boards[i], other_player_boards[i], depth, turn)
        new_player_boards += p1
        new_other_player_boards += p2
        terminated_games += t

    return new_player_boards, new_other_player_boards, terminated_games

def sense_boards(player_boards, other_player_boards):
    new_player_boards = []
    new_other_player_boards = []

    for i in tqdm(range(len(player_boards))):
        p1, p2 = simulate_sense(player_boards[i], other_player_boards[i])
        new_player_boards += p1
        new_other_player_boards += p2

    return new_player_boards, new_other_player_boards

def count_duplicate_boards(boards, depth):
    board_dict = {}
    for board in tqdm(boards):
        board = convert_to_array(board)
        
        perf, new_board = has_perfect_information(board, depth)  

        if perf:
            new_board = ''.join(new_board)
            
            if new_board in board_dict.keys():
                board_dict[new_board] += 1
            else:
                # print("Perfect information board: ", new_board)
                board_dict[new_board] = 1
        else:
            board = ''.join(board)
            
            if board in board_dict.keys():
                board_dict[board] += 1
            else:
                # print("Non-perfect information board: ", board)
                board_dict[board] = 1

    return len(board_dict.keys())

def count_unique_histories(boards):
    history_dict = {}
    for board in tqdm(boards):
        if board.history in history_dict.keys():
            history_dict[board.history] += 1
        else:
            # print("New history: ", board.history)
            history_dict[board.history] = 1

    return len(history_dict.keys())

#@title Simulate Games

def simulate_games(depth):
    game_count = 0
    player1_boards = [TicTacToeBoard()]
    player2_boards = [TicTacToeBoard()]

    # make first move
    player1_boards, player2_boards, _ = make_legal_moves(player1_boards[0], player2_boards[0], 0, 'x')

    for d in range(int((depth-1)/2)):
        # player 2 senses and gets observations
        print("Player 2 sensing at depth ", 2*d + 1)
        player2_boards, player1_boards = sense_boards(player2_boards, player1_boards)
        print("Number of unique histories (P2): ", count_unique_histories(player2_boards))
        
        # player 2 makes a move
        print("Player 2 making move at depth ", 2*d + 1)
        player2_boards, player1_boards, terminated_games = play_moves(player2_boards, player1_boards, 2*d + 1, 'o')
        print("Number of unique histories (P2): ", count_unique_histories(player2_boards + terminated_games))
        game_count += len(terminated_games)
        print("Terminated games: ", game_count)
        
        # player 1 senses and gets observations
        print("Player 1 sensing at depth ", 2*d + 2)
        player1_boards, player2_boards = sense_boards(player1_boards, player2_boards)
        print("Number of unique histories (P1): ", count_unique_histories(player1_boards))

        # player 1 makes a move
        print("Player 1 making move at depth ", 2*d + 2)
        player1_boards, player2_boards, terminated_games = play_moves(player1_boards, player2_boards, 2*d + 2, 'x')
        print("Number of unique histories (P1): ", count_unique_histories(player1_boards + terminated_games))
        game_count += len(terminated_games)
        print("Terminated games: ", game_count)

    game_count += len(player1_boards)
    print("Final game count:", game_count)

simulate_games(9)