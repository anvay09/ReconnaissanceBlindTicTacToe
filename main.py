from TicTacToe import TicTacToeBoard

if __name__ == '__main__':
    board = TicTacToeBoard()
    board.print_board()
    board.update_move(0, 'x')
    board.print_board()
    board.update_move(1, 'o')
    board.print_board()
    board.update_move(2, 'x')
    board.print_board()
    board.update_move(3, 'o')
    board.print_board()
    board.update_move(4, 'x')
    board.print_board()
    board.update_move(5, 'o')
    board.print_board()
    board.update_move(6, 'x')
    board.print_board()
    print(board.is_win())
    