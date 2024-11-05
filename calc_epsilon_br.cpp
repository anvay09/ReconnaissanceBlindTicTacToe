#include "cpp_headers/rbt_classes.hpp"
#include "cpp_headers/rbt_utilities.hpp"
#include <random>
int NUMBER_THREADS = 4;
int AVERAGE_DELAY = 5;

//avg
void calc_average_terms(char player, std::vector<std::string>& information_sets, PolicyVec& policy_obj, std::vector<std::vector<double>>& avg_policy_numerator, std::vector<double>& avg_policy_denominator, int t){
    //int weight = T > AVERAGE_DELAY ? T - AVERAGE_DELAY : 0;
    int weight = t;

    #pragma omp parallel for num_threads(NUMBER_THREADS) shared(avg_policy_numerator, avg_policy_denominator, policy_obj)
    for (long int i = 0; i < information_sets.size(); i++) {
        std::string I_hash = information_sets[i];
        bool move_flag = get_move_flag(I_hash, player);
        InformationSet I(player, move_flag, I_hash);

        std::vector<int> actions;
        I.get_actions(actions);
        for (int action: actions) {
            std::vector<double>& policy = policy_obj.policy_dict[I.get_index()];
            avg_policy_numerator[I.get_index()][action] += weight * policy[action];
            avg_policy_denominator[I.get_index()] += weight * policy[action];
        }

    }
}

void calc_average_policy(std::vector<std::string>& information_sets, PolicyVec& avg_policy_obj, std::vector<std::vector<double>> avg_policy_numerator, std::vector<double> avg_policy_denominator, char player){
    #pragma omp parallel for num_threads(NUMBER_THREADS) shared(avg_policy_obj, avg_policy_numerator, avg_policy_denominator)
    for (long int i = 0; i < information_sets.size(); i++) {
        std::string I_hash = information_sets[i];
        bool move_flag = get_move_flag(I_hash, player);
        InformationSet I(player, move_flag, I_hash);

        std::vector<int> actions;
        I.get_actions(actions);
        for (int action: actions) {
            std::vector<double>& policy = avg_policy_obj.policy_dict[I.get_index()];
            policy[action] = avg_policy_denominator[I.get_index()] > 0 ? avg_policy_numerator[I.get_index()][action] / avg_policy_denominator[I.get_index()] : 0;
        }
    }
}
//avg

void pretty_print(std::chrono::time_point<std::chrono::system_clock> start, std::chrono::time_point<std::chrono::system_clock> end, std::string msg, int flag) {
    if (flag) {
    std::chrono::duration<double> elapsed_seconds = end-start;
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);
    std::cout << "finished " << msg << " in " << elapsed_seconds.count() << "s" << std::endl;
    }
}


void print_histogram(std::vector<long int>& visited_infosets) {
    std::vector<long int> buckets(10, 0);
    // index 0: visited 0 times, index 1: visited 1 times, index 2: visited 2 times, index 3: visited 2-10 times, index 4: visited 11-100 times, index 5: visited 101-1000 times, 
    //index 6: visited 1001-10000 times, index 7: visited 10001-100000 times, index 8: visited 100001-1000000 times, index 9: visited 1000001+ times

    for (long int i = 0; i < visited_infosets.size(); i++) {
        if (visited_infosets[i] == 0) {
            buckets[0] += 1;
        }
        else if (visited_infosets[i] == 1) {
            buckets[1] += 1;
        }
        else if (visited_infosets[i] == 2) {
            buckets[2] += 1;
        }
        else if (visited_infosets[i] >= 3 && visited_infosets[i] <= 10) {
            buckets[3] += 1;
        }
        else if (visited_infosets[i] >= 11 && visited_infosets[i] <= 100) {
            buckets[4] += 1;
        }
        else if (visited_infosets[i] >= 101 && visited_infosets[i] <= 1000) {
            buckets[5] += 1;
        }
        else if (visited_infosets[i] >= 1001 && visited_infosets[i] <= 10000) {
            buckets[6] += 1;
        }
        else if (visited_infosets[i] >= 10001 && visited_infosets[i] <= 100000) {
            buckets[7] += 1;
        }
        else if (visited_infosets[i] >= 100001 && visited_infosets[i] <= 1000000) {
            buckets[8] += 1;
        }
        else {
            buckets[9] += 1;
        }
    }

    std::cout << "Histogram of visited information sets: " << std::endl;

    for (int i = 0; i < 10; i++) {
        std::cout << buckets[i] << "\t\t";
    }
    std::cout << std::endl;

    std::cout << "0\t\t1\t\t2\t\t3-10\t\t11-100\t\t101-1k\t\t1k-10k\t\t10k-100k\t\t100k-1M\t\t1M+" << std::endl;
}


