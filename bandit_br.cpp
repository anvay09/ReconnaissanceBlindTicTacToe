#include "cpp_headers/rbt_classes.hpp"
#include "cpp_headers/rbt_utilities.hpp"
#include <random>
int NUMBER_THREADS = 4;
int AVERAGE_DELAY = 5;



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


double sample_terminal_history(InformationSet& I_1, InformationSet& I_2, TicTacToeBoard& true_board, std::vector<std::vector<double>>& infoset_ucb_values, PolicyVec& opponent_policy, History& current_history, char player, char br_player) {
    InformationSet& I = player == 'x' ? I_1 : I_2;
    int action = 0;

    if (player == br_player){ // choose action with max UCB value
        std::vector<double>& action_ucbs = infoset_ucb_values[I.get_index()];
        double max_ucb = -1.0;

        for (int i = 0; i < 13; i++){
            if (action_ucbs[i] > max_ucb){
                max_ucb = action_ucbs[i];
            }
        }

        std::vector<double> best_arms(13, 0.0);
        double sum = 0.0;

        for (int i = 0; i < 13; i++){
            if (std::abs(action_ucbs[i] - max_ucb) < 1e-8){
                best_arms[i] = 1.0;
                sum += 1.0;
            }
        }

        for (int i = 0; i < 13; i++){
            best_arms[i] /= sum;
        }

        action = sampleIndex(best_arms);
    }
    else {
        std::vector<double> prob_dist = opponent_policy.policy_dict[I.get_index()];
        action = sampleIndex(prob_dist);
    }

    if (I.move_flag) {
        bool success = true_board.update_move(action, player);
        current_history.history.push_back(action);

        char winner;
        if (success && !true_board.is_win(winner) && !true_board.is_over()) {
            InformationSet new_I = I;
            new_I.update_move(action, player);
            new_I.reset_zeros();

            if (player == 'x') {
                return sample_terminal_history(new_I, I_2, true_board, infoset_ucb_values, opponent_policy, current_history, 'o', br_player);
            } else {
                return sample_terminal_history(I_1, new_I, true_board, infoset_ucb_values, opponent_policy, current_history, 'x', br_player);
            }
        } else {
            TerminalHistory H_T = TerminalHistory(current_history.history);
            H_T.set_reward();
            double reward = (double) H_T.reward[0];
            return reward;
        }
    }
    else {
        InformationSet new_I = I;
        new_I.simulate_sense(action, true_board);
        current_history.history.push_back(action);

        if (player == 'x') {
            return sample_terminal_history(new_I, I_2, true_board, infoset_ucb_values, opponent_policy, current_history, 'x', br_player);
        } else {
            return sample_terminal_history(I_1, new_I, true_board, infoset_ucb_values, opponent_policy, current_history, 'o', br_player);
        }
    }
}


double sample_terminal_history_wrapper(std::vector<std::vector<double>>& infoset_ucb_values, PolicyVec& opponent_policy, History& current_history, char br_player) {
    std::string board = "000000000";
    TicTacToeBoard true_board = TicTacToeBoard(board);
    std::string hash_1 = "";
    std::string hash_2 = "";
    InformationSet I_1 = InformationSet('x', true, hash_1);
    InformationSet I_2 = InformationSet('o', false, hash_2);
    return sample_terminal_history(I_1, I_2, true_board, infoset_ucb_values, opponent_policy, current_history, 'x', br_player);
}


void build_policy(){
    // TODO
}

void update_ucb(){
    // TODO
}


void calc_br_ucb(PolicyVec& opponent_policy, long int num_iterations, char player, std::vector<std::string>& player_information_sets, int log_flag) {
    std::vector<long int> infoset_time_steps(player_information_sets.size(), 0);
    std::vector<std::vector<double>> infoset_ucb_values(player_information_sets.size(), std::vector<double>(13, 0.0));
    std::vector<std::vector<double>> infoset_empirical_reward(player_information_sets.size(), std::vector<double>(13, 0.0));
    std::vector<std::vector<int>> infoset_pull_count(player_information_sets.size(), std::vector<int>(13, 0));

    for (long int t = 0; t < num_iterations; t++) {
        // sample terminal history
        std::vector<int> h = {};
        TerminalHistory start_history = TerminalHistory(h);
        double reward = 0;

        reward = sample_terminal_history_wrapper(infoset_ucb_values, opponent_policy, start_history, player);
        // update ucb values
        update_ucb();
    } 

    // TODO
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
