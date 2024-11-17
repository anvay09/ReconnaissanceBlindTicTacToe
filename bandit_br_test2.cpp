#include "cpp_headers/rbt_classes.hpp"
#include "cpp_headers/rbt_utilities.hpp"
#include <random>
#include <cmath>
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
                               PolicyVec& policy_obj_o, History& current_history, char player, double& reward, 
                               char update_player) {
    InformationSet& I = player == 'x' ? I_1 : I_2;
    PolicyVec& policy_obj = player == 'x' ? policy_obj_x : policy_obj_o;
    std::vector<double> prob_dist = policy_obj.policy_dict[I.get_index()];

    int action = sampleIndex(prob_dist);

    if (I.move_flag) {
        bool success = true_board.update_move(action, player);
        current_history.history.push_back(action);

        char winner;
        if (success && !true_board.is_win(winner) && !true_board.is_over()) {
            InformationSet new_I = I;
            new_I.update_move(action, player);
            new_I.reset_zeros();

            if (player == 'x') {
                return sample_terminal_history(new_I, I_2, true_board, policy_obj_x, policy_obj_o, current_history, 'o', reward, update_player);
            } else {
                return sample_terminal_history(I_1, new_I, true_board, policy_obj_x, policy_obj_o, current_history, 'x', reward, update_player);
            }
        } else {
            TerminalHistory H_T = TerminalHistory(current_history.history);
            H_T.set_reward();
            reward = (double) H_T.reward[0];
            return reward;
        }
    }
    else {
        InformationSet new_I = I;
        new_I.simulate_sense(action, true_board);
        current_history.history.push_back(action);

        if (player == 'x') {
            return sample_terminal_history(new_I, I_2, true_board, policy_obj_x, policy_obj_o, current_history, 'x', reward, update_player);
        } else {
            return sample_terminal_history(I_1, new_I, true_board, policy_obj_x, policy_obj_o, current_history, 'o', reward, update_player);
        }
    }
}


double sample_terminal_history_wrapper(PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, History& current_history, double& reward, char update_player) {
    std::string board = "000000000";
    TicTacToeBoard true_board = TicTacToeBoard(board);
    std::string hash_1 = "";
    std::string hash_2 = "";
    InformationSet I_1 = InformationSet('x', true, hash_1);
    InformationSet I_2 = InformationSet('o', false, hash_2);
    return sample_terminal_history(I_1, I_2, true_board, policy_obj_x, policy_obj_o, current_history, 'x', reward, update_player);
}


void update_ucb(std::vector<std::vector<double>>& infoset_ucb_values, std::vector<std::vector<double>>& infoset_empirical_reward, std::vector<std::vector<long int>>& infoset_pull_count, std::vector<long int>& infoset_time_steps, PolicyVec& policy_obj, double reward, TerminalHistory& history, char player, int C) {
    // TODO
    std::string board = "000000000";
    TicTacToeBoard true_board = TicTacToeBoard(board);
    std::string hash_1 = "";
    std::string hash_2 = "";
    InformationSet I_1 = InformationSet('x', true, hash_1);
    InformationSet I_2 = InformationSet('o', false, hash_2);
    char curr_player = 'x';
    double total_reward = 0.0;
    long int total_pull = 0;

    for (int action : history.history) {

        if (curr_player == player) {
            InformationSet I = curr_player == 'x' ? I_1 : I_2;
            total_pull = infoset_pull_count[I.get_index()][action];
            total_reward = infoset_empirical_reward[I.get_index()][action] * total_pull;
            infoset_pull_count[I.get_index()][action] += 1;
            infoset_empirical_reward[I.get_index()][action] =  (total_reward + reward) / (total_pull + 1);
            infoset_time_steps[I.get_index()] += 1;
            std::vector<int> legal_actions;
            I.get_actions(legal_actions);
            double sum_ucb = 0.0;
            for (int a : legal_actions){
                double ucb_value = 0.0;
                if (infoset_pull_count[I.get_index()][a] > 0){
                    ucb_value = infoset_empirical_reward[I.get_index()][a] + sqrt(C*log(infoset_time_steps[I.get_index()]) / infoset_pull_count[I.get_index()][a]);
                    infoset_ucb_values[I.get_index()][a] = ucb_value;
                }
                else {
                    ucb_value = infoset_empirical_reward[I.get_index()][a] + sqrt(C*log(infoset_time_steps[I.get_index()]) / 1);
                    infoset_ucb_values[I.get_index()][a] = ucb_value;
                }
                sum_ucb += ucb_value;
            }
            std::vector<double>& policy_infoset = policy_obj.policy_dict[I.get_index()];
            for (int a : legal_actions){
                if (sum_ucb > 0){
                    policy_infoset[a] = infoset_ucb_values[I.get_index()][a]/sum_ucb;
                }
                else {
                    policy_infoset[a] = 1.0/legal_actions.size();
                }
            }
        }

        if (action < 9) {
            if (curr_player == 'x') {
                I_1.update_move(action, curr_player);
                I_1.reset_zeros();
            } else {
                I_2.update_move(action, curr_player);
                I_2.reset_zeros();
            }
            true_board.update_move(action, curr_player);
            curr_player = (curr_player == 'x') ? 'o' : 'x';
        } 
        else {
            if (curr_player == 'x') {
                I_1.simulate_sense(action, true_board);
            } else {
                I_2.simulate_sense(action, true_board);
            }
        }
    }

}


