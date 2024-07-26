// Compile: g++-13 -O3 cfr_iterative.cpp rbt_classes.cpp rbt_utilities.cpp -o cfr_i -fopenmp -I /Users/anvay/Downloads/boost_1_84_0

#include "cpp_headers/rbt_classes.hpp"
#include "cpp_headers/rbt_utilities.hpp"

int number_threads = 4;

bool get_move_flag(std::string I_hash, char player){
    bool move_flag;
    if (I_hash.size() != 0){
        move_flag = I_hash[I_hash.size()-1] == '|' ? true : false;
    }
    else {
        if (player == 'x'){
            move_flag = true;
        }
        else {
            move_flag = false;
        }
    }
    return move_flag;
}

//cfr
void run_cfr(int T, std::vector<std::string>& information_sets, std::vector<std::vector<double>>& regret_list, Policy& policy_obj_x, Policy& policy_obj_o, char player){
        std::cout << "Starting iteration " << T << " for player " << player << "..." << std::endl;
        auto start = std::chrono::system_clock::now();

        #pragma omp parallel for num_threads(number_threads) shared(regret_list)
        for (int i = 0; i < information_sets.size(); i++) {
            std::string I_hash = information_sets[i];
            bool move_flag = get_move_flag(I_hash, player);
            InformationSet I(player, move_flag, I_hash);

            calc_cfr_policy_given_I(I, policy_obj_x, policy_obj_o, T, regret_list[i]);
        }

        auto end = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = end - start;
        std::time_t end_time = std::chrono::system_clock::to_time_t(end);
        std::cout << "finished computation at " << std::ctime(&end_time)
                << "elapsed time: " << elapsed_seconds.count() << "s"
                << std::endl;


        std::cout << "Updating policy for player " << player << "..." << std::endl;
        #pragma omp parallel for num_threads(number_threads)
        for (int i = 0; i < information_sets.size(); i++) {
            std::string I_hash = information_sets[i];
            bool move_flag = get_move_flag(I_hash, player);
            InformationSet I(player, move_flag, I_hash);
            std::vector<double>& regret_vector = regret_list[i];
            double total_regret = 0.0;
            std::vector<int> actions;
            I.get_actions(actions);

            for (int action : actions) {
                total_regret += regret_vector[action];
            }

            Policy& policy_obj = player == 'x' ? policy_obj_x : policy_obj_o;
            std::vector<double>& prob_dist = policy_obj.policy_dict[I_hash];
            if (total_regret > 0) {
                for (int action : actions) {
                    prob_dist[action] = regret_vector[action] / total_regret;
                }
            }
            else {
                for (int action : actions) {
                    prob_dist[action] = 1.0 / double(actions.size());
                }
            }
        }


}

void initialize_start(std::string policy_file, std::string information_set_file, std::vector<std::string>& information_sets, std::vector<std::vector<double>>& regret_list,  std::vector<double>& prob_reaching_list, Policy& policy_obj, Policy& avg_policy_obj, std::unordered_map<std::string, std::vector<double>>& avg_policy_numerator, std::unordered_map<std::string, double>& avg_policy_denominator, char player) {
    policy_obj = Policy(player, policy_file);
    avg_policy_obj = policy_obj;
    avg_policy_numerator = policy_obj.policy_dict;
    
    std::ifstream f_is(information_set_file);
    std::string line_is;
        
    while (std::getline(f_is, line_is)) {
        avg_policy_denominator[line_is] = 0.0;
        information_sets.push_back(line_is);
        std::vector<double> regret_vector;
        for (int i = 0; i < 13; i++) {
            regret_vector.push_back(0.0);
        }
        regret_list.push_back(regret_vector);
        prob_reaching_list.push_back(0.0);
    }
    f_is.close();
}

