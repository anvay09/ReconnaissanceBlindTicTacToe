#include "cpp_headers/rbt_classes.hpp"

// g++-13 -O3 fictitious_self_play.cpp rbt_classes.cpp -o fsp -fopenmp

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
                        depth_2_opponent_I.reset_zeros();

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
                new_I.reset_zeros();

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

    if (best_action != -1) {
        // update policy
        std::vector<double>& prob_dist = br.policy_dict[I.get_index()];
        for (int k = 0; k < prob_dist.size(); k++) {
            if (k == best_action) {
                prob_dist[k] = 1.0;
            } 
            else {
                prob_dist[k] = 0.0;
            }
        }
    }
    else {
        // uniform policy
        std::vector<double>& prob_dist = br.policy_dict[I.get_index()];
        for (int a = 0; a < actions.size(); a++) {
            prob_dist[a] = 1.0 / actions.size();
        }
        max_Q = 0.0;
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
                        depth_2_opponent_I.reset_zeros();

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

        # pragma omp parallel for num_threads(96)
        for (int a = 0; a < actions.size(); a++) {
            if (action_to_history_list[actions[a]].size() > 0) {
                InformationSet new_I = I;
                new_I.update_move(actions[a], I.player);
                new_I.reset_zeros();
                Q_values[actions[a]] += compute_best_response(new_I, br_player, action_to_true_board_list[actions[a]], action_to_history_list[actions[a]], action_to_reach_probability_list[actions[a]], action_to_opponent_I_list[actions[a]], br, policy_obj);
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
                new_I.reset_zeros();

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

    if (best_action != -1) {
        // update policy
        std::vector<double>& prob_dist = br.policy_dict[I.get_index()];
        for (int k = 0; k < prob_dist.size(); k++) {
            if (k == best_action) {
                prob_dist[k] = 1.0;
            } 
            else {
                prob_dist[k] = 0.0;
            }
        }
    }
    else {
        // uniform policy
        std::vector<double>& prob_dist = br.policy_dict[I.get_index()];
        for (int a = 0; a < actions.size(); a++) {
            prob_dist[a] = 1.0 / actions.size();
        }
        max_Q = 0.0;
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

            true_board_list.push_back(new_true_board);
            history_list.push_back(new_history);
            reach_probability_list.push_back(policy_obj.policy_dict[I_1.get_index()][actions[a]]);
            opponent_I_list.push_back(new_I);
        }

        expected_utility = - compute_best_response_parallel(I_2, br_player, true_board_list, history_list, reach_probability_list, opponent_I_list, br, policy_obj);
    }

    return expected_utility;
}


double get_reach_of_I(InformationSet& I, PolicyVec& policy_obj, char player) {
    double prob_reaching = 1.0;
    std::vector<int> actions;
    I.get_played_actions(actions);
    int count = 0;
    int i = 0;
    while (i < I.get_hash().size()) {
        std::string I_hash = I.get_hash().substr(0, i);
        bool move_flag = get_move_flag(I_hash, player);
        InformationSet I_new(player, move_flag, I_hash);

        prob_reaching *= policy_obj.policy_dict[I_new.get_index()][actions[count]];
        if (prob_reaching == 0.0) {
            return 0.0;
        }
        count += 1;
        
        if (move_flag) {
            i = i + 2;
        }
        else {
            i = i + 7;
        }
    }
    return prob_reaching;
}


void update_average_strategies(PolicyVec& sigma_t, PolicyVec& br, PolicyVec& sigma_t_next, int t, char player, std::vector<std::string>& information_sets) {
    # pragma omp parallel num_threads(96)
    for (long int i = 0; i < information_sets.size(); i++) {
        std::string I_hash = information_sets[i];
        bool move_flag = get_move_flag(I_hash, player);
        InformationSet I(player, move_flag, I_hash);
        std::vector<int> actions;
        I.get_actions(actions);

        double reach_br = get_reach_of_I(I, br, player);

        if (reach_br == 0.0) {
            continue;
        }

        double reach_sigma_t = get_reach_of_I(I, sigma_t, player);

        std::vector<double>& prob_dist_sigma_t = sigma_t.policy_dict[I.get_index()];
        std::vector<double>& prob_dist_br = br.policy_dict[I.get_index()];
        std::vector<double>& prob_dist_sigma_t_next = sigma_t_next.policy_dict[I.get_index()];

        for (int a = 0; a < actions.size(); a++) {
            double lambda = reach_br / (t * reach_sigma_t + reach_br);

            prob_dist_sigma_t_next[actions[a]] = prob_dist_sigma_t[actions[a]] + lambda * (prob_dist_br[actions[a]] - prob_dist_sigma_t[actions[a]]);
        }
    }
}


void update_average_strategies_recursive(InformationSet& I, char player, std::vector<TicTacToeBoard>& true_board_list, std::vector<History>& history_list, 
                                         double reach_probability_sigma_t, double reach_probability_br, std::vector<InformationSet>& opponent_I_list, 
                                         PolicyVec& br, PolicyVec& sigma_t, PolicyVec& sigma_t_next, int t) {
    std::cout << "Checkpoint 1" << std::endl;
    std::cout << "Information set: " << I.get_hash() << std::endl;

    std::vector<int> all_actions;
    I.get_actions(all_actions);
    std::vector<double>& prob_dist_sigma_t = sigma_t.policy_dict[I.get_index()];
    std::vector<double>& prob_dist_br = br.policy_dict[I.get_index()];
    std::vector<double>& prob_dist_sigma_t_next = sigma_t_next.policy_dict[I.get_index()];

    std::cout << "Reach probability sigma_t: " << reach_probability_sigma_t << std::endl;
    std::cout << "Reach probability br: " << reach_probability_br << std::endl;
    
    std::cout << "Size of all actions: " << all_actions.size() << std::endl;
    std::cout << "Size of prob_dist_sigma_t: " << prob_dist_sigma_t.size() << std::endl;
    std::cout << "Size of prob_dist_br: " << prob_dist_br.size() << std::endl;
    std::cout << "Size of prob_dist_sigma_t_next: " << prob_dist_sigma_t_next.size() << std::endl;

    for (int a = 0; a < all_actions.size(); a++) {
        double lambda = reach_probability_br / (t * reach_probability_sigma_t + reach_probability_br);

        prob_dist_sigma_t_next[all_actions[a]] = prob_dist_sigma_t[all_actions[a]] + lambda * (prob_dist_br[all_actions[a]] - prob_dist_sigma_t[all_actions[a]]);
    }

    // std::cout << "Checkpoint 2" << std::endl;

    // std::cout << "Information set: " << I.get_hash() << std::endl;
    // std::cout << "Reach probability sigma_t: " << reach_probability_sigma_t << std::endl;
    // std::cout << "Reach probability br: " << reach_probability_br << std::endl;

    std::vector<int> actions;
    I.get_actions_given_policy(actions, br);

    if (I.move_flag) {
        for (int a = 0; a < actions.size(); a++) {
            std::vector<TicTacToeBoard> depth_3_true_board_list;
            std::vector<History> depth_3_history_list;
            std::vector<InformationSet> depth_3_opponent_I_list;

            double depth_1_reach_probability_br = reach_probability_br * br.policy_dict[I.get_index()][actions[a]];
            double depth_1_reach_probability_sigma_t = reach_probability_sigma_t * sigma_t.policy_dict[I.get_index()][actions[a]];

            // std::cout << "Checkpoint 3" << std::endl;

            // std::cout << "Br action probability: " << br.policy_dict[I.get_index()][actions[a]] << std::endl;
            // std::cout << "Sigma_t action probability: " << sigma_t.policy_dict[I.get_index()][actions[a]] << std::endl;
            
            for (int h = 0; h < history_list.size(); h++) {
                TicTacToeBoard depth_1_true_board = true_board_list[h];
                History depth_1_history = history_list[h];
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
                    depth_1_opponent_I.get_actions(depth_1_opponent_actions);
                    std::vector<History> depth_2_history_list;
                    std::vector<InformationSet> depth_2_opponent_I_list;
                    std::vector<TicTacToeBoard> depth_2_true_board_list;

                    // std::cout << "Checkpoint 4" << std::endl;
                    // first simulate sense
                    for (int opponent_action : depth_1_opponent_actions) {
                        InformationSet depth_2_opponent_I = depth_1_opponent_I;
                        depth_2_opponent_I.simulate_sense(opponent_action, depth_1_true_board);
                        depth_2_opponent_I.reset_zeros();

                        History depth_2_history = depth_1_history;
                        depth_2_history.history.push_back(opponent_action);

                        TicTacToeBoard depth_2_true_board = depth_1_true_board;

                        depth_2_history_list.push_back(depth_2_history);
                        depth_2_opponent_I_list.push_back(depth_2_opponent_I);
                        depth_2_true_board_list.push_back(depth_2_true_board);
                    }

                    // std::cout << "Checkpoint 5" << std::endl;

                    for (int i = 0; i < depth_2_history_list.size(); i++) {
                        InformationSet depth_2_opponent_I = depth_2_opponent_I_list[i];
                        std::vector<int> depth_2_opponent_actions;
                        depth_2_opponent_I.get_actions(depth_2_opponent_actions);

                        for (int opponent_action : depth_2_opponent_actions) {
                            TicTacToeBoard depth_3_true_board = depth_2_true_board_list[i];
                            History depth_3_history = depth_2_history_list[i];

                            success = depth_3_true_board.update_move(opponent_action, depth_2_opponent_I_list[i].player);
                            depth_3_history.history.push_back(opponent_action);

                            if (success && !depth_3_true_board.is_win(winner) && !depth_3_true_board.is_over()) {
                                InformationSet depth_3_opponent_I = depth_2_opponent_I_list[i];
                                depth_3_opponent_I.update_move(opponent_action, depth_3_opponent_I.player);
                                depth_3_opponent_I.reset_zeros();

                                depth_3_true_board_list.push_back(depth_3_true_board);
                                depth_3_history_list.push_back(depth_3_history);
                                depth_3_opponent_I_list.push_back(depth_3_opponent_I);
                            }
                        }
                    }

                    // std::cout << "Checkpoint 6" << std::endl;
                }

                // std::cout << "Checkpoint 7" << std::endl;
            }

            // std::cout << "Checkpoint 8" << std::endl;

            if (depth_3_history_list.size() > 0) {
                InformationSet new_I = I;
                new_I.update_move(actions[a], I.player);
                new_I.reset_zeros();

                update_average_strategies_recursive(new_I, player, depth_3_true_board_list, depth_3_history_list, depth_1_reach_probability_sigma_t, depth_1_reach_probability_br, depth_3_opponent_I_list, br, sigma_t, sigma_t_next, t);
            }

            // std::cout << "Checkpoint 9" << std::endl;
        }
    }
    else {
        for (int a = 0; a < actions.size(); a++) {
            std::unordered_map<std::string, std::vector<TicTacToeBoard>> infoset_to_true_board;
            std::unordered_map<std::string, std::vector<History>> infoset_to_history;
            std::unordered_map<std::string, std::vector<InformationSet>> infoset_to_opponent_I;
            std::unordered_set<std::string> infoset_set;
            double depth_1_reach_probability_br = reach_probability_br * br.policy_dict[I.get_index()][actions[a]];
            double depth_1_reach_probability_sigma_t = reach_probability_sigma_t * sigma_t.policy_dict[I.get_index()][actions[a]];

            // std::cout << "Checkpoint 10" << std::endl;

            // std::cout << "Br action probability: " << br.policy_dict[I.get_index()][actions[a]] << std::endl;
            // std::cout << "Sigma_t action probability: " << sigma_t.policy_dict[I.get_index()][actions[a]] << std::endl;

            for (int h = 0; h < history_list.size(); h++) {
                TicTacToeBoard& true_board = true_board_list[h];
                History& history = history_list[h];

                InformationSet new_I = I;
                new_I.simulate_sense(actions[a], true_board);
                new_I.reset_zeros();

                History new_history = history;
                new_history.history.push_back(actions[a]);

                infoset_to_true_board[new_I.hash].push_back(true_board);
                infoset_to_history[new_I.hash].push_back(new_history);
                infoset_to_opponent_I[new_I.hash].push_back(opponent_I_list[h]);
                infoset_set.insert(new_I.hash);
            }

            // std::cout << "Checkpoint 11" << std::endl;

            for (int j = 0; j < infoset_set.size(); j++) {
                std::string new_I_hash = *std::next(infoset_set.begin(), j);
                bool move_flag = get_move_flag(new_I_hash, I.player);
                InformationSet new_I(I.player, move_flag, new_I_hash);
  
                if (infoset_to_history[new_I.hash].size() > 0) {
                    std::cout << "Index of new_I: " << new_I.get_index() << std::endl;
                    std::cout << "Size of prob_dist_sigma_t: " << br.policy_dict[new_I.get_index()].size() << std::endl;
                    std::cout << "Size of prob_dist_br: " << sigma_t.policy_dict[new_I.get_index()].size() << std::endl;
                    std::cout << "Size of prob_dist_sigma_t_next: " << sigma_t_next.policy_dict[new_I.get_index()].size() << std::endl;

                    update_average_strategies_recursive(new_I, player, infoset_to_true_board[new_I.hash], infoset_to_history[new_I.hash], depth_1_reach_probability_sigma_t, depth_1_reach_probability_br, infoset_to_opponent_I[new_I.hash], br, sigma_t, sigma_t_next, t);
                }
            }

            // std::cout << "Checkpoint 12" << std::endl;
        }
    }

    return;
}


