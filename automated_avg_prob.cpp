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

double get_prob_h_given_policy(InformationSet& I_1, InformationSet& I_2, 
                               TicTacToeBoard& true_board, char player, 
                               int next_action, Policy& policy_obj_x, 
                               Policy& policy_obj_o, double probability, 
                               History history_obj, char initial_player){

    InformationSet& I = player == 'x' ? I_1 : I_2;
    Policy& policy_obj = player == 'x' ? policy_obj_x : policy_obj_o;

    if (I.move_flag) {
        TicTacToeBoard new_true_board = true_board;
        bool success = new_true_board.update_move(next_action, player);

        if (I.player == initial_player) {
            probability *= policy_obj.policy_dict[I.get_hash()][next_action];
        }
        history_obj.track_traversal_index += 1;
        if (history_obj.track_traversal_index < history_obj.history.size()) {
            int new_next_action = history_obj.history[history_obj.track_traversal_index];

            char winner;
            if (success && !new_true_board.is_win(winner) && !new_true_board.is_over()) {
                InformationSet new_I(I);
                new_I.update_move(next_action, player);
                new_I.reset_zeros();

                if (player == 'x') {
                    probability = get_prob_h_given_policy(new_I, I_2, new_true_board, 'o', new_next_action, policy_obj_x, policy_obj_o, probability, history_obj, initial_player);
                }
                else {
                    probability = get_prob_h_given_policy(I_1, new_I, new_true_board, 'x', new_next_action, policy_obj_x, policy_obj_o, probability, history_obj, initial_player);
                }
            }
        }
    }
    else {
        InformationSet new_I(I);
        new_I.simulate_sense(next_action, true_board);
        TicTacToeBoard new_true_board = true_board;

        if (I.player == initial_player) {
            probability *= policy_obj.policy_dict[I.get_hash()][next_action];
        }
        history_obj.track_traversal_index += 1;
        if (history_obj.track_traversal_index < history_obj.history.size()) {
            int new_next_action = history_obj.history[history_obj.track_traversal_index];

            if (player == 'x') {
                probability = get_prob_h_given_policy(new_I, I_2, new_true_board, 'x', new_next_action, policy_obj_x, policy_obj_o, probability, history_obj, initial_player);
            }
            else {
                probability = get_prob_h_given_policy(I_1, new_I, new_true_board, 'o', new_next_action, policy_obj_x, policy_obj_o, probability, history_obj, initial_player);
            }
        }
    }

    return probability;
}


double get_prob_h_given_policy_wrapper(InformationSet& I_1, InformationSet& I_2, 
                                       TicTacToeBoard& true_board, char player, 
                                       int next_action, Policy& policy_obj_x, 
                                       Policy& policy_obj_o, double probability,
                                       History history_obj, InformationSet& curr_I_1, char initial_player){
    
    if (curr_I_1.get_hash() == "000000000m"){
        return 1.0;
    }
    else {
        return get_prob_h_given_policy(I_1, I_2, true_board, player, next_action, policy_obj_x, policy_obj_o, probability, history_obj, initial_player);
    }
}


void get_probability_of_reaching_all_h(InformationSet &I, Policy& policy_obj_x,
                                       Policy& policy_obj_o, vector<vector<int>>& starting_histories,
                                       char initial_player, vector<double>& prob_reaching_h_list_all) {

    for (vector<int> history: starting_histories) {
        NonTerminalHistory h_object = NonTerminalHistory(history);
        double prob_reaching_h;

        if (I.get_hash() == "000000000m") {
            prob_reaching_h = 1.0;
        }
        else {
            string board_1 = "000000000";
            string board_2 = "---------";
            string board = "000000000";

            InformationSet I_1('x', true, board_1);
            InformationSet I_2('o', false, board_2);
            TicTacToeBoard true_board = TicTacToeBoard(board);
            
            prob_reaching_h = get_prob_h_given_policy_wrapper(I_1, I_2, true_board, 'x', history[0], policy_obj_x, policy_obj_o, 1.0, h_object, I, initial_player);
        }

        prob_reaching_h_list_all.push_back(prob_reaching_h);
    }

    return;
}


double get_probability_of_reaching_I(InformationSet& I, Policy& policy_obj_x, Policy& policy_obj_o, char initial_player) {
    vector<vector<int>> starting_histories;
    vector<double> prob_reaching_h_list_all;

    upgraded_get_histories_given_I(I, policy_obj_x, policy_obj_o, starting_histories);
    get_probability_of_reaching_all_h(I, policy_obj_x, policy_obj_o, starting_histories, initial_player, prob_reaching_h_list_all);

    double prob_reaching_I = 0.0;
    for (double prob_reaching_h: prob_reaching_h_list_all) {
        prob_reaching_I += prob_reaching_h;
    }

    return prob_reaching_I;
}


int main() {
    string file_path_1 = "data/Iterative_1/cfr_policy/P1_cfr_policy_round_2.json";
    string file_path_2 = "data/Iterative_1/cfr_policy/P2_cfr_policy_round_2.json";

    Policy policy_obj_x('x', file_path_1);
    Policy policy_obj_o('o', file_path_2);
    string board = "xoxoxx0o-";
    InformationSet I('x', true, board);
    // vector<vector<int>> valid_histories_list;

    auto start = std::chrono::system_clock::now();   
    // upgraded_get_histories_given_I(I, policy_obj_x, policy_obj_o, valid_histories_list);
    // cout << "Number of valid histories: " << valid_histories_list.size() << endl;

    double prob_reaching_I = get_probability_of_reaching_I(I, policy_obj_x, policy_obj_o, 'x');
    cout << "Probability of reaching I: " << prob_reaching_I << endl;

    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end-start;
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);
 
    std::cout << "finished computation at " << std::ctime(&end_time)
              << "elapsed time: " << elapsed_seconds.count() << "s"
              << std::endl;

    return 0;
}