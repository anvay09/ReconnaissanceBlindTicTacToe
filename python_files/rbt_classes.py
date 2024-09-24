import copy

class TicToeToeBoard:
    def __init__(self, board = None):
        self.board = board if board is not None else ['0'] * 9

    def __getitem__(self, key):
        return self.board[key]

    def __setitem__(self, key, value):
        self.board[key] = value

    def __eq__(self, other):
        return self.board == other.board

    def copy(self):
        return copy.deepcopy(self)

    def is_win(self):
        for i in range(3):
            if (self.board[3 * i] == self.board[3 * i + 1] and self.board[3 * i + 1] == self.board[3 * i + 2] and self.board[3 * i] != '0'):
                return True

            if (self.board[i] == self.board[i + 3] and self.board[i + 3] == self.board[i + 6] and self.board[i] != '0'):
                return True

        if ((self.board[0] == self.board[4] and self.board[4] == self.board[8] and self.board[0] != '0')):
            return True

        if ((self.board[2] == self.board[4] and self.board[4] == self.board[6] and self.board[2] != '0')):
            return True

        return False

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
            print("|", end = "")
            for j in range(3):
                if self.board[3 * i + j] == 'x':
                    print(" x |", end = "")

                elif self.board[3 * i + j] == 'o':
                    print(" o |", end = "")

                else:
                    print("   |", end = "")
            print("\n+---+---+---+")


P1_hash_to_int_map = {}
P2_hash_to_int_map = {}
sense_square_dict = {9: [0, 1, 3, 4], 10: [1, 2, 4, 5], 11: [3, 4, 6, 7], 12: [4, 5, 7, 8]}
sense_square_mapping = {9: "0", 10: "1", 11: "2", 12: "3"}

class InformationSet(TicToeToeBoard):
    def __init__(self, player, move_flag, hash, board = ['0', '0', '0', '0', '0', '0', '0', '0', '0'], index = -1):
        self.player = player
        self.move_flag = move_flag
        self.hash = hash
        self.board = board
        self.index = index

        self.board = self.get_board_from_hash()

        if player == 'x':
            if hash not in P1_hash_to_int_map:
                self.index = -1
            else:
                self.index = P1_hash_to_int_map[hash]
        else:
            if hash not in P2_hash_to_int_map:
                self.index = -1
            else:
                self.index = P2_hash_to_int_map[hash]

    def __eq__(self, other):
        return self.hash == other.hash and self.player == other.player and self.move_flag == other.move_flag

    def other_player(self):
        return 'o' if self.player == 'x' else 'x'

    def copy(self):
        return InformationSet(self.player, self.move_flag, self.hash, self.board, self.index)

    def get_hash(self):
        return self.hash

    def get_v1_hash(self):
        v1_hash = self.board
        v1_hash += "m" if self.move_flag else "s"
        return v1_hash

    def get_index(self):
        return self.index

    def get_board_from_hash(self):
        new_board = ['0', '0', '0', '0', '0', '0', '0', '0', '0'] if self.player == 'x' else ['-', '-', '-', '-', '-', '-', '-', '-', '-']
        move_action = True if self.player == 'x' else False
        sense_action = False if self.player == 'x' else True
        observation = False
        curr_sense_move = -1
        i = 0

        while i < len(self.hash):
            if self.hash[i] == '|':
                if observation:
                    observation = False
                    move_action = True
                else:
                    observation = True
                    sense_action = False

                i += 1

            elif self.hash[i] == '_':
                move_action = False
                sense_action = True

                i += 1

            else:
                if move_action:
                    self.reset_zeros()
                    new_board[int(self.hash[i])] = self.player
                    i += 1
                elif sense_action:
                    curr_sense_move = int(self.hash[i]) + 9
                    i += 1
                elif observation:
                    for square in sense_square_dict[curr_sense_move]:
                        new_board[square] = self.hash[i]
                        i += 1

        return new_board

    def get_actions(self, actions):
        if self.move_flag:
            self.get_valid_moves(actions)
        else:
            self.get_useful_senses(actions)

    def get_actions_given_policy(self, actions, policy_obj):
        if self.index == -1:
            return
        else:
            if self.move_flag:
                prob_dist = policy_obj.policy_dict[self.index]
                for move in range(9):
                    if prob_dist[move] > 0:
                        actions.append(move)
            else:
                prob_dist = policy_obj.policy_dict[self.index]
                for sense in range(9, 13):
                    if prob_dist[sense] > 0:
                        actions.append(sense)

    def get_valid_moves(self, actions):
        w = self.win_exists()
        if w != -1:
            actions.append(w)
        else:
            for i in range(9):
                if self.board[i] == '0' or self.board[i] == '-':
                    actions.append(i)

    def get_played_actions(self, actions):
        move_action = True if self.player == 'x' else False
        sense_action = False if self.player == 'x' else True
        observation = False
        i = 0

        while i < len(self.hash):
            if self.hash[i] == '|':
                if observation:
                    observation = False
                    move_action = True
                else:
                    observation = True
                    sense_action = False

                i += 1

            elif self.hash[i] == '_':
                move_action = False
                sense_action = True

                i += 1

            else:
                if move_action:
                    actions.append(int(self.hash[i]))
                    i += 1
                elif sense_action:
                    actions.append(int(self.hash[i]) + 9)
                    i += 1
                elif observation:
                    i += 4
            
    def get_useful_senses(self, actions):
        for sense, squares in sense_square_dict.items():
            for i in range(4):
                if self.board[squares[i]] == '-':
                    actions.append(sense)

    def simulate_sense(self, action, true_board):
        self.reset_zeros()
        observation = ['-'] * 4
        count = 0
        for square in sense_square_dict[action]:
            self.board[square] = true_board[square]
            observation[count] = true_board[square]
            count += 1
        self.hash = self.hash + sense_square_mapping[action] + "|" + "".join(observation) + "|"
        self.move_flag = True
        if self.player == 'x':
            if self.hash not in P1_hash_to_int_map:
                self.index = -1
            else:
                self.index = P1_hash_to_int_map[self.hash]
        else:
            if self.hash not in P2_hash_to_int_map:
                self.index = -1
            else:
                self.index = P2_hash_to_int_map[self.hash]

    def reset_zeros(self, board):
        for i in range(9):
            if board[i] == '0':
                board[i] = '-'

    def reset_zeros(self):
        for i in range(9):
            if self.board[i] == '0':
                self.board[i] = '-'

    def is_valid_move(self, square):
        if square < 0 or square > 8:
            return False
        else:
            return self.board[square] == '0' or self.board[square] == '-'
    
    def update_move(self, square, player):
        if self.is_valid_move(square):
            self.board[square] = player
            self.hash = self.hash + str(square) + "_"
            self.move_flag = False
            if self.player == 'x':
                if self.hash not in P1_hash_to_int_map:
                    self.index = -1
                else:
                    self.index = P1_hash_to_int_map[self.hash]
            else:
                if self.hash not in P2_hash_to_int_map:
                    self.index = -1
                else:
                    self.index = P2_hash_to_int_map[self.hash]
            return True
        return False
    
    def is_win_for_player(self):
        for i in range(3):
            if (self.board[3 * i] == self.board[3 * i + 1] and self.board[3 * i + 1] == self.board[3 * i + 2] and self.board[3 * i] == self.player):
                return True

            if (self.board[i] == self.board[i + 3] and self.board[i + 3] == self.board[i + 6] and self.board[i] == self.player):
                return True

        if ((self.board[0] == self.board[4] and self.board[4] == self.board[8] and self.board[0] == self.player)):
            return True

        if ((self.board[2] == self.board[4] and self.board[4] == self.board[6] and self.board[2] == self.player)):
            return True

        return False
    
    def win_exists(self):
        for i in range(9):
            if self.board[i] == '0':
                self.board[i] = self.player
                if self.is_win_for_player():
                    self.board[i] = '0'
                    return i
                self.board[i] = '0'

        return -1
    
    def draw_exists(self):
        zeroes = []
        for i in range(9):
            if self.board[i] == '0':
                zeroes.append(i)

        for zero in zeroes:
            new_I_board = self.board
            new_I_board[zero] = self.player
            if InformationSet(self.player, self.move_flag, self.hash, self.board).is_over():
                return zero

        return -1
    
    def is_over(self):
        for i in range(9):
            if self.board[i] == '0' or self.board[i] == '-':
                return False
        return True
    

