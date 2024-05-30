// Compile: g++ -O3 calc_prob_iterative.cpp rbt_classes.cpp -o prob -fopenmp -I C:\Users\Ramsundar\Desktop\boost_1_82_0\boost_1_82_0                  

#include "cpp_headers/rbt_classes.hpp"
#include "cpp_headers/json.hpp"
using json = nlohmann::json;

void valid_histories_play(InformationSet& I_1, InformationSet& I_2, 
                          TicTacToeBoard& true_board, char player, 
                          History& current_history, InformationSet& end_I, 
                          std::vector<int>& played_actions, Policy& policy_obj_x, 
                          Policy& policy_obj_o, std::vector<std::vector<int>>& valid_histories_list){
    
    InformationSet& I = player == 'x' ? I_1 : I_2;
    std::vector<int> actions;

    if (player == 'x') {
        if (end_I.player == 'x'){
            I.get_actions_given_policy(actions, policy_obj_x);
            if (I.move_flag) {
                actions = intersection(actions, played_actions);
            }
        }
        else {
            I.get_actions_given_policy(actions, policy_obj_x);
        }
        
    } else {
        if (end_I.player == 'o'){
            I.get_actions_given_policy(actions, policy_obj_o);
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


void upgraded_get_histories_given_I(InformationSet& I, Policy& policy_obj_x, 
                                    Policy& policy_obj_o, std::vector<std::vector<int>>& valid_histories_list){
    
    if (I.get_hash() == "000000000m"){
        std::vector<int> init_h = {};
        valid_histories_list.push_back(init_h);
        return;
    }

    std::string board_1 = "000000000";
    std::string board_2 = "---------";
    InformationSet I_1('x', true, board_1);
    InformationSet I_2('o', false, board_2);
    TicTacToeBoard true_board = TicTacToeBoard(EMPTY_BOARD);
    char player = 'x';
    std::vector<int> played_actions;
    I.get_played_actions(played_actions);

    std::vector<int> h = {};
    History current_history(h);
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
                                       Policy& policy_obj_o, std::vector<std::vector<int>>& starting_histories,
                                       char initial_player, std::vector<double>& prob_reaching_h_list_all) {

    for (std::vector<int> history: starting_histories) {
        NonTerminalHistory h_object = NonTerminalHistory(history);
        double prob_reaching_h;

        if (I.get_hash() == "000000000m") {
            prob_reaching_h = 1.0;
        }
        else {
            std::string board_1 = "000000000";
            std::string board_2 = "---------";
            std::string board = "000000000";

            InformationSet I_1('x', true, board_1);
            InformationSet I_2('o', false, board_2);
            TicTacToeBoard true_board = TicTacToeBoard(board);
            
            prob_reaching_h = get_prob_h_given_policy_wrapper(I_1, I_2, true_board, 'x', history[0], policy_obj_x, policy_obj_o, 1.0, h_object, I, initial_player);
        }

        prob_reaching_h_list_all.push_back(prob_reaching_h);
    }

    return;
}


void get_probability_of_reaching_I(InformationSet& I, Policy& policy_obj_x, Policy& policy_obj_o, char initial_player, double& prob_reaching) {
    std::vector<std::vector<int>> starting_histories;
    std::vector<double> prob_reaching_h_list_all;

    upgraded_get_histories_given_I(I, policy_obj_x, policy_obj_o, starting_histories);
    get_probability_of_reaching_all_h(I, policy_obj_x, policy_obj_o, starting_histories, initial_player, prob_reaching_h_list_all);

    for (double prob_reaching_h: prob_reaching_h_list_all) {
        prob_reaching += prob_reaching_h;
    }
}


int main(int argc, char *argv[]) {
    int number_threads = std::stoi(argv[1]); //96;
    std::string base_path = argv[2]; //"data/Iterative_1";
    int start_iter = std::stoi(argv[3]); //1;
    int end_iter = std::stoi(argv[4]); //1000;
    std::string current_player = argv[5]; //"x";

    for (int T = start_iter; T <= end_iter; T++) {
         std::vector<double> prob_reaching_list;

        std::string P1_information_sets_file = "data/P1_information_sets.txt";
        std::string P2_information_sets_file = "data/P2_information_sets.txt";
        std::vector<std::string> P_information_sets;
        if (current_player == "x") {
            std::ifstream f1(P1_information_sets_file);
            std::string line;
            while (std::getline(f1, line)) {
            P_information_sets.push_back(line);
            prob_reaching_list.push_back(0.0);
        }
        f1.close();
        }
        else {
            std::ifstream f1(P2_information_sets_file);
            std::string line;
            while (std::getline(f1, line)) {
            P_information_sets.push_back(line);
            prob_reaching_list.push_back(0.0);
        }
        f1.close();
        }

        std::ofstream f_out_policy;
        std::string policy_file_x;
        std::string policy_file_o;
        if (T == 1) {
            policy_file_x = base_path + "/cfr/" + "/P1_iteration_" + std::to_string(T) + "_cfr_policy_cpp.json";
            policy_file_o = "./data/Iterative_1/average/P2_average_overall_policy_after_100_rounds_normalised.json";
        }
        else {
            if (T % 2 == 1) {
            policy_file_x = base_path + "/cfr/" + "/P1_iteration_" + std::to_string(T/2 + 1) + "_cfr_policy_cpp.json";
            policy_file_o = base_path + "/cfr/" + "/P2_iteration_" + std::to_string(T/2) + "_cfr_policy_cpp.json";
            }
            else {
            policy_file_x = base_path + "/cfr/" + "/P1_iteration_" + std::to_string(T/2) + "_cfr_policy_cpp.json";
            policy_file_o = base_path + "/cfr/" + "/P2_iteration_" + std::to_string(T/2) + "_cfr_policy_cpp.json";
            }
        }

        Policy policy_obj_x('x', policy_file_x);
        Policy policy_obj_o('o', policy_file_o);
        std::cout << "######################## Starting iteration ####################### " << T << std::endl;
        auto start = std::chrono::system_clock::now();

        std::cout << "Computing probabilities and avg policy numerator terms for iteration " << T << std::endl;
        #pragma omp parallel for num_threads(number_threads)
        for (int i = 0; i < P_information_sets.size(); i++) {
            std::string I_hash = P_information_sets[i];
            bool move_flag = I_hash[I_hash.size()-1] == 'm' ? true : false;
            I_hash.pop_back();
            InformationSet I(current_player[0], move_flag, I_hash);

            get_probability_of_reaching_I(I, policy_obj_x, policy_obj_o, current_player[0], prob_reaching_list[i]);
        }

        std::unordered_map<std::string, double> prob_reaching_map;
        std::unordered_map<std::string, std::vector<double> > avg_policy_numerator;

        for (int i = 0; i < P_information_sets.size(); i++) {
            std::string I_hash = P_information_sets[i];
            prob_reaching_map[I_hash] = prob_reaching_list[i];

            std::vector<double> prob_vector;
            for (int j = 0; j < 13; j++) {
                prob_vector.push_back(0.0);
            }
            for (int j = 0; j < 13; j++) {
                if (current_player == "x") {
                    prob_vector[j] = prob_reaching_map[I_hash] * policy_obj_x.policy_dict[I_hash][j];
                }
                else {
                    prob_vector[j] = prob_reaching_map[I_hash] * policy_obj_o.policy_dict[I_hash][j];
                }
            }
            avg_policy_numerator[I_hash] = prob_vector;
        }

        std::cout << "Saving probabilties..." << std::endl;
        if (current_player == "x") {
            std::string next_prob_reaching_file = base_path + "/prob_reaching" + "/P1_iteration_" + std::to_string(T) + "_prob_reaching_cpp.json";
            f_out_policy.open(next_prob_reaching_file, std::ios::trunc);
        }
        else {
            std::string next_prob_reaching_file = base_path + "/prob_reaching" + "/P2_iteration_" + std::to_string(T) + "_prob_reaching_cpp.json";
            f_out_policy.open(next_prob_reaching_file, std::ios::trunc);
        }

        json jso;
        for (auto& it: prob_reaching_map) {
                jso[it.first] = it.second;
        }
        f_out_policy << jso.dump() << std::endl;
        f_out_policy.close();

        std::cout << "Saving avg policy numerator terms for iteration..." << T << std::endl;
        if (current_player == "x") {
            std::string avg_pol_numerator_file = base_path + "/average" + "/P1_iteration_" + std::to_string(T) + "_avg_pol_numerator_term_cpp.json";
            f_out_policy.open(avg_pol_numerator_file, std::ios::trunc);
        }
        else {
            std::string avg_pol_numerator_file = base_path + "/average" + "/P2_iteration_" + std::to_string(T) + "_avg_pol_numerator_term_cpp.json";
            f_out_policy.open(avg_pol_numerator_file, std::ios::trunc);
        }

        json jsoavg;
        for (auto& it: avg_policy_numerator) {
            for (int i = 0; i < 13; i++) {
                jsoavg[it.first][std::to_string(i)] = it.second[i];
            }
        }
        f_out_policy << jsoavg.dump() << std::endl;
        f_out_policy.close();

        auto end = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = end - start;
        std::time_t end_time = std::chrono::system_clock::to_time_t(end);
    
        std::cout << "######################## finished computation at ######################## " << std::ctime(&end_time)
                << "elapsed time: " << elapsed_seconds.count() << "s"
                << std::endl;
    }

    return 0;
}