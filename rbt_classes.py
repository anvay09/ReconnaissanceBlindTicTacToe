"""
File for RBT classes TicTacToeBoard, InformationSet, History, TerminalHistory, NonTerminalHistory, and Policy
"""
from sympy.utilities.iterables import multiset_permutations
import yaml
from random import random
import copy

class TicTacToeBoard:
    """
    Class to maintain tictactoe board and perform board operations
    """

    def __init__(self, board=None):
        """
        :param board: list having nine values, each corresponding to a tictactoe square
        """
        if board is None:
            self.board = ['0', '0', '0', '0', '0', '0', '0', '0', '0']
        else:
            self.board = board.copy()

    def __getitem__(self, key):
        """
        Overload [] operator

        :param key: square int
        :return: square value
        """
        return self.board[key]

    def __setitem__(self, key, value):
        """
        Assign to [] operator

        :param key: square int
        :param value: square value to assign
        :return: None
        """
        self.board[key] = value

    def __eq__(self, other):
        return self.board == other.board

    def copy(self):
        return TicTacToeBoard(self.board)

    def is_win(self):
        """ Check win
        :return: a boolean (True or False) and the winner ('x or 'o' or None)
        """
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
        """ Check if board is full.

        :return: Bool True or False
        """
        for i in range(9):
            if self.board[i] == '0':
                return False
        return True

    def is_draw(self):
        """
        :return: Bool True or False
        """
        if not self.is_win() and self.is_over():
            return True
        return False

    def is_valid_move(self, square):
        """
        :param square: int
        :return: Bool True or False
        """
        if square < 0 or square > 8:
            return False
        else:
            return self.board[square] == '0'

    def update_move(self, square, player):
        """
        :param square: int (0-9)
        :param player: str 'x' or 'o'
        :return: update board if valid and return Bool True else return Bool False
        """
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


class InformationSet(TicTacToeBoard):
    """
    Inherit data and functions from TicTacToeBoard
    """

    def __init__(self, player, move_flag, board=None):
        """
        :param player: str 'x', 'o'
        :param board: list representing the information set of player
        """
        super().__init__(board)
        self.sense_square_dict = {9: [0, 1, 3, 4], 10: [1, 2, 4, 5], 11: [3, 4, 6, 7], 12: [4, 5, 7, 8]}
        self.player = player
        self.move_flag = move_flag

    def __eq__(self, other):
        return self.board == other.board and self.player == other.player and self.move_flag == other.move_flag

    def other_player(self):
        if self.player == 'x':
            return 'o'
        else:
            return 'x'

    def copy(self):
        return InformationSet(self.player, self.move_flag, self.board)

    def get_hash(self):
        if self.move_flag:
            return ''.join(self.board) + 'm'
        else:
            return ''.join(self.board) + 's'
        
    def get_states(self):
        """ Get the states part of the information set

        :return: list of TicTacToeBoard objects
        """
        num_unknown_opponent_moves = self.get_number_of_unknown_opponent_moves()

        board_copy = self.board.copy()
        for i in range(len(board_copy)):
            if board_copy[i] == '-':
                board_copy[i] = '0'

        if num_unknown_opponent_moves == 0:
            return [TicTacToeBoard(board=board_copy)]
        else:
            output_states = []
            uncertain_ind = self.get_uncertain_squares()
            base_perm = [self.other_player()] * num_unknown_opponent_moves + ['0'] * (
                    len(uncertain_ind) - num_unknown_opponent_moves)

            perm_itr = multiset_permutations(base_perm)
            for perm in perm_itr:
                new_state = TicTacToeBoard(board=board_copy)
                for j in range(len(perm)):
                    new_state[uncertain_ind[j]] = perm[j]
                if not new_state.is_win()[0] and not new_state.is_over():
                    output_states.append(new_state)

            return output_states

    def get_actions(self):
        """
        Get the valid actions that can be taken from the current information set.

        For move action, these include the squares which the player knows is empty '0' and the uncertain squares '-'.
        But note, if a win exists the player will play only that action.

        For sense moves, any sense which reveals some information is a valid sense move.

        :param move_flag: indicates whether the current action is move or sense
        :return: list of valid actions that can be taken from the current information set
        """
        if self.move_flag:
            return self.get_valid_moves()
        else:
            return self.get_useful_senses()

    def get_actions_given_policy(self, policy_obj):
        """
        Get the valid actions that can be taken from the current information set. These include the actions
        which have a positive probability in the policy.

        :param policy_obj: Policy Object
        :return: list of valid actions that can be taken from the current information set given policy
        """
        action_list = []
        if self.move_flag:
            for move in range(9):
                # TODO update this after policy is updated
                if policy_obj.policy_dict[self.get_hash()][move] > 0:
                    action_list.append(move)
        else:
            for sense in self.sense_square_dict.keys():
                # TODO update this after policy is updated
                if policy_obj.policy_dict[self.get_hash()][sense] > 0:
                    action_list.append(sense)
        return action_list

    def get_valid_moves(self):
        """
        :return: list of valid actions
        """
        w = self.win_exists()
        if w != -1:
            return [w]

        valid_moves = []
        for i in range(len(self.board)):
            if self.board[i] == '0' or self.board[i] == '-':
                valid_moves.append(i)
        return valid_moves

    def get_useful_senses(self):
        """
        :return: list of valid senses
        """
        valid_sense = []
        for key, value in self.sense_square_dict.items():
            for i in range(len(value)):
                if self.board[value[i]] == '-':
                    valid_sense.append(key)
                    break
        return valid_sense

    def get_number_of_unknown_opponent_moves(self):
        """
        :return: int
        """
        count_x = 0
        count_o = 0
        for i in range(len(self.board)):
            if self.board[i] == 'x':
                count_x += 1
            if self.board[i] == 'o':
                count_o += 1
        if self.player == 'x':
            return count_x - count_o
        else:
            return count_o - count_x + 1

    def get_uncertain_squares(self):
        """
        :return: list of uncertain squares
        """
        uncertain_squares = []
        for i in range(len(self.board)):
            if self.board[i] == '-':
                uncertain_squares.append(i)
        return uncertain_squares

    def simulate_sense(self, action, true_board):
        self.reset_zeros()
        for square in self.sense_square_dict[action]:
            self.board[square] = true_board[square]

        self.move_flag = True

    def reset_zeros(self):
        for i in range(len(self.board)):
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
            self.move_flag = False
            return True
        return False

    def is_win_for_player(self):  # returns a boolean (True or False)
        for i in range(3):
            if (self.board[3 * i] == self.board[3 * i + 1] == self.board[3 * i + 2] == self.player):
                return True

            if (self.board[i] == self.board[i + 3] == self.board[i + 6] == self.player):
                return True

        if (self.board[0] == self.board[4] == self.board[8] == self.player):
            return True

        if (self.board[2] == self.board[4] == self.board[6] == self.player):
            return True

        return False

    def win_exists(self):
        """
        :return: int, action that leads to a win
        """
        zeroes = []
        for i in range(len(self.board)):
            if self.board[i] == '0':
                zeroes.append(i)

        for zero in zeroes:
            new_I_board = self.board.copy()
            new_I_board[zero] = self.player
            if InformationSet(self.player, self.move_flag, new_I_board).is_win_for_player():
                return zero

        return -1


