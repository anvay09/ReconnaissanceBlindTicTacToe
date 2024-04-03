import pygame
import random
import json
import os
from random import choices

# pygame setup
pygame.init()
screen = pygame.display.set_mode((500, 530))

pygame.display.set_caption("Reconnaissance Blind Tic Tac Toe")

# global variables
clock = pygame.time.Clock()
click_delay = 500  # milliseconds
last_click_time = 0
dt = 0
running = True
turn = False
sensing = False
game_over = False
winning_line = None
winner = None
blank_screen = False
choose_player_screen = True
AI_player = None
P1_policy_name = 'P1_cfr_policy_round_120'
P1_file_name = 'data_files/{}.json'.format(P1_policy_name)
P1_stats_file = './{}_stats.json'.format(P1_policy_name)
P2_policy_name = 'P2_cfr_policy_round_120'
P2_file_name = 'data_files/{}.json'.format(P2_policy_name)
P2_stats_file = './{}_stats.json'.format(P2_policy_name)
use_policy = True
P1_policy = json.load(open(P1_file_name, 'r'))
P2_policy = json.load(open(P2_file_name, 'r'))
P1_sense_buffer = 0
P2_sense_buffer = choices([0, 1, 2, 3], list(P2_policy['---------s'].values()))[0]
stats_flag = 0

# define the colors
BOARD_COLOR = "white"
P1_COLOR = "darkgoldenrod2"
P2_COLOR = "crimson"
SENSE_COLOR = "cadetblue1"
# BG_COLOR = "paleturquoise4"
BG_COLOR = "black"

game_font = pygame.font.SysFont('American Typewriter', 36)
small_game_font = pygame.font.SysFont('American Typewriter', 24)
antialias_setting = True

board = ['0', '0', '0', '0', '0', '0', '0', '0', '0']
p1_boardview = ['0', '0', '0', '0', '0', '0', '0', '0', '0']
p2_boardview = ['-', '-', '-', '-', '-', '-', '-', '-', '-']

moves = set()
coordinates_to_board_index_map = {(100, 100): 0, (200, 100): 1, (300, 100): 2,
                                  (100, 200): 3, (200, 200): 4, (300, 200): 5,
                                  (100, 300): 6, (200, 300): 7, (300, 300): 8}

board_index_to_coordinates_map = {0: (100, 100), 1: (200, 100), 2: (300, 100),
                                  3: (100, 200), 4: (200, 200), 5: (300, 200),
                                  6: (100, 300), 7: (200, 300), 8: (300, 300)}

sense_index_to_sense_coordinates_map = {0: (100, 100), 1: (200, 100), 2: (100, 200), 3: (200, 200)}


def adjust_sense_coordinates(sense_coordinates):
    if sense_coordinates[0] >= 300:
        sense_coordinates = (sense_coordinates[0] - 100, sense_coordinates[1])
    if sense_coordinates[1] >= 300:
        sense_coordinates = (sense_coordinates[0], sense_coordinates[1] - 100)

    return sense_coordinates


def draw_cross(x_pos, y_pos, s):
    global screen
    pygame.draw.line(s, P1_COLOR, (x_pos + 25, y_pos + 25), (x_pos + 75, y_pos + 75), 10)
    pygame.draw.line(s, P1_COLOR, (x_pos + 25, y_pos + 75), (x_pos + 75, y_pos + 25), 10)
    screen.blit(s, (0, 0))


def draw_circle(x_pos, y_pos, s):
    global screen
    pygame.draw.circle(s, P2_COLOR, (x_pos + 50, y_pos + 50), 25, 10)
    screen.blit(s, (0, 0))


def draw_square(x_pos, y_pos, s):
    global screen
    pygame.draw.rect(s, SENSE_COLOR, (x_pos, y_pos, 200, 200))
    screen.blit(s, (0, 0))


def draw_shape(x_pos, y_pos, s, turn):
    if turn:
        draw_circle(x_pos, y_pos, s)
    else:
        draw_cross(x_pos, y_pos, s)


