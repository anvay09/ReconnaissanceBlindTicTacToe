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


double compute_best_response(InformationSet &I_1, InformationSet &I_2, TicTacToeBoard &true_board, char player, PolicyVec &policy_obj_x, 
                            PolicyVec &policy_obj_o, double probability, History& current_history, char initial_player, PolicyVec& br) {
    double expected_utility_h = 0.0;
    
    InformationSet& I = player == 'x' ? I_1 : I_2;
    PolicyVec& policy_obj = player == 'x' ? policy_obj_x : policy_obj_o;
    
    std::vector<int> actions;
    std::vector<double> Q_values;

    if (player == initial_player) {
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
            if (player == initial_player) {
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
                    if (player == initial_player) {
                        Q_values[a] = compute_best_response(new_I, I_2, new_true_board, 'o', policy_obj_x, policy_obj_o, probability_new, new_history, initial_player, br);
                    }
                    else {
                        expected_utility_h += compute_best_response(new_I, I_2, new_true_board, 'o', policy_obj_x, policy_obj_o, probability_new, new_history, initial_player, br);
                    }
                } else {
                    if (player == initial_player) {
                        Q_values[a] = compute_best_response(I_1, new_I, new_true_board, 'x', policy_obj_x, policy_obj_o, probability_new, new_history, initial_player, br);
                    }
                    else {
                        expected_utility_h += compute_best_response(I_1, new_I, new_true_board, 'x', policy_obj_x, policy_obj_o, probability_new, new_history, initial_player, br);
                    }
                }
            } else {
                TerminalHistory H_T = TerminalHistory(new_history.history);
                H_T.set_reward();
                if (initial_player == 'x'){
                    if (player == initial_player) {
                        Q_values[a] = H_T.reward[0] * probability_new;
                        expected_utility_h += Q_values[a];
                    }
                    else {
                        expected_utility_h += H_T.reward[0] * probability_new;
                    }
                }
                else{
                    if (player == initial_player) {
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
            if (player == initial_player) {
                probability_new = probability;
            }
            else {
                probability_new = probability * policy_obj.policy_dict[I.get_index()][actions[a]];
            }
            
            History new_history = current_history;
            new_history.history.push_back(actions[a]);

            if (player == 'x') {
                if (player == initial_player) {
                    Q_values[a] = compute_best_response(new_I, I_2, true_board, 'x', policy_obj_x, policy_obj_o, probability_new, new_history, initial_player, br);
                }
                else {
                    expected_utility_h += compute_best_response(new_I, I_2, true_board, 'x', policy_obj_x, policy_obj_o, probability_new, new_history, initial_player, br);
                }
            } else {
                if (player == initial_player) {
                    Q_values[a] = compute_best_response(I_1, new_I, true_board, 'o', policy_obj_x, policy_obj_o, probability_new, new_history, initial_player, br);
                }
                else {
                    expected_utility_h += compute_best_response(I_1, new_I, true_board, 'o', policy_obj_x, policy_obj_o, probability_new, new_history, initial_player, br);
                }
            }
        }
    }

    if (player == initial_player) {
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
                    prob_dist[a] += max_Q;
                } 
            }
        }
        expected_utility_h = max_Q;
    }

    return expected_utility_h;
}


