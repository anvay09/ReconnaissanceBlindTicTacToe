#include "rbt_classes.h"
#include <chrono>
#include <ctime>
#include <unordered_set>

int play(InformationSet &I_1, InformationSet &I_2, TicTacToeBoard &true_board, unordered_set<string>& I_1_set, unordered_set<string>& I_2_set, char player) {
    int num_histories = 0;
    
    I_1_set.insert(I_1.get_hash());
    I_2_set.insert(I_2.get_hash());

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
                    num_histories += play(new_I, I_2, new_true_board, I_1_set, I_2_set, 'o');
                } else {
                    num_histories += play(I_1, new_I, new_true_board, I_1_set, I_2_set, 'x');
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
                num_histories += play(new_I, I_2, new_true_board, I_1_set, I_2_set, 'x');
            } else {
                num_histories += play(I_1, new_I, new_true_board, I_1_set, I_2_set, 'o');
            }
        }
    }

    return num_histories;
}


int main() {
    TicTacToeBoard true_board = TicTacToeBoard({'x', '0', '0', '0', 'o', '0', '0', '0', '0'});
    InformationSet I_1 = InformationSet('x', true, {'x', '0', '-', '0', 'o', '-', '-', '-', '-'});
    InformationSet I_2 = InformationSet('o', false, {'x', '-', '-', '-', 'o', '-', '-', '-', '-'});
    unordered_set<string> I_1_set;
    unordered_set<string> I_2_set;
    char player = 'x';

    auto start = std::chrono::system_clock::now();   
    int output = play(I_1, I_2, true_board, I_1_set, I_2_set, player);
    cout << output << endl;
    cout << I_1_set.size() << endl;
    cout << I_2_set.size() << endl;

    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end-start;
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);
 
    std::cout << "finished computation at " << std::ctime(&end_time)
              << "elapsed time: " << elapsed_seconds.count() << "s"
              << std::endl;
    return 0;
}