int sampleIndex(const std::vector<double>& probabilities) {
    std::random_device rd;
    std::mt19937 generator(rd());
    std::discrete_distribution<int> distribution(probabilities.begin(), probabilities.end());
    return distribution(generator);
}


double sample_terminal_history(InformationSet& I_1, InformationSet& I_2, TicTacToeBoard& true_board, PolicyVec& policy_obj_x, 
                               PolicyVec& policy_obj_o, History& current_history, char player, double probability, double& reward, 
                               char update_player, PolicyVec& oppo_cumulative_sample_count, PolicyVec& oppo_strategy, std::vector<long int>& visited_infosets) {
    InformationSet& I = player == 'x' ? I_1 : I_2;
    PolicyVec& policy_obj = player == 'x' ? policy_obj_x : policy_obj_o;
    std::vector<double> prob_dist = policy_obj.policy_dict[I.get_index()];

    int action = sampleIndex(prob_dist);

    if (I.move_flag) {
        bool success = true_board.update_move(action, player);

        if (player == update_player) { // update the probability only if the player is the one we are updating
            probability = probability * prob_dist[action];
        }
        else {
            visited_infosets[I.get_index()] += 1;
            std::vector<double>& oppo_sample_count = oppo_cumulative_sample_count.policy_dict[I.get_index()];
            oppo_sample_count[action] += 1.0;
            double sum_infoset_sample_count = 0.0;
            for (int i = 0; i < 13; i++) {
                sum_infoset_sample_count += oppo_sample_count[i];
            }

            std::vector<double>& oppo_prob_dist = oppo_strategy.policy_dict[I.get_index()];
            for (int i = 0; i < 13; i++) {
                oppo_prob_dist[i] = oppo_sample_count[i] / sum_infoset_sample_count;
            }
        }

        current_history.history.push_back(action);

        char winner;
        if (success && !true_board.is_win(winner) && !true_board.is_over()) {
            InformationSet new_I = I;
            new_I.update_move(action, player);
            new_I.reset_zeros();

            if (player == 'x') {
                return sample_terminal_history(new_I, I_2, true_board, policy_obj_x, policy_obj_o, current_history, 'o', probability, reward, update_player, oppo_cumulative_sample_count, oppo_strategy, visited_infosets);
            } else {
                return sample_terminal_history(I_1, new_I, true_board, policy_obj_x, policy_obj_o, current_history, 'x', probability, reward, update_player, oppo_cumulative_sample_count, oppo_strategy, visited_infosets);
            }
        } else {
            TerminalHistory H_T = TerminalHistory(current_history.history);
            H_T.set_reward();
            reward = (double) H_T.reward[0];
            return probability;
        }
    }
    else {
        InformationSet new_I = I;
        new_I.simulate_sense(action, true_board);
        
        if (player == update_player) { // update the probability only if the player is the one we are updating
            probability = probability * prob_dist[action];
        }
        else {
            visited_infosets[I.get_index()] += 1;
            std::vector<double>& oppo_sample_count = oppo_cumulative_sample_count.policy_dict[I.get_index()];
            oppo_sample_count[action] += 1.0;
            double sum_infoset_sample_count = 0.0;
            for (int i = 0; i < 13; i++) {
                sum_infoset_sample_count += oppo_sample_count[i];
            }

            std::vector<double>& oppo_prob_dist = oppo_strategy.policy_dict[I.get_index()];
            for (int i = 0; i < 13; i++) {
                oppo_prob_dist[i] = oppo_sample_count[i] / sum_infoset_sample_count;
            }
        }
        current_history.history.push_back(action);

        if (player == 'x') {
            return sample_terminal_history(new_I, I_2, true_board, policy_obj_x, policy_obj_o, current_history, 'x', probability, reward, update_player, oppo_cumulative_sample_count, oppo_strategy, visited_infosets);
        } else {
            return sample_terminal_history(I_1, new_I, true_board, policy_obj_x, policy_obj_o, current_history, 'o', probability, reward, update_player, oppo_cumulative_sample_count, oppo_strategy, visited_infosets);
        }
    }
}


