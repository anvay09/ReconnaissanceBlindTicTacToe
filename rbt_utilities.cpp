#include "cpp_headers/rbt_classes.hpp"
#include "cpp_headers/rbt_utilities.hpp"
#include "cpp_headers/json.hpp"
using json = nlohmann::json;

char toggle_player(char player) {
    return (player == 'x') ? 'o' : 'x';
}


void valid_histories_play(InformationSet& I_1, InformationSet& I_2, TicTacToeBoard& true_board, char player, History& current_history, InformationSet& end_I, std::vector<std::vector<bool>>& allowed_move_masks,
                          std::vector<int>& played_actions, int current_action_index, int other_player_turn_index, PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, std::vector<std::vector<int>>& valid_histories_list){
    InformationSet& I = player == 'x' ? I_1 : I_2;
    std::vector<int> actions;

    if (player == 'x') {
        if (end_I.player == 'x'){
            actions.push_back(played_actions[current_action_index++]);
        }
        else {
            if (I.move_flag) {
                if (other_player_turn_index >= allowed_move_masks.size()) {
                    I.get_actions_given_policy(actions, policy_obj_x);
                }
                else {
                    std::vector<int> temp_actions;
                    I.get_actions_given_policy(temp_actions, policy_obj_x);
                    for (int action : temp_actions) {
                        if (allowed_move_masks[other_player_turn_index][action]) {
                            actions.push_back(action);
                        }
                    }
                    other_player_turn_index++;
                }
            }
            else {
                I.get_actions_given_policy(actions, policy_obj_x);
            }
        }
        
    } 
    else {
        if (end_I.player == 'o'){
            actions.push_back(played_actions[current_action_index++]);
        }
        else {
            if (I.move_flag) {
                if (other_player_turn_index >= allowed_move_masks.size()) {
                    I.get_actions_given_policy(actions, policy_obj_o);
                }
                else {
                    std::vector<int> temp_actions;
                    I.get_actions_given_policy(temp_actions, policy_obj_o);

                    for (int action : temp_actions) {
                        if (allowed_move_masks[other_player_turn_index][action]) {
                            actions.push_back(action);
                        }
                    }
                    other_player_turn_index++;
                }
            }
            else {
                I.get_actions_given_policy(actions, policy_obj_o);
            }
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
                InformationSet new_I = I;
                new_I.update_move(action, player);
                new_I.reset_zeros();

                if (player == 'x') {
                    if (end_I.player == 'x') {
                        valid_histories_play(new_I, I_2, new_true_board, 'o', new_history, end_I, allowed_move_masks, played_actions, current_action_index, other_player_turn_index, policy_obj_x, policy_obj_o, valid_histories_list);
                    }
                    else {
                        if (I_2 == end_I){
                            valid_histories_list.push_back(new_history.history);
                        }
                        else {
                            valid_histories_play(new_I, I_2, new_true_board, 'o', new_history, end_I, allowed_move_masks, played_actions, current_action_index, other_player_turn_index, policy_obj_x, policy_obj_o, valid_histories_list);
                        }
                    }
                }
                else {
                    if (end_I.player == 'o') {
                        valid_histories_play(I_1, new_I, new_true_board, 'x', new_history, end_I, allowed_move_masks, played_actions, current_action_index, other_player_turn_index, policy_obj_x, policy_obj_o, valid_histories_list);
                    }
                    else {
                        if (I_1 == end_I){
                            valid_histories_list.push_back(new_history.history);
                        }
                        else {
                            valid_histories_play(I_1, new_I, new_true_board, 'x', new_history, end_I, allowed_move_masks, played_actions, current_action_index, other_player_turn_index, policy_obj_x, policy_obj_o, valid_histories_list);
                        }
                    }
                }
            }
        }
    }
    else {
        for (int action : actions) {
            InformationSet new_I = I;
            new_I.simulate_sense(action, true_board);
            TicTacToeBoard new_true_board = true_board;

            History new_history = current_history;
            new_history.history.push_back(action);

            if (player == 'x') {
                if (end_I.player == 'x') {
                    if (!(new_I == end_I)){
                        valid_histories_play(new_I, I_2, new_true_board, 'x', new_history, end_I, allowed_move_masks, played_actions, current_action_index, other_player_turn_index, policy_obj_x, policy_obj_o, valid_histories_list);
                    }
                    else {
                        valid_histories_list.push_back(new_history.history);
                    }
                }
                else {
                    valid_histories_play(new_I, I_2, new_true_board, 'x', new_history, end_I, allowed_move_masks, played_actions, current_action_index, other_player_turn_index, policy_obj_x, policy_obj_o, valid_histories_list);
                }
            }
            else {
                if (end_I.player == 'o') {
                    if (!(new_I == end_I)){
                        valid_histories_play(I_1, new_I, new_true_board, 'o', new_history, end_I, allowed_move_masks, played_actions, current_action_index, other_player_turn_index, policy_obj_x, policy_obj_o, valid_histories_list);
                    }
                    else {
                        valid_histories_list.push_back(new_history.history);
                    }
                }
                else {
                    valid_histories_play(I_1, new_I, new_true_board, 'o', new_history, end_I, allowed_move_masks, played_actions, current_action_index, other_player_turn_index, policy_obj_x, policy_obj_o, valid_histories_list);
                }
            }
        }
    }
}


