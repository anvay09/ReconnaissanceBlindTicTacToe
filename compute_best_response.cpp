#include "cpp_headers/rbt_classes.hpp"

// g++-13 -O3 evaluate_policy.cpp rbt_classes.cpp -o evaluate_policy -fopenmp

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


double get_expected_utility(InformationSet &I_1, InformationSet &I_2, TicTacToeBoard &true_board, char player, PolicyVec &policy_obj_x, 
                            PolicyVec &policy_obj_o, double probability, History& current_history, char initial_player) {
    double expected_utility_h = 0.0;
    
    InformationSet& I = player == 'x' ? I_1 : I_2;
    PolicyVec& policy_obj = player == 'x' ? policy_obj_x : policy_obj_o;
    
    std::vector<int> actions;
    I.get_actions_given_policy(actions, policy_obj);
    
    if (I.move_flag) {
        for (int action : actions) {
            TicTacToeBoard new_true_board = true_board;
            bool success = new_true_board.update_move(action, player);

            double probability_new = probability * policy_obj.policy_dict[I.get_index()][action];
            History new_history = current_history;
            new_history.history.push_back(action);
            
            char winner;
            if (success && !new_true_board.is_win(winner) && !new_true_board.is_over()) {
                InformationSet new_I = I;
                new_I.update_move(action, player);
                new_I.reset_zeros();
                
                if (player == 'x') {
                    expected_utility_h += get_expected_utility(new_I, I_2, new_true_board, 'o', policy_obj_x, policy_obj_o, probability_new, new_history, initial_player);
                } else {
                    expected_utility_h += get_expected_utility(I_1, new_I, new_true_board, 'x', policy_obj_x, policy_obj_o, probability_new, new_history, initial_player);
                }
            } else {
                TerminalHistory H_T = TerminalHistory(new_history.history);
                H_T.set_reward();
                if (initial_player == 'x'){
                    expected_utility_h += H_T.reward[0] * probability_new;
                }
                else{
                    expected_utility_h += H_T.reward[1] * probability_new;
                }
            }
        }
    } else {
        for (int action : actions) {
            InformationSet new_I = I;
            new_I.simulate_sense(action, true_board);
            
            double probability_new = probability * policy_obj.policy_dict[I.get_index()][action];
            History new_history = current_history;
            new_history.history.push_back(action);

            if (player == 'x') {
                expected_utility_h += get_expected_utility(new_I, I_2, true_board, 'x', policy_obj_x, policy_obj_o, probability_new, new_history, initial_player);
            } else {
                expected_utility_h += get_expected_utility(I_1, new_I, true_board, 'o', policy_obj_x, policy_obj_o, probability_new, new_history, initial_player);
            }
        }
    }

    return expected_utility_h;
}


