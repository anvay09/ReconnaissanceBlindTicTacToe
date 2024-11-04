#include "cpp_headers/rbt_classes.hpp"
#include "cpp_headers/rbt_utilities.hpp"
#include <random>


int sampleIndex(const std::vector<double>& probabilities) {
    std::random_device rd;
    std::mt19937 generator(rd());
    std::discrete_distribution<int> distribution(probabilities.begin(), probabilities.end());
    return distribution(generator);
}


double sample_terminal_history(InformationSet& I_1, InformationSet& I_2, TicTacToeBoard& true_board, PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, History& current_history, char player, double probability, double& reward, char update_player, PolicyVec& oppo_cumulative_sample_count, PolicyVec& oppo_strategy) {
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
            oppo_cumulative_sample_count.policy_dict[I.get_index()][action] += 1.0;
            double sum_infoset_sample_count = 0.0;
            for (int i = 0; i < 13; i++) {
                sum_infoset_sample_count += oppo_cumulative_sample_count.policy_dict[I.get_index()][i];
            }
            for (int i = 0; i < 13; i++) {
                oppo_strategy.policy_dict[I.get_index()][i] = oppo_cumulative_sample_count.policy_dict[I.get_index()][i] / sum_infoset_sample_count;
            }
        }

        current_history.history.push_back(action);

        char winner;
        if (success && !true_board.is_win(winner) && !true_board.is_over()) {
            InformationSet new_I = I;
            new_I.update_move(action, player);
            new_I.reset_zeros();

            if (player == 'x') {
                return sample_terminal_history(new_I, I_2, true_board, policy_obj_x, policy_obj_o, current_history, 'o', probability, reward, update_player, oppo_cumulative_sample_count, oppo_strategy);
            } else {
                return sample_terminal_history(I_1, new_I, true_board, policy_obj_x, policy_obj_o, current_history, 'x', probability, reward, update_player, oppo_cumulative_sample_count, oppo_strategy);
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
            oppo_cumulative_sample_count.policy_dict[I.get_index()][action] += 1.0;
            double sum_infoset_sample_count = 0.0;
            for (int i = 0; i < 13; i++) {
                sum_infoset_sample_count += oppo_cumulative_sample_count.policy_dict[I.get_index()][i];
            }
            for (int i = 0; i < 13; i++) {
                oppo_strategy.policy_dict[I.get_index()][i] = oppo_cumulative_sample_count.policy_dict[I.get_index()][i] / sum_infoset_sample_count;
            }
        }
        current_history.history.push_back(action);

        if (player == 'x') {
            return sample_terminal_history(new_I, I_2, true_board, policy_obj_x, policy_obj_o, current_history, 'x', probability, reward, update_player, oppo_cumulative_sample_count, oppo_strategy);
        } else {
            return sample_terminal_history(I_1, new_I, true_board, policy_obj_x, policy_obj_o, current_history, 'o', probability, reward, update_player, oppo_cumulative_sample_count, oppo_strategy);
        }
    }
}


double sample_terminal_history_wrapper(PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, History& current_history, double& reward, char update_player, PolicyVec& oppo_cumulative_sample_count, PolicyVec& oppo_strategy) {
    std::string board = "000000000";
    TicTacToeBoard true_board = TicTacToeBoard(board);
    std::string hash_1 = "";
    std::string hash_2 = "";
    InformationSet I_1 = InformationSet('x', true, hash_1);
    InformationSet I_2 = InformationSet('o', false, hash_2);
    return sample_terminal_history(I_1, I_2, true_board, policy_obj_x, policy_obj_o, current_history, 'x', 1.0, reward, update_player, oppo_cumulative_sample_count, oppo_strategy);
}