void get_allowed_move_masks_for_other_player(InformationSet& I, std::vector<std::vector<bool>>& allowed_move_masks, std::vector<int>& played_actions){
    std::vector<std::vector<bool>> known_moves_at_each_stage;
    std::vector<int> other_player_sense_moves;
    std::vector<std::string> observation_list;

    bool move_action = I.player == 'x' ? true : false;
    bool sense_action = I.player == 'x' ? false : true;
    bool observation = false;
    int i = 0;
    int other_player_move_index = -1;
    std::unordered_map<int, std::vector<int> > sense_square_dict = {{9, {0, 1, 3, 4}}, {10, {1, 2, 4, 5}}, {11, {3, 4, 6, 7}}, {12, {4, 5, 7, 8}}};

    while (i < I.hash.size()) {
        switch (I.hash[i]) { 
            case '|': 
                if (observation) { 
                    observation = false;
                    move_action = true;
                }
                else{
                    observation = true;
                    sense_action = false;
                    if (other_player_move_index == -1) {
                        known_moves_at_each_stage.push_back(std::vector<bool>(9, false));
                    }
                    else {
                        std::vector<bool> prev_known_moves = known_moves_at_each_stage[other_player_move_index];
                        known_moves_at_each_stage.push_back(prev_known_moves);
                    }
                    observation_list.push_back("");
                    other_player_move_index++;
                }

                i++;
                break;

            case '_': 
                move_action = false;
                sense_action = true;

                i++;
                break;

            default: 
                if (move_action) {
                    i++;
                }
                else if (sense_action) { 
                    other_player_sense_moves.push_back(I.hash[i] - '0' + 9);
                    i++;
                }
                else if (observation) { 
                    
                    for (int square: sense_square_dict[other_player_sense_moves[other_player_move_index]]) {
                        if (I.hash[i] == 'o'){
                            known_moves_at_each_stage[other_player_move_index][square] = true;
                        }
                        observation_list[other_player_move_index] += I.hash[i];
                        i++;
                    }
                }
        }
    }

    std::vector<bool> mask(9, true);
    for (int i = 0; i < played_actions.size(); i++) {
        mask[played_actions[i]] = false;
    }

    for (int i = other_player_sense_moves.size() - 1; i >= 0; i--) {
        int num_known_moves = 0;
        for (int j = 0; j < 9; j++) {
            if (known_moves_at_each_stage[i][j]) {
                num_known_moves++;
            }
        }

        if (num_known_moves == i + 1){
            mask = known_moves_at_each_stage[i];
        }
        else {
            std::vector<int> sense_index_list = sense_square_dict[other_player_sense_moves[i]];
            for (int s = 0; s < 4; s++) {
                if (observation_list[i][s] == '0') {
                    mask[sense_index_list[s]] = false;
                }
            }
        }

        allowed_move_masks.insert(allowed_move_masks.begin(), mask);
    }
}


