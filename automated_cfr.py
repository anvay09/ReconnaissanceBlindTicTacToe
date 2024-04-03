from rbt_classes import TicTacToeBoard, InformationSet, Policy
from multiprocessing import Pool
import json
import logging
from config import num_workers
from rbt_utilties import get_histories_given_I, upgraded_get_histories_given_I
from rbt_utilties import calc_cfr_policy_given_I
import argparse
from tqdm import tqdm

logging.basicConfig(format='%(levelname)s - %(asctime)s - %(message)s', datefmt='%d-%b-%y %H:%M:%S',
                    level=logging.INFO)


def toggle_player(player):
    return 'x' if player == 'o' else 'o'


def get_information_set_from_states(states, player):
    """
    :param states: list of TicTacTie boards
    :param player: 'x' or 'o'
    :return: Information set object
    """
    I = InformationSet(player)
    for i in range(9):
        flag = 0
        temp = states[0][i]
        for state in states:
            if not state[i] == temp:
                flag = 1
                break
        if flag:
            I[i] = '-'
        else:
            I[i] = temp

    return I


def play(I_1, I_2, true_board, player, p1_policy_obj, p2_policy_obj):
    """
    :param I_1:
    :param I_2:
    :param true_board:
    :param player:
    :param move_flag:
    :return:
    """
    num_histories = 0

    I_1_set = set((I_1.get_hash(),))

    if I_2.num_self_moves() == 4:
        I_2_set = set()
    else:
        I_2_set = set((I_2.get_hash(),))

    if player == 'x':
        I = I_1
        actions = I.get_actions_given_policy(p1_policy_obj)
    else:
        I = I_2
        actions = I.get_actions_given_policy(p2_policy_obj)

    if I.move_flag:
        for action in actions:
            new_true_board = true_board.copy()
            success = new_true_board.update_move(action, player)

            if success and not new_true_board.is_win()[0] and not new_true_board.is_over():
                new_I = I.copy()
                new_I.update_move(action, player)
                new_I.reset_zeros()

                if player == 'x':
                    num_histories_future, future_I_1_set, future_I_2_set = play(new_I, I_2, new_true_board, 'o',
                                                                                p1_policy_obj, p2_policy_obj)
                    num_histories += num_histories_future
                    I_1_set = I_1_set.union(future_I_1_set)
                    I_2_set = I_2_set.union(future_I_2_set)
                else:
                    num_histories_future, future_I_1_set, future_I_2_set = play(I_1, new_I, new_true_board, 'x',
                                                                                p1_policy_obj, p2_policy_obj)
                    num_histories += num_histories_future
                    I_1_set = I_1_set.union(future_I_1_set)
                    I_2_set = I_2_set.union(future_I_2_set)
            else:
                num_histories += 1

    else:
        for action in actions:
            new_I = I.copy()
            new_I.simulate_sense(action, true_board)
            new_true_board = true_board.copy()

            if player == 'x':
                num_histories_future, future_I_1_set, future_I_2_set = play(new_I, I_2, new_true_board, 'x',
                                                                            p1_policy_obj, p2_policy_obj)
                num_histories += num_histories_future
                I_1_set = I_1_set.union(future_I_1_set)
                I_2_set = I_2_set.union(future_I_2_set)
            else:
                num_histories_future, future_I_1_set, future_I_2_set = play(I_1, new_I, new_true_board, 'o',
                                                                            p1_policy_obj, p2_policy_obj)
                num_histories += num_histories_future
                I_1_set = I_1_set.union(future_I_1_set)
                I_2_set = I_2_set.union(future_I_2_set)

    return num_histories, I_1_set, I_2_set