def draw_board(s):
    global moves, winning_line, turn, sensing, board, p1_boardview, p2_boardview, game_over, game_font, winner, board_index_to_coordinates_map, blank_screen, stats_flag, P1_stats_file, P2_stats_file, AI_player
    s.fill(BG_COLOR)

    if blank_screen:
        if game_over:
            blank_screen = False
        return

    pygame.draw.line(s, BOARD_COLOR, (100, 100), (100, 400), 5)
    pygame.draw.line(s, BOARD_COLOR, (200, 100), (200, 400), 5)
    pygame.draw.line(s, BOARD_COLOR, (300, 100), (300, 400), 5)
    pygame.draw.line(s, BOARD_COLOR, (400, 100), (400, 400), 5)

    pygame.draw.line(s, BOARD_COLOR, (100, 100), (400, 100), 5)
    pygame.draw.line(s, BOARD_COLOR, (100, 200), (400, 200), 5)
    pygame.draw.line(s, BOARD_COLOR, (100, 300), (400, 300), 5)
    pygame.draw.line(s, BOARD_COLOR, (100, 400), (400, 400), 5)

    if winning_line and game_over and winner is not None:
        for move in moves:
            draw_shape(move[0], move[1], s, move[2])

        if winning_line[2]:
            pygame.draw.line(s, P2_COLOR, winning_line[0], winning_line[1], 15)
        else:
            pygame.draw.line(s, P1_COLOR, winning_line[0], winning_line[1], 15)

        text = 'Player ' + str(int(winner)) + ' Wins!'
        img = game_font.render(text, antialias_setting, BOARD_COLOR, BG_COLOR)

        if not stats_flag:
            if AI_player == 1:
                stats_file = P1_stats_file
            elif AI_player == 2:
                stats_file = P2_stats_file

            if os.path.exists(stats_file):
                stats_dict = json.load(open(stats_file, 'r'))
            else:
                stats_dict = {'win': 0, 'loss': 0, 'draw': 0, 'total': 0}
            if int(winner) == 1:
                stats_dict['win'] += 1
                stats_dict['total'] += 1
            else:
                stats_dict['loss'] += 1
                stats_dict['total'] += 1

            with open(stats_file, 'w') as f:
                json.dump(stats_dict, f)
            stats_flag = 1

        s.blit(img, img.get_rect(center=(250, 50)))

    elif game_over and winner is None:
        for move in moves:
            draw_shape(move[0], move[1], s, move[2])

        text = 'Draw!'
        img = game_font.render(text, antialias_setting, BOARD_COLOR, BG_COLOR)
        
        if not stats_flag:
            if AI_player == 1:
                stats_file = P1_stats_file
            elif AI_player == 2:
                stats_file = P2_stats_file

            if os.path.exists(stats_file):
                stats_dict = json.load(open(stats_file, 'r'))
            else:
                stats_dict = {'win': 0, 'loss': 0, 'draw': 0, 'total': 0}
            stats_dict['draw'] += 1
            stats_dict['total'] += 1
            with open(stats_file, 'w') as f:
                json.dump(stats_dict, f)
            stats_flag = 1
        s.blit(img, img.get_rect(center=(250, 50)))

    elif winner is not None:
        for move in moves:
            draw_shape(move[0], move[1], s, move[2])

        text = 'Player ' + str(int(winner)) + ' Wins!'
        img = game_font.render(text, antialias_setting, BOARD_COLOR, BG_COLOR)
        
        if not stats_flag:
            if AI_player == 1:
                stats_file = P1_stats_file
            elif AI_player == 2:
                stats_file = P2_stats_file

            if os.path.exists(stats_file):
                stats_dict = json.load(open(stats_file, 'r'))
            else:
                stats_dict = {'win': 0, 'loss': 0, 'draw': 0, 'total': 0}
            if int(winner) == 1:
                stats_dict['win'] += 1
                stats_dict['total'] += 1
            else:
                stats_dict['loss'] += 1
                stats_dict['total'] += 1

            with open(stats_file, 'w') as f:
                json.dump(stats_dict, f)
            stats_flag = 1

        s.blit(img, img.get_rect(center=(250, 50)))

    else:
        for move in moves:
            ind = coordinates_to_board_index_map[(move[0], move[1])]
            if turn:
                if p2_boardview[ind] != '0' and p2_boardview[ind] != '-':
                    draw_shape(move[0], move[1], s, move[2])
            else:
                if p1_boardview[ind] != '0' and p1_boardview[ind] != '-':
                    draw_shape(move[0], move[1], s, move[2])

        if turn:
            if sensing:
                text = 'Player 2 Sense'
                img = game_font.render(text, antialias_setting, BOARD_COLOR, BG_COLOR)
            else:
                text = 'Player 2 Move'
                img= game_font.render(text, antialias_setting, BOARD_COLOR, BG_COLOR)
        else:
            if sensing:
                text = 'Player 1 Sense'
                img = game_font.render(text, antialias_setting, BOARD_COLOR, BG_COLOR)
            else:
                text = 'Player 1 Move'
                img = game_font.render(text, antialias_setting, BOARD_COLOR, BG_COLOR)
                
        s.blit(img, img.get_rect(center=(250, 50)))


