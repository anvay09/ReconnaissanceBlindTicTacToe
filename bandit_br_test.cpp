#include "cpp_headers/rbt_classes.hpp"
#include "cpp_headers/rbt_utilities.hpp"
#include <random>
#include <cmath>
int NUMBER_THREADS = 4;
int AVERAGE_DELAY = 5;

void pretty_print(std::chrono::time_point<std::chrono::system_clock> start, std::chrono::time_point<std::chrono::system_clock> end, std::string msg, int flag) {
    if (flag) {
    std::chrono::duration<double> elapsed_seconds = end-start;
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);
    std::cout << "finished " << msg << " in " << elapsed_seconds.count() << "s" << std::endl;
    }
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


void calc_br_ucb(PolicyVec& opponent_policy, long int num_iterations, char br_player, std::vector<std::string>& player_information_sets, std::vector<std::string>& opponent_information_sets,  int log_flag, int log_frequency, int C, PolicyVec& opponent_ucb_policy, PolicyVec& player_ucb_policy) {
    std::vector<long int> oppo_infoset_time_steps(opponent_information_sets.size(), 0);
    std::vector<std::vector<double>> oppo_infoset_ucb_values(opponent_information_sets.size(), std::vector<double>(13, 0.0));
    std::vector<std::vector<double>> oppo_infoset_empirical_reward(opponent_information_sets.size(), std::vector<double>(13, 0.0));
    std::vector<std::vector<long int>> oppo_infoset_pull_count(opponent_information_sets.size(), std::vector<long int>(13, 0));
    std::vector<long int> player_infoset_time_steps(player_information_sets.size(), 0);
    std::vector<std::vector<double>> player_infoset_ucb_values(player_information_sets.size(), std::vector<double>(13, 0.0));
    std::vector<std::vector<double>> player_infoset_empirical_reward(player_information_sets.size(), std::vector<double>(13, 0.0));
    std::vector<std::vector<long int>> player_infoset_pull_count(player_information_sets.size(), std::vector<long int>(13, 0));

    for (long int t = 0; t < num_iterations; t++) {
        std::vector<int> h = {};
        TerminalHistory start_history = TerminalHistory(h);
        double reward = 0.0;
        sample_terminal_history_wrapper(player_ucb_policy, opponent_policy, start_history, reward, br_player);
        // update ucb values
        update_ucb(oppo_infoset_ucb_values, oppo_infoset_empirical_reward, oppo_infoset_pull_count, oppo_infoset_time_steps, opponent_ucb_policy, reward, start_history, toggle_player(br_player), C);
        update_ucb(player_infoset_ucb_values, player_infoset_empirical_reward, player_infoset_pull_count, player_infoset_time_steps, player_ucb_policy, 0.0-reward, start_history, br_player, C);

        if (t % log_frequency == 0 && t != 0){
            double expected_utility = 0.0;
            PolicyVec br_policy(br_player, player_information_sets);
            compute_best_response_wrapper(opponent_ucb_policy, br_policy, br_player);
            if (br_player == 'x'){
                expected_utility = get_expected_utility_wrapper(br_policy, opponent_policy);
            }
            else {
                expected_utility = get_expected_utility_wrapper(opponent_policy, br_policy);
            }
            std::cout << "Expected utility after iteration " << t << ": " << expected_utility << std::endl;
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
        int log_frequency = 10000;
        char player = 'x';
        int C = 1;
        std::cout << "Enter number of iterations: ";
        std::cin >> num_iterations;
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
            calc_br_ucb(policy_obj_o, num_iterations, player, P1_information_sets, P2_information_sets, log_flag, log_frequency, C, start_policy_obj_o, start_policy_obj_x);
        }
        else {
            calc_br_ucb(policy_obj_x, num_iterations, player, P2_information_sets, P1_information_sets, log_flag, log_frequency, C, start_policy_obj_x, start_policy_obj_o);
        }
        std::cout << "Continue experiments? (y/n): ";
        std::cin >> continue_exp;
    }
    
}
