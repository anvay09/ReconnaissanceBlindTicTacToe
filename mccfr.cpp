#include "cpp_headers/rbt_classes.hpp"
#include "cpp_headers/rbt_utilities.hpp"
#include <random>


bool get_move_flag(std::string I_hash, char player){
    bool move_flag;
    if (I_hash.size() != 0){
        move_flag = I_hash[I_hash.size()-1] == '|' ? true : false;
    }
    else {
        move_flag = player == 'x' ? true : false;
    }
    return move_flag;
}


void simulate_opponent_turn(TicTacToeBoard& true_board, History& history, double reach_probability, InformationSet& opponent_I, PolicyVec& policy_obj,  
                            std::vector<TicTacToeBoard>& true_board_list, std::vector<History>& history_list, std::vector<double>& reach_probability_list,  
                            std::vector<InformationSet>& opponent_I_list, std::vector<double>& Q_values, char br_player, int played_action) {
    std::vector<int> depth_1_opponent_actions;
    opponent_I.get_actions_given_policy(depth_1_opponent_actions, policy_obj);
    std::vector<History> depth_2_history_list;
    std::vector<double> depth_2_reach_probability_list;
    std::vector<InformationSet> depth_2_opponent_I_list;
    std::vector<TicTacToeBoard> depth_2_true_board_list;

    // first simulate sense
    for (int opponent_action : depth_1_opponent_actions) {
        InformationSet depth_2_opponent_I = opponent_I;
        depth_2_opponent_I.simulate_sense(opponent_action, true_board);

        History depth_2_history = history;
        depth_2_history.history.push_back(opponent_action);

        double depth_2_reach_probability = reach_probability * policy_obj.policy_dict[opponent_I.get_index()][opponent_action];
        TicTacToeBoard depth_2_true_board = true_board;

        depth_2_reach_probability_list.push_back(depth_2_reach_probability);
        depth_2_history_list.push_back(depth_2_history);
        depth_2_opponent_I_list.push_back(depth_2_opponent_I);
        depth_2_true_board_list.push_back(depth_2_true_board);
    }

    for (int i = 0; i < depth_2_history_list.size(); i++) {
        InformationSet depth_2_opponent_I = depth_2_opponent_I_list[i];
        std::vector<int> depth_2_opponent_actions;
        depth_2_opponent_I.get_actions_given_policy(depth_2_opponent_actions, policy_obj);

        for (int opponent_action : depth_2_opponent_actions) {
            TicTacToeBoard depth_3_true_board = depth_2_true_board_list[i];
            History depth_3_history = depth_2_history_list[i];
            double depth_3_reach_probability = depth_2_reach_probability_list[i] * policy_obj.policy_dict[depth_2_opponent_I_list[i].get_index()][opponent_action];
            
            char winner;
            bool success = depth_3_true_board.update_move(opponent_action, depth_2_opponent_I_list[i].player);
            depth_3_history.history.push_back(opponent_action);

            if (success && !depth_3_true_board.is_win(winner) && !depth_3_true_board.is_over()) {
                InformationSet depth_3_opponent_I = depth_2_opponent_I_list[i];
                depth_3_opponent_I.update_move(opponent_action, depth_3_opponent_I.player);
                depth_3_opponent_I.reset_zeros();

                true_board_list.push_back(depth_3_true_board);
                history_list.push_back(depth_3_history);
                reach_probability_list.push_back(depth_3_reach_probability);
                opponent_I_list.push_back(depth_3_opponent_I);
            }
            else {
                TerminalHistory H_T = TerminalHistory(depth_3_history.history);
                H_T.set_reward();

                if (br_player == 'x'){
                    Q_values[played_action] += H_T.reward[0] * depth_3_reach_probability;
                }
                else{
                    Q_values[played_action] += H_T.reward[1] * depth_3_reach_probability;
                }
            }
        }
    }
}


double get_max_Q_value_and_update_policy(std::vector<double>& Q_values, std::vector<int>& actions, PolicyVec& br, InformationSet& I) {
    double max_Q = -1.0;
    int best_action = -1;

    for (int a = 0; a < actions.size(); a++) {
        if (Q_values[actions[a]] >= max_Q) {
            max_Q = Q_values[actions[a]];
            best_action = actions[a];
        }
    }

    std::vector<double>& prob_dist = br.policy_dict[I.get_index()];
    for (int k = 0; k < prob_dist.size(); k++) {
        if (k == best_action) {
            prob_dist[k] = 1.0;
        } 
        else {
            prob_dist[k] = 0.0;
        }
    }

    return max_Q;
}


