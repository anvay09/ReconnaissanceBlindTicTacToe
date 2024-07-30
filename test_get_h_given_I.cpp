#include "cpp_headers/rbt_classes.hpp"

// g++-13 -O3 rbt_classes.cpp test_get_h_given_I.cpp -o test_get_h_given_I

void valid_histories_play(InformationSet& I_1, InformationSet& I_2, TicTacToeBoard& true_board, char player, History& current_history, InformationSet& end_I, 
                          std::vector<int>& played_actions, int current_action_index, Policy& policy_obj_x, Policy& policy_obj_o, std::vector<std::vector<int>>& valid_histories_list){
    InformationSet& I = player == 'x' ? I_1 : I_2;
    std::vector<int> actions;

    if (player == 'x') {
        if (end_I.player == 'x'){
            actions.push_back(played_actions[current_action_index++]);
        }
        else {
            I.get_actions_given_policy(actions, policy_obj_x);
        }
        
    } else {
        if (end_I.player == 'o'){
            actions.push_back(played_actions[current_action_index++]);
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
                InformationSet new_I = I;
                new_I.update_move(action, player);
                new_I.reset_zeros();

                if (player == 'x') {
                    if (end_I.player == 'x') {
                        valid_histories_play(new_I, I_2, new_true_board, 'o', new_history, end_I, played_actions, current_action_index, policy_obj_x, policy_obj_o, valid_histories_list);
                    }
                    else {
                        if (I_2 == end_I){
                            valid_histories_list.push_back(new_history.history);
                        }
                        else {
                            valid_histories_play(new_I, I_2, new_true_board, 'o', new_history, end_I, played_actions, current_action_index, policy_obj_x, policy_obj_o, valid_histories_list);
                        }
                    }
                }
                else {
                    if (end_I.player == 'o') {
                        valid_histories_play(I_1, new_I, new_true_board, 'x', new_history, end_I, played_actions, current_action_index, policy_obj_x, policy_obj_o, valid_histories_list);
                    }
                    else {
                        if (I_1 == end_I){
                            valid_histories_list.push_back(new_history.history);
                        }
                        else {
                            valid_histories_play(I_1, new_I, new_true_board, 'x', new_history, end_I, played_actions, current_action_index, policy_obj_x, policy_obj_o, valid_histories_list);
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
                        valid_histories_play(new_I, I_2, new_true_board, 'x', new_history, end_I, played_actions, current_action_index, policy_obj_x, policy_obj_o, valid_histories_list);
                    }
                    else {
                        valid_histories_list.push_back(new_history.history);
                    }
                }
                else {
                    valid_histories_play(new_I, I_2, new_true_board, 'x', new_history, end_I, played_actions, current_action_index, policy_obj_x, policy_obj_o, valid_histories_list);
                }
            }
            else {
                if (end_I.player == 'o') {
                    if (!(new_I == end_I)){
                        valid_histories_play(I_1, new_I, new_true_board, 'o', new_history, end_I, played_actions, current_action_index, policy_obj_x, policy_obj_o, valid_histories_list);
                    }
                    else {
                        valid_histories_list.push_back(new_history.history);
                    }
                }
                else {
                    valid_histories_play(I_1, new_I, new_true_board, 'o', new_history, end_I, played_actions, current_action_index, policy_obj_x, policy_obj_o, valid_histories_list);
                }
            }
        }
    }
}


void get_forbidden_move_masks_for_other_player(InformationSet& I, std::vector<std::vector<bool>>& forbidden_move_masks){
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

    for (int i = other_player_sense_moves.size() - 1; i >= 0; i--) {
        std::vector<bool> mask(9, true);
        std::vector<int> sense_index_list = sense_square_dict[other_player_sense_moves[i]];
        for (int s = 0; s < 4; s++) {
            if (observation_list[i][s] == 'x' || observation_list[i][s] == '0') {
                mask[sense_index_list[s]] = false;
            }
        }
        forbidden_move_masks.insert(forbidden_move_masks.begin(), mask);
    }

    for (int i = 0; i < forbidden_move_masks.size(); i++) {
        for (int j = 0; j < 9; j++) {
            std::cout << forbidden_move_masks[i][j] << " ";
        }
        std::cout << std::endl;
    }
}


void upgraded_get_histories_given_I(InformationSet& I, Policy& policy_obj_x, Policy& policy_obj_o, std::vector<std::vector<int>>& valid_histories_list){
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
    std::vector<std::vector<bool>> forbidden_move_masks;
    get_forbidden_move_masks_for_other_player(I, forbidden_move_masks);
    int current_action_index = 0;

    std::vector<int> h = {};
    NonTerminalHistory current_history(h);
    valid_histories_play(I_1, I_2, true_board, player, current_history, I, played_actions, current_action_index, policy_obj_x, policy_obj_o, valid_histories_list);
    return;
}   



int main(){
    std::string policy_file_x = "data/P1_uniform_policy_v2.json";
    std::string policy_file_o = "data/P2_uniform_policy_v2.json";
    std::cout << "Loading policies..." << std::endl;
    Policy policy_obj_x('x', policy_file_x);
    Policy policy_obj_o('o', policy_file_o);

    std::string hash = "4_3|x0o0|6_0|000x|1_0|ox0x|8_1|x0xo|";
    InformationSet I('x', true, hash);

    std::vector<std::vector<int>> valid_histories_list;
    std::cout << "Getting valid histories..." << std::endl;
    upgraded_get_histories_given_I(I, policy_obj_x, policy_obj_o, valid_histories_list);

    std::cout << "Number of valid histories: " << valid_histories_list.size() << std::endl;
}