double sample_terminal_history_wrapper(PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, History& current_history, double& reward, char update_player, PolicyVec& oppo_cumulative_sample_count, PolicyVec& oppo_strategy, std::vector<long int>& visited_infosets) {
    std::string board = "000000000";
    TicTacToeBoard true_board = TicTacToeBoard(board);
    std::string hash_1 = "";
    std::string hash_2 = "";
    InformationSet I_1 = InformationSet('x', true, hash_1);
    InformationSet I_2 = InformationSet('o', false, hash_2);
    return sample_terminal_history(I_1, I_2, true_board, policy_obj_x, policy_obj_o, current_history, 'x', 1.0, reward, update_player, oppo_cumulative_sample_count, oppo_strategy, visited_infosets);
}


void calc_epsilon_best_response(PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, PolicyVec& uniform_strategy_x, PolicyVec& uniform_strategy_o, std::vector<std::string>& P1_information_sets, std::vector<std::string>& P2_information_sets, char player, int T, int update_step_size, int log_flag, int average_flag) {
    auto start = std::chrono::system_clock::now();
    PolicyVec player_strategy = player == 'x' ? uniform_strategy_x : uniform_strategy_o;
    PolicyVec avg_player_strategy = player == 'x' ? uniform_strategy_x : uniform_strategy_o;
    PolicyVec opponent_strategy = player == 'x' ? uniform_strategy_o : uniform_strategy_x;
    PolicyVec opponent_cumulative_sample_count;
    player_strategy.player = player;
    avg_player_strategy.player = player;
    opponent_strategy.player = player == 'x' ? 'o' : 'x';
    opponent_cumulative_sample_count.player = player == 'x' ? 'o' : 'x';
    std::vector<std::string> player_information_sets = player == 'x' ? P1_information_sets : P2_information_sets;
    std::vector<std::string> opponent_information_sets = player == 'x' ? P2_information_sets : P1_information_sets;
    std::vector<std::vector<double>> avg_player_policy_numerator(player_information_sets.size(), std::vector<double>(13, 0));
    std::vector<double> avg_player_policy_denominator;
    std::vector<long int> opponent_visited_infosets(opponent_information_sets.size(), 0);

    for (long int i = 0; i < opponent_information_sets.size(); i++) {
        std::vector<double> probability_dist(13, 0.0);
        opponent_cumulative_sample_count.policy_dict.push_back(probability_dist);
    }
    for (long int i = 0; i < player_information_sets.size(); i++) {
        avg_player_policy_denominator.push_back(0.0);
    }
    auto end = std::chrono::system_clock::now();
    pretty_print(start, end, "initializing player and opponent strategies", log_flag);

    for (int t = 0; t < T; t++) {
        start = std::chrono::system_clock::now();   
        std::vector<int> h = {};
        TerminalHistory start_history = TerminalHistory(h);
        double q_z = 0.0;
        double reward = 0;
        double eps = 1.0/(((t*1.0)/(update_step_size*1.0))+1.0);
        std::vector<double> prob_dist = {eps, 1-eps};
        if (sampleIndex(prob_dist)){
            if (player == 'x') {
                q_z = sample_terminal_history_wrapper(player_strategy, policy_obj_o, start_history, reward, player, opponent_cumulative_sample_count, opponent_strategy, opponent_visited_infosets);
            }
            else {
                q_z = sample_terminal_history_wrapper(policy_obj_x, player_strategy, start_history, reward, player, opponent_cumulative_sample_count, opponent_strategy, opponent_visited_infosets);
            }
        }
        else {
            if (player == 'x') {
                q_z = sample_terminal_history_wrapper(uniform_strategy_x, policy_obj_o, start_history, reward, player, opponent_cumulative_sample_count, opponent_strategy, opponent_visited_infosets);
            }
            else {
                q_z = sample_terminal_history_wrapper(policy_obj_x, uniform_strategy_o, start_history, reward, player, opponent_cumulative_sample_count, opponent_strategy, opponent_visited_infosets);
            }
        }
        if (t % update_step_size == 0) {
            double expected_utility = 0.0;
            start = std::chrono::system_clock::now();
            expected_utility = compute_best_response_wrapper(opponent_strategy, player_strategy, player);
            end = std::chrono::system_clock::now();
            pretty_print(start, end, "best response computation iteration " + std::to_string(t), log_flag);
            //averaging
            start = std::chrono::system_clock::now();
            if (average_flag) {
                calc_average_terms(player, player_information_sets, player_strategy, avg_player_policy_numerator, avg_player_policy_denominator, t);
                calc_average_policy(player_information_sets, avg_player_strategy, avg_player_policy_numerator, avg_player_policy_denominator, player);
            }
            else {
                avg_player_strategy = player_strategy;
            }
            end = std::chrono::system_clock::now();
            pretty_print(start, end, "average computation iteration " + std::to_string(t), log_flag);
            start = std::chrono::system_clock::now();
            if (player == 'x'){
                expected_utility = get_expected_utility_wrapper(avg_player_strategy, policy_obj_o);
            }
            else {
                expected_utility = get_expected_utility_wrapper(policy_obj_x, avg_player_strategy);
            }
            end = std::chrono::system_clock::now();
            std::cout << "Expected utility avg after iteration " << t << ": " << expected_utility << std::endl;
            pretty_print(start, end, "expected utility computation iteration " + std::to_string(t), log_flag);
            // std::cout << "Epsilon: " << eps << std::endl;

            print_histogram(opponent_visited_infosets);
        }
        end = std::chrono::system_clock::now();
        pretty_print(start, end, "iteration " + std::to_string(t), log_flag);
    }
}