void calc_epsilon_best_response(PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, PolicyVec& uniform_strategy_x, PolicyVec& uniform_strategy_o, std::vector<std::string>& P1_information_sets, std::vector<std::string>& P2_information_sets, char player, int T, int update_step_size) {
    PolicyVec player_strategy = player == 'x' ? uniform_strategy_x : uniform_strategy_o;
    PolicyVec opponent_strategy = player == 'x' ? uniform_strategy_o : uniform_strategy_x;
    PolicyVec opponent_cumulative_sample_count;
    player_strategy.player = player;
    opponent_strategy.player = player == 'x' ? 'o' : 'x';
    opponent_cumulative_sample_count.player = player == 'x' ? 'o' : 'x';
    std::vector<std::string> player_information_sets = player == 'x' ? P1_information_sets : P2_information_sets;
    std::vector<std::string> opponent_information_sets = player == 'x' ? P2_information_sets : P1_information_sets;

    
    // for (long int i = 0; i < player_information_sets.size(); i++) {
    //     std::vector<double> probability_dist(13, 0.0);
    //     player_strategy.policy_dict.push_back(probability_dist);
    // }

    for (long int i = 0; i < opponent_information_sets.size(); i++) {
        std::vector<double> probability_dist(13, 0.0);
        // opponent_strategy.policy_dict.push_back(probability_dist);
        opponent_cumulative_sample_count.policy_dict.push_back(probability_dist);
    }

    for (int t = 0; t < T; t++) {
        std::vector<int> h = {};
        TerminalHistory start_history = TerminalHistory(h);
        double q_z = 0.0;
        double reward = 0;
        double eps = 1.0/(t+1.0);
        std::vector<double> prob_dist = {eps, 1-eps};
        if (sampleIndex(prob_dist)){
            if (player == 'x') {
                q_z = sample_terminal_history_wrapper(player_strategy, policy_obj_o, start_history, reward, player, opponent_cumulative_sample_count, opponent_strategy);
            }
            else {
                q_z = sample_terminal_history_wrapper(policy_obj_x, player_strategy, start_history, reward, player, opponent_cumulative_sample_count, opponent_strategy);
            }
        }
        else {
            if (player == 'x') {
                q_z = sample_terminal_history_wrapper(uniform_strategy_x, policy_obj_o, start_history, reward, player, opponent_cumulative_sample_count, opponent_strategy);
            }
            else {
                q_z = sample_terminal_history_wrapper(policy_obj_x, uniform_strategy_o, start_history, reward, player, opponent_cumulative_sample_count, opponent_strategy);
            }
        }
        if (t % update_step_size == 0) {
            double expected_utility = 0.0;
            expected_utility = compute_best_response_wrapper(opponent_strategy, player_strategy, player);
            if (player == 'x'){
                expected_utility = get_expected_utility_wrapper(player_strategy, policy_obj_o);
            }
            else {
                expected_utility = get_expected_utility_wrapper(policy_obj_x, player_strategy);
            }
            std::cout << "Expected utility after iteration " << t << ": " << expected_utility << std::endl;
        }
    }
}


int main(int argc, char* argv[]) {
    std::cout.precision(17);
    std::string file_path_1 = argv[1];
    std::string file_path_2 = argv[2];
    std::string uniform_file_path_1 = argv[3];
    std::string uniform_file_path_2 = argv[4];

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
    PolicyVec policy_obj_x('x', file_path_1);
    PolicyVec policy_obj_o('o', file_path_2);
    PolicyVec uniform_policy_obj_x('x', uniform_file_path_1);
    PolicyVec uniform_policy_obj_o('o', uniform_file_path_2);
    PolicyVec br_x('x', P1_information_sets);
    PolicyVec br_o('o', P2_information_sets);
    std::cout << "Policies loaded." << std::endl;

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
            double expected_utility = compute_best_response_wrapper(policy_obj_o, br_x, 'x');
            std::cout << "Expected utility of the best response x: " << expected_utility << std::endl;
        }
        else {
            // compute expected utility of the best response (i.e., when opponent policy is known)
            double expected_utility = compute_best_response_wrapper(policy_obj_x, br_o, 'o');
            std::cout << "Expected utility of the best response o: " << expected_utility << std::endl;
        }

        calc_epsilon_best_response(policy_obj_x, policy_obj_o, uniform_policy_obj_x, uniform_policy_obj_o, P1_information_sets, P2_information_sets, player, num_iterations, update_step_size);

        std::cout << "Continue experiments? (y/n): ";
        std::cin >> continue_exp;
    }
    
}