void initialize_continue(std::string policy_file, std::string information_set_file, std::vector<std::string>& information_sets, std::vector<std::vector<double>>& regret_list, std::unordered_map<std::string, std::vector<double>>& regret_map, std::vector<double>& prob_reaching_list, Policy& policy_obj, Policy& avg_policy_obj, std::unordered_map<std::string, std::vector<double>>& avg_policy_numerator, std::unordered_map<std::string, double>& avg_policy_denominator, char player) {
    policy_obj = Policy(player, policy_file);
    avg_policy_obj = policy_obj;
    avg_policy_numerator = policy_obj.policy_dict;
    
    std::ifstream f_is(information_set_file);
    std::string line_is;
        
    while (std::getline(f_is, line_is)) {
        avg_policy_denominator[line_is] = 0.0;
        information_sets.push_back(line_is);
        std::vector<double> regret_vector;
        for (int i = 0; i < 13; i++) {
            regret_vector.push_back(regret_map[line_is][i]); // maybe can be merged with initialize_start
        }
        regret_list.push_back(regret_vector);
        prob_reaching_list.push_back(0.0);
    }
    f_is.close();
}

void save_map_json(std::string output_file, std::unordered_map<std::string, std::vector<double>>& map){
    std::ofstream f_out;
    f_out.open(output_file, std::ios::trunc);
    json jx;
    for (auto& it: map) {
        for (int i = 0; i < 13; i++) {
            jx[it.first][std::to_string(i)] = it.second[i];
        }
    }
    f_out << jx.dump() << std::endl;
    f_out.close();
}


void save_output(std::string output_policy_file, std::string output_regret_file, char player, std::vector<std::string>& information_sets, std::vector<std::vector<double>>& regret_list, Policy& policy_obj) {
    std::unordered_map<std::string, std::vector<double> > regret_map;
    std::cout << "Saving regrets for player " << player << "..." << std::endl;
    for (int i = 0; i < information_sets.size(); i++) {
        std::string I_hash = information_sets[i];
        std::vector<double>& regret_vector = regret_list[i];
        regret_map[I_hash] = regret_vector;
    }
    save_map_json(output_regret_file, regret_map);

    std::cout << "Saving policy for player " << player << "..." << std::endl;
    save_map_json(output_policy_file, policy_obj.policy_dict);
}

//cfr

// prob
void valid_histories_play_prob(InformationSet& I_1, InformationSet& I_2, 
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
            I.get_actions(actions);
        }
        
    } else {
        if (end_I.player == 'o'){
            I.get_actions_given_policy(actions, policy_obj_o);
            if (I.move_flag) {
                actions = intersection(actions, played_actions);
            }
        }
        else {
            I.get_actions(actions);
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
                        valid_histories_play_prob(new_I, I_2, new_true_board, 'o', new_history, end_I, played_actions, policy_obj_x, policy_obj_o, valid_histories_list);
                    }
                    else {
                        if (I_2 == end_I){
                            valid_histories_list.push_back(new_history.history);
                        }
                        else {
                            valid_histories_play_prob(new_I, I_2, new_true_board, 'o', new_history, end_I, played_actions, policy_obj_x, policy_obj_o, valid_histories_list);
                        }
                    }
                }
                else {
                    if (end_I.player == 'o') {
                        valid_histories_play_prob(I_1, new_I, new_true_board, 'x', new_history, end_I, played_actions, policy_obj_x, policy_obj_o, valid_histories_list);
                    }
                    else {
                        if (I_1 == end_I){
                            valid_histories_list.push_back(new_history.history);
                        }
                        else {
                            valid_histories_play_prob(I_1, new_I, new_true_board, 'x', new_history, end_I, played_actions, policy_obj_x, policy_obj_o, valid_histories_list);
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
                        valid_histories_play_prob(new_I, I_2, new_true_board, 'x', new_history, end_I, played_actions, policy_obj_x, policy_obj_o, valid_histories_list);
                    }
                    else {
                        valid_histories_list.push_back(new_history.history);
                    }
                }
                else {
                    valid_histories_play_prob(new_I, I_2, new_true_board, 'x', new_history, end_I, played_actions, policy_obj_x, policy_obj_o, valid_histories_list);
                }
            }
            else {
                if (end_I.player == 'o') {
                    if (!(new_I == end_I)){
                        valid_histories_play_prob(I_1, new_I, new_true_board, 'o', new_history, end_I, played_actions, policy_obj_x, policy_obj_o, valid_histories_list);
                    }
                    else {
                        valid_histories_list.push_back(new_history.history);
                    }
                }
                else {
                    valid_histories_play_prob(I_1, new_I, new_true_board, 'o', new_history, end_I, played_actions, policy_obj_x, policy_obj_o, valid_histories_list);
                }
            }
        }
    }
}