double compute_best_response(InformationSet& I, char br_player, std::vector<TicTacToeBoard>& true_board_list, std::vector<History>& history_list, 
                 std::vector<double>& reach_probability_list, std::vector<InformationSet>& opponent_I_list, PolicyVec& br, PolicyVec& policy_obj) {    
    std::vector<int> actions;
    std::vector<double> Q_values;
    I.get_actions(actions);
    for (int i = 0; i < 13; i++) {
        Q_values.push_back(0.0);
    }

    if (I.move_flag) {
        for (int a = 0; a < actions.size(); a++) {
            std::vector<TicTacToeBoard> depth_3_true_board_list;
            std::vector<History> depth_3_history_list;
            std::vector<double> depth_3_reach_probability_list;
            std::vector<InformationSet> depth_3_opponent_I_list;
            
            for (int h = 0; h < history_list.size(); h++) {
                TicTacToeBoard depth_1_true_board = true_board_list[h];
                History depth_1_history = history_list[h];
                double depth_1_reach_probability = reach_probability_list[h];
                InformationSet depth_1_opponent_I = opponent_I_list[h];

                bool success = depth_1_true_board.update_move(actions[a], I.player);
                depth_1_history.history.push_back(actions[a]);

                char winner;
                if (success && !depth_1_true_board.is_win(winner) && !depth_1_true_board.is_over()) {
                    InformationSet new_I = I;
                    new_I.update_move(actions[a], I.player);
                    new_I.reset_zeros();
                    // simulate opponent's turn
                    
                    simulate_opponent_turn(depth_1_true_board, depth_1_history, depth_1_reach_probability, depth_1_opponent_I, policy_obj, depth_3_true_board_list, depth_3_history_list, depth_3_reach_probability_list, depth_3_opponent_I_list, Q_values, br_player, actions[a]);
                }
                else {
                    TerminalHistory H_T = TerminalHistory(depth_1_history.history);
                    H_T.set_reward();

                    if (br_player == 'x'){
                        Q_values[actions[a]] += H_T.reward[0] * depth_1_reach_probability;
                    }
                    else{
                        Q_values[actions[a]] += H_T.reward[1] * depth_1_reach_probability;
                    }
                }
            }

            if (depth_3_history_list.size() > 0) {
                InformationSet new_I = I;
                new_I.update_move(actions[a], I.player);
                new_I.reset_zeros();
                Q_values[actions[a]] += compute_best_response(new_I, br_player, depth_3_true_board_list, depth_3_history_list, depth_3_reach_probability_list, depth_3_opponent_I_list, br, policy_obj);
            }
        }
    }
    else {
        for (int a = 0; a < actions.size(); a++) {
            std::unordered_map<std::string, std::vector<TicTacToeBoard>> infoset_to_true_board;
            std::unordered_map<std::string, std::vector<History>> infoset_to_history;
            std::unordered_map<std::string, std::vector<double>> infoset_to_reach_probability;
            std::unordered_map<std::string, std::vector<InformationSet>> infoset_to_opponent_I;
            std::unordered_set<std::string> infoset_set;

            for (int h = 0; h < history_list.size(); h++) {
                TicTacToeBoard& true_board = true_board_list[h];
                History& history = history_list[h];
                double reach_probability = reach_probability_list[h];

                InformationSet new_I = I;
                new_I.simulate_sense(actions[a], true_board);

                History new_history = history;
                new_history.history.push_back(actions[a]);

                infoset_to_true_board[new_I.hash].push_back(true_board);
                infoset_to_history[new_I.hash].push_back(new_history);
                infoset_to_reach_probability[new_I.hash].push_back(reach_probability);
                infoset_to_opponent_I[new_I.hash].push_back(opponent_I_list[h]);
                infoset_set.insert(new_I.hash);
            }
            
            for (int t = 0; t < infoset_set.size(); t++) {
                std::string new_I_hash = *std::next(infoset_set.begin(), t);
                bool move_flag = get_move_flag(new_I_hash, I.player);
                InformationSet new_I(I.player, move_flag, new_I_hash);
  
                if (infoset_to_history[new_I.hash].size() > 0) {
                    Q_values[actions[a]] += compute_best_response(new_I, br_player, infoset_to_true_board[new_I.hash], infoset_to_history[new_I.hash], infoset_to_reach_probability[new_I.hash], infoset_to_opponent_I[new_I.hash], br, policy_obj);
                }
            }
        }
    }

    return get_max_Q_value_and_update_policy(Q_values, actions, br, I);
}


