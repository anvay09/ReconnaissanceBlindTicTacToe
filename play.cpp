#include "rbt_classes.h"
#include <chrono>
#include <ctime>

// convert the below code to C++ code

tuple<int, set<string>*, set<string>*> play(InformationSet &I_1, InformationSet &I_2, TicTacToeBoard &true_board, char player) {
    int num_histories = 0;

    set<string>* I_1_set = new set<string>();
    set<string>* I_2_set = new set<string>();
    I_1_set->insert(I_1.get_hash());
    I_2_set->insert(I_2.get_hash());

    InformationSet* I;
    if (player == 'x') {
        I = &I_1;
    } else {
        I = &I_2;
    }

    vector<int> actions = I->get_actions();

    if (I->move_flag) {
        for (int action : actions) {
            TicTacToeBoard new_true_board = true_board;
            bool success = new_true_board.update_move(action, player);

            if (success && !new_true_board.is_win().first && !new_true_board.is_over()) {
                InformationSet new_I(*I);
                new_I.update_move(action, player);
                new_I.reset_zeros();

                if (player == 'x') {
                    tuple<int, set<string>*, set<string>* > future = play(new_I, I_2, new_true_board, 'o');
                    num_histories += get<0>(future);
                    I_1_set->insert(get<1>(future)->begin(), get<1>(future)->end());
                    I_2_set->insert(get<2>(future)->begin(), get<2>(future)->end());
                } else {
                    tuple<int, set<string>*, set<string>* > future = play(I_1, new_I, new_true_board, 'x');
                    num_histories += get<0>(future);
                    I_1_set->insert(get<1>(future)->begin(), get<1>(future)->end());
                    I_2_set->insert(get<2>(future)->begin(), get<2>(future)->end());
                }
            } else {
                num_histories += 1;
            }
        }
    } else {
        for (int action : actions) {
            InformationSet new_I(*I);
            new_I.simulate_sense(action, true_board);
            TicTacToeBoard new_true_board = true_board;

            if (player == 'x') {
                tuple<int, set<string>*, set<string>* > future = play(new_I, I_2, new_true_board, 'x');
                num_histories += get<0>(future);
                I_1_set->insert(get<1>(future)->begin(), get<1>(future)->end());
                I_2_set->insert(get<2>(future)->begin(), get<2>(future)->end());
            } else {
                tuple<int, set<string>*, set<string>* > future = play(I_1, new_I, new_true_board, 'o');
                num_histories += get<0>(future);
                I_1_set->insert(get<1>(future)->begin(), get<1>(future)->end());
                I_2_set->insert(get<2>(future)->begin(), get<2>(future)->end());
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

    auto start = std::chrono::system_clock::now();   
    tuple<int, set<string>*, set<string>* > output = play(I_1, I_2, true_board, player);
    cout << get<0>(output) << endl;
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end-start;
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);
 
    std::cout << "finished computation at " << std::ctime(&end_time)
              << "elapsed time: " << elapsed_seconds.count() << "s"
              << std::endl;
    return 0;
}