void upgraded_get_histories_given_I_prob(InformationSet& I, Policy& policy_obj_x, 
                                    Policy& policy_obj_o, std::vector<std::vector<int>>& valid_histories_list){
    
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

    std::vector<int> h = {};
    NonTerminalHistory current_history(h);
    valid_histories_play_prob(I_1, I_2, true_board, player, current_history, I, played_actions, policy_obj_x, policy_obj_o, valid_histories_list);
    return;
}   

double get_prob_h_given_policy_prob(InformationSet& I_1, InformationSet& I_2, 
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
                    probability = get_prob_h_given_policy_prob(new_I, I_2, new_true_board, 'o', new_next_action, policy_obj_x, policy_obj_o, probability, history_obj, initial_player);
                }
                else {
                    probability = get_prob_h_given_policy_prob(I_1, new_I, new_true_board, 'x', new_next_action, policy_obj_x, policy_obj_o, probability, history_obj, initial_player);
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
                probability = get_prob_h_given_policy_prob(new_I, I_2, new_true_board, 'x', new_next_action, policy_obj_x, policy_obj_o, probability, history_obj, initial_player);
            }
            else {
                probability = get_prob_h_given_policy_prob(I_1, new_I, new_true_board, 'o', new_next_action, policy_obj_x, policy_obj_o, probability, history_obj, initial_player);
            }
        }
    }

    return probability;
}


double get_prob_h_given_policy_wrapper_prob(InformationSet& I_1, InformationSet& I_2, 
                                       TicTacToeBoard& true_board, char player, 
                                       int next_action, Policy& policy_obj_x, 
                                       Policy& policy_obj_o, double probability,
                                       History history_obj, InformationSet& curr_I_1, char initial_player){
    
    if (curr_I_1.board == "000000000"){
        return 1.0;
    }
    else {
        return get_prob_h_given_policy_prob(I_1, I_2, true_board, player, next_action, policy_obj_x, policy_obj_o, probability, history_obj, initial_player);
    }
}


void get_probability_of_reaching_all_h_prob(InformationSet &I, Policy& policy_obj_x,
                                       Policy& policy_obj_o, std::vector<std::vector<int>>& starting_histories,
                                       char initial_player, std::vector<double>& prob_reaching_h_list_all) {

    for (std::vector<int> history: starting_histories) {
        NonTerminalHistory h_object = NonTerminalHistory(history);
        double prob_reaching_h;

        if (I.board == "000000000") {
            prob_reaching_h = 1.0;
        }
        else {
            std::string board = "000000000";
            std::string hash_1 = "";
            std::string hash_2 = "";
            InformationSet I_1('x', true, hash_1);
            InformationSet I_2('o', false, hash_2);
            TicTacToeBoard true_board = TicTacToeBoard(board);
            
            prob_reaching_h = get_prob_h_given_policy_wrapper_prob(I_1, I_2, true_board, 'x', history[0], policy_obj_x, policy_obj_o, 1.0, h_object, I, initial_player);
        }

        prob_reaching_h_list_all.push_back(prob_reaching_h);
    }

    return;
}


double get_probability_of_reaching_I_prob(InformationSet& I, Policy& policy_obj_x, Policy& policy_obj_o, char initial_player) {
    std::vector<std::vector<int>> starting_histories;
    std::vector<double> prob_reaching_h_list_all;
    double prob_reaching = 0.0;

    upgraded_get_histories_given_I_prob(I, policy_obj_x, policy_obj_o, starting_histories);
    get_probability_of_reaching_all_h_prob(I, policy_obj_x, policy_obj_o, starting_histories, initial_player, prob_reaching_h_list_all);

    for (double prob_reaching_h: prob_reaching_h_list_all) {
        prob_reaching += prob_reaching_h;
    }

    return prob_reaching;
}

