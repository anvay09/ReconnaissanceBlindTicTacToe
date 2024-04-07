#include "rbt_classes.h"
#include <chrono>
#include <ctime>

vector<tuple<History, double, int>> terminal_histories;

int get_expected_utility(InformationSet &I_1, InformationSet &I_2, TicTacToeBoard &true_board, char player, Policy &policy_obj_x, Policy &policy_obj_o, double probability, History current_history, char initial_player) {
    double expected_utility_h = 0;
    
    InformationSet* I;
    Policy* policy_obj;
    if (player == 'x') {
        I = &I_1;
        policy_obj = &policy_obj_x;
    } else {
        I = &I_2;
        policy_obj = &policy_obj_o;
    }

    vector<int> actions = I->get_actions_given_policy(*policy_obj);
    
    if (I->move_flag) {
        for (int action : actions) {
            TicTacToeBoard new_true_board = true_board;
            bool success = new_true_board.update_move(action, player);

            double probability_new = probability * policy_obj->policy_dict[I->get_hash()][action];
            History new_history = current_history;
            new_history.history.push_back(action);
                
            if (success && !new_true_board.is_win().first && !new_true_board.is_over()) {
                InformationSet new_I(*I);
                new_I.update_move(action, player);
                new_I.reset_zeros();
                
                if (player == 'x') {
                    expected_utility_h += get_expected_utility(new_I, I_2, new_true_board, 'o', policy_obj_x, policy_obj_o, probability_new, new_history, initial_player);
                } else {
                    expected_utility_h += get_expected_utility(I_1, new_I, new_true_board, 'x', policy_obj_x, policy_obj_o, probability_new, new_history, initial_player);
                }
            } else {
                TerminalHistory terminal_history = TerminalHistory(new_history.history);
                terminal_history.set_reward();
                terminal_histories.push_back(make_tuple(terminal_history, probability_new, terminal_history.reward[initial_player]));
                expected_utility_h += terminal_history.reward[initial_player] * probability_new;
            }
        }
    } else {
        for (int action : actions) {
            InformationSet new_I(*I);
            new_I.simulate_sense(action, true_board);
            TicTacToeBoard new_true_board = true_board;

            double probability_new = probability * policy_obj->policy_dict[I->get_hash()][action];
            History new_history = current_history;
            new_history.history.push_back(action);

            if (player == 'x') {
                expected_utility_h += get_expected_utility(new_I, I_2, new_true_board, 'o', policy_obj_x, policy_obj_o, probability_new, new_history, initial_player);
            } else {
                expected_utility_h += get_expected_utility(I_1, new_I, new_true_board, 'x', policy_obj_x, policy_obj_o, probability_new, new_history, initial_player);
            }
        }
    }

    return expected_utility_h;
}


int main() {
    TicTacToeBoard true_board = TicTacToeBoard({'x', '0', '0', '0', 'o', '0', '0', '0', '0'});
    InformationSet I_1 = InformationSet('x', true, {'x', '0', '-', '0', 'o', '-', '-', '-', '-'});
    InformationSet I_2 = InformationSet('o', false, {'x', '-', '-', '-', 'o', '-', '-', '-', '-'});
    
    char player = 'x';
    // Policy policy_obj_x;
    // Policy policy_obj_o;

    auto start = std::chrono::system_clock::now();   
    // double expected_utility = get_expected_utility(I_1, I_2, true_board, player, policy_obj_x, policy_obj_o, 1, History({}), player);
    // cout << expected_utility << endl;
    

    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end-start;
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);
 
    std::cout << "finished computation at " << std::ctime(&end_time)
              << "elapsed time: " << elapsed_seconds.count() << "s"
              << std::endl;
    return 0;
}