class History:
    def __init__(self, history):
        if not history:
            self.history = []
        else:
            self.history = history
        self.track_traversal_index = 0

    def other_player(self, player):
        return 'o' if player == 'x' else 'x'

    def get_board(self, true_board):
        curr_player = 'x'
        for action in self.history:
            if action < 9:
                if not true_board.update_move(action, curr_player):
                    return True
                curr_player = self.other_player(curr_player)
        curr_player = '0'
        return False, curr_player

    def get_information_sets(self, I_1, I_2):
        true_board = TicToeToeBoard()
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


class TerminalHistory(History):
    def __init__(self, history, reward):
        super().__init__(history)
        if not reward:
            self.reward = [0.0, 0.0]
        else:
            self.reward = reward

    def copy(self):
        return TerminalHistory(self.history, self.reward)

    def set_reward(self):
        true_board = TicToeToeBoard()
        overlapping_move_flag, overlapping_move_player = self.get_board(true_board)

        if overlapping_move_flag:
            if overlapping_move_player == 'x':
                self.reward[0] = -1.0
                self.reward[1] = 1.0
            else:
                self.reward[0] = 1.0
                self.reward[1] = -1.0
        else:
            winner = true_board.is_win()
            if winner:
                if winner == 'x':
                    self.reward[0] = 1.0
                    self.reward[1] = -1.0
                else:
                    self.reward[0] = -1.0
                    self.reward[1] = 1.0


class NonTerminalHistory(History):
    def __init__(self, history):
        super().__init__(history)

    def copy(self):
        return NonTerminalHistory(self.history)


class PolicyVec:
    def __init__(self):
        self.player = '0'
        self.policy_dict = []

    def __init__(self, player, file_path):
        self.player = player
        self.policy_dict = self.read_policy_from_txt(file_path, player)

    def copy(self):
        return PolicyVec(self.player, self.policy_dict)
    
    def read_policy_from_txt(self, file_path, player):
        policy_list = []
        
        with open(file_path) as i_file:
            index = 0
            for line in i_file:
                token_idx = 0
                tokens = line.split()

                I_hash = tokens[token_idx]
                if I_hash == '*':
                    I_hash = ""
                    if player == 'x':
                        move_flag = True
                    else:
                        move_flag = False
                    
                else:
                    move_flag = True if I_hash[-1] == '|' else False

                if player == 'x':
                    P1_hash_to_int_map[I_hash] = index
                else:
                    P2_hash_to_int_map[I_hash] = index

                token_idx += 1
                I = InformationSet(player, move_flag, I_hash)

                probability_distribution = [0.0] * 13

                while token_idx < len(tokens) - 1:
                    key = int(tokens[token_idx])
                    value = float(tokens[token_idx + 1])
                    probability_distribution[key] = value
                    token_idx += 2

                policy_list.append(probability_distribution)
                index += 1

        return policy_list
    