double get_expected_utility_parallel(InformationSet &I_1, InformationSet &I_2, TicTacToeBoard &true_board, char player, PolicyVec &policy_obj_x, 
                                     PolicyVec &policy_obj_o, double probability, History& current_history, char initial_player) {
    double expected_utility_h = 0.0;
    std::vector<InformationSet> Depth_1_P1_Isets;
    std::vector<InformationSet> Depth_1_P2_Isets;
    std::vector<TicTacToeBoard> Depth_1_boards;
    std::vector<char> Depth_1_players;
    std::vector<double> Depth_1_probabilities;
    std::vector<History> Depth_1_histories;
    
    InformationSet I = player == 'x' ? I_1 : I_2;
    PolicyVec policy_obj = player == 'x' ? policy_obj_x : policy_obj_o;

    std::vector<int> actions;
    I.get_actions_given_policy(actions, policy_obj);

    if (I.move_flag) {
        for (int action : actions) {
            TicTacToeBoard new_true_board = true_board;
            bool success = new_true_board.update_move(action, player);

            double probability_new = probability * policy_obj.policy_dict[I.get_index()][action];
            History new_history = current_history;
            new_history.history.push_back(action);

            char winner;
            if (success && !new_true_board.is_win(winner) && !new_true_board.is_over()) {
                InformationSet new_I = I;
                new_I.update_move(action, player);
                new_I.reset_zeros();

                if (player == 'x') {
                    Depth_1_P1_Isets.push_back(new_I);
                    Depth_1_P2_Isets.push_back(I_2);
                    Depth_1_boards.push_back(new_true_board);
                    Depth_1_players.push_back('o');
                    Depth_1_probabilities.push_back(probability_new);
                    Depth_1_histories.push_back(new_history);
                } else {
                    Depth_1_P1_Isets.push_back(I_1);
                    Depth_1_P2_Isets.push_back(new_I);
                    Depth_1_boards.push_back(new_true_board);
                    Depth_1_players.push_back('x');
                    Depth_1_probabilities.push_back(probability_new);
                    Depth_1_histories.push_back(new_history);
                }
            } else {
                TerminalHistory H_T = TerminalHistory(new_history.history);
                H_T.set_reward();
                if (initial_player == 'x'){
                    expected_utility_h += H_T.reward[0] * probability_new;
                }
                else{
                    expected_utility_h += H_T.reward[1] * probability_new;
                }
            }
        
        }
    }
    else {
        for (int action : actions) {
            InformationSet new_I = I;
            new_I.simulate_sense(action, true_board);

            double probability_new = probability * policy_obj.policy_dict[I.get_index()][action];
            History new_history = current_history;
            new_history.history.push_back(action);

            if (player == 'x') {
                Depth_1_P1_Isets.push_back(new_I);
                Depth_1_P2_Isets.push_back(I_2);
                Depth_1_boards.push_back(true_board);
                Depth_1_players.push_back('x');
                Depth_1_probabilities.push_back(probability_new);
                Depth_1_histories.push_back(new_history);
            } else {
                Depth_1_P1_Isets.push_back(I_1);
                Depth_1_P2_Isets.push_back(new_I);
                Depth_1_boards.push_back(true_board);
                Depth_1_players.push_back('o');
                Depth_1_probabilities.push_back(probability_new);
                Depth_1_histories.push_back(new_history);
            }   
        }
    }

    # pragma omp parallel for num_threads(96)
    for (int i = 0; i < Depth_1_P1_Isets.size(); i++) {
        expected_utility_h += get_expected_utility(Depth_1_P1_Isets[i], Depth_1_P2_Isets[i], Depth_1_boards[i], Depth_1_players[i], policy_obj_x, policy_obj_o, Depth_1_probabilities[i], Depth_1_histories[i], initial_player);
    }

    return expected_utility_h;
}


double get_expected_utility_wrapper(PolicyVec& policy_obj_x, PolicyVec& policy_obj_o){
    std::string board = "000000000";
    TicTacToeBoard true_board = TicTacToeBoard(board);
    std::string hash_1 = "";
    std::string hash_2 = "";
    InformationSet I_1 = InformationSet('x', true, hash_1);
    InformationSet I_2 = InformationSet('o', false, hash_2);
    std::vector<int> h = {};
    TerminalHistory start_history = TerminalHistory(h);

    double expected_utility = get_expected_utility_parallel(I_1, I_2, true_board, 'x', policy_obj_x, policy_obj_o, 1, start_history, 'x');
    return expected_utility;
}