class History:
    """
    Parent class for RBT histories
    """

    def __init__(self, history=None):
        """
        :param history: list of actions
        """
        if history is None:
            self.history = []
        else:
            self.history = history.copy()
        self.track_traversal_index = 0

    @staticmethod
    def other_player(player):
        if player == 'x':
            return 'o'
        else:
            return 'x'

    def get_board(self):
        """ Get board for the current history after playing out the moves.

        :return: TicTacToe object, bool (True, False), str (player 'x', 'o')
        If all moves are played on empty squares return TicTacToe object, False, None
        Else return TicTacToe object, True, player who played on the non-empty square and lost the game
        """
        true_board = TicTacToeBoard()
        curr_player = 'x'
        for action in self.history:
            if action < 9:
                if not true_board.update_move(action, curr_player):
                    return true_board, True, curr_player

                curr_player = self.other_player(curr_player)
        return true_board, False, None

    def get_information_sets(self):
        """ Get information set of both players for the current history

        :return: Two information set objects
        """
        I_1 = InformationSet(player='x', move_flag=True, board=['0', '0', '0', '0', '0', '0', '0', '0', '0'])
        I_2 = InformationSet(player='o', move_flag=False, board=['-', '-', '-', '-', '-', '-', '-', '-', '-'])
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


class TerminalHistory(History):
    """
    Child class for terminal histories
    """

    def __init__(self, history=None, reward=None):
        """
        :param history: list of actions
        :param reward: dictionary for rewards of both players
        """
        super().__init__(history)
        if reward is None:
            self.reward = {'x': 0, 'o': 0}
        else:
            self.reward = reward.copy()

    def copy(self):
        return TerminalHistory(self.history, self.reward)

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
        super().__init__(history)

    def copy(self):
        return NonTerminalHistory(self.history)


class Policy:
    """
    Class for a policy
    """

    def __init__(self, player, policy_dict=None, policy_file=None):
        """
        :param player: (str) 'x' or 'o'
        :param policy_file: yaml file containing the policy is reuired format
        """
        self.player = player
        if policy_dict is None:
            if policy_file is None:
                self.policy_dict = None
                if player == 'x':
                    policy_file = './data_files/P1_information_sets.yml'
                else:
                    policy_file = './data_files/P2_information_sets.yml'

                with open(policy_file, 'r') as file:
                    self.policy_dict = yaml.safe_load(file)

                for key, _ in self.policy_dict.items():
                    board = [*key]
                    move_flag = board[-1] == 'm'
                    board = board[:-1]
                    
                    information_set_obj = InformationSet(player=self.player, move_flag=move_flag, board=board)
                    valid_actions = information_set_obj.get_actions()
                    uniform_prob = 1 / len(valid_actions)

                    if not move_flag:
                        self.policy_dict[key] = {}
                        for i in range(9, 13):
                            if i in valid_actions:
                                self.policy_dict[key][i] = uniform_prob
                            else:
                                self.policy_dict[key][i] = 0

                    else:
                        self.policy_dict[key] = {}
                        for i in range(0, 9):
                            if i in valid_actions:
                                self.policy_dict[key][i] = uniform_prob
                            else:
                                self.policy_dict[key][i] = 0
            else:
                with open(policy_file, 'r') as file:
                    self.policy_dict = yaml.safe_load(file)
        else:
            self.policy_dict = copy.deepcopy(policy_dict)

    def copy(self):
        return Policy(self.player, policy_dict = copy.deepcopy(self.policy_dict))

    def update_policy_for_given_information_set(self, information_set, prob_distribution):
        """
        :param information_set: Information set object
        :param prob_distribution: list of probabilities over move or sense actions
        :return: None, update policy dict inplace
        """
        keys = sorted(list(self.policy_dict[information_set.get_hash()].keys()))
        for i in range(len(keys)):
            self.policy_dict[information_set.get_hash()][keys[i]] = prob_distribution[i]

    def save_policy_to_file(self, file_name):
        """
        :param file_name: str
        :return: None, save policy to file
        """
        with open(file_name, 'w') as file:
            yaml.safe_dump(self.policy_dict, file, sort_keys=True, default_style='')