double compute_best_response_parallel(InformationSet &I_1, InformationSet &I_2, TicTacToeBoard &true_board, char player, PolicyVec &policy_obj_x, 
                                     PolicyVec &policy_obj_o, double probability, History& current_history, char initial_player, PolicyVec& br) {
    double expected_utility_h = 0.0;
    std::vector<InformationSet> Depth_1_P1_Isets;
    std::vector<InformationSet> Depth_1_P2_Isets;
    std::vector<TicTacToeBoard> Depth_1_boards;
    std::vector<char> Depth_1_players;
    std::vector<double> Depth_1_probabilities;
    std::vector<History> Depth_1_histories;
    std::vector<std::vector<int>> Depth_1_actions;
    std::vector<std::vector<double>> Depth_1_Q_values;

    InformationSet I = player == 'x' ? I_1 : I_2;
    PolicyVec policy_obj = player == 'x' ? policy_obj_x : policy_obj_o;

    std::vector<int> actions;
    std::vector<double> Q_values;

    if (player == initial_player) {
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
            if (player == initial_player) {
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
                    Depth_1_actions.push_back(actions);
                    Depth_1_Q_values.push_back(Q_values);
                } 
                else {
                    Depth_1_P1_Isets.push_back(I_1);
                    Depth_1_P2_Isets.push_back(new_I);
                    Depth_1_boards.push_back(new_true_board);
                    Depth_1_players.push_back('x');
                    Depth_1_probabilities.push_back(probability_new);
                    Depth_1_histories.push_back(new_history);
                    Depth_1_actions.push_back(actions);
                    Depth_1_Q_values.push_back(Q_values);
                }
            }

            else {
                TerminalHistory H_T = TerminalHistory(new_history.history);
                H_T.set_reward();
                if (initial_player == 'x'){
                    if (player == initial_player) {
                        Q_values[a] = H_T.reward[0] * probability_new;
                    }
                    else {
                        expected_utility_h += H_T.reward[0] * probability_new;
                    }
                }
                else{
                    if (player == initial_player) {
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
            if (player == initial_player) {
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
                Depth_1_actions.push_back(actions);
                Depth_1_Q_values.push_back(Q_values);
            } 
            else {
                Depth_1_P1_Isets.push_back(I_1);
                Depth_1_P2_Isets.push_back(new_I);
                Depth_1_boards.push_back(true_board);
                Depth_1_players.push_back('o');
                Depth_1_probabilities.push_back(probability_new);
                Depth_1_histories.push_back(new_history);
                Depth_1_actions.push_back(actions);
                Depth_1_Q_values.push_back(Q_values);
            }
        }
    }

    # pragma omp parallel for num_threads(96)
    for (int i = 0; i < Depth_1_P1_Isets.size(); i++) {
        expected_utility_h += compute_best_response(Depth_1_P1_Isets[i], Depth_1_P2_Isets[i], Depth_1_boards[i], Depth_1_players[i], policy_obj_x, policy_obj_o, Depth_1_probabilities[i], Depth_1_histories[i], initial_player, br);
    }

    if (player == initial_player) {
        for (int i = 0; i < Depth_1_P1_Isets.size(); i++) {
            double max_Q = -1.0;
            int best_action = 0;

            for (int a = 0; a < Depth_1_Q_values[i].size(); a++) {
                if (Depth_1_Q_values[i][a] >= max_Q) {
                    max_Q = Depth_1_Q_values[i][a];
                    best_action = Depth_1_actions[i][a];
                }
            }

            // update policy
            std::vector<double>& prob_dist = br.policy_dict[Depth_1_P1_Isets[i].get_index()];
            for (int a = 0; a < prob_dist.size(); a++) {
                if (a == best_action) {
                    prob_dist[a] = 1.0;
                } else {
                    prob_dist[a] = 0.0;
                }
            }

            expected_utility_h += max_Q;
        }
    }

    return expected_utility_h;

}

double compute_best_response_wrapper(PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, PolicyVec& br){
    std::string board = "000000000";
    TicTacToeBoard true_board = TicTacToeBoard(board);
    std::string hash_1 = "";
    std::string hash_2 = "";
    InformationSet I_1 = InformationSet('x', true, hash_1);
    InformationSet I_2 = InformationSet('o', false, hash_2);
    std::vector<int> h = {};
    TerminalHistory start_history = TerminalHistory(h);

    double expected_utility = compute_best_response(I_1, I_2, true_board, 'x', policy_obj_x, policy_obj_o, 1, start_history, 'x', br);
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

    std::cout << "Copying policy..." << std::endl;
    PolicyVec br = policy_obj_x;

    std::cout << "Computing best response..." << std::endl;
    start = std::chrono::system_clock::now();
    expected_utility = compute_best_response_wrapper(policy_obj_x, policy_obj_o, br);
    std::cout << "Expected utility: " << expected_utility << std::endl;

    end = std::chrono::system_clock::now();
    elapsed_seconds = end-start;
    end_time = std::chrono::system_clock::to_time_t(end);
    std::cout << "finished computation at " << std::ctime(&end_time)
              << "elapsed time: " << elapsed_seconds.count() << "s"
              << std::endl;

    // find max of each information set
    std::cout << "Editing Policy..." << std::endl;
    for (long int i = 0; i < P1_information_sets.size(); i++) {
        std::vector<double>& prob_dist = br.policy_dict[i];
        
        double max_Q = -std::numeric_limits<double>::infinity();
        int best_action = -1;
        for (int a = 0; a < prob_dist.size(); a++) {
            if (prob_dist[a] >= max_Q) {
                max_Q = prob_dist[a];
                best_action = a;
            }
        }

        if (best_action != -1) {
            for (int a = 0; a < prob_dist.size(); a++) {
                if (a == best_action) {
                    prob_dist[a] = 1.0;
                } else {
                    prob_dist[a] = 0.0;
                }
            }
        }
        else {
            // equal probability to all actions
            std::vector<int> actions;
            std::string hash = P1_information_sets[i];
            bool move_flag = get_move_flag(hash, 'x');
            InformationSet I = InformationSet('x', move_flag, hash);
            I.get_actions(actions);
            
            for (int i = 0; i < actions.size(); i++) {
                prob_dist[i] = 1.0 / actions.size();
            }
        }
    }


    std::cout << "Getting expected utility..." << std::endl;
    start = std::chrono::system_clock::now();
    expected_utility = get_expected_utility_wrapper(br, policy_obj_o);
    std::cout << "Expected utility: " << expected_utility << std::endl;

    end = std::chrono::system_clock::now();
    elapsed_seconds = end-start;
    end_time = std::chrono::system_clock::to_time_t(end);
    std::cout << "finished computation at " << std::ctime(&end_time)
              << "elapsed time: " << elapsed_seconds.count() << "s"
              << std::endl;
    return 0;
}
