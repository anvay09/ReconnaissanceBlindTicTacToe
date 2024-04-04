from rbt_classes import TicTacToeBoard, InformationSet, Policy, NonTerminalHistory, TerminalHistory
from multiprocessing import Pool
from config import num_workers
import argparse
import logging
import json

logging.basicConfig(format='%(levelname)s - %(asctime)s - %(message)s', datefmt='%d-%b-%y %H:%M:%S',
                    level=logging.INFO)


def get_expected_utility_parallel(I_1, I_2, true_board, player, policy_obj_x, policy_obj_o, probability,
                                  current_history, initial_player):
    depth_1_args = []
    # games = []
    expected_utility_h = 0

    if player == 'x':
        I = I_1
        policy_obj = policy_obj_x
    else:
        I = I_2
        policy_obj = policy_obj_o

    actions = I.get_actions_given_policy(policy_obj)

    if I.move_flag:
        for action in actions:
            new_true_board = true_board.copy()
            success = new_true_board.update_move(action, player)

            probability_new = probability * policy_obj.policy_dict[I.get_hash()][action]
            new_history = current_history.copy()
            new_history.history.append(action)

            if success and not new_true_board.is_win()[0] and not new_true_board.is_over():
                new_I = I.copy()
                new_I.update_move(action, player)
                new_I.reset_zeros()

                if player == 'x':
                    depth_1_args.append((new_I, I_2, new_true_board, 'o', policy_obj_x, policy_obj_o, probability_new,
                                         new_history, initial_player))
                else:
                    depth_1_args.append((I_1, new_I, new_true_board, 'x', policy_obj_x, policy_obj_o, probability_new,
                                         new_history, initial_player))
            else:
                terminal_history = TerminalHistory(new_history.history.copy())
                terminal_history.set_reward()

                # games.append((terminal_history, probability_new, terminal_history.reward[initial_player]))
                expected_utility_h += probability_new * terminal_history.reward[initial_player]

    else:
        for action in actions:
            new_I = I.copy()
            new_I.simulate_sense(action, true_board)
            new_true_board = true_board.copy()

            probability_new = probability * policy_obj.policy_dict[I.get_hash()][action]
            new_history = current_history.copy()
            new_history.history.append(action)

            if player == 'x':
                depth_1_args.append((new_I, I_2, new_true_board, 'x', policy_obj_x, policy_obj_o, probability_new,
                                     new_history, initial_player))
            else:
                depth_1_args.append((I_1, new_I, new_true_board, 'o', policy_obj_x, policy_obj_o, probability_new,
                                     new_history, initial_player))

    depth_2_args = []
    for arg in depth_1_args:
        I_1, I_2, true_board, player, policy_obj_x, policy_obj_o, probability, current_history, initial_player = arg

        if player == 'x':
            I = I_1
            policy_obj = policy_obj_x
        else:
            I = I_2
            policy_obj = policy_obj_o

        actions = I.get_actions_given_policy(policy_obj)

        if I.move_flag:
            for action in actions:
                new_true_board = true_board.copy()
                success = new_true_board.update_move(action, player)

                probability_new = probability * policy_obj.policy_dict[I.get_hash()][action]
                new_history = current_history.copy()
                new_history.history.append(action)

                if success and not new_true_board.is_win()[0] and not new_true_board.is_over():
                    new_I = I.copy()
                    new_I.update_move(action, player)
                    new_I.reset_zeros()

                    if player == 'x':
                        depth_2_args.append((new_I, I_2, new_true_board, 'o', policy_obj_x, policy_obj_o,
                                             probability_new, new_history, initial_player))
                    else:
                        depth_2_args.append((I_1, new_I, new_true_board, 'x', policy_obj_x, policy_obj_o,
                                             probability_new, new_history, initial_player))
                else:
                    terminal_history = TerminalHistory(new_history.history.copy())
                    terminal_history.set_reward()

                    # games.append((terminal_history, probability_new, terminal_history.reward[initial_player]))
                    expected_utility_h += probability_new * terminal_history.reward[initial_player]

        else:
            for action in actions:
                new_I = I.copy()
                new_I.simulate_sense(action, true_board)
                new_true_board = true_board.copy()

                probability_new = probability * policy_obj.policy_dict[I.get_hash()][action]
                new_history = current_history.copy()
                new_history.history.append(action)

                if player == 'x':
                    depth_2_args.append((new_I, I_2, new_true_board, 'x', policy_obj_x, policy_obj_o, probability_new,
                                         new_history, initial_player))
                else:
                    depth_2_args.append((I_1, new_I, new_true_board, 'o', policy_obj_x, policy_obj_o, probability_new,
                                         new_history, initial_player))

    with Pool(num_workers) as p:
        results = p.starmap(get_expected_utility, depth_2_args)
        for result in results:
            expected_utility_h += result
            # games = games + result[1]

    return expected_utility_h


