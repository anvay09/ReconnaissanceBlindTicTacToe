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


void make_pure_strategy(PolicyVec& br, char player, std::vector<std::string>& information_sets){
    std::cout << "Creating pure strategy..." << std::endl;
    for (long int i = 0; i < information_sets.size(); i++) {
        std::vector<double>& prob_dist = br.policy_dict[i];
        
        double max_Q = -std::numeric_limits<double>::infinity();
        int best_action = -1;
        for (int a = 0; a < prob_dist.size(); a++) {
            if (prob_dist[a] >= max_Q) {
                max_Q = prob_dist[a];
                best_action = a;
            }
        }

        std::cout << "Infoset: " << information_sets[i];

        if (best_action != -1) {
            for (int a = 0; a < prob_dist.size(); a++) {
                std::cout << " " << a << ": " << prob_dist[a];
                if (a == best_action) {
                    prob_dist[a] = 1.0;
                } else {
                    prob_dist[a] = 0.0;
                }
            }
            std::cout << std::endl;
        }
        else {
            // equal probability to all actions
            std::vector<int> actions;
            std::string hash = information_sets[i];
            bool move_flag = get_move_flag(hash, player);
            InformationSet I = InformationSet(player, move_flag, hash);
            I.get_actions(actions);

            for (int i = 0; i < actions.size(); i++) {
                prob_dist[i] = 1.0 / actions.size();
            }
        }
    }
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


double compute_best_response(InformationSet &I_1, InformationSet &I_2, TicTacToeBoard &true_board, char player, PolicyVec &policy_obj_x, 
                            PolicyVec &policy_obj_o, double probability, History& current_history, char initial_player, PolicyVec& br, char br_player) {
    double expected_utility_h = 0.0;
    
    InformationSet& I = player == 'x' ? I_1 : I_2;
    PolicyVec& policy_obj = player == 'x' ? policy_obj_x : policy_obj_o;
    
    std::vector<int> actions;
    std::vector<double> Q_values;

    if (player == br_player) {
        I.get_actions(actions);
        for (int action : actions) {
            Q_values.push_back(0.0);
        }
    }
    else {
        I.get_actions_given_policy(actions, policy_obj);
    }
    
    if (I.move_flag) {
        for (int a = 0; a < actions.size(); a++) {
            TicTacToeBoard new_true_board = true_board;
            bool success = new_true_board.update_move(actions[a], player);

            double probability_new = 0.0;
            if (player == br_player) {
                probability_new = probability;
            }
            else {
                probability_new = probability * policy_obj.policy_dict[I.get_index()][actions[a]];
            }

            History new_history = current_history;
            new_history.history.push_back(actions[a]);
            
            char winner;
            if (success && !new_true_board.is_win(winner) && !new_true_board.is_over()) {
                InformationSet new_I = I;
                new_I.update_move(actions[a], player);
                new_I.reset_zeros();
                
                if (player == 'x') {
                    if (player == br_player) {
                        Q_values[a] = compute_best_response(new_I, I_2, new_true_board, 'o', policy_obj_x, policy_obj_o, probability_new, new_history, initial_player, br, br_player);
                    }
                    else {
                        expected_utility_h += compute_best_response(new_I, I_2, new_true_board, 'o', policy_obj_x, policy_obj_o, probability_new, new_history, initial_player, br, br_player);
                    }
                } else {
                    if (player == br_player) {
                        Q_values[a] = compute_best_response(I_1, new_I, new_true_board, 'x', policy_obj_x, policy_obj_o, probability_new, new_history, initial_player, br, br_player);
                    }
                    else {
                        expected_utility_h += compute_best_response(I_1, new_I, new_true_board, 'x', policy_obj_x, policy_obj_o, probability_new, new_history, initial_player, br, br_player);
                    }
                }
            } else {
                TerminalHistory H_T = TerminalHistory(new_history.history);
                H_T.set_reward();
                if (initial_player == 'x'){
                    if (player == br_player) {
                        Q_values[a] = H_T.reward[0] * probability_new;
                        expected_utility_h += Q_values[a];
                    }
                    else {
                        expected_utility_h += H_T.reward[0] * probability_new;
                    }
                }
                else{
                    if (player == br_player) {
                        Q_values[a] = H_T.reward[1] * probability_new;
                        expected_utility_h += Q_values[a];
                    }
                    else {
                        expected_utility_h += H_T.reward[1] * probability_new;
                    }
                }
            }
        }
    } else {
        for (int a = 0; a < actions.size(); a++) {
            InformationSet new_I = I;
            new_I.simulate_sense(actions[a], true_board);
            
            double probability_new = 0.0;
            if (player == br_player) {
                probability_new = probability;
            }
            else {
                probability_new = probability * policy_obj.policy_dict[I.get_index()][actions[a]];
            }
            
            History new_history = current_history;
            new_history.history.push_back(actions[a]);

            if (player == 'x') {
                if (player == br_player) {
                    Q_values[a] = compute_best_response(new_I, I_2, true_board, 'x', policy_obj_x, policy_obj_o, probability_new, new_history, initial_player, br, br_player);
                }
                else {
                    expected_utility_h += compute_best_response(new_I, I_2, true_board, 'x', policy_obj_x, policy_obj_o, probability_new, new_history, initial_player, br, br_player);
                }
            } else {
                if (player == br_player) {
                    Q_values[a] = compute_best_response(I_1, new_I, true_board, 'o', policy_obj_x, policy_obj_o, probability_new, new_history, initial_player, br, br_player);
                }
                else {
                    expected_utility_h += compute_best_response(I_1, new_I, true_board, 'o', policy_obj_x, policy_obj_o, probability_new, new_history, initial_player, br, br_player);
                }
            }
        }
    }

    if (player == br_player) {
        if (player == 'x') {
            double max_Q = -1.0;
            int best_action = -1;

            for (int a = 0; a < Q_values.size(); a++) {
                if (Q_values[a] >= max_Q) {
                    max_Q = Q_values[a];
                    best_action = actions[a];
                }
            }

            if (best_action != -1) {
                // update policy
                std::vector<double>& prob_dist = br.policy_dict[I.get_index()];
                for (int a = 0; a < prob_dist.size(); a++) {
                    if (a == best_action) {
                        prob_dist[a] += max_Q * probability;
                    } 
                }
            }
            expected_utility_h = max_Q;
        }
        else {
            double min_Q = 1.0;
            int best_action = -1;

            for (int a = 0; a < Q_values.size(); a++) {
                if (Q_values[a] <= min_Q) {
                    min_Q = Q_values[a];
                    best_action = actions[a];
                }
            }

            if (best_action != -1) {
                // update policy
                std::vector<double>& prob_dist = br.policy_dict[I.get_index()];
                for (int a = 0; a < prob_dist.size(); a++) {
                    if (a == best_action) {
                        prob_dist[a] += min_Q * probability;
                    } 
                }
            }
            expected_utility_h = min_Q;
        }
    }

    return expected_utility_h;
}


double compute_best_response_parallel(InformationSet &I_1, InformationSet &I_2, TicTacToeBoard &true_board, char player, PolicyVec &policy_obj_x, 
                                     PolicyVec &policy_obj_o, double probability, History& current_history, char initial_player, PolicyVec& br, char br_player) {
    double expected_utility_h = 0.0;
    std::vector<InformationSet> Depth_1_P1_Isets;
    std::vector<InformationSet> Depth_1_P2_Isets;
    std::vector<TicTacToeBoard> Depth_1_boards;
    std::vector<char> Depth_1_players;
    std::vector<double> Depth_1_probabilities;
    std::vector<History> Depth_1_histories;
    std::vector<int> Depth_1_actions;

    InformationSet I = player == 'x' ? I_1 : I_2;
    PolicyVec policy_obj = player == 'x' ? policy_obj_x : policy_obj_o;

    std::vector<int> actions;
    std::vector<double> Q_values;

    if (player == br_player) {
        I.get_actions(actions);
        for (int action : actions) {
            Q_values.push_back(0.0);
        }
    }
    else {
        I.get_actions_given_policy(actions, policy_obj);
    }

    if (I.move_flag) {
        for (int a = 0; a < actions.size(); a++) {
            TicTacToeBoard new_true_board = true_board;
            bool success = new_true_board.update_move(actions[a], player);

            double probability_new = 0.0;
            if (player == br_player) {
                probability_new = probability;
            }
            else {
                probability_new = probability * policy_obj.policy_dict[I.get_index()][actions[a]];
            }

            History new_history = current_history;
            new_history.history.push_back(actions[a]);

            char winner;
            if (success && !new_true_board.is_win(winner) && !new_true_board.is_over()) {
                InformationSet new_I = I;
                new_I.update_move(actions[a], player);
                new_I.reset_zeros();

                if (player == 'x') {
                    Depth_1_P1_Isets.push_back(new_I);
                    Depth_1_P2_Isets.push_back(I_2);
                    Depth_1_boards.push_back(new_true_board);
                    Depth_1_players.push_back('o');
                    Depth_1_probabilities.push_back(probability_new);
                    Depth_1_histories.push_back(new_history);
                    Depth_1_actions.push_back(actions[a]);
                } 
                else {
                    Depth_1_P1_Isets.push_back(I_1);
                    Depth_1_P2_Isets.push_back(new_I);
                    Depth_1_boards.push_back(new_true_board);
                    Depth_1_players.push_back('x');
                    Depth_1_probabilities.push_back(probability_new);
                    Depth_1_histories.push_back(new_history);
                    Depth_1_actions.push_back(actions[a]);
                }
            }

            else {
                TerminalHistory H_T = TerminalHistory(new_history.history);
                H_T.set_reward();
                if (initial_player == 'x'){
                    if (player == br_player) {
                        Q_values[a] = H_T.reward[0] * probability_new;
                    }
                    else {
                        expected_utility_h += H_T.reward[0] * probability_new;
                    }
                }
                else{
                    if (player == br_player) {
                        Q_values[a] = H_T.reward[1] * probability_new;
                    }
                    else {
                        expected_utility_h += H_T.reward[1] * probability_new;
                    }
                }
            }
        }
    }

    else {
        for (int a = 0; a < actions.size(); a++) {
            InformationSet new_I = I;
            new_I.simulate_sense(actions[a], true_board);

            double probability_new = 0.0;
            if (player == br_player) {
                probability_new = probability;
            }
            else {
                probability_new = probability * policy_obj.policy_dict[I.get_index()][actions[a]];
            }

            History new_history = current_history;
            new_history.history.push_back(actions[a]);

            if (player == 'x') {
                Depth_1_P1_Isets.push_back(new_I);
                Depth_1_P2_Isets.push_back(I_2);
                Depth_1_boards.push_back(true_board);
                Depth_1_players.push_back('x');
                Depth_1_probabilities.push_back(probability_new);
                Depth_1_histories.push_back(new_history);
                Depth_1_actions.push_back(actions[a]);
            } 
            else {
                Depth_1_P1_Isets.push_back(I_1);
                Depth_1_P2_Isets.push_back(new_I);
                Depth_1_boards.push_back(true_board);
                Depth_1_players.push_back('o');
                Depth_1_probabilities.push_back(probability_new);
                Depth_1_histories.push_back(new_history);
                Depth_1_actions.push_back(actions[a]);
            }
        }
    }

    # pragma omp parallel for num_threads(96)
    for (int a = 0; a < Depth_1_P1_Isets.size(); a++) {
        if (player == br_player) {
            Q_values[Depth_1_actions[a]] = compute_best_response(Depth_1_P1_Isets[a], Depth_1_P2_Isets[a], Depth_1_boards[a], Depth_1_players[a], policy_obj_x, policy_obj_o, Depth_1_probabilities[a], Depth_1_histories[a], initial_player, br, br_player);
        }
        else {
            expected_utility_h += compute_best_response(Depth_1_P1_Isets[a], Depth_1_P2_Isets[a], Depth_1_boards[a], Depth_1_players[a], policy_obj_x, policy_obj_o, Depth_1_probabilities[a], Depth_1_histories[a], initial_player, br, br_player);
        }
    }

    if (player == br_player) {
        if (player == 'x'){
            double max_Q = -1.0;
            int best_action = -1;

            for (int a = 0; a < Q_values.size(); a++) {
                if (Q_values[a] >= max_Q) {
                    max_Q = Q_values[a];
                    best_action = actions[a];
                }
            }

            if (best_action != -1) {
                // update policy
                std::vector<double>& prob_dist = br.policy_dict[I.get_index()];
                for (int a = 0; a < prob_dist.size(); a++) {
                    if (a == best_action) {
                        prob_dist[a] += max_Q * probability;
                    } 
                }
            }
            expected_utility_h = max_Q;
        }
        else {
            double min_Q = 1.0;
            int best_action = -1;

            for (int a = 0; a < Q_values.size(); a++) {
                if (Q_values[a] <= min_Q) {
                    min_Q = Q_values[a];
                    best_action = actions[a];
                }
            }

            if (best_action != -1) {
                // update policy
                std::vector<double>& prob_dist = br.policy_dict[I.get_index()];
                for (int a = 0; a < prob_dist.size(); a++) {
                    if (a == best_action) {
                        prob_dist[a] += min_Q * probability;
                    } 
                }
            }
            expected_utility_h = min_Q;
        }
    }

    return expected_utility_h;
}


double compute_best_response_wrapper(PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, PolicyVec& br, char br_player){
    std::string board = "000000000";
    TicTacToeBoard true_board = TicTacToeBoard(board);
    std::string hash_1 = "";
    std::string hash_2 = "";
    InformationSet I_1 = InformationSet('x', true, hash_1);
    InformationSet I_2 = InformationSet('o', false, hash_2);
    std::vector<int> h = {};
    TerminalHistory start_history = TerminalHistory(h);

    double expected_utility = compute_best_response_parallel(I_1, I_2, true_board, 'x', policy_obj_x, policy_obj_o, 1, start_history, 'x', br, br_player);
    return expected_utility;
}


double WALKTREES(InformationSet& I, char br_player, std::vector<TicTacToeBoard>& true_board_list, std::vector<History>& history_list, 
                 std::vector<double>& reach_probability_list, std::vector<InformationSet>& opponent_I_list, PolicyVec& br, PolicyVec& policy_obj){
    std::cout << "Checkpoint 1" << std::endl;
    double expected_utility = 0.0;

    std::vector<int> actions;
    std::vector<double> Q_values;
    I.get_actions(actions);
    for (int action : actions) {
        Q_values.push_back(0.0);
    }

    std::cout << "Checkpoint 1.1" << std::endl;

    if (I.move_flag) {
        for (int a = 0; a < actions.size(); a++) {
            std::cout << "Checkpoint 1.2" << std::endl;
            for (int t = 0; t < true_board_list.size(); t++) {
                std::cout << "Checkpoint 1.3" << std::endl;
                TicTacToeBoard& true_board = true_board_list[t];
                History& history = history_list[t];
                double reach_probability = reach_probability_list[t];
                
                std::cout << "Checkpoint 1.4" << std::endl;

                TicTacToeBoard new_true_board = true_board;
                History new_history = history;
                bool success = new_true_board.update_move(actions[a], I.player);
                new_history.history.push_back(actions[a]);

                std::cout << "Checkpoint 1.5" << std::endl;

                char winner;
                if (success && !new_true_board.is_win(winner) && !new_true_board.is_over()) {
                    InformationSet new_I = I;
                    new_I.update_move(actions[a], I.player);
                    new_I.reset_zeros();

                    // simulate opponent's move
                    std::cout << "Checkpoint 1.6" << std::endl;
                    
                    InformationSet opponent_I = opponent_I_list[t];
                    std::vector<int> opponent_actions;
        
                    std::vector<History> post_sense_history_list;
                    std::vector<double> post_sense_reach_probability_list;
                    std::vector<InformationSet> post_sense_opponent_I_list;

                    opponent_I.get_actions_given_policy(opponent_actions, policy_obj);

                    std::cout << "Checkpoint 2" << std::endl;
                    // first simulate sense

                    for (int opponent_action : opponent_actions) {
                        InformationSet new_opponent_I = opponent_I;
                        new_opponent_I.simulate_sense(opponent_action, new_true_board);
                        new_opponent_I.reset_zeros();

                        post_sense_reach_probability_list.push_back(reach_probability * policy_obj.policy_dict[opponent_I.get_index()][opponent_action]);
                        post_sense_history_list.push_back(new_history);
                        post_sense_opponent_I_list.push_back(new_opponent_I);
                    }
                
                    std::cout << "Checkpoint 3" << std::endl;
                    // then simulate move

                    std::vector<TicTacToeBoard> post_move_true_board_list;
                    std::vector<History> post_move_history_list;
                    std::vector<double> post_move_reach_probability_list;
                    std::vector<InformationSet> post_move_opponent_I_list;

                    for (int i = 0; i < post_sense_history_list.size(); i++) {
                        InformationSet post_sense_opponent_I = post_sense_opponent_I_list[i];
                        std::vector<int> post_sense_opponent_actions;
                        post_sense_opponent_I.get_actions_given_policy(post_sense_opponent_actions, policy_obj);

                        for (int post_sense_opponent_action : post_sense_opponent_actions) {
                            TicTacToeBoard post_move_new_true_board = new_true_board;
                            History post_move_new_history = post_sense_history_list[i];
                            double post_move_reach_probability = post_sense_reach_probability_list[i] * policy_obj.policy_dict[post_sense_opponent_I.get_index()][post_sense_opponent_action];
                            success = post_move_new_true_board.update_move(post_sense_opponent_action, opponent_I.player);
                            post_move_new_history.history.push_back(post_sense_opponent_action);

                            std::cout << "Checkpoint 4" << std::endl;

                            if (success && !post_move_new_true_board.is_win(winner) && !post_move_new_true_board.is_over()) {
                                InformationSet post_move_oppoent_I = post_sense_opponent_I;
                                post_move_oppoent_I.update_move(post_sense_opponent_action, opponent_I.player);
                                post_move_oppoent_I.reset_zeros();

                                post_move_true_board_list.push_back(post_move_new_true_board);
                                post_move_history_list.push_back(post_move_new_history);
                                post_move_reach_probability_list.push_back(post_move_reach_probability);
                                post_move_opponent_I_list.push_back(post_move_oppoent_I);

                                std::cout << "Checkpoint 5" << std::endl;
                            }
                            else {
                                TerminalHistory H_T = TerminalHistory(post_move_new_history.history);
                                H_T.set_reward();
                                if (br_player == 'x'){
                                    Q_values[a] += H_T.reward[0] * post_move_reach_probability * reach_probability;
                                }
                                else{
                                    Q_values[a] += H_T.reward[1] * post_move_reach_probability * reach_probability;
                                }
                            }
                        }
                    }

                    for (int i = 0; i < post_move_true_board_list.size(); i++) {
                        Q_values[a] += reach_probability * WALKTREES(new_I, br_player, post_move_true_board_list, post_move_history_list, post_move_reach_probability_list, post_move_opponent_I_list, br, policy_obj);
                    }
                }
                else {
                    TerminalHistory H_T = TerminalHistory(new_history.history);
                    H_T.set_reward();
                    if (br_player == 'x'){
                        Q_values[a] += H_T.reward[0] * reach_probability;
                    }
                    else{
                        Q_values[a] += H_T.reward[1] * reach_probability;
                    }
                }
            }
        }
    }
    else {
        for (int a = 0; a < actions.size(); a++) {
            std::vector<History> new_history_list;
  
            for (int t = 0; t < true_board_list.size(); t++) {
                TicTacToeBoard& true_board = true_board_list[t];
                History& history = history_list[t];
                double reach_probability = reach_probability_list[t];

                InformationSet new_I = I;
                new_I.simulate_sense(actions[a], true_board);
                new_I.reset_zeros();

                History new_history = history;
                new_history.history.push_back(actions[a]);

                new_history_list.push_back(new_history);

                std::cout << "Checkpoint 6" << std::endl;
                Q_values[a] += reach_probability * WALKTREES(new_I, br_player, true_board_list, new_history_list, reach_probability_list, opponent_I_list, br, policy_obj);
            }
        }
    }
    
    if (I.player == 'x') {
        double max_Q = -1.0;
        int best_action = -1;

        for (int a = 0; a < Q_values.size(); a++) {
            if (Q_values[a] >= max_Q) {
                max_Q = Q_values[a];
                best_action = actions[a];
            }
        }

        if (best_action != -1) {
            // update policy
            std::vector<double>& prob_dist = br.policy_dict[I.get_index()];
            for (int a = 0; a < prob_dist.size(); a++) {
                if (a == best_action) {
                    prob_dist[a] = 1.0;
                } 
                else {
                    prob_dist[a] = 0.0;
                }
            }
        }
        else {
            // uniform policy

            std::vector<double>& prob_dist = br.policy_dict[I.get_index()];
            for (int a = 0; a < actions.size(); a++) {
                prob_dist[a] = 1.0 / actions.size();
            }
        }

        expected_utility = max_Q;
    }
    else {
        double min_Q = 1.0;
        int best_action = -1;

        for (int a = 0; a < Q_values.size(); a++) {
            if (Q_values[a] <= min_Q) {
                min_Q = Q_values[a];
                best_action = actions[a];
            }
        }

        if (best_action != -1) {
            // update policy
            std::vector<double>& prob_dist = br.policy_dict[I.get_index()];
            for (int a = 0; a < prob_dist.size(); a++) {
                if (a == best_action) {
                    prob_dist[a] = 1.0;
                } 
                else {
                    prob_dist[a] = 0.0;
                }
            }
        }
        else {
            // uniform policy

            std::vector<double>& prob_dist = br.policy_dict[I.get_index()];
            for (int a = 0; a < actions.size(); a++) {
                prob_dist[a] = 1.0 / actions.size();
            }
        }

        expected_utility = min_Q;
    }

    return expected_utility;
}


double WALKTREES_wrapper(PolicyVec& policy_obj, PolicyVec& br, char br_player){
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

    true_board_list.push_back(true_board);
    history_list.push_back(start_history);
    reach_probability_list.push_back(1.0);
    opponent_I_list.push_back(I_2);

    double expected_utility = WALKTREES(I_1, br_player, true_board_list, history_list, reach_probability_list, opponent_I_list, br, policy_obj);
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
    
    std::cout << "Policies loaded." << std::endl;
    std::cout << "Getting expected utility..." << std::endl;
    auto start = std::chrono::system_clock::now();   
    
    double expected_utility = get_expected_utility_wrapper(policy_obj_x, policy_obj_o);
    std::cout << "Expected utility: " << expected_utility << std::endl;

    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end-start;
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);
    std::cout << "finished computation at " << std::ctime(&end_time)
              << "elapsed time: " << elapsed_seconds.count() << "s"
              << std::endl;

    std::cout << "Copying policies..." << std::endl;
    PolicyVec br_x = policy_obj_x;
    // PolicyVec br_o = policy_obj_o;

    std::cout << "Computing best response..." << std::endl;
    start = std::chrono::system_clock::now();
    // expected_utility = compute_best_response_wrapper(policy_obj_x, policy_obj_o, br_x, 'x');
    // std::cout << "Expected utility: " << expected_utility << std::endl;
    // expected_utility = compute_best_response_wrapper(policy_obj_x, policy_obj_o, br_o, 'o');
    // std::cout << "Expected utility: " << expected_utility << std::endl;

    expected_utility = WALKTREES_wrapper(policy_obj_o, br_x, 'x');

    end = std::chrono::system_clock::now();
    elapsed_seconds = end-start;
    end_time = std::chrono::system_clock::to_time_t(end);
    std::cout << "finished computation at " << std::ctime(&end_time)
              << "elapsed time: " << elapsed_seconds.count() << "s"
              << std::endl;

    // make_pure_strategy(br_x, 'x', P1_information_sets);
    // make_pure_strategy(br_o, 'o', P2_information_sets);

    std::cout << "Getting expected utility..." << std::endl;
    start = std::chrono::system_clock::now();
    expected_utility = get_expected_utility_wrapper(br_x, policy_obj_o);
    std::cout << "Expected utility of best response against P2: " << expected_utility << std::endl;
    // expected_utility = get_expected_utility_wrapper(policy_obj_x, br_o);
    // std::cout << "Expected utility of best response against P1: " << expected_utility << std::endl;

    end = std::chrono::system_clock::now();
    elapsed_seconds = end-start;
    end_time = std::chrono::system_clock::to_time_t(end);
    std::cout << "finished computation at " << std::ctime(&end_time)
              << "elapsed time: " << elapsed_seconds.count() << "s"
              << std::endl;
    return 0;
}