int main(int argc, char* argv[]) {
    std::cout.precision(17);
    std::string file_path_1 = argv[1];
    std::string file_path_2 = argv[2];
    std::string uniform_file_path_1 = argv[3];
    std::string uniform_file_path_2 = argv[4];
    int log_flag = std::stoi(argv[5]);
    int average_flag = std::stoi(argv[6]);
    NUMBER_THREADS = std::stoi(argv[7]); //96;

    // load information sets
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

    // create hash to int maps
    for (long int i = 0; i < P1_information_sets.size(); i++) {
        InformationSet::P1_hash_to_int_map[P1_information_sets[i]] = i;
    }
    for (long int i = 0; i < P2_information_sets.size(); i++) {
        InformationSet::P2_hash_to_int_map[P2_information_sets[i]] = i;
    }

    // load policies
    std::cout << "Loading policies..." << std::endl;
    auto start = std::chrono::system_clock::now(); 
    PolicyVec policy_obj_x('x', file_path_1);
    PolicyVec policy_obj_o('o', file_path_2);
    PolicyVec uniform_policy_obj_x('x', uniform_file_path_1);
    PolicyVec uniform_policy_obj_o('o', uniform_file_path_2);
    PolicyVec br_x('x', P1_information_sets);
    PolicyVec br_o('o', P2_information_sets);
    auto end = std::chrono::system_clock::now();
    pretty_print(start, end, "loading policies", log_flag);

    // compute epsilon best response
    char continue_exp = 'y';
    while (continue_exp == 'y') {
        int num_iterations = 10000;
        int update_step_size = 100;
        char player = 'x';
        std::cout << "Enter number of iterations: ";
        std::cin >> num_iterations;
        std::cout << "Enter update step size: ";
        std::cin >> update_step_size;
        std::cout << "Enter player for best response calculation: ";
        std::cin >> player;

        if (player == 'x') {
            // compute expected utility of the best response (i.e., when opponent policy is known)
            start = std::chrono::system_clock::now();  
            double expected_utility = compute_best_response_wrapper(policy_obj_o, br_x, 'x');
            end = std::chrono::system_clock::now();
            std::cout << "Expected utility of the best response x: " << expected_utility << std::endl;
            pretty_print(start, end, "computing best response x", log_flag);
        }
        else {
            // compute expected utility of the best response (i.e., when opponent policy is known)
            start = std::chrono::system_clock::now(); 
            double expected_utility = compute_best_response_wrapper(policy_obj_x, br_o, 'o');
            end = std::chrono::system_clock::now();
            std::cout << "Expected utility of the best response o: " << expected_utility << std::endl;
            pretty_print(start, end, "computing best response o", log_flag);
        }

        calc_epsilon_best_response(policy_obj_x, policy_obj_o, uniform_policy_obj_x, uniform_policy_obj_o, P1_information_sets, P2_information_sets, player, num_iterations, update_step_size, log_flag, average_flag);

        std::cout << "Continue experiments? (y/n): ";
        std::cin >> continue_exp;
    }
    
}