def parallel_play(I_1, I_2, true_board, player, p1_policy_obj, p2_policy_obj, current_player_run, cfr_round):
    Total_histories = 0
    I_1_vars = []
    I_2_vars = []
    true_board_vars = []
    P1_information_sets = set((I_1.get_hash(),))
    P2_information_sets = set((I_2.get_hash(),))

    if player == 'x':
        I = I_1
        actions = I.get_actions_given_policy(p1_policy_obj)
    else:
        I = I_2
        actions = I.get_actions_given_policy(p2_policy_obj)

    for action in actions:
        new_true_board = true_board.copy()
        success = new_true_board.update_move(action, player)

        if success and not new_true_board.is_win()[0] and not new_true_board.is_over():
            new_I = I.copy()
            new_I.update_move(action, player)
            new_I.reset_zeros()

            if player == 'x':
                I_1_vars.append(new_I)
                I_2_vars.append(I_2.copy())
            else:
                I_1_vars.append(I_1.copy())
                I_2_vars.append(new_I)

            true_board_vars.append(new_true_board)
        else:
            Total_histories += 1

    if player == 'x':
        player = 'o'
    else:
        player = 'x'

    with Pool(len(I_1_vars)) as pool:
        obj_list = pool.starmap(play,
                                [(I_1_vars[i], I_2_vars[i], true_board_vars[i], player, p1_policy_obj, p2_policy_obj)
                                 for i in
                                 range(len(I_1_vars))])

    for item in obj_list:
        Total_histories += item[0]
        P1_information_sets = P1_information_sets.union(item[1])
        P2_information_sets = P2_information_sets.union(item[2])

    print('Total Histories: ', Total_histories)
    if current_player_run == 'x':
        print('P1 Information Sets: ', len(P1_information_sets))
        with open('data_files/reachable_P1_information_sets_round_{}.txt'.format(cfr_round), 'w') as f:
            for item in P1_information_sets:
                f.write(item + '\n')

    if current_player_run == 'o':
        print('P2 Information Sets: ', len(P2_information_sets))
        with open('data_files/reachable_P2_information_sets_round_{}.txt'.format(cfr_round), 'w') as f:
            for item in P2_information_sets:
                f.write(item + '\n')

    return


terminal_histories = []


def parse_commandline_args():
    """Parse command line arguments"""
    parser = argparse.ArgumentParser()
    parser.add_argument('--CurrentPlayer', type=str, required=True)
    parser.add_argument('--PolicyFileX', type=str, required=True)
    parser.add_argument('--PolicyFileO', type=str, required=True)
    parser.add_argument('--Round', type=str, required=True)
    parser.add_argument('--ReachableISFlag', type=str, required=True)
    parser.add_argument('--FilterValidHistoriesFlag', type=str, required=True)
    arguments = parser.parse_args()
    return arguments.CurrentPlayer, arguments.PolicyFileX, arguments.PolicyFileO, int(arguments.Round), int(
        arguments.ReachableISFlag), int(arguments.FilterValidHistoriesFlag)