double compute_best_response_parallel(InformationSet& I, char br_player, std::vector<TicTacToeBoard>& true_board_list, std::vector<History>& history_list, 
                 std::vector<double>& reach_probability_list, std::vector<InformationSet>& opponent_I_list, PolicyVec& br, PolicyVec& policy_obj) {    
    std::vector<int> actions;
    std::vector<double> Q_values;
    I.get_actions(actions);
    for (int i = 0; i < 13; i++) {
        Q_values.push_back(0.0);
    }

    if (I.move_flag) {
        std::unordered_map<int, std::vector<TicTacToeBoard>> action_to_true_board_list;
        std::unordered_map<int, std::vector<History>> action_to_history_list;
        std::unordered_map<int, std::vector<double>> action_to_reach_probability_list;
        std::unordered_map<int, std::vector<InformationSet>> action_to_opponent_I_list; 

        for (int a = 0; a < actions.size(); a++) {
            std::vector<TicTacToeBoard> depth_3_true_board_list;
            std::vector<History> depth_3_history_list;
            std::vector<double> depth_3_reach_probability_list;
            std::vector<InformationSet> depth_3_opponent_I_list;
            
            for (int h = 0; h < history_list.size(); h++) {
                TicTacToeBoard depth_1_true_board = true_board_list[h];
                History depth_1_history = history_list[h];
                double depth_1_reach_probability = reach_probability_list[h];
                InformationSet depth_1_opponent_I = opponent_I_list[h];

                bool success = depth_1_true_board.update_move(actions[a], I.player);
                depth_1_history.history.push_back(actions[a]);

                char winner;
                if (success && !depth_1_true_board.is_win(winner) && !depth_1_true_board.is_over()) {
                    InformationSet new_I = I;
                    new_I.update_move(actions[a], I.player);
                    new_I.reset_zeros();

                    // simulate opponent's turn     
                    simulate_opponent_turn(depth_1_true_board, depth_1_history, depth_1_reach_probability, depth_1_opponent_I, policy_obj, depth_3_true_board_list, depth_3_history_list, depth_3_reach_probability_list, depth_3_opponent_I_list, Q_values, br_player, actions[a]);
                }
                else {
                    TerminalHistory H_T = TerminalHistory(depth_1_history.history);
                    H_T.set_reward();

                    if (br_player == 'x'){
                        Q_values[actions[a]] += H_T.reward[0] * depth_1_reach_probability;
                    }
                    else{
                        Q_values[actions[a]] += H_T.reward[1] * depth_1_reach_probability;
                    }
                }
            }

            action_to_true_board_list[actions[a]] = depth_3_true_board_list;
            action_to_history_list[actions[a]] = depth_3_history_list;
            action_to_reach_probability_list[actions[a]] = depth_3_reach_probability_list;
            action_to_opponent_I_list[actions[a]] = depth_3_opponent_I_list;
        }

        std::unordered_map<std::string, std::vector<TicTacToeBoard>> infoset_to_true_board;
        std::unordered_map<std::string, std::vector<History>> infoset_to_history;
        std::unordered_map<std::string, std::vector<double>> infoset_to_reach_probability;
        std::unordered_map<std::string, std::vector<InformationSet>> infoset_to_opponent_I;
        std::unordered_map<std::string, int> infoset_to_first_action_taken;
        std::unordered_map<std::string, int> infoset_to_second_action_taken;
        std::unordered_set<std::string> infoset_set;

        std::vector<std::vector<double>> depth_4_Q_values;
        std::vector<std::vector<int>> depth_4_actions;

        for (int i = 0; i < 13; i++) {
            std::vector<double> Q_value_vector;
            std::vector<int> action_vector;

            for (int j = 0; j < 13; j++) {
                Q_value_vector.push_back(0.0);
            }

            depth_4_Q_values.push_back(Q_value_vector);
            depth_4_actions.push_back(action_vector);
        }

        for (int a = 0; a < actions.size(); a++) {
            if (action_to_history_list[actions[a]].size() > 0) {
                InformationSet new_I = I;
                new_I.update_move(actions[a], I.player);
                new_I.reset_zeros();
    
                new_I.get_actions(depth_4_actions[actions[a]]);

                for (int b = 0; b < depth_4_actions[actions[a]].size(); b++) {
                    for (int h = 0; h < action_to_history_list[actions[a]].size(); h++) {
                        TicTacToeBoard& true_board = action_to_true_board_list[actions[a]][h];
                        History& history = action_to_history_list[actions[a]][h];
                        double reach_probability = action_to_reach_probability_list[actions[a]][h];

                        InformationSet depth_4_I = new_I;
                        depth_4_I.simulate_sense(depth_4_actions[actions[a]][b], true_board);

                        History depth_4_history = history;
                        depth_4_history.history.push_back(depth_4_actions[actions[a]][b]);

                        infoset_to_true_board[depth_4_I.hash].push_back(true_board);
                        infoset_to_history[depth_4_I.hash].push_back(depth_4_history);
                        infoset_to_reach_probability[depth_4_I.hash].push_back(reach_probability);
                        infoset_to_opponent_I[depth_4_I.hash].push_back(action_to_opponent_I_list[actions[a]][h]);
                        infoset_to_first_action_taken[depth_4_I.hash] = actions[a];
                        infoset_to_second_action_taken[depth_4_I.hash] = depth_4_actions[actions[a]][b];

                        infoset_set.insert(depth_4_I.hash);
                    }
                }
            }
        }

        # pragma omp parallel for num_threads(96)
        for (int t = 0; t < infoset_set.size(); t++) {
            std::string new_I_hash = *std::next(infoset_set.begin(), t);
            bool move_flag = get_move_flag(new_I_hash, I.player);
            InformationSet new_I(I.player, move_flag, new_I_hash);

            int a_val = infoset_to_first_action_taken[new_I.hash];
            int b_val = infoset_to_second_action_taken[new_I.hash];

            if (infoset_to_history[new_I.hash].size() > 0) {
                depth_4_Q_values[a_val][b_val] += compute_best_response(new_I, br_player, infoset_to_true_board[new_I.hash], infoset_to_history[new_I.hash], infoset_to_reach_probability[new_I.hash], infoset_to_opponent_I[new_I.hash], br, policy_obj);
            }
        }

        # pragma omp parallel for num_threads(96)
        for (int a = 0; a < actions.size(); a++) {
            if (action_to_history_list[actions[a]].size() > 0) {
                InformationSet new_I = I;
                new_I.update_move(actions[a], I.player);
                new_I.reset_zeros();

                Q_values[actions[a]] = get_max_Q_value_and_update_policy(depth_4_Q_values[actions[a]], depth_4_actions[actions[a]], br, new_I);
            }
        }
    }
    else {
        std::unordered_map<std::string, std::vector<TicTacToeBoard>> infoset_to_true_board;
        std::unordered_map<std::string, std::vector<History>> infoset_to_history;
        std::unordered_map<std::string, std::vector<double>> infoset_to_reach_probability;
        std::unordered_map<std::string, std::vector<InformationSet>> infoset_to_opponent_I;
        std::unordered_map<std::string, int> infoset_to_action_taken;
        std::unordered_set<std::string> infoset_set;

        for (int a = 0; a < actions.size(); a++) {
            for (int h = 0; h < history_list.size(); h++) {
                TicTacToeBoard& true_board = true_board_list[h];
                History& history = history_list[h];
                double reach_probability = reach_probability_list[h];

                InformationSet new_I = I;
                new_I.simulate_sense(actions[a], true_board);

                History new_history = history;
                new_history.history.push_back(actions[a]);

                infoset_to_true_board[new_I.hash].push_back(true_board);
                infoset_to_history[new_I.hash].push_back(new_history);
                infoset_to_reach_probability[new_I.hash].push_back(reach_probability);
                infoset_to_opponent_I[new_I.hash].push_back(opponent_I_list[h]);
                infoset_to_action_taken[new_I.hash] = actions[a];
                infoset_set.insert(new_I.hash);
            }
        }
        
        std::unordered_map<std::string, std::vector<TicTacToeBoard>> depth_2_infoset_to_true_board;
        std::unordered_map<std::string, std::vector<History>> depth_2_infoset_to_history;
        std::unordered_map<std::string, std::vector<double>> depth_2_infoset_to_reach_probability;
        std::unordered_map<std::string, std::vector<InformationSet>> depth_2_infoset_to_opponent_I;
        std::unordered_map<std::string, std::string> depth_2_infoset_to_parent;
        std::unordered_map<std::string, int> depth_2_infoset_to_second_action_taken;
        std::unordered_set<std::string> depth_2_infoset_set;

        std::unordered_map<std::string, std::vector<double>> depth_2_Q_values;
        std::unordered_map<std::string, std::vector<int>> depth_2_actions;

        for (int t = 0; t < infoset_set.size(); t++) {
            std::vector<double> Q_value_vector;
            std::vector<int> action_vector;

            for (int i = 0; i < 13; i++) {
                Q_value_vector.push_back(0.0);
            }

            std::string new_I_hash = *std::next(infoset_set.begin(), t);
            bool move_flag = get_move_flag(new_I_hash, I.player);
            InformationSet new_I(I.player, move_flag, new_I_hash);

            depth_2_Q_values[new_I.hash] = Q_value_vector;
            depth_2_actions[new_I.hash] = action_vector;
        }

        for (int t = 0; t < infoset_set.size(); t++) {
            std::string new_I_hash = *std::next(infoset_set.begin(), t);
            bool move_flag = get_move_flag(new_I_hash, I.player);
            InformationSet new_I(I.player, move_flag, new_I_hash);

            if (infoset_to_history[new_I.hash].size() > 0) {
                int a_val = infoset_to_action_taken[new_I.hash];
                new_I.get_actions(depth_2_actions[new_I.hash]);
               
                for (int b = 0; b < depth_2_actions[new_I.hash].size(); b++) {
                    std::vector<TicTacToeBoard> depth_4_true_board_list;
                    std::vector<History> depth_4_history_list;
                    std::vector<double> depth_4_reach_probability_list;
                    std::vector<InformationSet> depth_4_opponent_I_list;

                    for (int h = 0; h < infoset_to_history[new_I.hash].size(); h++) {
                        TicTacToeBoard depth_2_true_board = infoset_to_true_board[new_I.hash][h];
                        History depth_2_history = infoset_to_history[new_I.hash][h];
                        double depth_2_reach_probability = infoset_to_reach_probability[new_I.hash][h];
                        InformationSet depth_2_opponent_I = infoset_to_opponent_I[new_I.hash][h];

                        bool success = depth_2_true_board.update_move(depth_2_actions[new_I.hash][b], new_I.player);
                        depth_2_history.history.push_back(depth_2_actions[new_I.hash][b]);

                        char winner;
                        if (success && !depth_2_true_board.is_win(winner) && !depth_2_true_board.is_over()) {
                            InformationSet depth_3_I = new_I;
                            depth_3_I.update_move(depth_2_actions[new_I.hash][b], new_I.player);
                            depth_3_I.reset_zeros();

                            simulate_opponent_turn(depth_2_true_board, depth_2_history, depth_2_reach_probability, depth_2_opponent_I, policy_obj, depth_4_true_board_list, depth_4_history_list, depth_4_reach_probability_list, depth_4_opponent_I_list, depth_2_Q_values[new_I.hash], br_player, depth_2_actions[new_I.hash][b]);
                        }
                        else {
                            TerminalHistory H_T = TerminalHistory(depth_2_history.history);
                            H_T.set_reward();

                            if (br_player == 'x'){
                                depth_2_Q_values[new_I.hash][depth_2_actions[new_I.hash][b]] += H_T.reward[0] * depth_2_reach_probability;
                            }
                            else{
                                depth_2_Q_values[new_I.hash][depth_2_actions[new_I.hash][b]] += H_T.reward[1] * depth_2_reach_probability;
                            }
                        }
                    }

                    InformationSet depth_3_I = new_I;
                    depth_3_I.update_move(depth_2_actions[new_I.hash][b], new_I.player);
                    depth_3_I.reset_zeros();

                    depth_2_infoset_to_true_board[depth_3_I.hash] = depth_4_true_board_list;
                    depth_2_infoset_to_history[depth_3_I.hash] = depth_4_history_list;
                    depth_2_infoset_to_reach_probability[depth_3_I.hash] = depth_4_reach_probability_list;
                    depth_2_infoset_to_opponent_I[depth_3_I.hash] = depth_4_opponent_I_list;
                    depth_2_infoset_to_parent[depth_3_I.hash] = new_I.hash;
                    depth_2_infoset_to_second_action_taken[depth_3_I.hash] = depth_2_actions[new_I.hash][b];

                    depth_2_infoset_set.insert(depth_3_I.hash);
                }
            }
        }
        
        # pragma omp parallel for num_threads(96)
        for (int t = 0; t < depth_2_infoset_set.size(); t++) {
            std::string new_I_hash = *std::next(depth_2_infoset_set.begin(), t);
            bool move_flag = get_move_flag(new_I_hash, I.player);
            InformationSet new_I(I.player, move_flag, new_I_hash);

            std::string parent_hash = depth_2_infoset_to_parent[new_I.hash];
            int b_val = depth_2_infoset_to_second_action_taken[new_I.hash];

            if (depth_2_infoset_to_history[new_I.hash].size() > 0) {
                depth_2_Q_values[parent_hash][b_val] += compute_best_response(new_I, br_player, depth_2_infoset_to_true_board[new_I.hash], depth_2_infoset_to_history[new_I.hash], depth_2_infoset_to_reach_probability[new_I.hash], depth_2_infoset_to_opponent_I[new_I.hash], br, policy_obj);
            }
        }

        for (int t = 0; t < infoset_set.size(); t++) {
            std::string new_I_hash = *std::next(infoset_set.begin(), t);
            bool move_flag = get_move_flag(new_I_hash, I.player);
            InformationSet new_I(I.player, move_flag, new_I_hash);

            if (infoset_to_history[new_I.hash].size() > 0) {
                int a_val = infoset_to_action_taken[new_I.hash];
                Q_values[a_val] += get_max_Q_value_and_update_policy(depth_2_Q_values[new_I.hash], depth_2_actions[new_I.hash], br, new_I);
            }
        }

    }
  
    return get_max_Q_value_and_update_policy(Q_values, actions, br, I);
}


