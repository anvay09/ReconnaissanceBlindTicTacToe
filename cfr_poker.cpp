// Compile: g++-13 -O3 cfr_poker.cpp poker_classes.cpp poker_utilities.cpp -o cfr_p -fopenmp 

#include "cpp_headers/poker_classes.hpp"
#include "cpp_headers/poker_utilities.hpp"

int NUMBER_THREADS = 4;
int AVERAGE_DELAY = 5;

void save_map_json(std::string output_file, std::vector<std::vector<double>>& map, std::vector<std::string>& information_sets){
    std::ofstream f_out;
    f_out.open(output_file, std::ios::trunc);
    json jx;
    for (long int j = 0; j < map.size(); j++) {
        for (int i = 0; i < 6; i++) {
            jx[information_sets[j]][std::to_string(i)] = map[j][i];
        }
    }
    f_out << jx.dump() << std::endl;
    f_out.close();
}

//cfr
void run_cfr(int T, std::vector<std::string>& information_sets, std::vector<std::vector<double>>& regret_list, PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, char player, std::string base_path, char game){
    // std::cout << "Starting iteration " << T << " for player " << player << "..." << std::endl;

    #pragma omp parallel for num_threads(NUMBER_THREADS) shared(regret_list, policy_obj_x, policy_obj_o)
    for (int i = 0; i < information_sets.size(); i++) {
        std::string I_hash = information_sets[i];
        bool move_flag = get_move_flag(I_hash, player);
        InformationSet I(player, move_flag, I_hash, game);
        calc_cfr_policy_given_I(I, policy_obj_x, policy_obj_o, T, regret_list[i]);
    }

    #pragma omp parallel for num_threads(NUMBER_THREADS) shared(regret_list, policy_obj_x, policy_obj_o)
    for (int i = 0; i < information_sets.size(); i++) {
        std::string I_hash = information_sets[i];
        bool move_flag = get_move_flag(I_hash, player);
        InformationSet I(player, move_flag, I_hash, game);
        std::vector<double>& regret_vector = regret_list[i];
        double total_regret = 0.0;
        std::vector<int> actions;
        I.get_actions(actions);

        for (int action : actions) {
            total_regret += regret_vector[action];
        }

        PolicyVec& policy_obj = player == 'x' ? policy_obj_x : policy_obj_o;
        std::vector<double>& prob_dist = policy_obj.policy_dict[I.get_index()];
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

void initialize_start(std::string information_set_file, std::vector<std::string>& information_sets, std::vector<std::vector<double>>& regret_list, std::vector<double>& prob_reaching_list, PolicyVec& policy_obj, PolicyVec& avg_policy_obj, std::vector<double>& avg_policy_denominator, char player, char game) {
    std::cout << "initialize_start for player " << player << std::endl;
    policy_obj = PolicyVec(player, information_sets, game);
    avg_policy_obj = policy_obj;
    
    std::ifstream f_is(information_set_file);
    std::string line_is;
        
    while (std::getline(f_is, line_is)) {
        avg_policy_denominator.push_back(0.0);
        std::vector<double> regret_vector;
        for (int i = 0; i < 6; i++) {
            regret_vector.push_back(0.0);
        }
        regret_list.push_back(regret_vector);
        prob_reaching_list.push_back(0.0);
    }
    f_is.close();
}

void initialize_continue(std::string information_set_file, std::vector<std::string>& information_sets, std::vector<std::vector<double>>& regret_list, std::vector<std::vector<double>>& regret_map, std::vector<double>& prob_reaching_list, PolicyVec& policy_obj, PolicyVec& avg_policy_obj, std::vector<double>& avg_policy_denominator, char player, char game) {
    policy_obj = PolicyVec(player, information_sets, game);
    avg_policy_obj = policy_obj;
    
    std::ifstream f_is(information_set_file);
    std::string line_is;
    int line_number = 0;
        
    while (std::getline(f_is, line_is)) {
        avg_policy_denominator.push_back(0.0);
        std::vector<double> regret_vector;
        for (int i = 0; i < 6; i++) {
            regret_vector.push_back(regret_map[line_number][i]); // maybe can be merged with initialize_start
        }
        regret_list.push_back(regret_vector);
        prob_reaching_list.push_back(0.0);
        line_number += 1;
    }
    f_is.close();
}

void save_output(std::string output_policy_file, std::string output_regret_file, char player, std::vector<std::string>& information_sets, std::vector<std::vector<double>>& regret_list, PolicyVec& policy_obj) {
    std::vector<std::vector<double> > regret_map;
    std::cout << "Saving regrets for player " << player << "..." << std::endl;
    for (long int i = 0; i < information_sets.size(); i++) {
        std::string I_hash = information_sets[i];
        std::vector<double>& regret_vector = regret_list[i];
        regret_map.push_back(regret_vector);
    }
    save_map_json(output_regret_file, regret_map, information_sets);

    std::cout << "Saving policy for player " << player << "..." << std::endl;
    save_map_json(output_policy_file, policy_obj.policy_dict, information_sets);
}
//cfr

// prob
void valid_histories_play_prob(InformationSet& I_1, InformationSet& I_2, PokerTable& true_cards, History& current_history, InformationSet& end_I,
                               PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, std::vector<std::vector<int>>& valid_histories_list){
    InformationSet& I = true_cards.player_to_move == 'x' ? I_1 : I_2;
    std::vector<int> actions;

    if (I == end_I){
        valid_histories_list.push_back(current_history.history);
        return;
    }

    if (I.player == 'x') {
        I.get_actions_given_policy(actions, policy_obj_x);
    } 
    else {
        I.get_actions_given_policy(actions, policy_obj_o);
    }

    if (I.move_flag){
        for (int action : actions) {
            PokerTable new_true_cards = true_cards;
            bool success = new_true_cards.update_move(action);

            History new_history = current_history;
            new_history.history.push_back(action);

            char winner;
            if (success && !new_true_cards.is_win(winner) && !new_true_cards.is_over()) {
                InformationSet new_I = I;
                new_I.update_move(action);

                if (I.player == 'x') {
                    valid_histories_play_prob(new_I, I_2, new_true_cards, new_history, end_I, policy_obj_x, policy_obj_o, valid_histories_list);
                }
                else {
                    valid_histories_play_prob(I_1, new_I, new_true_cards, new_history, end_I, policy_obj_x, policy_obj_o, valid_histories_list);
                }
            }
        }
    }
    else {
        for (int action : actions) {
            InformationSet new_I = I;
            new_I.simulate_sense(action, true_cards);
            PokerTable new_true_cards = true_cards;

            History new_history = current_history;
            new_history.history.push_back(action);

            if (I.player == 'x') {
                valid_histories_play_prob(new_I, I_2, new_true_cards, new_history, end_I, policy_obj_x, policy_obj_o, valid_histories_list);
            }
            else {
                valid_histories_play_prob(I_1, new_I, new_true_cards, new_history, end_I, policy_obj_x, policy_obj_o, valid_histories_list);
            }
        }
    }
}


void upgraded_get_histories_given_I_prob(InformationSet& I, PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, std::vector<std::vector<int>>& valid_histories_list){    
    std::vector<std::string>& unique_draws = I.game == 'L' ? unique_draws_leduc : unique_draws_kuhn;
    for (std::string draw : unique_draws){
        std::string hash_1 = "a-" + std::string(1, draw[0]) + "--";
        std::string hash_2 = "o-" + std::string(1, draw[1]) + "--";
    
        InformationSet I_1('x', true, hash_1, I.game);
        InformationSet I_2('o', false, hash_2, I.game);
        PokerTable true_cards = PokerTable(draw);
        true_cards.game = I.game;
    
        std::vector<int> h = {};
        h.push_back(draw[0]);
        h.push_back(draw[1]);
        h.push_back(draw[2]);
        NonTerminalHistory current_history(h);

        valid_histories_play_prob(I_1, I_2, true_cards, current_history, I, policy_obj_x, policy_obj_o, valid_histories_list);
    }
    return;
}


double get_prob_h_given_policy_prob(InformationSet& I_1, InformationSet& I_2, PokerTable& true_cards, int next_action, 
                               PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, double probability, History history_obj, char initial_player){

    InformationSet& I = true_cards.player_to_move == 'x' ? I_1 : I_2;
    PolicyVec& policy_obj = I.player == 'x' ? policy_obj_x : policy_obj_o;

    if (I.move_flag) {
        PokerTable new_true_cards = true_cards;
        bool success = new_true_cards.update_move(next_action);

        if (I.player == initial_player) {
            probability *= policy_obj.policy_dict[I.get_index()][next_action];
        }
        history_obj.track_traversal_index += 1;
        if (history_obj.track_traversal_index < history_obj.history.size()) {
            int new_next_action = history_obj.history[history_obj.track_traversal_index];

            char winner;
            if (success && !new_true_cards.is_win(winner) && !new_true_cards.is_over()) {
                InformationSet new_I = I;
                new_I.update_move(next_action);

                if (I.player == 'x') {
                    probability = get_prob_h_given_policy_prob(new_I, I_2, new_true_cards, new_next_action, policy_obj_x, policy_obj_o, probability, history_obj, initial_player);
                }
                else {
                    probability = get_prob_h_given_policy_prob(I_1, new_I, new_true_cards, new_next_action, policy_obj_x, policy_obj_o, probability, history_obj, initial_player);
                }
            }
        }
    }
    else {
        InformationSet new_I = I;
        new_I.simulate_sense(next_action, true_cards);
        PokerTable new_true_cards = true_cards;

        if (I.player == initial_player) {
            probability *= policy_obj.policy_dict[I.get_index()][next_action];
        }
        history_obj.track_traversal_index += 1;
        if (history_obj.track_traversal_index < history_obj.history.size()) {
            int new_next_action = history_obj.history[history_obj.track_traversal_index];

            if (I.player == 'x') {
                probability = get_prob_h_given_policy_prob(new_I, I_2, new_true_cards, new_next_action, policy_obj_x, policy_obj_o, probability, history_obj, initial_player);
            }
            else {
                probability = get_prob_h_given_policy_prob(I_1, new_I, new_true_cards, new_next_action, policy_obj_x, policy_obj_o, probability, history_obj, initial_player);
            }
        }
    }

    return probability;
}

double get_prob_h_given_policy_wrapper_prob(InformationSet& I_1, InformationSet& I_2, PokerTable& true_cards,
                                       int next_action, PolicyVec& policy_obj_x, PolicyVec& policy_obj_o,
                                       History history_obj, InformationSet& curr_I_1, char initial_player){
    double q = 1.0 / 30.0;
    if (true_cards.game == 'L'){
        if (true_cards.cards[0] != true_cards.cards[1] && true_cards.cards[1] != true_cards.cards[2] && true_cards.cards[0] != true_cards.cards[2]){
            q = 2.0 / 30.0;
        }
    }
    else if (true_cards.game == 'K'){
        q = 1.0 / 6.0;
    }

    if (curr_I_1.get_hash().size() == 5){
        return q;
    }
    else {
        return get_prob_h_given_policy_prob(I_1, I_2, true_cards, next_action, policy_obj_x, policy_obj_o, q, history_obj, initial_player);
    }
}


void get_probability_of_reaching_all_h_prob(InformationSet& I, PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, std::vector<std::vector<int>>& starting_histories, 
                                       char initial_player, std::vector<double>& prob_reaching_h_list_all) {
    for (std::vector<int> h : starting_histories) {
        NonTerminalHistory h_object(h);

        std::string cards = "---";
        cards[0] = h_object.history[0];
        cards[1] = h_object.history[1];
        cards[2] = h_object.history[2];
        
        std::string hash_1 = "a-" + std::string(1, cards[0]) + "--";
        std::string hash_2 = "o-" + std::string(1, cards[1]) + "--";
        InformationSet I_1('x', true, hash_1, I.game);
        InformationSet I_2('o', false, hash_2, I.game);
        PokerTable true_cards = PokerTable(cards);
        true_cards.game = I.game;
        h_object.track_traversal_index = 3;
        double probability_reaching_h = get_prob_h_given_policy_wrapper_prob(I_1, I_2, true_cards, h_object.history[3], policy_obj_x, policy_obj_o, h_object, I, initial_player);
        prob_reaching_h_list_all.push_back(probability_reaching_h);
    }
}


double get_probability_of_reaching_I_prob(InformationSet& I, PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, char initial_player) {
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


void get_prob_reaching(std::vector<std::string>& information_sets, std::vector<double>& prob_reaching_list, char player, PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, char game){
    #pragma omp parallel for num_threads(NUMBER_THREADS) shared(policy_obj_x, policy_obj_o, prob_reaching_list)
    for (long int i = 0; i < information_sets.size(); i++) {
        prob_reaching_list[i] = 0.0;
        std::string I_hash = information_sets[i];
        bool move_flag = get_move_flag(I_hash, player);
        InformationSet I(player, move_flag, I_hash, game);

        prob_reaching_list[i] = get_probability_of_reaching_I_prob(I, policy_obj_x, policy_obj_o, player);
    }
}
//prob

//avg
void calc_average_terms(char player, std::vector<std::string>& information_sets, PolicyVec& policy_obj, std::vector<double>& prob_reaching_list, std::vector<std::vector<double>>& avg_policy_numerator, std::vector<double>& avg_policy_denominator, int T, char game){
    int weight = T > AVERAGE_DELAY ? T - AVERAGE_DELAY : 0;
    
    #pragma omp parallel for num_threads(NUMBER_THREADS) shared(avg_policy_numerator, avg_policy_denominator, policy_obj, prob_reaching_list)
    for (long int i = 0; i < information_sets.size(); i++) {
        std::string I_hash = information_sets[i];
        bool move_flag = get_move_flag(I_hash, player);
        InformationSet I(player, move_flag, I_hash, game);

        std::vector<int> actions;
        I.get_actions(actions);
        for (int action: actions) {
            std::vector<double>& policy = policy_obj.policy_dict[I.get_index()];
            avg_policy_numerator[I.get_index()][action] += prob_reaching_list[i] * weight * policy[action];
            avg_policy_denominator[I.get_index()] += prob_reaching_list[i] * weight * policy[action];
        }
    }
}

void calc_average_policy(std::vector<std::string>& information_sets, PolicyVec& avg_policy_obj, std::vector<std::vector<double>> avg_policy_numerator, std::vector<double> avg_policy_denominator, char player, char game){
    #pragma omp parallel for num_threads(NUMBER_THREADS) shared(avg_policy_obj, avg_policy_numerator, avg_policy_denominator)
    for (long int i = 0; i < information_sets.size(); i++) {
        std::string I_hash = information_sets[i];
        bool move_flag = get_move_flag(I_hash, player);
        InformationSet I(player, move_flag, I_hash, game);

        std::vector<int> actions;
        I.get_actions(actions);
        std::vector<double>& policy = avg_policy_obj.policy_dict[I.get_index()];
        for (int action: actions) {
            policy[action] = avg_policy_denominator[I.get_index()] > 0 ? avg_policy_numerator[I.get_index()][action] / avg_policy_denominator[I.get_index()] : 0;
        }
    }
}
//avg

int main(int argc, char* argv[])  {
    std::cout.precision(17);
    NUMBER_THREADS = std::stoi(argv[1]); //96;
    std::string base_path = argv[2]; //"data/Iterative_1";
    int start_iter = std::stoi(argv[3]); //1;
    int end_iter = std::stoi(argv[4]); //1000;
    char game = argv[5][0]; //'L';
    int log_frequency = 10;

    // read information set file
    std::vector<std::string> P1_information_sets;
    std::vector<std::string> P2_information_sets;
    std::string P1_information_sets_file = game == 'L' ? "P1_information_sets_Leduc_Poker.txt" : "P1_information_sets_Kuhn_Poker.txt";
    std::string P2_information_sets_file = game == 'L' ? "P2_information_sets_Leduc_Poker.txt" : "P2_information_sets_Kuhn_Poker.txt";

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

    PolicyVec policy_obj_x;
    PolicyVec policy_obj_o;
    PolicyVec avg_policy_obj_x;
    PolicyVec avg_policy_obj_o;
    std::vector<std::vector<double>> avg_policy_numerator_x(P1_information_sets.size(), std::vector<double>(6, 0));
    std::vector<std::vector<double>> avg_policy_numerator_o(P2_information_sets.size(), std::vector<double>(6, 0));
    std::vector<double> avg_policy_denominator_x;
    std::vector<double> avg_policy_denominator_o;
    std::vector<std::vector<double>> regret_list_x;
    std::vector<std::vector<double>> regret_list_o;
    std::vector<double> prob_reaching_list_x;
    std::vector<double> prob_reaching_list_o;

    if (start_iter == 1) {
        initialize_start(P1_information_sets_file, P1_information_sets, regret_list_x, prob_reaching_list_x, policy_obj_x, avg_policy_obj_x, avg_policy_denominator_x, 'x', game);
        initialize_start(P2_information_sets_file, P2_information_sets, regret_list_o, prob_reaching_list_o, policy_obj_o, avg_policy_obj_o, avg_policy_denominator_o, 'o', game);
    }

    else {
        std::string prev_regret_file_x = base_path + "/regret/P1_iteration_" + std::to_string(start_iter-1) + "_regret_cpp.json";
        std::string prev_regret_file_o = base_path + "/regret/P2_iteration_" + std::to_string(start_iter-1) + "_regret_cpp.json";
        std::vector<std::vector<double> > regret_map_x;
        std::vector<std::vector<double> > regret_map_o;
        regret_map_x = get_prev_regrets(prev_regret_file_x, 'x');
        regret_map_o = get_prev_regrets(prev_regret_file_o, 'o');
        initialize_continue(P1_information_sets_file, P1_information_sets, regret_list_x, regret_map_x, prob_reaching_list_x, policy_obj_x, avg_policy_obj_x, avg_policy_denominator_x, 'x', game);
        initialize_continue(P2_information_sets_file, P2_information_sets, regret_list_o, regret_map_o, prob_reaching_list_o, policy_obj_o, avg_policy_obj_o, avg_policy_denominator_o, 'o', game);
    }

    for (int T = start_iter; T <= end_iter; T++) {
        run_cfr(T, P1_information_sets, regret_list_x, policy_obj_x, policy_obj_o, 'x', base_path, game);
        run_cfr(T, P2_information_sets, regret_list_o, policy_obj_x, policy_obj_o, 'o', base_path, game);
        get_prob_reaching(P1_information_sets, prob_reaching_list_x, 'x', policy_obj_x, policy_obj_o, game);
        get_prob_reaching(P2_information_sets, prob_reaching_list_o, 'o', policy_obj_x, policy_obj_o, game);
        calc_average_terms('x', P1_information_sets, policy_obj_x, prob_reaching_list_x, avg_policy_numerator_x, avg_policy_denominator_x, T, game);
        calc_average_terms('o', P2_information_sets, policy_obj_o, prob_reaching_list_o, avg_policy_numerator_o, avg_policy_denominator_o, T, game);

        std::string output_policy_file_x = base_path + "/cfr" + "/P1_iteration_" + std::to_string(end_iter) + "_cfr_policy_cpp.json";
        std::string output_policy_file_o = base_path + "/cfr" + "/P2_iteration_" + std::to_string(end_iter) + "_cfr_policy_cpp.json";
        save_map_json(output_policy_file_x, policy_obj_x.policy_dict, P1_information_sets);
        save_map_json(output_policy_file_o, policy_obj_o.policy_dict, P2_information_sets);

        if (T % log_frequency == 0){
            std::cout << "Finished iteration " << T << std::endl;
            double expected_utility = get_expected_utility_wrapper(policy_obj_x, policy_obj_o, game);
            std::cout << "Expected utility of iteration policies: " << expected_utility << std::endl; 

            calc_average_policy(P1_information_sets, avg_policy_obj_x, avg_policy_numerator_x, avg_policy_denominator_x, 'x', game);
            calc_average_policy(P2_information_sets, avg_policy_obj_o, avg_policy_numerator_o, avg_policy_denominator_o, 'o', game);

            expected_utility = get_expected_utility_wrapper(avg_policy_obj_x, avg_policy_obj_o, game);
            std::cout << "Expected utility of average policies: " << expected_utility << std::endl;

            // best response
            double exploitability = 0.0;
            PolicyVec br_x('x', P1_information_sets, game);
            PolicyVec br_o('o', P2_information_sets, game);

            std::cout << "Computing best response for player x" << std::endl;
            expected_utility = compute_best_response_wrapper(avg_policy_obj_o, br_x, 'x', game);
            expected_utility = get_expected_utility_wrapper(br_x, avg_policy_obj_o, game);
            exploitability += expected_utility;
            std::cout << "Expected utility of br_x: " << expected_utility << std::endl;

            std::cout << "Computing best response for player o" << std::endl;
            expected_utility = compute_best_response_wrapper(avg_policy_obj_x, br_o, 'o', game);
            expected_utility = get_expected_utility_wrapper(avg_policy_obj_x, br_o, game);
            exploitability -= expected_utility;
            std::cout << "Expected utility of br_o: " << expected_utility << std::endl;

            std::cout << "Exploitability: " << exploitability << std::endl;

        }
    }

    calc_average_policy(P1_information_sets, avg_policy_obj_x, avg_policy_numerator_x, avg_policy_denominator_x, 'x', game);
    calc_average_policy(P2_information_sets, avg_policy_obj_o, avg_policy_numerator_o, avg_policy_denominator_o, 'o', game);

    std::string output_policy_file_x = base_path + "/average" + "/P1_iteration_" + std::to_string(end_iter) + "_average_cfr_policy_cpp.json";
    std::string output_policy_file_o = base_path + "/average" + "/P2_iteration_" + std::to_string(end_iter) + "_average_cfr_policy_cpp.json";
    std::string output_regret_file_x = base_path + "/regret/P1_iteration_" + std::to_string(end_iter) + "_regret_cpp.json";
    std::string output_regret_file_o = base_path + "/regret/P2_iteration_" + std::to_string(end_iter) + "_regret_cpp.json";
    save_output(output_policy_file_x, output_regret_file_x, 'x', P1_information_sets, regret_list_x, avg_policy_obj_x);
    save_output(output_policy_file_o, output_regret_file_o, 'o', P2_information_sets, regret_list_o, avg_policy_obj_o);
    double expected_utility = get_expected_utility_wrapper(avg_policy_obj_x, avg_policy_obj_o, game);
    std::cout << "Expected utility avg: " << expected_utility << std::endl; 
}
