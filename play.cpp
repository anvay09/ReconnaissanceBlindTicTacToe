#include "rbt_classes.h"

// convert the below code to C++ code

tuple<int, set<string>, set<string>> play(InformationSet I_1, InformationSet I_2, TicTacToeBoard true_board, char player) {
    int num_histories = 0;
    set<string> I_1_set = {I_1.get_hash()};
    set<string> I_2_set = {I_2.get_hash()};

    InformationSet I;
    if (player == 'x') {
        I = I_1;
    } else {
        I = I_2;
    }

    vector<int> actions = I.get_actions();

    if (I.move_flag) {
        for (int action : actions) {
            TicTacToeBoard new_true_board = true_board;
            bool success = new_true_board.update_move(action, player);

            if (success && !new_true_board.is_win().first && !new_true_board.is_over()) {
                InformationSet new_I = I;
                new_I.update_move(action, player);
                new_I.reset_zeros();

                if (player =='x') {
                    tuple<int, set<string>, set<string>> future = play(new_I, I_2, new_true_board, 'o');
                    num_histories += get<0>(future);
                    I_1_set.insert(get<1>(future).begin(), get<1>(future).end());
                    I_2_set.insert(get<2>(future).begin(), get<2>(future).end());
                } else {
                    tuple<int, set<string>, set<string>> future = play(I_1, new_I, new_true_board, 'x');
                    num_histories += get<0>(future);
                    I_1_set.insert(get<1>(future).begin(), get<1>(future).end());
                    I_2_set.insert(get<2>(future).begin(), get<2>(future).end());
                }
            } else {
                num_histories += 1;
            }
        }
    } else {
        for (int action : actions) {
            InformationSet new_I = I;
            new_I.simulate_sense(action, true_board);
            TicTacToeBoard new_true_board = true_board;

            if (player == 'x') {
                tuple<int, set<string>, set<string>> future = play(new_I, I_2, new_true_board, 'x');
                num_histories += get<0>(future);
                I_1_set.insert(get<1>(future).begin(), get<1>(future).end());
                I_2_set.insert(get<2>(future).begin(), get<2>(future).end());
            } else {
                tuple<int, set<string>, set<string>> future = play(I_1, new_I, new_true_board, 'o');
                num_histories += get<0>(future);
                I_1_set.insert(get<1>(future).begin(), get<1>(future).end());
                I_2_set.insert(get<2>(future).begin(), get<2>(future).end());
            }
        }
    }

    return make_tuple(num_histories, I_1_set, I_2_set);
}

int main() {
    TicTacToeBoard true_board = TicTacToeBoard({'x', '0', '0', '0', 'o', '0', '0', '0', '0'});
    InformationSet I_1 = InformationSet('x', true, {'x', '0', '-', '0', 'o', '-', '-', '-', '-'});
    InformationSet I_2 = InformationSet('o', false, {'x', '-', '-', '-', 'o', '-', '-', '-', '-'});
    char player = 'x';

    tuple<int, set<string>, set<string>> output = play(I_1, I_2, true_board, player);
    cout << get<0>(output) << endl;
    return 0;
}


// def play(I_1, I_2, true_board, player):
//     """
//     :param I_1:
//     :param I_2:
//     :param true_board:
//     :param player:
//     :param move_flag:
//     :return:
//     """
//     num_histories = 0
//     I_1_set = set((I_1.get_hash(),))
//     I_2_set = set((I_2.get_hash(),))

//     if player == 'x':
//         I = I_1
//     else:
//         I = I_2

//     actions = I.get_actions()

//     if I.move_flag:
//         for action in actions:
//             new_true_board = true_board.copy()
//             success = new_true_board.update_move(action, player)

//             if success and not new_true_board.is_win()[0] and not new_true_board.is_over():
//                 new_I = I.copy()
//                 new_I.update_move(action, player)
//                 new_I.reset_zeros()

//                 if player == 'x':
//                     num_histories_future, future_I_1_set, future_I_2_set = play(new_I, I_2, new_true_board, 'o')
//                     num_histories += num_histories_future
//                     I_1_set = I_1_set.union(future_I_1_set)
//                     I_2_set = I_2_set.union(future_I_2_set)
//                 else:
//                     num_histories_future, future_I_1_set, future_I_2_set = play(I_1, new_I, new_true_board, 'x')
//                     num_histories += num_histories_future
//                     I_1_set = I_1_set.union(future_I_1_set)
//                     I_2_set = I_2_set.union(future_I_2_set)
//             else:
//                 num_histories += 1

//     else:
//         for action in actions:
//             new_I = I.copy()
//             new_I.simulate_sense(action, true_board)
//             new_true_board = true_board.copy()

//             if player == 'x':
//                 num_histories_future, future_I_1_set, future_I_2_set = play(new_I, I_2, new_true_board, 'x')
//                 num_histories += num_histories_future
//                 I_1_set = I_1_set.union(future_I_1_set)
//                 I_2_set = I_2_set.union(future_I_2_set)
//             else:
//                 num_histories_future, future_I_1_set, future_I_2_set = play(I_1, new_I, new_true_board, 'o')
//                 num_histories += num_histories_future
//                 I_1_set = I_1_set.union(future_I_1_set)
//                 I_2_set = I_2_set.union(future_I_2_set)

//     return num_histories, I_1_set, I_2_set

// if __name__ == "__main__":
//     true_board = TicTacToeBoard(board=['o', '0', '0', 'x', 'x', 'o', 'o', '0', 'x'])
//     I_1 = InformationSet(player='x', move_flag=True, board=['-', '-', '-', 'x', 'x', 'o', 'o', '-', 'x'])
//     I_2 = InformationSet(player='o', move_flag=False, board=['o', '-', '-', 'x', 'x', 'o', 'o', '-', 'x'])
//     player = 'x'
  
//     play(I_1, I_2, true_board, player)