double compute_best_response_wrapper(PolicyVec& policy_obj, PolicyVec& br, char br_player) {
    std::string board = "000000000";
    TicTacToeBoard true_board = TicTacToeBoard(board);
    std::string hash_1 = "";
    std::string hash_2 = "";
    InformationSet I_1 = InformationSet('x', true, hash_1);
    InformationSet I_2 = InformationSet('o', false, hash_2);
    std::vector<int> h = {};
    TerminalHistory start_history = TerminalHistory(h);

    std::vector<TicTacToeBoard> true_board_list;
    std::vector<History> history_list;
    std::vector<double> reach_probability_list;
    std::vector<InformationSet> opponent_I_list;

    double expected_utility = 0.0;

    if (br_player == 'x') {
        true_board_list.push_back(true_board);
        history_list.push_back(start_history);
        reach_probability_list.push_back(1.0);
        opponent_I_list.push_back(I_2);

        expected_utility = compute_best_response_parallel(I_1, br_player, true_board_list, history_list, reach_probability_list, opponent_I_list, br, policy_obj);
    } 
    else {
        std::vector<int> actions;
        I_1.get_actions_given_policy(actions, policy_obj);

        for (int a = 0; a < actions.size(); a++){
            TicTacToeBoard new_true_board = true_board;
            bool success = new_true_board.update_move(actions[a], I_1.player);

            History new_history = start_history;
            new_history.history.push_back(actions[a]);

            InformationSet new_I = I_1;
            new_I.update_move(actions[a], I_1.player);
            new_I.reset_zeros();

            true_board_list.push_back(new_true_board);
            history_list.push_back(new_history);
            reach_probability_list.push_back(policy_obj.policy_dict[I_1.get_index()][actions[a]]);
            opponent_I_list.push_back(new_I);
        }

        expected_utility = - compute_best_response_parallel(I_2, br_player, true_board_list, history_list, reach_probability_list, opponent_I_list, br, policy_obj);
    }

    return expected_utility;
}


