#include "cpp_headers/rbt_classes.hpp"

// g++-13 -O3 rbt_classes.cpp test_get_h_given_I.cpp -o test_get_h_given_I

void valid_histories_play(InformationSet& I_1, InformationSet& I_2, TicTacToeBoard& true_board, char player, History& current_history, InformationSet& end_I, std::vector<std::vector<bool>>& allowed_move_masks,
                          std::vector<int>& played_actions, int current_action_index, int other_player_turn_index, PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, std::vector<std::vector<int>>& valid_histories_list){
    InformationSet& I = player == 'x' ? I_1 : I_2;
    std::vector<int> actions;
    std::cout << "Information set: " << I.hash << " ";

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

    std::cout << "Actions: ";
    for (int action : actions){
        std::cout << action << " ";
    }
    std::cout << std::endl;

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
                // std::cout << "New Information set after M, player " << player << ": " << new_I.hash << std::endl;   

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
            // std::cout << "New Information set after S, player " << player << ": " << new_I.hash << std::endl;
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

    for (int i = 0; i < allowed_move_masks.size(); i++) {
        for (int j = 0; j < 9; j++) {
            std::cout << allowed_move_masks[i][j] << " ";
        }
        std::cout << std::endl;
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

int main(){
    // read information set file

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

    std::string policy_file_x = "data/P1_uniform_policy_v2.json";
    std::string policy_file_o = "data/P2_uniform_policy_v2.json";
    std::cout << "Loading policies..." << std::endl;
    PolicyVec policy_obj_x('x', policy_file_x);
    PolicyVec policy_obj_o('o', policy_file_o);
    std::cout << "Policies loaded." << std::endl;

    std::string hash1 = "5_3|0x00|1_3|0x00|7_0|ox0o|6_3|oxx0|";
    InformationSet I1('x', true, hash1);

    std::string hash2 = "1|000x|0_1|x00x|2_2|000x|4_2|0oxx|";
    InformationSet I2('o', true, hash2);

    std::vector<float>& prob_d = policy_obj_o.policy_dict[I2.get_index()];
    std::cout << "Policy for I2: ";
    for (int i = 0; i < prob_d.size(); i++) {
        std::cout << prob_d[i] << " ";
    }
    std::cout << std::endl;
    // std::cout << "Index assigned to I1: " << I1.get_index() << std::endl;

    // std::string hash2 = "3_0|o0x0|5_0|o0xo|1_3|oxo0|8_2|xo0o|";
    // InformationSet I2('x', true, hash2);
    // // std::cout << "Index assigned to I2: " << I2.get_index() << std::endl;

    // std::string hash3 = "1_3|0o00|8_0|0x00|3_2|x0o0|7_";
    // InformationSet I3('x', false, hash3);
    // // std::cout << "Index assigned to I3: " << I3.get_index() << std::endl;

    // std::string hash4 = "1|0000|6_2|00o0|1_1|ox00|5_2|x0ox|8_";
    // InformationSet I4('o', false, hash4);
    // // std::cout << "Index assigned to I4: " << I4.get_index() << std::endl;

    // std::string hash5 = "2|0000|6_3|x00x|0_1|00x0|5_0|o00x|";
    // InformationSet I5('o', true, hash5);
    // // std::cout << "Index assigned to I5: " << I5.get_index() << std::endl;

    std::vector<std::vector<int>> valid_histories_list_1;
    auto start = std::chrono::system_clock::now();
    upgraded_get_histories_given_I(I1, policy_obj_x, policy_obj_o, valid_histories_list_1);
    std::cout << "Number of valid histories for I1: " << valid_histories_list_1.size() << std::endl;
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);
    std::cout << "finished computation at " << std::ctime(&end_time)
            << "elapsed time: " << elapsed_seconds.count() << "s"
            << std::endl;

    // std::vector<std::vector<int>> valid_histories_list_2;
    // start = std::chrono::system_clock::now();
    // upgraded_get_histories_given_I(I2, policy_obj_x, policy_obj_o, valid_histories_list_2);
    // std::cout << "Number of valid histories for I2: " << valid_histories_list_2.size() << std::endl;
    // end = std::chrono::system_clock::now();
    // elapsed_seconds = end - start;
    // end_time = std::chrono::system_clock::to_time_t(end);
    // std::cout << "finished computation at " << std::ctime(&end_time)
    //         << "elapsed time: " << elapsed_seconds.count() << "s"
    //         << std::endl;

    // std::vector<std::vector<int>> valid_histories_list_3;
    // start = std::chrono::system_clock::now();
    // upgraded_get_histories_given_I(I3, policy_obj_x, policy_obj_o, valid_histories_list_3);
    // std::cout << "Number of valid histories for I3: " << valid_histories_list_3.size() << std::endl;
    // end = std::chrono::system_clock::now();
    // elapsed_seconds = end - start;
    // end_time = std::chrono::system_clock::to_time_t(end);
    // std::cout << "finished computation at " << std::ctime(&end_time)
    //         << "elapsed time: " << elapsed_seconds.count() << "s"
    //         << std::endl;

    // std::vector<std::vector<int>> valid_histories_list_4;
    // start = std::chrono::system_clock::now();
    // upgraded_get_histories_given_I(I4, policy_obj_x, policy_obj_o, valid_histories_list_4);
    // std::cout << "Number of valid histories for I4: " << valid_histories_list_4.size() << std::endl;
    // end = std::chrono::system_clock::now();
    // elapsed_seconds = end - start;
    // end_time = std::chrono::system_clock::to_time_t(end);
    // std::cout << "finished computation at " << std::ctime(&end_time)
    //         << "elapsed time: " << elapsed_seconds.count() << "s"
    //         << std::endl;

    // std::vector<std::vector<int>> valid_histories_list_5;
    // start = std::chrono::system_clock::now();
    // upgraded_get_histories_given_I(I5, policy_obj_x, policy_obj_o, valid_histories_list_5);
    // std::cout << "Number of valid histories for I5: " << valid_histories_list_5.size() << std::endl;
    // end = std::chrono::system_clock::now();
    // elapsed_seconds = end - start;
    // end_time = std::chrono::system_clock::to_time_t(end);
    // std::cout << "finished computation at " << std::ctime(&end_time)
    //         << "elapsed time: " << elapsed_seconds.count() << "s"
    //         << std::endl;
}