void update_average_strategies_recursive_parallel(InformationSet& I, char player, std::vector<TicTacToeBoard>& true_board_list, std::vector<History>& history_list, 
                                                  double reach_probability_sigma_t, double reach_probability_br, std::vector<InformationSet>& opponent_I_list, 
                                                  PolicyVec& br, PolicyVec& sigma_t, PolicyVec& sigma_t_next, int t){
    std::vector<int> all_actions;
    I.get_actions(all_actions);
    std::vector<double>& prob_dist_sigma_t = sigma_t.policy_dict[I.get_index()];
    std::vector<double>& prob_dist_br = br.policy_dict[I.get_index()];
    std::vector<double>& prob_dist_sigma_t_next = sigma_t_next.policy_dict[I.get_index()];

    for (int a = 0; a < all_actions.size(); a++) {
        double lambda = reach_probability_br / (t * reach_probability_sigma_t + reach_probability_br);

        prob_dist_sigma_t_next[all_actions[a]] = prob_dist_sigma_t[all_actions[a]] + lambda * (prob_dist_br[all_actions[a]] - prob_dist_sigma_t[all_actions[a]]);
    }

    std::vector<int> actions;
    I.get_actions_given_policy(actions, br);

    if (I.move_flag) {
        std::unordered_map<int, std::vector<TicTacToeBoard>> action_to_true_board_list;
        std::unordered_map<int, std::vector<History>> action_to_history_list;
        std::unordered_map<int, double> action_to_reach_probability_br;
        std::unordered_map<int, double> action_to_reach_probability_sigma_t;
        std::unordered_map<int, std::vector<InformationSet>> action_to_opponent_I_list; 

        for (int a = 0; a < actions.size(); a++) {
            std::vector<TicTacToeBoard> depth_3_true_board_list;
            std::vector<History> depth_3_history_list;
            std::vector<InformationSet> depth_3_opponent_I_list;

            double depth_1_reach_probability_br = reach_probability_br * br.policy_dict[I.get_index()][actions[a]];
            double depth_1_reach_probability_sigma_t = reach_probability_sigma_t * sigma_t.policy_dict[I.get_index()][actions[a]];
            
            for (int h = 0; h < history_list.size(); h++) {
                TicTacToeBoard depth_1_true_board = true_board_list[h];
                History depth_1_history = history_list[h];
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
                    depth_1_opponent_I.get_actions(depth_1_opponent_actions);
                    std::vector<History> depth_2_history_list;
                    std::vector<InformationSet> depth_2_opponent_I_list;
                    std::vector<TicTacToeBoard> depth_2_true_board_list;

                    // first simulate sense
                    for (int opponent_action : depth_1_opponent_actions) {
                        InformationSet depth_2_opponent_I = depth_1_opponent_I;
                        depth_2_opponent_I.simulate_sense(opponent_action, depth_1_true_board);
                        depth_2_opponent_I.reset_zeros();

                        History depth_2_history = depth_1_history;
                        depth_2_history.history.push_back(opponent_action);

                        TicTacToeBoard depth_2_true_board = depth_1_true_board;

                        depth_2_history_list.push_back(depth_2_history);
                        depth_2_opponent_I_list.push_back(depth_2_opponent_I);
                        depth_2_true_board_list.push_back(depth_2_true_board);
                    }

                    for (int i = 0; i < depth_2_history_list.size(); i++) {
                        InformationSet depth_2_opponent_I = depth_2_opponent_I_list[i];
                        std::vector<int> depth_2_opponent_actions;
                        depth_2_opponent_I.get_actions(depth_2_opponent_actions);

                        for (int opponent_action : depth_2_opponent_actions) {
                            TicTacToeBoard depth_3_true_board = depth_2_true_board_list[i];
                            History depth_3_history = depth_2_history_list[i];

                            success = depth_3_true_board.update_move(opponent_action, depth_2_opponent_I_list[i].player);
                            depth_3_history.history.push_back(opponent_action);

                            if (success && !depth_3_true_board.is_win(winner) && !depth_3_true_board.is_over()) {
                                InformationSet depth_3_opponent_I = depth_2_opponent_I_list[i];
                                depth_3_opponent_I.update_move(opponent_action, depth_3_opponent_I.player);
                                depth_3_opponent_I.reset_zeros();

                                depth_3_true_board_list.push_back(depth_3_true_board);
                                depth_3_history_list.push_back(depth_3_history);
                                depth_3_opponent_I_list.push_back(depth_3_opponent_I);
                            }
                        }
                    }
                }
            }

            action_to_true_board_list[actions[a]] = depth_3_true_board_list;
            action_to_history_list[actions[a]] = depth_3_history_list;
            action_to_reach_probability_br[actions[a]] = depth_1_reach_probability_br;
            action_to_reach_probability_sigma_t[actions[a]] = depth_1_reach_probability_sigma_t;
            action_to_opponent_I_list[actions[a]] = depth_3_opponent_I_list;
        }

        # pragma omp parallel for num_threads(96)
        for (int a = 0; a < actions.size(); a++) {
            if (action_to_history_list[actions[a]].size() > 0) {
                InformationSet new_I = I;
                new_I.update_move(actions[a], I.player);
                new_I.reset_zeros();
                update_average_strategies_recursive(new_I, player, action_to_true_board_list[actions[a]], action_to_history_list[actions[a]], action_to_reach_probability_sigma_t[actions[a]], action_to_reach_probability_br[actions[a]], action_to_opponent_I_list[actions[a]], br, sigma_t, sigma_t_next, t);
            }
        }
    }
    else {
        std::unordered_map<std::string, std::vector<TicTacToeBoard>> infoset_to_true_board;
        std::unordered_map<std::string, std::vector<History>> infoset_to_history;
        std::unordered_map<std::string, std::vector<InformationSet>> infoset_to_opponent_I;
        std::unordered_map<std::string, double> infoset_to_reach_br;
        std::unordered_map<std::string, double> infoset_to_reach_sigma_t;
        std::unordered_map<std::string, int> infoset_to_action_taken;
        std::unordered_set<std::string> infoset_set;

        for (int a = 0; a < actions.size(); a++) {
            double depth_1_reach_probability_br = reach_probability_br * br.policy_dict[I.get_index()][actions[a]];
            double depth_1_reach_probability_sigma_t = reach_probability_sigma_t * sigma_t.policy_dict[I.get_index()][actions[a]];

            for (int h = 0; h < history_list.size(); h++) {
                TicTacToeBoard& true_board = true_board_list[h];
                History& history = history_list[h];

                InformationSet new_I = I;
                new_I.simulate_sense(actions[a], true_board);
                new_I.reset_zeros();

                History new_history = history;
                new_history.history.push_back(actions[a]);

                infoset_to_true_board[new_I.hash].push_back(true_board);
                infoset_to_history[new_I.hash].push_back(new_history);
                infoset_to_opponent_I[new_I.hash].push_back(opponent_I_list[h]);
                infoset_to_reach_br[new_I.hash] = depth_1_reach_probability_br;
                infoset_to_reach_sigma_t[new_I.hash] = depth_1_reach_probability_sigma_t;
                infoset_to_action_taken[new_I.hash] = actions[a];

                infoset_set.insert(new_I.hash);
            }
        }

        # pragma omp parallel for num_threads(96)
        for (int j = 0; j < infoset_set.size(); j++) {
            std::string new_I_hash = *std::next(infoset_set.begin(), j);
            bool move_flag = get_move_flag(new_I_hash, I.player);
            InformationSet new_I(I.player, move_flag, new_I_hash);
  
            if (infoset_to_history[new_I.hash].size() > 0) {
                update_average_strategies_recursive(new_I, player, infoset_to_true_board[new_I.hash], infoset_to_history[new_I.hash], infoset_to_reach_sigma_t[new_I.hash], infoset_to_reach_br[new_I.hash], infoset_to_opponent_I[new_I.hash], br, sigma_t, sigma_t_next, t);
            }
        }
    }

    return;
}

            
void update_average_strategies_recursive_wrapper(PolicyVec& br, PolicyVec& sigma_t, PolicyVec& sigma_t_next, char player, int t) {
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
    std::vector<InformationSet> opponent_I_list;

    if (player == 'x') {
        true_board_list.push_back(true_board);
        history_list.push_back(start_history);
        opponent_I_list.push_back(I_2);

        update_average_strategies_recursive(I_1, player, true_board_list, history_list, 1.0, 1.0, opponent_I_list, br, sigma_t, sigma_t_next, t);
    } 
    else {
        std::vector<int> actions;
        I_1.get_actions_given_policy(actions, br);

        for (int a = 0; a < actions.size(); a++){
            TicTacToeBoard new_true_board = true_board;
            bool success = new_true_board.update_move(actions[a], I_1.player);

            History new_history = start_history;
            new_history.history.push_back(actions[a]);

            InformationSet new_I = I_1;
            new_I.update_move(actions[a], I_1.player);

            true_board_list.push_back(new_true_board);
            history_list.push_back(new_history);
            opponent_I_list.push_back(new_I);
        }

        update_average_strategies_recursive(I_2, player, true_board_list, history_list, 1.0, 1.0, opponent_I_list, br, sigma_t, sigma_t_next, t);
    }

    return;
}