double compute_best_response(InformationSet& I, char br_player, std::vector<TicTacToeBoard>& true_board_list, std::vector<History>& history_list, 
                 std::vector<double>& reach_probability_list, std::vector<InformationSet>& opponent_I_list, PolicyVec& br, PolicyVec& policy_obj) {    
    double expected_utility = 0.0;

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
                    
                    std::vector<int> depth_1_opponent_actions;
                    depth_1_opponent_I.get_actions_given_policy(depth_1_opponent_actions, policy_obj);
                    std::vector<History> depth_2_history_list;
                    std::vector<double> depth_2_reach_probability_list;
                    std::vector<InformationSet> depth_2_opponent_I_list;
                    std::vector<TicTacToeBoard> depth_2_true_board_list;

                    // first simulate sense
                    for (int opponent_action : depth_1_opponent_actions) {
                        InformationSet depth_2_opponent_I = depth_1_opponent_I;
                        depth_2_opponent_I.simulate_sense(opponent_action, depth_1_true_board);

                        History depth_2_history = depth_1_history;
                        depth_2_history.history.push_back(opponent_action);

                        double depth_2_reach_probability = depth_1_reach_probability * policy_obj.policy_dict[depth_1_opponent_I.get_index()][opponent_action];
                        TicTacToeBoard depth_2_true_board = depth_1_true_board;

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
                            
                            success = depth_3_true_board.update_move(opponent_action, depth_2_opponent_I_list[i].player);
                            depth_3_history.history.push_back(opponent_action);

                            if (success && !depth_3_true_board.is_win(winner) && !depth_3_true_board.is_over()) {
                                InformationSet depth_3_opponent_I = depth_2_opponent_I_list[i];
                                depth_3_opponent_I.update_move(opponent_action, depth_3_opponent_I.player);
                                depth_3_opponent_I.reset_zeros();

                                depth_3_true_board_list.push_back(depth_3_true_board);
                                depth_3_history_list.push_back(depth_3_history);
                                depth_3_reach_probability_list.push_back(depth_3_reach_probability);
                                depth_3_opponent_I_list.push_back(depth_3_opponent_I);
                            }
                            else {
                                TerminalHistory H_T = TerminalHistory(depth_3_history.history);
                                H_T.set_reward();

                                if (br_player == 'x'){
                                    Q_values[actions[a]] += H_T.reward[0] * depth_3_reach_probability;
                                }
                                else{
                                    Q_values[actions[a]] += H_T.reward[1] * depth_3_reach_probability;
                                }
                            }
                        }
                    }
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
    
    expected_utility = max_Q;
    return expected_utility;
}


double compute_best_response_parallel(InformationSet& I, char br_player, std::vector<TicTacToeBoard>& true_board_list, std::vector<History>& history_list, 
                 std::vector<double>& reach_probability_list, std::vector<InformationSet>& opponent_I_list, PolicyVec& br, PolicyVec& policy_obj) {    
    double expected_utility = 0.0;

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
                    
                    std::vector<int> depth_1_opponent_actions;
                    depth_1_opponent_I.get_actions_given_policy(depth_1_opponent_actions, policy_obj);
                    std::vector<History> depth_2_history_list;
                    std::vector<double> depth_2_reach_probability_list;
                    std::vector<InformationSet> depth_2_opponent_I_list;
                    std::vector<TicTacToeBoard> depth_2_true_board_list;

                    // first simulate sense
                    for (int opponent_action : depth_1_opponent_actions) {
                        InformationSet depth_2_opponent_I = depth_1_opponent_I;
                        depth_2_opponent_I.simulate_sense(opponent_action, depth_1_true_board);

                        History depth_2_history = depth_1_history;
                        depth_2_history.history.push_back(opponent_action);

                        double depth_2_reach_probability = depth_1_reach_probability * policy_obj.policy_dict[depth_1_opponent_I.get_index()][opponent_action];
                        TicTacToeBoard depth_2_true_board = depth_1_true_board;

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
                            
                            success = depth_3_true_board.update_move(opponent_action, depth_2_opponent_I_list[i].player);
                            depth_3_history.history.push_back(opponent_action);

                            if (success && !depth_3_true_board.is_win(winner) && !depth_3_true_board.is_over()) {
                                InformationSet depth_3_opponent_I = depth_2_opponent_I_list[i];
                                depth_3_opponent_I.update_move(opponent_action, depth_3_opponent_I.player);
                                depth_3_opponent_I.reset_zeros();

                                depth_3_true_board_list.push_back(depth_3_true_board);
                                depth_3_history_list.push_back(depth_3_history);
                                depth_3_reach_probability_list.push_back(depth_3_reach_probability);
                                depth_3_opponent_I_list.push_back(depth_3_opponent_I);
                            }
                            else {
                                TerminalHistory H_T = TerminalHistory(depth_3_history.history);
                                H_T.set_reward();

                                if (br_player == 'x'){
                                    Q_values[actions[a]] += H_T.reward[0] * depth_3_reach_probability;
                                }
                                else{
                                    Q_values[actions[a]] += H_T.reward[1] * depth_3_reach_probability;
                                }
                            }
                        }
                    }
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

                double max_Q = -1.0;
                int best_action = -1;

                for (int b = 0; b < depth_4_actions[actions[a]].size(); b++) {
                    if (depth_4_Q_values[actions[a]][depth_4_actions[actions[a]][b]] >= max_Q) {
                        max_Q = depth_4_Q_values[actions[a]][depth_4_actions[actions[a]][b]];
                        best_action = depth_4_actions[actions[a]][b];
                    }
                }

                std::vector<double>& prob_dist = br.policy_dict[new_I.get_index()];
                for (int k = 0; k < prob_dist.size(); k++) {
                    if (k == best_action) {
                        prob_dist[k] = 1.0;
                    } 
                    else {
                        prob_dist[k] = 0.0;
                    }
                }

                Q_values[actions[a]] = max_Q;
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
        
        # pragma omp parallel for num_threads(96)
        for (int t = 0; t < infoset_set.size(); t++) {
            std::string new_I_hash = *std::next(infoset_set.begin(), t);
            bool move_flag = get_move_flag(new_I_hash, I.player);
            InformationSet new_I(I.player, move_flag, new_I_hash);

            if (infoset_to_history[new_I.hash].size() > 0) {
                Q_values[infoset_to_action_taken[new_I.hash]] += compute_best_response(new_I, br_player, infoset_to_true_board[new_I.hash], infoset_to_history[new_I.hash], infoset_to_reach_probability[new_I.hash], infoset_to_opponent_I[new_I.hash], br, policy_obj);
            }
        }
    }
  
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

    expected_utility = max_Q;
    return expected_utility;
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