def check_win():
    global winning_line, board
    if board[0] == board[1] == board[2] and board[0] != '0':
        if board[0] == 'x':
            winning_line = [(125, 150), (375, 150), False]
        else:
            winning_line = [(125, 150), (375, 150), True]
        return board[0]
    elif board[3] == board[4] == board[5] and board[3] != '0':
        if board[3] == 'x':
            winning_line = [(125, 250), (375, 250), False]
        else:
            winning_line = [(125, 250), (375, 250), True]
        return board[3]
    elif board[6] == board[7] == board[8] and board[6] != '0':
        if board[6] == 'x':
            winning_line = [(125, 350), (375, 350), False]
        else:
            winning_line = [(125, 350), (375, 350), True]
        return board[6]
    elif board[0] == board[3] == board[6] and board[0] != '0':
        if board[0] == 'x':
            winning_line = [(150, 125), (150, 375), False]
        else:
            winning_line = [(150, 125), (150, 375), True]
        return board[0]
    elif board[1] == board[4] == board[7] and board[1] != '0':
        if board[1] == 'x':
            winning_line = [(250, 125), (250, 375), False]
        else:
            winning_line = [(250, 125), (250, 375), True]
        return board[1]
    elif board[2] == board[5] == board[8] and board[2] != '0':
        if board[2] == 'x':
            winning_line = [(350, 125), (350, 375), False]
        else:
            winning_line = [(350, 125), (350, 375), True]
        return board[2]
    elif board[0] == board[4] == board[8] and board[0] != '0':
        if board[0] == 'x':
            winning_line = [(125, 125), (375, 375), False]
        else:
            winning_line = [(125, 125), (375, 375), True]
        return board[0]
    elif board[2] == board[4] == board[6] and board[2] != '0':
        if board[2] == 'x':
            winning_line = [(375, 125), (125, 375), False]
        else:
            winning_line = [(375, 125), (125, 375), True]
        return board[2]
    else:
        return False


def check_draw():
    global board
    for i in range(9):
        if board[i] == '0':
            return False
    return True


def make_move(move):
    global moves, board, p1_boardview, p2_boardview, turn, game_over, winner, blank_screen
    moves.add(move)

    ind = move[0] // 100 + 3 * (move[1] // 100 - 1) - 1
    if move[2]:
        board[ind] = 'o'
        p2_boardview[ind] = 'o'
    else:
        board[ind] = 'x'
        p1_boardview[ind] = 'x'

    win = check_win()
    draw = check_draw()
    if win == 'x':
        winner = 1
        blank_screen = False
        return 'x'
    elif win == 'o':
        winner = 2
        blank_screen = False
        return 'o'
    elif draw:
        blank_screen = False
        return 'draw'

    return False


def update_boardview(sense_coordinates):
    global board, p1_boardview, p2_boardview, turn

    if sense_coordinates == (100, 100):
        sense_region = [0, 1, 3, 4]
    elif sense_coordinates == (200, 100):
        sense_region = [1, 2, 4, 5]
    elif sense_coordinates == (100, 200):
        sense_region = [3, 4, 6, 7]
    elif sense_coordinates == (200, 200):
        sense_region = [4, 5, 7, 8]
    else:
        print('Error: Invalid sense coordinates')
        exit(1)

    if turn:
        for i in range(9):
            if i in sense_region:
                p2_boardview[i] = board[i]
            else:
                if p2_boardview[i] == '0':
                    p2_boardview[i] = '-'

    else:
        for i in range(9):
            if i in sense_region:
                p1_boardview[i] = board[i]
            else:
                if p1_boardview[i] == '0':
                    p1_boardview[i] = '-'