void get_prob_reaching(std::vector<std::string>& information_sets, std::vector<double>& prob_reaching_list, char player,  Policy& policy_obj_x, Policy& policy_obj_o){
    #pragma omp parallel for num_threads(number_threads)
    for (int i = 0; i < information_sets.size(); i++) {
        prob_reaching_list[i] = 0.0;
        std::string I_hash = information_sets[i];
        bool move_flag = get_move_flag(I_hash, player);
        InformationSet I(player, move_flag, I_hash);

        prob_reaching_list[i] = get_probability_of_reaching_I_prob(I, policy_obj_x, policy_obj_o, player);
    }
}
//prob

//avg
void calc_average_terms(char player, std::vector<std::string>& information_sets, Policy& policy_obj, std::vector<double>& prob_reaching_list, std::unordered_map<std::string, std::vector<double>>& avg_policy_numerator, std::unordered_map<std::string, double>& avg_policy_denominator){
    #pragma omp parallel for num_threads(number_threads)
    for (int i = 0; i < information_sets.size(); i++) {
        std::string I_hash = information_sets[i];
        bool move_flag = get_move_flag(I_hash, player);
        InformationSet I(player, move_flag, I_hash);

        std::vector<int> actions;
        I.get_actions(actions);
        for (int action: actions) {
            std::vector<double>& policy = policy_obj.policy_dict[I_hash];
            avg_policy_numerator[I_hash][action] += prob_reaching_list[i] * policy[action];
            avg_policy_denominator[I_hash] += prob_reaching_list[i] * policy[action];
        }
    }
}

void calc_average_policy(std::vector<std::string>& information_sets, Policy& avg_policy_obj, std::unordered_map<std::string, std::vector<double>> avg_policy_numerator, std::unordered_map<std::string, double> avg_policy_denominator, char player){
    #pragma omp parallel for num_threads(number_threads)
    for (int i = 0; i < information_sets.size(); i++) {
        std::string I_hash = information_sets[i];
        bool move_flag = get_move_flag(I_hash, player);
        InformationSet I(player, move_flag, I_hash);

        std::vector<int> actions;
        I.get_actions(actions);
        for (int action: actions) {
            std::vector<double>& policy = avg_policy_obj.policy_dict[I_hash];
            policy[action] = avg_policy_denominator[I_hash] > 0 ? avg_policy_numerator[I_hash][action] / avg_policy_denominator[I_hash] : 0;
        }
    }
}
//avg


