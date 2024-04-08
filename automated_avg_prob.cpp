#include "rbt_classes.h"
#include <chrono>
#include <ctime>
#include <algorithm>
#include <cassert>

vector<int> intersection(vector<int> const& left_vector, vector<int> const& right_vector) {
    auto left = left_vector.begin();
    auto left_end = left_vector.end();
    auto right = right_vector.begin();
    auto right_end = right_vector.end();

    assert(is_sorted(left, left_end));
    assert(is_sorted(right, right_end));

    vector<int> result;

    while (left != left_end && right != right_end) {
        if (*left == *right) {
            result.push_back(*left);
            ++left;
            ++right;
            continue;
        }

        if (*left < *right) {
            ++left;
            continue;
        }

        assert(*left > *right);
        ++right;
    }

    return result;
}


void valid_histories_play(InformationSet& I_1, InformationSet& I_2, 
                          TicTacToeBoard& true_board, char player, 
                          History& current_history, InformationSet& end_I, 
                          vector<int>& played_actions, Policy& policy_obj_x, 
                          Policy& policy_obj_o, vector<vector<int>>& valid_histories_list){
    
    InformationSet& I = player == 'x' ? I_1 : I_2;
    vector<int> actions;

    if (player == 'x') {
        if (end_I.player == 'x'){
            I.get_actions_given_policy(actions, policy_obj_x);
            if (I.move_flag) {
                actions = intersection(actions, played_actions);
            }
        }
        else {
            I.get_actions(actions);
        }
        
    } else {
        if (end_I.player == 'o'){
            I.get_actions_given_policy(actions, policy_obj_o);
            if (I.move_flag) {
                actions = intersection(actions, played_actions);
            }
        }
        else {
            I.get_actions(actions);
        }
    }

    if (I.move_flag){
        for (int action : actions) {
            TicTacToeBoard new_true_board = true_board;
            bool success = new_true_board.update_move(action, player);

            History new_history = current_history;
            new_history.history.push_back(action);

            char winner;
            if (success && !new_true_board.is_win(winner) && !new_true_board.is_over()) {
                InformationSet new_I(I);
                new_I.update_move(action, player);
                new_I.reset_zeros();

                if (player == 'x') {
                    if (end_I.player == 'x') {
                        valid_histories_play(new_I, I_2, new_true_board, 'o', new_history, end_I, played_actions, policy_obj_x, policy_obj_o, valid_histories_list);
                    }
                    else {
                        if (I_2 == end_I){
                            valid_histories_list.push_back(new_history.history);
                        }
                        else {
                            valid_histories_play(new_I, I_2, new_true_board, 'o', new_history, end_I, played_actions, policy_obj_x, policy_obj_o, valid_histories_list);
                        }
                    }
                }
                else {
                    if (end_I.player == 'o') {
                        valid_histories_play(I_1, new_I, new_true_board, 'x', new_history, end_I, played_actions, policy_obj_x, policy_obj_o, valid_histories_list);
                    }
                    else {
                        if (I_1 == end_I){
                            valid_histories_list.push_back(new_history.history);
                        }
                        else {
                            valid_histories_play(I_1, new_I, new_true_board, 'x', new_history, end_I, played_actions, policy_obj_x, policy_obj_o, valid_histories_list);
                        }
                    }
                }
            }
        }
    }
    else {
        for (int action : actions) {
            InformationSet new_I(I);
            new_I.simulate_sense(action, true_board);
            TicTacToeBoard new_true_board = true_board;

            History new_history = current_history;
            new_history.history.push_back(action);

            if (player == 'x') {
                if (end_I.player == 'x') {
                    if (!(new_I == end_I)){
                        valid_histories_play(new_I, I_2, new_true_board, 'x', new_history, end_I, played_actions, policy_obj_x, policy_obj_o, valid_histories_list);
                    }
                    else {
                        valid_histories_list.push_back(new_history.history);
                    }
                }
                else {
                    valid_histories_play(new_I, I_2, new_true_board, 'x', new_history, end_I, played_actions, policy_obj_x, policy_obj_o, valid_histories_list);
                }
            }
            else {
                if (end_I.player == 'o') {
                    if (!(new_I == end_I)){
                        valid_histories_play(I_1, new_I, new_true_board, 'o', new_history, end_I, played_actions, policy_obj_x, policy_obj_o, valid_histories_list);
                    }
                    else {
                        valid_histories_list.push_back(new_history.history);
                    }
                }
                else {
                    valid_histories_play(I_1, new_I, new_true_board, 'o', new_history, end_I, played_actions, policy_obj_x, policy_obj_o, valid_histories_list);
                }
            }
        }
    }
}


void upgraded_get_histories_given_I(InformationSet& I, Policy& policy_obj_x, 
                                    Policy& policy_obj_o, vector<vector<int>>& valid_histories_list){
    
    if (I.get_hash() == "000000000m"){
        return;
    }

    string board_1 = "000000000";
    string board_2 = "---------";
    InformationSet I_1('x', true, board_1);
    InformationSet I_2('o', false, board_2);
    TicTacToeBoard true_board = TicTacToeBoard(EMPTY_BOARD);
    char player = 'x';
    vector<int> played_actions;
    I.get_played_actions(played_actions);

    History current_history({});
    valid_histories_play(I_1, I_2, true_board, player, current_history, I, played_actions, policy_obj_x, policy_obj_o, valid_histories_list);
    return;
}   


int main() {
    string file_path_1 = "data/Iterative_1/cfr_policy/P1_cfr_policy_round_2.json";
    string file_path_2 = "data/Iterative_1/cfr_policy/P2_cfr_policy_round_2.json";

    Policy policy_obj_x('x', file_path_1);
    Policy policy_obj_o('o', file_path_2);
    string board = "xoxoxx0o-";
    InformationSet I('x', true, board);
    vector<vector<int>> valid_histories_list;

    auto start = std::chrono::system_clock::now();   
    upgraded_get_histories_given_I(I, policy_obj_x, policy_obj_o, valid_histories_list);
    cout << "Number of valid histories: " << valid_histories_list.size() << endl;

    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end-start;
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);
 
    std::cout << "finished computation at " << std::ctime(&end_time)
              << "elapsed time: " << elapsed_seconds.count() << "s"
              << std::endl;

    return 0;
}