if __name__ == "__main__":
    # %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% create reachable IS file %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    true_board = TicTacToeBoard(board=['0', '0', '0', '0', '0', '0', '0', '0', '0'])
    I_1 = InformationSet(player='x', move_flag=True, board=['0', '0', '0', '0', '0', '0', '0', '0', '0'])
    I_2 = InformationSet(player='o', move_flag=False, board=['-', '-', '-', '-', '-', '-', '-', '-', '-'])
    player = 'x'
    cfr_player, policy_file_x, policy_file_o, cfr_round, reachable_IS_flag, filter_valid_histories_flag = parse_commandline_args()

    if cfr_player == 'o':
        reachable_IS_file_player = 'data_files/reachable_P2_information_sets_round_{}.txt'.format(cfr_round)
        valid_histories_file_player = 'data_files/p2_valid_histories_for_reachable_I_round_{}.json'.format(cfr_round)
    else:
        reachable_IS_file_player = 'data_files/reachable_P1_information_sets_round_{}.txt'.format(cfr_round)
        valid_histories_file_player = 'data_files/p1_valid_histories_for_reachable_I_round_{}.json'.format(cfr_round)

    p1_policy_dict = json.load(open(policy_file_x, 'r'))
    policy_obj_x = Policy(policy_dict=p1_policy_dict, player='x')
    p2_policy_dict = json.load(open(policy_file_o, 'r'))
    policy_obj_o = Policy(policy_dict=p2_policy_dict, player='o')

    if reachable_IS_flag:
        parallel_play(I_1, I_2, true_board, player, policy_obj_x, policy_obj_o, cfr_player, cfr_round)

    # %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% filter valid histories and save %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    player_reachable_information_sets = set()
    with open(reachable_IS_file_player, 'r') as f:
        lines = f.readlines()
        for line in lines:
            player_reachable_information_sets.add(line.strip())

    histories = {}
    args = []

    for I_hash in player_reachable_information_sets:
        I = InformationSet(player=cfr_player, move_flag=I_hash[-1] == 'm', board=[*I_hash[:-1]])
        if cfr_player == 'x':
            args.append((I, None, policy_obj_o))
        else:
            args.append((I, policy_obj_x, None))

    if filter_valid_histories_flag:
        logging.info('Filtering valid histories for player {} information sets...'.format(cfr_player))
        with Pool(num_workers) as p:
            H = p.starmap(upgraded_get_histories_given_I, tqdm(args, total=len(args)))

        logging.info('Saving valid histories for cfr player {} information sets...'.format(cfr_player))

        for idx in range(len(args)):
            I_hash = args[idx][0].get_hash()
            histories[I_hash] = H[idx]

        with open(valid_histories_file_player, 'w') as f:
            json.dump(histories, f)

    # %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% cfr run %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    player_reachable_information_sets = []
    with open(reachable_IS_file_player, 'r') as f:
        lines = f.readlines()
        for line in lines:
            player_reachable_information_sets.append(line.strip())

    if cfr_player == 'x':
        if cfr_round == 1:
            p1_policy_dict_for_IS_keys = json.load(open('./data_files/P1_uniform_policy.json', 'r'))
            prev_regret_list_player = {}
            for key in p1_policy_dict_for_IS_keys.keys():
                prev_regret_list_player[key] = [0 for _ in range(13)]
            # prev_regret_list_player = {I_hash: [0 for _ in range(13)] for I_hash in player_reachable_information_sets}
        else:
            prev_regret_list_player = json.load(
                open('./data_files/P1_prev_regret_list_round_{}.json'.format(cfr_round - 1), 'r'))
    else:
        if cfr_round == 1:
            p2_policy_dict_for_IS_keys = json.load(open('./data_files/P2_uniform_policy.json', 'r'))
            prev_regret_list_player = {}
            for key in p2_policy_dict_for_IS_keys.keys():
                prev_regret_list_player[key] = [0 for _ in range(13)]
            # prev_regret_list_player = {I_hash: [0 for _ in range(13)] for I_hash in player_reachable_information_sets}
        else:
            prev_regret_list_player = json.load(
                open('./data_files/P2_prev_regret_list_round_{}.json'.format(cfr_round - 1), 'r'))

    # logging.info('Loading valid histories...')
    # with open(valid_histories_file_player, 'r') as f:
    #    histories = json.load(f)

    args = []

    # logging.info('Converting histories to bit arrays and generating arguments...')
    logging.info('Generating arguments...')
    for I_hash in player_reachable_information_sets:
        I = InformationSet(player=cfr_player, move_flag=I_hash[-1] == 'm', board=[*I_hash[:-1]])
        # starting_histories = histories[I_hash]
        starting_histories = None

        args.append((I, policy_obj_x, policy_obj_o, cfr_round, prev_regret_list_player[I_hash], starting_histories))

    # del p1_valid_histories_for_I
    # gc.collect()
    logging.info('cfr round {}...'.format(cfr_round))
    with Pool(num_workers) as p:
        regrets = p.starmap(calc_cfr_policy_given_I, tqdm(args, total=len(args)))

    logging.info('Updating policy objects...')
    for arg in args:
        actions = arg[0].get_actions()
        I_hash = arg[0].get_hash()
        prev_regret_list_player[I_hash] = regrets.pop(0)
        total_regret = sum(prev_regret_list_player[I_hash])

        if total_regret > 0:
            for action in actions:
                if cfr_player == 'x':
                    policy_obj_x.policy_dict[I_hash][action] = prev_regret_list_player[I_hash][
                                                                   action] / total_regret
                else:
                    policy_obj_o.policy_dict[I_hash][action] = prev_regret_list_player[I_hash][
                                                                   action] / total_regret
        else:
            for action in actions:
                if cfr_player == 'x':
                    policy_obj_x.policy_dict[I_hash][action] = 1 / len(actions)
                else:
                    policy_obj_o.policy_dict[I_hash][action] = 1 / len(actions)

    logging.info('Completed cfr_round {}...'.format(cfr_round))
    logging.info('Saving policy objects...')

    if cfr_player == 'x':
        with open('./data_files/P1_cfr_policy_round_{}.json'.format(cfr_round), 'w') as f:
            json.dump(policy_obj_x.policy_dict, f)
        with open('./data_files/P1_prev_regret_list_round_{}.json'.format(cfr_round), 'w') as f:
            json.dump(prev_regret_list_player, f)
    else:
        with open('./data_files/P2_cfr_policy_round_{}.json'.format(cfr_round), 'w') as f:
            json.dump(policy_obj_o.policy_dict, f)
        with open('./data_files/P2_prev_regret_list_round_{}.json'.format(cfr_round), 'w') as f:
            json.dump(prev_regret_list_player, f)