int main(int argc, char* argv[])  {
    std::cout.precision(17);
    number_threads = std::stoi(argv[1]); //96;
    std::string base_path = argv[2]; //"data/Iterative_1";
    int start_iter = std::stoi(argv[3]); //1;
    int end_iter = std::stoi(argv[4]); //1000;
    std::string policy_file_x = argv[5]; //"data/P1_uniform_policy.json";
    std::string policy_file_o = argv[6]; //"data/P2_uniform_policy.json";
    
    std::string P1_information_sets_file = "data/P1_information_sets_v2.txt";
    std::string P2_information_sets_file = "data/P2_information_sets_v2.txt";
    std::string P1_information_sets_mapping_file = "data/P1_information_sets_mapping.txt";
    std::string P2_information_sets_mapping_file = "data/P2_information_sets_mapping.txt";
    Policy policy_obj_x;
    Policy policy_obj_o;
    Policy avg_policy_obj_x;
    Policy avg_policy_obj_o;
    std::unordered_map<std::string, std::vector<double>> avg_policy_numerator_x;
    std::unordered_map<std::string, std::vector<double>> avg_policy_numerator_o;
    std::unordered_map<std::string, double> avg_policy_denominator_x;
    std::unordered_map<std::string, double> avg_policy_denominator_o;
    std::vector<std::string> P1_information_sets;
    std::vector<std::string> P2_information_sets;
    std::vector<std::vector<double>> regret_list_x;
    std::vector<std::vector<double>> regret_list_o;
    std::vector<double> prob_reaching_list_x;
    std::vector<double> prob_reaching_list_o;

    if (start_iter == 1) {
        initialize_start(policy_file_x, P1_information_sets_file, P1_information_sets, regret_list_x, prob_reaching_list_x, policy_obj_x, avg_policy_obj_x, avg_policy_numerator_x, avg_policy_denominator_x, 'x');
        initialize_start(policy_file_o, P2_information_sets_file, P2_information_sets, regret_list_o, prob_reaching_list_o, policy_obj_o, avg_policy_obj_o, avg_policy_numerator_o, avg_policy_denominator_o, 'o');
    }

    else {
        std::string prev_regret_file_x = base_path + "/regret/P1_iteration_" + std::to_string(start_iter-1) + "_regret_cpp.json";
        std::string prev_regret_file_o = base_path + "/regret/P2_iteration_" + std::to_string(start_iter-1) + "_regret_cpp.json";
        std::unordered_map<std::string, std::vector<double> > regret_map_x;
        std::unordered_map<std::string, std::vector<double> > regret_map_o;
        regret_map_x = get_prev_regrets(prev_regret_file_x, 'x');
        regret_map_o = get_prev_regrets(prev_regret_file_o, 'o');
        initialize_continue(policy_file_x, P1_information_sets_file, P1_information_sets, regret_list_x, regret_map_x, prob_reaching_list_x, policy_obj_x, avg_policy_obj_x, avg_policy_numerator_x, avg_policy_denominator_x, 'x');
        initialize_continue(policy_file_o, P2_information_sets_file, P2_information_sets, regret_list_o, regret_map_o, prob_reaching_list_o, policy_obj_o, avg_policy_obj_o, avg_policy_numerator_o, avg_policy_denominator_o, 'o');
    }

    std::cout << "Information set size for player 1: " << P1_information_sets.size() << std::endl;
    std::cout << "Information set size for player 2: " << P2_information_sets.size() << std::endl;
    std::cout << "Prob reaching size for player 1: " << prob_reaching_list_x.size() << std::endl;
    std::cout << "Prob reaching size for player 2: " << prob_reaching_list_o.size() << std::endl;
    std::cout << "Regret list size for player 1: " << regret_list_x.size() << std::endl;
    std::cout << "Regret list size for player 2: " << regret_list_o.size() << std::endl;


    for (int T = start_iter; T <= end_iter; T++) {
        double expected_utility = get_expected_utility_wrapper(policy_obj_x, policy_obj_o);
        std::cout << "Expected utility: " << expected_utility << std::endl; 
        run_cfr(T, P1_information_sets, regret_list_x, policy_obj_x, policy_obj_o, 'x');
        run_cfr(T, P2_information_sets, regret_list_o, policy_obj_x, policy_obj_o, 'o');
        get_prob_reaching(P1_information_sets, prob_reaching_list_x, 'x', policy_obj_x, policy_obj_o);
        get_prob_reaching(P2_information_sets, prob_reaching_list_o, 'o', policy_obj_x, policy_obj_o);
        calc_average_terms('x', P1_information_sets, policy_obj_x, prob_reaching_list_x, avg_policy_numerator_x, avg_policy_denominator_x);
        calc_average_terms('o', P2_information_sets, policy_obj_o, prob_reaching_list_o, avg_policy_numerator_o, avg_policy_denominator_o);
    }

    calc_average_policy(P1_information_sets, avg_policy_obj_x, avg_policy_numerator_x, avg_policy_denominator_x, 'x');
    calc_average_policy(P2_information_sets, avg_policy_obj_o, avg_policy_numerator_o, avg_policy_denominator_o, 'o');

    std::string output_policy_file_x = base_path + "/average" + "/P1_iteration_" + std::to_string(end_iter) + "_average_cfr_policy_cpp.json";
    std::string output_policy_file_o = base_path + "/average" + "/P2_iteration_" + std::to_string(end_iter) + "_average_cfr_policy_cpp.json";
    std::string output_regret_file_x = base_path + "/regret/P1_iteration_" + std::to_string(end_iter) + "_regret_cpp.json";
    std::string output_regret_file_o = base_path + "/regret/P2_iteration_" + std::to_string(end_iter) + "_regret_cpp.json";
    save_output(output_policy_file_x, output_regret_file_x, 'x', P1_information_sets, regret_list_x, avg_policy_obj_x);
    save_output(output_policy_file_o, output_regret_file_o, 'o', P2_information_sets, regret_list_o, avg_policy_obj_o);
}