def get_expected_utility(I_1, I_2, true_board, player, policy_obj_x, policy_obj_o, probability, current_history,
                         initial_player):
    expected_utility_h = 0
    # games = []

    if player == 'x':
        I = I_1
        policy_obj = policy_obj_x
    else:
        I = I_2
        policy_obj = policy_obj_o

    actions = I.get_actions_given_policy(policy_obj)

    if I.move_flag:
        for action in actions:
            new_true_board = true_board.copy()
            success = new_true_board.update_move(action, player)

            probability_new = probability * policy_obj.policy_dict[I.get_hash()][action]
            new_history = current_history.copy()
            new_history.history.append(action)

            if success and not new_true_board.is_win()[0] and not new_true_board.is_over():
                new_I = I.copy()
                new_I.update_move(action, player)
                new_I.reset_zeros()

                if player == 'x':
                    expected_utility_subtree = get_expected_utility(new_I, I_2, new_true_board, 'o', policy_obj_x,
                                                                    policy_obj_o,
                                                                    probability_new, new_history, initial_player)
                    expected_utility_h += expected_utility_subtree
                    # games = games + games_subtree
                else:
                    expected_utility_subtree = get_expected_utility(I_1, new_I, new_true_board, 'x', policy_obj_x,
                                                                    policy_obj_o,
                                                                    probability_new, new_history, initial_player)
                    expected_utility_h += expected_utility_subtree
                    # games = games + games_subtree
            else:
                terminal_history = TerminalHistory(new_history.history.copy())
                terminal_history.set_reward()

                # games.append((terminal_history, probability_new, terminal_history.reward[initial_player]))
                expected_utility_h += probability_new * terminal_history.reward[initial_player]

    else:
        for action in actions:
            new_I = I.copy()
            new_I.simulate_sense(action, true_board)
            new_true_board = true_board.copy()

            probability_new = probability * policy_obj.policy_dict[I.get_hash()][action]
            new_history = current_history.copy()
            new_history.history.append(action)

            if player == 'x':
                expected_utility_subtree = get_expected_utility(new_I, I_2, new_true_board, 'x', policy_obj_x,
                                                                policy_obj_o,
                                                                probability_new, new_history, initial_player)
                expected_utility_h += expected_utility_subtree
                # games = games + games_subtree
            else:
                expected_utility_subtree = get_expected_utility(I_1, new_I, new_true_board, 'o', policy_obj_x,
                                                                policy_obj_o,
                                                                probability_new, new_history, initial_player)
                expected_utility_h += expected_utility_subtree
                # games = games + games_subtree

    return expected_utility_h


if __name__ == "__main__":
    true_board = TicTacToeBoard(board=['0', '0', '0', '0', '0', '0', '0', '0', '0'])
    I_1 = InformationSet(player='x', move_flag=True, board=['0', '0', '0', '0', '0', '0', '0', '0', '0'])
    I_2 = InformationSet(player='o', move_flag=False, board=['-', '-', '-', '-', '-', '-', '-', '-', '-'])
    player = 'x'

    parser = argparse.ArgumentParser()
    parser.add_argument('--PolicyFileX', type=str, required=True)
    parser.add_argument('--PolicyFileO', type=str, required=True)
    arguments = parser.parse_args()

    p1_policy_dict = json.load(open(arguments.PolicyFileX, 'r'))
    p1_policy_obj = Policy(policy_dict=p1_policy_dict, player='x')

    p2_policy_dict = json.load(open(arguments.PolicyFileO, 'r'))
    p2_policy_obj = Policy(policy_dict=p2_policy_dict, player='o')

    logging.info("Getting expected utility...")
    expected_utility = get_expected_utility_parallel(I_1, I_2, true_board, player, p1_policy_obj, p2_policy_obj, 1,
                                                     NonTerminalHistory(), player)
    logging.info("Expected Utility: {}".format(expected_utility))

    # expected_utility, games = get_expected_utility(I_1, I_2, true_board, player, p1_policy_obj, p2_policy_obj, 1, NonTerminalHistory(), player)
    # logging.info("Expected Utility: {}".format(expected_utility))

    # games.sort(key=lambda x: x[2], reverse=True)

    # with open('data_files/games.txt', 'w') as f:
    #     for item in games:
    #         f.write('History: {}, Probability: {}, Reward: {}\n'.format(item[0].history, item[1], item[2]))