void calc_br_ucb(PolicyVec& opponent_policy, long int num_iterations, char br_player, std::vector<std::string>& player_information_sets, std::vector<std::string>& opponent_information_sets,  int log_flag, int log_frequency, int update_step_size, int C, PolicyVec& opponent_ucb_policy, PolicyVec& player_average_policy) {
    std::vector<long int> oppo_infoset_time_steps(opponent_information_sets.size(), 0);
    std::vector<std::vector<double>> oppo_infoset_ucb_values(opponent_information_sets.size(), std::vector<double>(13, 0));
    std::vector<std::vector<double>> oppo_infoset_empirical_reward(opponent_information_sets.size(), std::vector<double>(13, 0.0));
    std::vector<std::vector<long int>> oppo_infoset_pull_count(opponent_information_sets.size(), std::vector<long int>(13, 0));
    std::vector<std::vector<double>> avg_player_policy_numerator(player_information_sets.size(), std::vector<double>(13, 0.0));
    std::vector<double> avg_player_policy_denominator;
    for (long int i = 0; i < player_information_sets.size(); i++) {
        avg_player_policy_denominator.push_back(0.0);
    }

    for (long int t = 0; t < num_iterations; t++) {
        std::vector<int> h = {};
        TerminalHistory start_history = TerminalHistory(h);
        double reward = 0.0;
        auto start1 = std::chrono::system_clock::now();
        pretty_print(start1, start1, "Iteration start " + std::to_string(t), log_flag);
        sample_terminal_history_wrapper(player_average_policy, opponent_policy, start_history, reward, br_player);
        // update ucb values
        update_ucb(oppo_infoset_ucb_values, oppo_infoset_empirical_reward, oppo_infoset_pull_count, oppo_infoset_time_steps, opponent_ucb_policy, reward, start_history, toggle_player(br_player), C);
        auto end1 = std::chrono::system_clock::now();
        pretty_print(start1, end1, "Iteration end " + std::to_string(t), log_flag);

        PolicyVec br_policy(br_player, player_information_sets);
        if (t % update_step_size == 0 && t != 0){
            // update average policy
            auto start = std::chrono::system_clock::now();
            pretty_print(start, start, "best reponse start " + std::to_string(t), log_flag);
            compute_best_response_wrapper(opponent_ucb_policy, br_policy, br_player);
            auto end = std::chrono::system_clock::now();
            pretty_print(start, end, "best reponse end " + std::to_string(t), log_flag);
            //averaging
            start = std::chrono::system_clock::now();
            pretty_print(start, start, "averaging start " + std::to_string(t), log_flag);
            calc_average_terms(br_player, player_information_sets, br_policy, avg_player_policy_numerator, avg_player_policy_denominator, t);
            calc_average_policy(player_information_sets, player_average_policy, avg_player_policy_numerator, avg_player_policy_denominator, br_player);    
            end = std::chrono::system_clock::now();
            pretty_print(start, end, "averaging end " + std::to_string(t), log_flag);
        } 

        if (t % log_frequency == 0 && t != 0){
           double expected_utility = 0.0;
           auto start = std::chrono::system_clock::now();
           pretty_print(start, start, "expected utility start " + std::to_string(t), log_flag);
            if (br_player == 'x'){
                expected_utility = get_expected_utility_wrapper(br_policy, opponent_policy);
            }
            else {
                expected_utility = get_expected_utility_wrapper(opponent_policy, br_policy);
            }
            auto end = std::chrono::system_clock::now();
            pretty_print(start, end, "expected utility end " + std::to_string(t), log_flag);
            std::cout << "Expected utility avg after iteration " << t << ": " << expected_utility << std::endl;
        }
    } 
}

int main(int argc, char* argv[]) {
    std::cout.precision(17);
    std::string file_path_1 = argv[1];
    std::string file_path_2 = argv[2];
    std::string start_file_path_1 = argv[3];
    std::string start_file_path_2 = argv[4];
    int log_flag = std::stoi(argv[5]);
    NUMBER_THREADS = std::stoi(argv[6]); //96;

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
    auto start = std::chrono::system_clock::now(); 
    PolicyVec policy_obj_x('x', file_path_1);
    PolicyVec policy_obj_o('o', file_path_2);
    PolicyVec br_x('x', P1_information_sets);
    PolicyVec br_o('o', P2_information_sets);
    PolicyVec start_policy_obj_x('x', start_file_path_1);
    PolicyVec start_policy_obj_o('o', start_file_path_2);
    auto end = std::chrono::system_clock::now();
    pretty_print(start, end, "loading policies", log_flag);

    // compute epsilon best response
    char continue_exp = 'y';
    while (continue_exp == 'y') {
        int num_iterations = 10000;
        int update_step_size = 10000;
        int log_frequency = 10000;
        char player = 'x';
        int C = 1;
        std::cout << "Enter number of iterations: ";
        std::cin >> num_iterations;
        std::cout << "Enter update step size: ";
        std::cin >> update_step_size;
        std::cout << "Enter log frequnecy: ";
        std::cin >> log_frequency;
        std::cout << "Enter player for best response calculation: ";
        std::cin >> player;
        std::cout << "Enter hyperparameter c for UCB:";
        std::cin >> C;

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
        if (player == 'x') {
            calc_br_ucb(policy_obj_o, num_iterations, player, P1_information_sets, P2_information_sets, log_flag, log_frequency, update_step_size, C, start_policy_obj_o, start_policy_obj_x);
        }
        else {
            calc_br_ucb(policy_obj_x, num_iterations, player, P2_information_sets, P1_information_sets, log_flag, log_frequency, update_step_size, C, start_policy_obj_x, start_policy_obj_o);
        }
        
        std::cout << "Continue experiments? (y/n): ";
        std::cin >> continue_exp;
    }
    
}
