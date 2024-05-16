#include "cpp_headers/rbt_classes.hpp"
#include "cpp_headers/rbt_utilities.hpp"
#include "cpp_headers/json.hpp"
using json = nlohmann::json;

char toggle_player(char player) {
    return (player == 'x') ? 'o' : 'x';
}


void valid_histories_play(InformationSet& I_1, InformationSet& I_2, TicTacToeBoard& true_board, char player, History& current_history, InformationSet& end_I, 
                          std::vector<int>& played_actions, Policy& policy_obj_x, Policy& policy_obj_o, std::vector<std::vector<int>>& valid_histories_list){
    
    InformationSet& I = player == 'x' ? I_1 : I_2;
    std::vector<int> actions;

    if (player == 'x') {
        if (end_I.player == 'x'){
            I.get_actions(actions);
            if (I.move_flag) {
                actions = intersection(actions, played_actions);
            }
        }
        else {
            I.get_actions_given_policy(actions, policy_obj_x);
        }
        
    } else {
        if (end_I.player == 'o'){
            I.get_actions(actions);
            if (I.move_flag) {
                actions = intersection(actions, played_actions);
            }
        }
        else {
            I.get_actions_given_policy(actions, policy_obj_o);
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


void upgraded_get_histories_given_I(InformationSet& I, Policy& policy_obj_x, Policy& policy_obj_o, std::vector<std::vector<int>>& valid_histories_list){
    
    if (I.get_hash() == "000000000m"){
        return;
    }

    std::string board_1 = "000000000";
    std::string board_2 = "---------";
    std::string board = "000000000";
    InformationSet I_1('x', true, board_1);
    InformationSet I_2('o', false, board_2);
    TicTacToeBoard true_board = TicTacToeBoard(board);
    char player = 'x';
    std::vector<int> played_actions;
    I.get_played_actions(played_actions);

    std::vector<int> h = {};
    NonTerminalHistory current_history(h);
    valid_histories_play(I_1, I_2, true_board, player, current_history, I, played_actions, policy_obj_x, policy_obj_o, valid_histories_list);
    return;
}   


double get_expected_utility(InformationSet &I_1, InformationSet &I_2, TicTacToeBoard &true_board, char player, Policy &policy_obj_x, 
                            Policy &policy_obj_o, double probability, History& current_history, char initial_player) {
    double expected_utility_h = 0;
    
    InformationSet& I = player == 'x' ? I_1 : I_2;
    Policy& policy_obj = player == 'x' ? policy_obj_x : policy_obj_o;
    
    std::vector<int> actions;
    I.get_actions_given_policy(actions, policy_obj);
    
    if (I.move_flag) {
        for (int action : actions) {
            TicTacToeBoard new_true_board = true_board;
            bool success = new_true_board.update_move(action, player);

            double probability_new = probability * policy_obj.policy_dict[I.get_hash()][action];
            History new_history = current_history;
            new_history.history.push_back(action);
                
            char winner;
            if (success && !new_true_board.is_win(winner) && !new_true_board.is_over()) {
                InformationSet new_I(I);
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
            InformationSet new_I(I);
            new_I.simulate_sense(action, true_board);
            
            double probability_new = probability * policy_obj.policy_dict[I.get_hash()][action];
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


double get_expected_utility_parallel(InformationSet &I_1, InformationSet &I_2, TicTacToeBoard &true_board, char player, Policy &policy_obj_x, 
                                     Policy &policy_obj_o, double probability, History& current_history, char initial_player) {
    double expected_utility_h = 0;
    std::vector<InformationSet> Depth_1_P1_Isets;
    std::vector<InformationSet> Depth_1_P2_Isets;
    std::vector<TicTacToeBoard> Depth_1_boards;
    std::vector<char> Depth_1_players;
    std::vector<double> Depth_1_probabilities;
    std::vector<History> Depth_1_histories;
    
    InformationSet I = player == 'x' ? I_1 : I_2;
    Policy policy_obj = player == 'x' ? policy_obj_x : policy_obj_o;

    std::vector<int> actions;
    I.get_actions_given_policy(actions, policy_obj);

    if (I.move_flag) {
        for (int action : actions) {
            TicTacToeBoard new_true_board = true_board;
            bool success = new_true_board.update_move(action, player);

            double probability_new = probability * policy_obj.policy_dict[I.get_hash()][action];
            History new_history = current_history;
            new_history.history.push_back(action);

            char winner;
            if (success && !new_true_board.is_win(winner) && !new_true_board.is_over()) {
                InformationSet new_I(I);
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
            InformationSet new_I(I);
            new_I.simulate_sense(action, true_board);

            double probability_new = probability * policy_obj.policy_dict[I.get_hash()][action];
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


double get_expected_utility_wrapper(Policy& policy_obj_x, Policy& policy_obj_o){
    std::string board = "000000000";
    TicTacToeBoard true_board = TicTacToeBoard(board);
    std::string board_1 = "000000000";
    std::string board_2 = "---------";
    InformationSet I_1 = InformationSet('x', true, board_1);
    InformationSet I_2 = InformationSet('o', false, board_2);
    std::vector<int> h = {};
    TerminalHistory start_history = TerminalHistory(h);

    double expected_utility = get_expected_utility_parallel(I_1, I_2, true_board, 'x', policy_obj_x, policy_obj_o, 1, start_history, 'x');
    return expected_utility;
}


double get_prob_h_given_policy(InformationSet& I_1, InformationSet& I_2, TicTacToeBoard& true_board, char player, int next_action, 
                               Policy& policy_obj_x, Policy& policy_obj_o, double probability, History history_obj, char initial_player){

    InformationSet& I = player == 'x' ? I_1 : I_2;
    Policy& policy_obj = player == 'x' ? policy_obj_x : policy_obj_o;

    if (I.move_flag) {
        TicTacToeBoard new_true_board = true_board;
        bool success = new_true_board.update_move(next_action, player);

        if (I.player == toggle_player(initial_player)) {
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

        if (I.player == toggle_player(initial_player)) {
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


double get_prob_h_given_policy_wrapper(InformationSet& I_1, InformationSet& I_2, TicTacToeBoard& true_board, char player, int next_action, Policy& policy_obj_x, 
                                       Policy& policy_obj_o, double probability, History history_obj, InformationSet& curr_I_1, char initial_player){
    
    if (curr_I_1.get_hash() == "000000000m"){
        return 1.0;
    }
    else {
        return get_prob_h_given_policy(I_1, I_2, true_board, player, next_action, policy_obj_x, policy_obj_o, probability, history_obj, initial_player);
    }
}


double get_counter_factual_utility(InformationSet& I, Policy& policy_obj_x, Policy& policy_obj_o, std::vector<std::vector<int>>& starting_histories, std::vector<double>& prob_reaching_h_list) {
    double counter_factual_utility = 0;
    int count = 0;
    for (std::vector<int> h : starting_histories) {
        NonTerminalHistory h_object(h);
        std::string board_1 = "000000000";
        std::string board_2 = "---------";
        std::string board = "000000000";
        InformationSet curr_I_1('x', true, board_1);
        InformationSet curr_I_2('o', false, board_2);
        TicTacToeBoard true_board = TicTacToeBoard(board);
        h_object.get_information_sets(curr_I_1, curr_I_2);
        char curr_player = 'x';
        h_object.get_board(true_board, curr_player);

        double probability_reaching_h;
        double expected_utility_h;

        if (prob_reaching_h_list[count] > 0) {
            expected_utility_h = get_expected_utility(curr_I_1, curr_I_2, true_board, I.player, policy_obj_x, policy_obj_o, 1, h_object, I.player);

            if (!(curr_I_1.get_hash() == "000000000m")){
                probability_reaching_h = prob_reaching_h_list[count];
            }
            else {
                probability_reaching_h = 1.0;
            }

            counter_factual_utility += expected_utility_h * probability_reaching_h;
        }
        
        count += 1;
    }
    return counter_factual_utility;
}


void get_probability_of_reaching_all_h(InformationSet& I, Policy& policy_obj_x, Policy& policy_obj_o, std::vector<std::vector<int>>& starting_histories, 
                                       char initial_player, std::vector<double>& prob_reaching_h_list_all) {
    for (std::vector<int> h : starting_histories) {
        NonTerminalHistory h_object(h);

        if (!(I.get_hash() == "000000000m")) {
            std::string board = "000000000";
            std::string board_1 = "000000000";
            std::string board_2 = "---------";
            InformationSet I_1('x', true, board_1);
            InformationSet I_2('o', false, board_2);
            TicTacToeBoard true_board = TicTacToeBoard(board);
            double probability_reaching_h = get_prob_h_given_policy_wrapper(I_1, I_2, true_board, 'x', h[0], policy_obj_x, policy_obj_o, 1, h_object, I, initial_player);
            prob_reaching_h_list_all.push_back(probability_reaching_h);
        }
        else {
            prob_reaching_h_list_all.push_back(1.0);
        }
    }
}


double calc_util_a_given_I_and_action(InformationSet& I, int action, Policy& policy_obj_x, Policy& policy_obj_o, 
                                      std::vector<std::vector<int>>& starting_histories, std::vector<double>& prob_reaching_h_list) {
    double util_a = 0;
    std::vector <double> prob_dist(13);
    std::vector <double> old_prob_dist(13);
    Policy& policy_obj = I.player == 'x' ? policy_obj_x : policy_obj_o;

    for (int i = 0; i < 13; i++) {
        prob_dist[i] = 0;
    }
    prob_dist[action] = 1;

    old_prob_dist = policy_obj.policy_dict[I.get_hash()];
    policy_obj.policy_dict[I.get_hash()] = prob_dist;
    util_a = get_counter_factual_utility(I, policy_obj_x, policy_obj_o, starting_histories, prob_reaching_h_list);
    policy_obj.policy_dict[I.get_hash()] = old_prob_dist;

    return util_a;
}


void calc_cfr_policy_given_I(InformationSet& I, Policy& policy_obj_x, Policy& policy_obj_o, int T, std::vector<double>& regret_list) {
    double util = 0;
    std::vector<int> actions;
    Policy& policy_obj = I.player == 'x' ? policy_obj_x : policy_obj_o;
    std::vector<std::vector<int>> starting_histories;
    std::vector<double> prob_reaching_h_list;
    std::vector<double> util_a_list;
    
    for (int i = 0; i < 13; i++) {
        util_a_list.push_back(0);
    }

    I.get_actions(actions);
    
    upgraded_get_histories_given_I(I, policy_obj_x, policy_obj_o, starting_histories);
    get_probability_of_reaching_all_h(I, policy_obj_x, policy_obj_o, starting_histories, I.player, prob_reaching_h_list);
    
    for (int action : actions) {
        double util_a = calc_util_a_given_I_and_action(I, action, policy_obj_x, policy_obj_o, starting_histories, prob_reaching_h_list); 
        util += util_a * policy_obj.policy_dict[I.get_hash()][action];
        util_a_list[action] = util_a;
    }
    
    for (int action : actions) {
        double regret_T = 0;
        if (T == 0) {
            regret_T = util_a_list[action] - util;
        } else {
            regret_T = (1.0 / double(T)) * ((double(T) - 1.0) * regret_list[action] + util_a_list[action] - util);
        }

        regret_T = regret_T > 0 ? regret_T : 0;
        regret_list[action] = regret_T;
    }
}


std::unordered_map<std::string, std::vector<double> > get_prev_regrets(std::string& file_path){
    std::ifstream i(file_path);
    json regret_obj;
    i >> regret_obj;
    std::unordered_map<std::string, std::vector<double> > regret_map;
        
    for (json::iterator it = regret_obj.begin(); it != regret_obj.end(); ++it) {
        std::string key = it.key();
        std::vector <double> probability_distribution(13);
        // initialise all values to zero
        for (int i = 0; i < 13; i++) {
            probability_distribution[i] = 0;
        }

        if (key.back() == 's') {
            std::vector<std::string> sense_keys = {"9", "10", "11", "12"};
            for (int i = 0; i < sense_keys.size(); i++) {
                probability_distribution[stoi(sense_keys[i])] = regret_obj[key][sense_keys[i]];
            }
        }
        else if (key.back() == 'm') {
            std::vector<std::string> move_keys = {"0", "1", "2", "3", "4", "5", "6", "7", "8"};
            for (int i = 0; i < move_keys.size(); i++) {
                probability_distribution[stoi(move_keys[i])] = regret_obj[key][move_keys[i]];
            }
        }
        regret_map[key] = probability_distribution;
    }

    return regret_map;
}