def in_square(x, y, square):
    top_left_corner = board_index_to_coordinates_map[square]
    if x > top_left_corner[0] and x < top_left_corner[0] + 100 and y > top_left_corner[1] and y < top_left_corner[
        1] + 100:
        return True
    else:
        return False


def return_square(x, y):
    if in_square(x, y, 0):
        return 0
    elif in_square(x, y, 1):
        return 1
    elif in_square(x, y, 2):
        return 2
    elif in_square(x, y, 3):
        return 3
    elif in_square(x, y, 4):
        return 4
    elif in_square(x, y, 5):
        return 5
    elif in_square(x, y, 6):
        return 6
    elif in_square(x, y, 7):
        return 7
    elif in_square(x, y, 8):
        return 8
    else:
        return None


def sense_action(event, last_click_time, square, surface):
    global click_delay, screen, sensing
    sense_coordinates = adjust_sense_coordinates(board_index_to_coordinates_map[square])

    if event.type == pygame.MOUSEBUTTONDOWN:
        current_time = pygame.time.get_ticks()

        if current_time - last_click_time > click_delay:
            update_boardview(sense_coordinates)
            sensing = False
            draw_board(screen)

        # Update last click time
        last_click_time = current_time
    else:
        draw_board(screen)
        draw_square(sense_coordinates[0], sense_coordinates[1], surface)

    return last_click_time


def move_action(event, last_click_time, square, surface):
    global click_delay, moves, turn, screen, winner, game_over, sensing, blank_screen
    move_coordinates = board_index_to_coordinates_map[square]

    if (move_coordinates[0], move_coordinates[1], turn) and (
            move_coordinates[0], move_coordinates[1], not turn) not in moves:
        if event.type == pygame.MOUSEBUTTONDOWN:
            current_time = pygame.time.get_ticks()
            if current_time - last_click_time > click_delay:
                game_over = make_move((move_coordinates[0], move_coordinates[1], turn))
                turn = not turn
                sensing = True
                blank_screen = True
                draw_board(screen)

            # Update last click time
            last_click_time = current_time
        else:
            draw_board(screen)
            draw_shape(move_coordinates[0], move_coordinates[1], surface, turn)
    else:
        ind = coordinates_to_board_index_map[move_coordinates]
        if turn:
            if p2_boardview[ind] == '0' or p2_boardview[ind] == '-':
                if event.type == pygame.MOUSEBUTTONDOWN:
                    current_time = pygame.time.get_ticks()
                    if current_time - last_click_time > click_delay:
                        game_over = True
                        winner = 1
                else:
                    draw_board(screen)
                    draw_shape(move_coordinates[0], move_coordinates[1], surface, turn)
        else:
            if p1_boardview[ind] == '0' or p1_boardview[ind] == '-':
                if event.type == pygame.MOUSEBUTTONDOWN:
                    current_time = pygame.time.get_ticks()
                    if current_time - last_click_time > click_delay:
                        game_over = True
                        winner = 2
                else:
                    draw_board(screen)
                    draw_shape(move_coordinates[0], move_coordinates[1], surface, turn)

    return last_click_time