int sampleIndex(const std::vector<double>& probabilities) {
    std::random_device rd;
    std::mt19937 generator(rd());
    std::discrete_distribution<int> distribution(probabilities.begin(), probabilities.end());
    return distribution(generator);
}


double sample_terminal_history(InformationSet& I_1, InformationSet& I_2, TicTacToeBoard& true_board, PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, History& current_history, char player, double probability, double& reward, char update_player) {
    InformationSet& I = player == 'x' ? I_1 : I_2;
    PolicyVec& policy_obj = player == 'x' ? policy_obj_x : policy_obj_o;
    std::vector<double> prob_dist = policy_obj.policy_dict[I.get_index()];

    if (player == update_player) { // explore with a small epsilon
        std::vector<int> actions;
        I.get_actions(actions);
        double eps = 0.1;
        double sum = 1.0;

        for (int i = 0; i < actions.size(); i++) {
            if (prob_dist[actions[i]] == 0.0) {
                prob_dist[actions[i]] = eps;
                sum += eps;
            }
        }
        // renormalize
        for (int i = 0; i < actions.size(); i++) {
            prob_dist[actions[i]] /= sum;
        }
    }

    int action = sampleIndex(prob_dist);

    if (I.move_flag) {
        bool success = true_board.update_move(action, player);

        if (player == update_player) { // update the probability only if the player is the one we are updating
            probability = probability * prob_dist[action];
        }

        current_history.history.push_back(action);

        char winner;
        if (success && !true_board.is_win(winner) && !true_board.is_over()) {
            InformationSet new_I = I;
            new_I.update_move(action, player);
            new_I.reset_zeros();

            if (player == 'x') {
                return sample_terminal_history(new_I, I_2, true_board, policy_obj_x, policy_obj_o, current_history, 'o', probability, reward, update_player);
            } else {
                return sample_terminal_history(I_1, new_I, true_board, policy_obj_x, policy_obj_o, current_history, 'x', probability, reward, update_player);
            }
        } else {
            TerminalHistory H_T = TerminalHistory(current_history.history);
            H_T.set_reward();
            reward = (double) H_T.reward[0];
            return probability;
        }
    }
    else {
        InformationSet new_I = I;
        new_I.simulate_sense(action, true_board);
        
        if (player == update_player) { // update the probability only if the player is the one we are updating
            probability = probability * prob_dist[action];
        }
        current_history.history.push_back(action);

        if (player == 'x') {
            return sample_terminal_history(new_I, I_2, true_board, policy_obj_x, policy_obj_o, current_history, 'x', probability, reward, update_player);
        } else {
            return sample_terminal_history(I_1, new_I, true_board, policy_obj_x, policy_obj_o, current_history, 'o', probability, reward, update_player);
        }
    }
}