void XFP(PolicyVec& sigma_t_x, PolicyVec& sigma_t_o, int T, std::vector<std::string>& P1_information_sets, std::vector<std::string>& P2_information_sets) {
    // for (long int i = 0; i < P1_information_sets.size(); i++) {
    //     std::vector<double>& prob_dist = sigma_t_x.policy_dict[i];
    //     if (prob_dist.size() == 0) {
    //         std::cout << "Sigma_t_x information set: " << P1_information_sets[i] << " has size 0" << std::endl;
    //         for (int j = 0; j < 0; j++) {
    //             prob_dist.push_back(0.0);
    //         }
    //     }
    // }

    // for (long int i = 0; i < P2_information_sets.size(); i++) {
    //     std::vector<double>& prob_dist = sigma_t_o.policy_dict[i];
    //     if (prob_dist.size() == 0) {
    //         std::cout << "Sigma_t_o information set: " << P2_information_sets[i] << " has size 0" << std::endl;
    //         for (int j = 0; j < 0; j++) {
    //             prob_dist.push_back(0.0);
    //         }
    //     }
    // }

    PolicyVec sigma_t_next_x = sigma_t_x;
    PolicyVec sigma_t_next_o = sigma_t_o;
    PolicyVec br_x = sigma_t_x;
    PolicyVec br_o = sigma_t_o;

    for (int t = 1; t <= T; t++) {
        std::cout << "Iteration: " << t << std::endl;
        double expected_utility = get_expected_utility_wrapper(sigma_t_x, sigma_t_o);
        std::cout << "Expected utility: " << expected_utility << std::endl;

        // time
        std::cout << "Computing best responses.." << std::endl;
        auto start = std::chrono::system_clock::now();
        double exploitability = 0.0;
        
        expected_utility = compute_best_response_wrapper(sigma_t_o, br_x, 'x');
        std::cout << "Expected utility of best response against P2: " << expected_utility << std::endl;

        auto end = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = end-start;
        std::time_t end_time = std::chrono::system_clock::to_time_t(end);
        std::cout << "finished computation at " << std::ctime(&end_time)
                  << "elapsed time: " << elapsed_seconds.count() << "s"
                  << std::endl;

        exploitability = expected_utility;

        expected_utility = compute_best_response_wrapper(sigma_t_x, br_o, 'o');
        std::cout << "Expected utility of best response against P1: " << expected_utility << std::endl;

        end = std::chrono::system_clock::now();
        elapsed_seconds = end-start;
        end_time = std::chrono::system_clock::to_time_t(end);
        std::cout << "finished computation at " << std::ctime(&end_time)
                  << "elapsed time: " << elapsed_seconds.count() << "s"
                  << std::endl;

        exploitability -= expected_utility;

        std::cout << "Exploitability: " << exploitability << std::endl;

        std::cout << "Updating average strategies..." << std::endl;
        // update_average_strategies(sigma_t_x, br_x, sigma_t_next_x, t, 'x', P1_information_sets);
        // update_average_strategies(sigma_t_o, br_o, sigma_t_next_o, t, 'o', P2_information_sets);
        update_average_strategies_recursive_wrapper(br_x, sigma_t_x, sigma_t_next_x, 'x', t);
        update_average_strategies_recursive_wrapper(br_o, sigma_t_o, sigma_t_next_o, 'o', t);
        
        end = std::chrono::system_clock::now();
        elapsed_seconds = end-start;
        end_time = std::chrono::system_clock::to_time_t(end);
        std::cout << "finished computation at " << std::ctime(&end_time)
                  << "elapsed time: " << elapsed_seconds.count() << "s"
                  << std::endl;

        sigma_t_x = sigma_t_next_x;
        sigma_t_o = sigma_t_next_o;
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
    
    std::cout << "Policies loaded." << std::endl;
    
    XFP(policy_obj_x, policy_obj_o, num_iterations, P1_information_sets, P2_information_sets);

    return 0;
}