int main(int argc, char* argv[]) {
    std::cout.precision(17);
    std::string file_path_1 = argv[1];
    std::string file_path_2 = argv[2];

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
    char player = 'x';
    PolicyVec policy_obj_x('x', file_path_1);
    PolicyVec policy_obj_o('o', file_path_2);
    PolicyVec br_x('x', P1_information_sets);
    PolicyVec br_o('o', P2_information_sets);
    
    std::cout << "Policies loaded." << std::endl;
    std::cout << "Getting expected utility..." << std::endl;
    auto start = std::chrono::system_clock::now();   
    
    double exploitability = 0.0;
    double expected_utility = get_expected_utility_wrapper(policy_obj_x, policy_obj_o);
    std::cout << "Expected utility: " << expected_utility << std::endl;

    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end-start;
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);
    std::cout << "finished computation at " << std::ctime(&end_time)
              << "elapsed time: " << elapsed_seconds.count() << "s"
              << std::endl;

    std::cout << "Computing best response..." << std::endl;
    start = std::chrono::system_clock::now();

    expected_utility = compute_best_response_wrapper(policy_obj_o, br_x, 'x');
    exploitability = expected_utility;
    std::cout << "Expected utility of best response against P2: " << expected_utility << std::endl;

    end = std::chrono::system_clock::now();
    elapsed_seconds = end-start;
    end_time = std::chrono::system_clock::to_time_t(end);
    std::cout << "finished computation at " << std::ctime(&end_time)
              << "elapsed time: " << elapsed_seconds.count() << "s"
              << std::endl;

    expected_utility = compute_best_response_wrapper(policy_obj_x, br_o, 'o');
    exploitability -= expected_utility;
    std::cout << "Expected utility of best response against P1: " << expected_utility << std::endl;
    std::cout << "Exploitability: " << exploitability << std::endl;

    end = std::chrono::system_clock::now();
    elapsed_seconds = end-start;
    end_time = std::chrono::system_clock::to_time_t(end);
    std::cout << "finished computation at " << std::ctime(&end_time)
              << "elapsed time: " << elapsed_seconds.count() << "s"
              << std::endl;

    return 0;
}