double sample_terminal_history_wrapper(PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, History& current_history, double& reward, char update_player) {
    std::string board = "000000000";
    TicTacToeBoard true_board = TicTacToeBoard(board);
    std::string hash_1 = "";
    std::string hash_2 = "";
    InformationSet I_1 = InformationSet('x', true, hash_1);
    InformationSet I_2 = InformationSet('o', false, hash_2);
    return sample_terminal_history(I_1, I_2, true_board, policy_obj_x, policy_obj_o, current_history, 'x', 1.0, reward, update_player);
}


double compute_regrets_along_history(InformationSet& I_1, InformationSet& I_2, TicTacToeBoard& true_board, PolicyVec& best_response, char br_player, 
                                     std::vector<std::vector<double>>& regret_list, History& current_history, double q_z, double reward, int traversal_index, char current_player) {
    if (traversal_index == current_history.history.size()) {
        return 1.0;
    }

    InformationSet& I = current_player == 'x' ? I_1 : I_2;
    int action = current_history.history[traversal_index];
    traversal_index += 1;

    if (current_player == br_player){
        std::vector<int> actions;
        I.get_actions(actions);
        std::vector<double>& br_prob_dist = best_response.policy_dict[I.get_index()];
        std::vector<double>& regret_I = regret_list[I.get_index()];
        double played_action_prob = br_prob_dist[action];
        double reach_prob = 0.0;

        if (I.move_flag) {
            true_board.update_move(action, current_player);
            I.update_move(action, current_player);
            I.reset_zeros();

            if (current_player == 'x') {
                reach_prob = played_action_prob * compute_regrets_along_history(I, I_2, true_board, best_response, br_player, regret_list, current_history, q_z, reward, traversal_index, 'o');
            } else {
                reach_prob = played_action_prob * compute_regrets_along_history(I_1, I, true_board, best_response, br_player, regret_list, current_history, q_z, reward, traversal_index, 'x');
            }
        }
        else {
            I.simulate_sense(action, true_board);

            if (current_player == 'x') {
                reach_prob = played_action_prob * compute_regrets_along_history(I, I_2, true_board, best_response, br_player, regret_list, current_history, q_z, reward, traversal_index, 'x');
            } else {
                reach_prob = played_action_prob * compute_regrets_along_history(I_1, I, true_board, best_response, br_player, regret_list, current_history, q_z, reward, traversal_index, 'o');
            }
        }

        double regret_sum = 0.0;

        for (int i = 0; i < actions.size(); i++) {
            if (actions[i] == action) {
                regret_I[actions[i]] += (reward * reach_prob * (1 - played_action_prob)) / (q_z * played_action_prob);
            } else {
                regret_I[actions[i]] += -reward * reach_prob / q_z;
            }

            regret_sum += regret_I[actions[i]] > 0 ? regret_I[actions[i]] : 0;
        }

        // regret matching
        for (int i = 0; i < actions.size(); i++) {
            if (regret_sum > 0) {
                br_prob_dist[actions[i]] = regret_I[actions[i]] > 0 ? regret_I[actions[i]] / regret_sum : 0.0;
            } else {
                br_prob_dist[actions[i]] = 1.0 / actions.size();
            }
        }

        return reach_prob;
    }
    else {
        if (I.move_flag) {
            true_board.update_move(action, current_player);
            I.update_move(action, current_player);
            I.reset_zeros();

            if (current_player == 'x') {
                return compute_regrets_along_history(I, I_2, true_board, best_response, br_player, regret_list, current_history, q_z, reward, traversal_index, 'o');
            } else {
                return compute_regrets_along_history(I_1, I, true_board, best_response, br_player, regret_list, current_history, q_z, reward, traversal_index, 'x');
            }
        }
        else {
            I.simulate_sense(action, true_board);

            if (current_player == 'x') {
                return compute_regrets_along_history(I, I_2, true_board, best_response, br_player, regret_list, current_history, q_z, reward, traversal_index, 'x');
            } else {
                return compute_regrets_along_history(I_1, I, true_board, best_response, br_player, regret_list, current_history, q_z, reward, traversal_index, 'o');
            }
        }
    }
}