void upgraded_get_histories_given_I(InformationSet& I, PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, std::vector<std::vector<int>>& valid_histories_list){
    if (I.board == "000000000"){
        std::vector<int> init_h = {};
        valid_histories_list.push_back(init_h);
        return;
    }
    std::string hash_1 = "";
    std::string hash_2 = "";
    std::string board = "000000000";
    InformationSet I_1('x', true, hash_1);
    InformationSet I_2('o', false, hash_2);
    TicTacToeBoard true_board = TicTacToeBoard(board);
    char player = 'x';
    std::vector<int> played_actions;
    I.get_played_actions(played_actions);
    std::vector<std::vector<bool>> allowed_move_masks;
    get_allowed_move_masks_for_other_player(I, allowed_move_masks, played_actions);

    std::vector<int> h = {};
    NonTerminalHistory current_history(h);
    valid_histories_play(I_1, I_2, true_board, player, current_history, I, allowed_move_masks, played_actions, 0, 0, policy_obj_x, policy_obj_o, valid_histories_list);
    return;
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

            if (I.get_index() == -1){
                std::cout << "KEY ERROR: Get exp u" << std::endl;
            }
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

double get_expected_utility_action_version(InformationSet &I_1, InformationSet &I_2, TicTacToeBoard &true_board, char player, PolicyVec &policy_obj_x, 
                            PolicyVec &policy_obj_o, double probability, History& current_history, char initial_player, int action) {
    double expected_utility_h = 0.0;
    
    InformationSet& I = player == 'x' ? I_1 : I_2;
    PolicyVec& policy_obj = player == 'x' ? policy_obj_x : policy_obj_o;
    
    if (I.move_flag) {
        TicTacToeBoard new_true_board = true_board;
        bool success = new_true_board.update_move(action, player);

        double probability_new = probability * 1;
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
    else {
        InformationSet new_I = I;
        new_I.simulate_sense(action, true_board);
        
        double probability_new = probability * 1;
        History new_history = current_history;
        new_history.history.push_back(action);

        if (player == 'x') {
            expected_utility_h += get_expected_utility(new_I, I_2, true_board, 'x', policy_obj_x, policy_obj_o, probability_new, new_history, initial_player);
        } else {
            expected_utility_h += get_expected_utility(I_1, new_I, true_board, 'o', policy_obj_x, policy_obj_o, probability_new, new_history, initial_player);
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


double get_prob_h_given_policy(InformationSet& I_1, InformationSet& I_2, TicTacToeBoard& true_board, char player, int next_action, 
                               PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, double probability, History history_obj, char initial_player, InformationSet& end_I){

    InformationSet& I = player == 'x' ? I_1 : I_2;
    PolicyVec& policy_obj = player == 'x' ? policy_obj_x : policy_obj_o;

    if (I.move_flag) {
        TicTacToeBoard new_true_board = true_board;
        bool success = new_true_board.update_move(next_action, player);

        if (I.player == toggle_player(initial_player)) {
            if (I.get_index() == -1){
                std::cout << "KEY ERROR: Get Prob h, size of history: " << history_obj.history.size() << " Information set: " << end_I.get_hash() << std::endl;
                std::cout << "Invalid I: " << I.get_hash() << std::endl;
                for (int z=0; z < history_obj.history.size(); z++){
                    std::cout << history_obj.history[z] << " ";
                }
                std::cout << std::endl;
                exit(1);
            }
            probability *= policy_obj.policy_dict[I.get_index()][next_action];
        }
        history_obj.track_traversal_index += 1;
        if (history_obj.track_traversal_index < history_obj.history.size()) {
            int new_next_action = history_obj.history[history_obj.track_traversal_index];

            char winner;
            if (success && !new_true_board.is_win(winner) && !new_true_board.is_over()) {
                InformationSet new_I = I;
                new_I.update_move(next_action, player);
                new_I.reset_zeros();

                if (player == 'x') {
                    probability = get_prob_h_given_policy(new_I, I_2, new_true_board, 'o', new_next_action, policy_obj_x, policy_obj_o, probability, history_obj, initial_player, end_I);
                }
                else {
                    probability = get_prob_h_given_policy(I_1, new_I, new_true_board, 'x', new_next_action, policy_obj_x, policy_obj_o, probability, history_obj, initial_player, end_I);
                }
            }
        }
    }
    else {
        InformationSet new_I = I;
        new_I.simulate_sense(next_action, true_board);
        TicTacToeBoard new_true_board = true_board;

        if (I.player == toggle_player(initial_player)) {
            if (I.get_index() == -1){
                std::cout << "KEY ERROR: Get Prob h, size of history: " << history_obj.history.size() << " Information set: " << end_I.get_hash() << std::endl;
                std::cout << "Invalid I: " << I.get_hash() << std::endl;
                for (int z=0; z < history_obj.history.size(); z++){
                    std::cout << history_obj.history[z] << " ";
                }
                std::cout << std::endl;
                exit(1);
            }
            probability *= policy_obj.policy_dict[I.get_index()][next_action];
        }
        history_obj.track_traversal_index += 1;
        if (history_obj.track_traversal_index < history_obj.history.size()) {
            int new_next_action = history_obj.history[history_obj.track_traversal_index];

            if (player == 'x') {
                probability = get_prob_h_given_policy(new_I, I_2, new_true_board, 'x', new_next_action, policy_obj_x, policy_obj_o, probability, history_obj, initial_player, end_I);
            }
            else {
                probability = get_prob_h_given_policy(I_1, new_I, new_true_board, 'o', new_next_action, policy_obj_x, policy_obj_o, probability, history_obj, initial_player, end_I);
            }
        }
    }

    return probability;
}


double get_prob_h_given_policy_wrapper(InformationSet& I_1, InformationSet& I_2, TicTacToeBoard& true_board, char player, int next_action, PolicyVec& policy_obj_x, 
                                       PolicyVec& policy_obj_o, double probability, History history_obj, InformationSet& curr_I_1, char initial_player){
    
    if (curr_I_1.board == "000000000"){
        return 1.0;
    }
    else {
        return get_prob_h_given_policy(I_1, I_2, true_board, player, next_action, policy_obj_x, policy_obj_o, probability, history_obj, initial_player, curr_I_1);
    }
}


double get_counter_factual_utility(InformationSet& I, PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, std::vector<std::vector<int>>& starting_histories, std::vector<double>& prob_reaching_h_list, int action) {
    double counter_factual_utility = 0.0;
    int count = 0;
    for (std::vector<int> h : starting_histories) {
        NonTerminalHistory h_object(h);
        std::string hash_1 = "";
        std::string hash_2 = "";
        std::string board = "000000000";
        InformationSet curr_I_1('x', true, hash_1);
        InformationSet curr_I_2('o', false, hash_2);
        TicTacToeBoard true_board = TicTacToeBoard(board);
        h_object.get_information_sets(curr_I_1, curr_I_2);
        char curr_player = 'x';
        h_object.get_board(true_board, curr_player);

        double probability_reaching_h;
        double expected_utility_h;

        if (prob_reaching_h_list[count] > 0) {
            expected_utility_h = get_expected_utility_action_version(curr_I_1, curr_I_2, true_board, I.player, policy_obj_x, policy_obj_o, 1, h_object, I.player, action);

            if (!(curr_I_1.board == "000000000")){
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


void get_probability_of_reaching_all_h(InformationSet& I, PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, std::vector<std::vector<int>>& starting_histories, 
                                       char initial_player, std::vector<double>& prob_reaching_h_list_all) {
    for (std::vector<int> h : starting_histories) {
        NonTerminalHistory h_object(h);

        if (!(I.board == "000000000")) {
            std::string board = "000000000";
            std::string hash_1 = "";
            std::string hash_2 = "";
            InformationSet I_1('x', true, hash_1);
            InformationSet I_2('o', false, hash_2);
            TicTacToeBoard true_board = TicTacToeBoard(board);
            double probability_reaching_h = get_prob_h_given_policy_wrapper(I_1, I_2, true_board, 'x', h[0], policy_obj_x, policy_obj_o, 1.0, h_object, I, initial_player);
            prob_reaching_h_list_all.push_back(probability_reaching_h);
        }
        else {
            prob_reaching_h_list_all.push_back(1.0);
        }
    }
}


double calc_util_a_given_I_and_action(InformationSet& I, int action, PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, 
                                      std::vector<std::vector<int>>& starting_histories, std::vector<double>& prob_reaching_h_list) {
    
    double util_a = 0.0;
    util_a = get_counter_factual_utility(I, policy_obj_x, policy_obj_o, starting_histories, prob_reaching_h_list, action);
    return util_a;
}


void calc_cfr_policy_given_I(InformationSet& I, PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, int T, std::vector<double>& regret_list) {
    auto start = std::chrono::system_clock::now();
    
    double util = 0.0;
    std::vector<int> actions;
    PolicyVec& policy_obj = I.player == 'x' ? policy_obj_x : policy_obj_o;


    std::vector<std::vector<int>> starting_histories;
    std::vector<double> prob_reaching_h_list;
    std::vector<double> util_a_list;
    
    for (int i = 0; i < 13; i++) {
        util_a_list.push_back(0.0);
    }

    I.get_actions(actions);
    // std::cout << "Got actions for " << I.hash << ", index " << I.get_index() << std::endl;
    upgraded_get_histories_given_I(I, policy_obj_x, policy_obj_o, starting_histories);
    // std::cout << "Got " << starting_histories.size() << " valid histories for " << I.hash << ", index " << I.get_index() << std::endl;
    get_probability_of_reaching_all_h(I, policy_obj_x, policy_obj_o, starting_histories, I.player, prob_reaching_h_list);
    // std::cout << "Got probs " << I.hash << ", index " << I.get_index() << std::endl;

    for (int action : actions) {
        double util_a = 0.0;
        if (I.player == 'x'){
            util_a = calc_util_a_given_I_and_action(I, action, policy_obj_x, policy_obj_o, starting_histories, prob_reaching_h_list);
        }
        else {
            util_a = calc_util_a_given_I_and_action(I, action, policy_obj_x, policy_obj_o, starting_histories, prob_reaching_h_list);
        }
        util += util_a * policy_obj.policy_dict[I.get_index()][action];
        util_a_list[action] = util_a;
    }

    // std::cout << "Got utils for " << I.hash << ", index " << I.get_index() << std::endl;

    for (int action : actions) {
        double regret_T = 0.0;
        if (T == 0) {
            regret_T = util_a_list[action] - util;
        } else {
            regret_T = regret_list[action] + util_a_list[action] - util;
        }

        regret_T = regret_T > 0.0 ? regret_T : 0.0;
        regret_list[action] = regret_T;
    }

    // std::cout << "Got regrets for " << I.hash << ", index " << I.get_index() << std::endl;
    // std::cout << "Regret for information set " << I.hash << " is " << std::endl;
    // for (int i = 0; i < regret_list.size(); i++) {
    //     std::cout << "Action: " << i << " Regret: " << regret_list[i] << " " << std::endl;
    // }
}


std::vector<std::vector<double> > get_prev_regrets(std::string& file_path, char player){
    std::ifstream i(file_path);
    json regret_obj;
    i >> regret_obj;
    std::vector<std::vector<double> > regret_map;
        
    for (json::iterator it = regret_obj.begin(); it != regret_obj.end(); ++it) {
        std::string I_hash = it.key();
        bool move_flag;
        if (I_hash.size() != 0){
            move_flag = I_hash[I_hash.size()-1] == '|' ? true : false;
        }
        else {
            move_flag = player == 'x' ? true : false;
        }

        InformationSet I(player, move_flag, I_hash);

        std::vector <double> probability_distribution(13);
        // initialise all values to zero
        for (int i = 0; i < 13; i++) {
            probability_distribution[i] = 0.0;
        }

        if (I_hash.back() == '_') {
            std::vector<std::string> sense_keys = {"9", "10", "11", "12"};
            for (int i = 0; i < sense_keys.size(); i++) {
                probability_distribution[stoi(sense_keys[i])] = regret_obj[I_hash][sense_keys[i]];
            }
        }
        else if (I_hash.back() == '|') {
            std::vector<std::string> move_keys = {"0", "1", "2", "3", "4", "5", "6", "7", "8"};
            for (int i = 0; i < move_keys.size(); i++) {
                probability_distribution[stoi(move_keys[i])] = regret_obj[I_hash][move_keys[i]];
            }
        }
        else {
            if (player == 'x'){
                std::vector<std::string> move_keys = {"0", "1", "2", "3", "4", "5", "6", "7", "8"};
                for (int i = 0; i < move_keys.size(); i++) {
                    probability_distribution[stoi(move_keys[i])] = regret_obj[I_hash][move_keys[i]];
                }
            }
            else{
                std::vector<std::string> sense_keys = {"9", "10", "11", "12"};
                for (int i = 0; i < sense_keys.size(); i++) {
                    probability_distribution[stoi(sense_keys[i])] = regret_obj[I_hash][sense_keys[i]];
                }

            }
        }
        regret_map[I.get_index()] = probability_distribution;
    }

    return regret_map;
}