# game loop
while running:
    # poll for events
    # pygame.QUIT event means the user clicked X to close your window
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False

        if event.type == pygame.KEYDOWN:
            if event.key == pygame.K_RETURN:
                # toggle blank_screen when enter pressed
                blank_screen = False
            elif event.key == pygame.K_y:
                # reset game when y pressed
                board = ['0', '0', '0', '0', '0', '0', '0', '0', '0']
                p1_boardview = ['0', '0', '0', '0', '0', '0', '0', '0', '0']
                p2_boardview = ['-', '-', '-', '-', '-', '-', '-', '-', '-']
                moves = set()
                turn = False
                sensing = False
                game_over = False
                winning_line = None
                winner = None
                blank_screen = False
                stats_flag = 0
                P1_sense_buffer = 0
                P2_sense_buffer = choices([0, 1, 2, 3], list(P2_policy['---------s'].values()))[0]
                choose_player_screen = True
                AI_player = None
            elif event.key == pygame.K_n:
                # quit game when n pressed
                running = False

    x = pygame.mouse.get_pos()[0]
    y = pygame.mouse.get_pos()[1]
    # create a surface to draw on of the same size as the screen
    surface = pygame.Surface((500, 530))
    surface.set_alpha(100)

    if choose_player_screen:
        screen.fill(BG_COLOR)
        draw_circle(325, 250, screen)
        draw_cross(75, 250, screen)
        # show message "Choose player: 1/2"
        text = 'Choose player'
        img = game_font.render(text, antialias_setting, BOARD_COLOR, BG_COLOR)
        screen.blit(img, img.get_rect(center=(250, 150)))
       
        if x < 250 and y > 0 and y < 530:
            pygame.draw.rect(surface, SENSE_COLOR, (0, 0, 250, 530))
            screen.blit(surface, (0, 0))
        elif x > 250 and y > 0 and y < 530:
            pygame.draw.rect(surface, SENSE_COLOR, (250, 0, 250, 530))
            screen.blit(surface, (0, 0))

        if event.type == pygame.MOUSEBUTTONDOWN:
            current_time = pygame.time.get_ticks()

            if current_time - last_click_time > click_delay:
                if x < 250:
                    AI_player = 2 # Player 2 is AI
                else:
                    AI_player = 1 # Player 1 is AI
                choose_player_screen = False

            # Update last click time
            last_click_time = current_time
        
    elif blank_screen:
        # waiting animation just because it looks cool
        draw_board(screen)
        for i in range(8):
            pygame.draw.arc(screen, BOARD_COLOR, (220, 220, 60, 60), 0.33 * i * 3.14, 0.33 * (i + 1) * 3.14, 8)
            pygame.display.flip()
            pygame.time.wait(10)
            pygame.draw.arc(screen, BG_COLOR, (220, 220, 60, 60), 0.33 * i * 3.14, 0.33 * (i + 1) * 3.14, 8)

        blank_screen = False

    elif not game_over:
        if use_policy and not turn and AI_player == 1:
            if sensing:
                sense_coordinates = sense_index_to_sense_coordinates_map[P1_sense_buffer]
                update_boardview(sense_coordinates)
                sensing = False
            else:
                P1_information_set = ''.join(p1_boardview) + 'm'
                if P1_information_set not in P1_policy.keys():
                    print('Error: Invalid information set:', P1_information_set)
                    exit(1)
                # choose a move uniformly at random from P1_policy[P1_information_set]
                available_plays = P1_policy[P1_information_set]
                random_number = random.uniform(0, 1)
                sum = 0
                for key, val in available_plays.items():
                    sum += available_plays[key]
                    if random_number <= sum:
                        chosen_play = key
                        break
                move_coordinates = board_index_to_coordinates_map[int(chosen_play)]
                P1_information_set_next = ['-' for i in range(len(P1_information_set))]
                for i in range(len(P1_information_set)):
                    if i == int(chosen_play):
                        P1_information_set_next[i] = 'x'
                    else:
                        if P1_information_set[i] == 'x' or P1_information_set[i] == 'o':
                            P1_information_set_next[i] = P1_information_set[i]
                P1_information_set_next[-1] = 's'
                P1_information_set_next = ''.join(P1_information_set_next)

                if P1_information_set_next in P1_policy.keys():
                    available_plays = P1_policy[P1_information_set_next]
                    random_number = random.uniform(0, 1)
                    sum = 0
                    for key, val in available_plays.items():
                        sum += available_plays[key]
                        if random_number <= sum:
                            chosen_play = key
                            break

                    P1_sense_buffer = int(chosen_play) - 9
                else:
                    P1_sense_buffer = 0

                # print(P1_information_set, chosen_play[0], chosen_play[1])

                if (move_coordinates[0], move_coordinates[1], turn) and (
                        move_coordinates[0], move_coordinates[1], not turn) not in moves:
                    game_over = make_move((move_coordinates[0], move_coordinates[1], turn))
                    turn = not turn
                    sensing = True
                    blank_screen = True
                    draw_board(screen)
                else:
                    ind = coordinates_to_board_index_map[move_coordinates]
                    if p1_boardview[ind] == '0' or p1_boardview[ind] == '-':
                        game_over = True
                        winner = 2

        elif use_policy and turn and AI_player == 2:
            if sensing:
                sense_coordinates = sense_index_to_sense_coordinates_map[P2_sense_buffer]
                update_boardview(sense_coordinates)
                sensing = False
            else:
                P2_information_set = ''.join(p2_boardview) + 'm'
                if P2_information_set not in P2_policy.keys():
                    print('Error: Invalid information set:', P2_information_set)
                    exit(1)
                # choose a move uniformly at random from P2_policy[P2_information_set]
                available_plays = P2_policy[P2_information_set]
                random_number = random.uniform(0, 1)
                sum = 0
                for key, val in available_plays.items():
                    sum += available_plays[key]
                    if random_number <= sum:
                        chosen_play = key
                        break
                move_coordinates = board_index_to_coordinates_map[int(chosen_play)]
                P2_information_set_next = ['-' for i in range(len(P2_information_set))]
                for i in range(len(P2_information_set)):
                    if i == int(chosen_play):
                        P2_information_set_next[i] = 'o'
                    else:
                        if P2_information_set[i] == 'x' or P2_information_set[i] == 'o':
                            P2_information_set_next[i] = P2_information_set[i]
                P2_information_set_next[-1] = 's'
                P2_information_set_next = ''.join(P2_information_set_next)

                if P2_information_set_next in P2_policy.keys():
                    available_plays = P2_policy[P2_information_set_next]
                    random_number = random.uniform(0, 1)
                    sum = 0
                    for key, val in available_plays.items():
                        sum += available_plays[key]
                        if random_number <= sum:
                            chosen_play = key
                            break

                    P2_sense_buffer = int(chosen_play) - 9
                else:
                    P2_sense_buffer = 0

                # print(P1_information_set, chosen_play[0], chosen_play[1])

                if (move_coordinates[0], move_coordinates[1], turn) and (
                        move_coordinates[0], move_coordinates[1], not turn) not in moves:
                    game_over = make_move((move_coordinates[0], move_coordinates[1], turn))
                    turn = not turn
                    sensing = True
                    blank_screen = True
                    draw_board(screen)
                else:
                    ind = coordinates_to_board_index_map[move_coordinates]
                    if p2_boardview[ind] == '0' or p2_boardview[ind] == '-':
                        game_over = True
                        winner = 1

        else:
            square = return_square(x, y)
            if square is None:
                draw_board(screen)
            else:
                if sensing:
                    last_click_time = sense_action(event, last_click_time, square, surface)
                else:
                    last_click_time = move_action(event, last_click_time, square, surface)
    else:
        draw_board(screen)
        # show message "Play again? y/n"
        text = 'Play again? y/n'
        img = game_font.render(text, antialias_setting, BOARD_COLOR, BG_COLOR)

        if AI_player == 1:
            if os.path.exists(P1_stats_file):
                stats_dict = json.load(open(P1_stats_file, 'r'))
                text = 'Wins: ' + str(stats_dict['loss']) + '\t\tLosses: ' + str(stats_dict['win']) + '\t\tDraws: ' + str(stats_dict['draw']) + '\t\tTotal: ' + str(stats_dict['total'])
                img2 = small_game_font.render(text, antialias_setting, BOARD_COLOR, BG_COLOR)
        elif AI_player == 2:
            if os.path.exists(P2_stats_file):
                stats_dict = json.load(open(P2_stats_file, 'r'))
                text = 'Wins: ' + str(stats_dict['win']) + '\t\tLosses: ' + str(stats_dict['loss']) + '\t\tDraws: ' + str(stats_dict['draw']) + '\t\tTotal: ' + str(stats_dict['total'])
                img2 = small_game_font.render(text, antialias_setting, BOARD_COLOR, BG_COLOR)

        screen.blit(img, img.get_rect(center=(250, 440)))
        screen.blit(img2, img2.get_rect(center=(250, 490)))
    # flip() the display to put your work on screen
    pygame.display.flip()

    # limits FPS to 60
    # dt is delta time in seconds since last frame, used for framerate-
    # independent physics.
    dt = clock.tick(60) / 1000

pygame.quit()