void mccfr_outcome_sampling_best_response(PolicyVec& policy_obj, PolicyVec& best_response, char br_player, int T, std::vector<std::string>& information_sets) {
    std::vector<std::vector<double>> regret_list;
    for (long int i = 0; i < information_sets.size(); i++) {
        regret_list.push_back(std::vector<double>(13, 0.0));
    }

    for (int t = 0; t < T; t++) {
        std::vector<int> h = {};
        TerminalHistory start_history = TerminalHistory(h);
        double q_z = 0.0;
        double reward = 0;

        if (br_player == 'x') {
            q_z = sample_terminal_history_wrapper(best_response, policy_obj, start_history, reward, br_player);
        } else {
            q_z = sample_terminal_history_wrapper(policy_obj, best_response, start_history, reward, br_player);
        }

        // traverse history and update regrets
        std::string board = "000000000";
        TicTacToeBoard true_board = TicTacToeBoard(board);
        std::string hash_1 = "";
        std::string hash_2 = "";
        InformationSet I_1 = InformationSet('x', true, hash_1);
        InformationSet I_2 = InformationSet('o', false, hash_2);

        compute_regrets_along_history(I_1, I_2, true_board, best_response, br_player, regret_list, start_history, q_z, reward, 0, 'x');        

        if (t % 500 == 0) {
            double expected_utility = 0.0;

            if (br_player == 'x'){
                expected_utility = get_expected_utility_wrapper(best_response, policy_obj);
            }
            else {
                expected_utility = get_expected_utility_wrapper(policy_obj, best_response);
            }

            std::cout << "Expected utility after iteration " << t << ": " << expected_utility << std::endl;
        }
    }
}


int main(int argc, char* argv[]) {
    std::cout.precision(17);
    std::string file_path_1 = argv[1];
    std::string file_path_2 = argv[2];
    int num_iterations = std::stoi(argv[3]);

    std::vector<std::string> P1_information_sets;
    std::vector<std::string> P2_information_sets;
    std::string P1_information_sets_file = "data/P1_information_sets_V2.txt";
    std::string P2_information_sets_file = "data/P2_information_sets_V2.txt";

    std::ifstream P1_f_is(P1_information_sets_file);
    std::string P1_line_is;
    while (std::getline(P1_f_is, P1_line_is)) {
        P1_information_sets.push_back(P1_line_is);
    }
    P1_f_is.close();

    std::ifstream P2_f_is(P2_information_sets_file);
    std::string P2_line_is;
    while (std::getline(P2_f_is, P2_line_is)) {
        P2_information_sets.push_back(P2_line_is);
    }
    P2_f_is.close();

    for (long int i = 0; i < P1_information_sets.size(); i++) {
        InformationSet::P1_hash_to_int_map[P1_information_sets[i]] = i;
    }
    for (long int i = 0; i < P2_information_sets.size(); i++) {
        InformationSet::P2_hash_to_int_map[P2_information_sets[i]] = i;
    }

    std::cout << "Loading policies..." << std::endl;

    PolicyVec policy_obj_x('x', file_path_1);
    PolicyVec policy_obj_o('o', file_path_2);
    PolicyVec br_x('x', P1_information_sets);

    std::cout << "Policies loaded." << std::endl;

    double expected_utility = compute_best_response_wrapper(policy_obj_o, br_x, 'x');
    std::cout << "Expected utility of the best response: " << expected_utility << std::endl;
    mccfr_outcome_sampling_best_response(policy_obj_o, policy_obj_x, 'x', num_iterations, P1_information_sets);
}