#include "cpp_headers/rbt_classes.hpp"
#include "cpp_headers/rbt_utilities.hpp"
#include <random>
int NUM_THREADS = 96;

int sampleIndex(const std::vector<double>& probabilities) {
    std::random_device rd;
    std::mt19937 generator(rd());
    std::discrete_distribution<int> distribution(probabilities.begin(), probabilities.end());
    return distribution(generator);
}

double sample_terminal_history(InformationSet& I_1, InformationSet& I_2, TicTacToeBoard& true_board, PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, History& current_history, char player, double probability, double& reward, char update_player, double eps) {
    InformationSet& I = player == 'x' ? I_1 : I_2;
    PolicyVec& policy_obj = player == 'x' ? policy_obj_x : policy_obj_o;
    std::vector<double> prob_dist = policy_obj.policy_dict[I.get_index()];

    if (player == update_player) { // explore with a small epsilon
        std::vector<int> actions;
        I.get_actions(actions);
        double sum = 1.0;

        for (int i = 0; i < actions.size(); i++) {
            if (prob_dist[actions[i]] == 0.0) {
                prob_dist[actions[i]] = eps;
                sum += eps;
            }
        }
        // renormalize
        for (int i = 0; i < actions.size(); i++) {
            prob_dist[actions[i]] /= sum;
        }
    }

    int action = sampleIndex(prob_dist);

    if (I.move_flag) {
        bool success = true_board.update_move(action, player);

        if (player == update_player) { // update the probability only if the player is the one we are updating
            probability = probability * prob_dist[action];
        }

        current_history.history.push_back(action);

        char winner;
        if (success && !true_board.is_win(winner) && !true_board.is_over()) {
            InformationSet new_I = I;
            new_I.update_move(action, player);
            new_I.reset_zeros();

            if (player == 'x') {
                return sample_terminal_history(new_I, I_2, true_board, policy_obj_x, policy_obj_o, current_history, 'o', probability, reward, update_player, eps);
            } else {
                return sample_terminal_history(I_1, new_I, true_board, policy_obj_x, policy_obj_o, current_history, 'x', probability, reward, update_player, eps);
            }
        } else {
            TerminalHistory H_T = TerminalHistory(current_history.history);
            H_T.set_reward();
            if (update_player == 'x'){
                reward = (double) H_T.reward[0];
            } else {
                reward = (double) H_T.reward[1];
            }
            return probability;
        }
    }
    else {
        InformationSet new_I = I;
        new_I.simulate_sense(action, true_board);
        
        if (player == update_player) { // update the probability only if the player is the one we are updating
            probability = probability * prob_dist[action];
        }
        current_history.history.push_back(action);

        if (player == 'x') {
            return sample_terminal_history(new_I, I_2, true_board, policy_obj_x, policy_obj_o, current_history, 'x', probability, reward, update_player, eps);
        } else {
            return sample_terminal_history(I_1, new_I, true_board, policy_obj_x, policy_obj_o, current_history, 'o', probability, reward, update_player, eps);
        }
    }
}


double sample_terminal_history_wrapper(PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, History& current_history, double& reward, char update_player, double eps) {
    std::string board = "000000000";
    TicTacToeBoard true_board = TicTacToeBoard(board);
    std::string hash_1 = "";
    std::string hash_2 = "";
    InformationSet I_1 = InformationSet('x', true, hash_1);
    InformationSet I_2 = InformationSet('o', false, hash_2);
    return sample_terminal_history(I_1, I_2, true_board, policy_obj_x, policy_obj_o, current_history, 'x', 1.0, reward, update_player, eps);
}


double compute_regrets_along_history(InformationSet& I_1, InformationSet& I_2, TicTacToeBoard& true_board, 
                                        PolicyVec& player_br_policy, PolicyVec& player_cumulative_strategy, char br_player, 
                                        long int t, double forward_reach, std::vector<std::vector<double>>& regret_list, 
                                        std::vector<long int>& markers, History& current_history, double q_z, double reward, 
                                        int traversal_index, char current_player) {
    if (traversal_index == current_history.history.size()) {
        return 1.0;
    }

    InformationSet& I = current_player == 'x' ? I_1 : I_2;
    int action = current_history.history[traversal_index]; 
    traversal_index += 1;

    if (current_player == br_player){
        std::vector<int> actions;
        I.get_actions(actions);
        std::vector<double>& br_prob_dist = player_br_policy.policy_dict[I.get_index()];
        std::vector<double>& cumulative_prob_table = player_cumulative_strategy.policy_dict[I.get_index()];
        std::vector<double>& regret_I = regret_list[I.get_index()];
        double played_action_prob = br_prob_dist[action];
        double reach_prob = 0.0;

        if (I.move_flag) {
            true_board.update_move(action, current_player);
            InformationSet new_I = I;
            new_I.update_move(action, current_player);
            new_I.reset_zeros();

            if (current_player == 'x') {
                reach_prob = compute_regrets_along_history(new_I, I_2, true_board, player_br_policy, player_cumulative_strategy, br_player, t, forward_reach * played_action_prob, regret_list, markers, current_history, q_z, reward, traversal_index, 'o');
            } else {
                reach_prob = compute_regrets_along_history(I_1, new_I, true_board, player_br_policy, player_cumulative_strategy, br_player, t, forward_reach * played_action_prob, regret_list, markers, current_history, q_z, reward, traversal_index, 'x');
            }
        }
        else {
            InformationSet new_I = I;
            new_I.simulate_sense(action, true_board);

            if (current_player == 'x') {
                reach_prob = compute_regrets_along_history(new_I, I_2, true_board, player_br_policy, player_cumulative_strategy, br_player, t, forward_reach * played_action_prob, regret_list, markers, current_history, q_z, reward, traversal_index, 'x');
            } else {
                reach_prob = compute_regrets_along_history(I_1, new_I, true_board, player_br_policy, player_cumulative_strategy, br_player, t, forward_reach * played_action_prob, regret_list, markers, current_history, q_z, reward, traversal_index, 'o');
            }
        }

        double regret_sum = 0.0;

        for (int i = 0; i < actions.size(); i++) {
            if (actions[i] == action) {
                if (played_action_prob > 0) {
                    regret_I[actions[i]] += (reward * reach_prob * (1 - played_action_prob)) / q_z;
                }
                else {
                    regret_I[actions[i]] += (reward * reach_prob) / (q_z);
                }
            } else {
                regret_I[actions[i]] += -reward * reach_prob * played_action_prob / q_z;
            }

            cumulative_prob_table[actions[i]] += (t - markers[I.get_index()]) * br_prob_dist[actions[i]] * forward_reach;
            regret_sum += regret_I[actions[i]] > 0 ? regret_I[actions[i]] : 0;
        }

        markers[I.get_index()] = t;
        // regret matching
        for (int i = 0; i < actions.size(); i++) {
            if (regret_sum > 0) {
                br_prob_dist[actions[i]] = regret_I[actions[i]] > 0 ? regret_I[actions[i]] / regret_sum : 0.0;
            } else {
                br_prob_dist[actions[i]] = 1.0 / actions.size();
            }
        }

        return reach_prob*played_action_prob;
    }
    else {
        if (I.move_flag) {
            true_board.update_move(action, current_player);
            I.update_move(action, current_player);
            I.reset_zeros();

            if (current_player == 'x') {
                return compute_regrets_along_history(I, I_2, true_board, player_br_policy, player_cumulative_strategy, br_player, t, forward_reach, regret_list, markers, current_history, q_z, reward, traversal_index, 'o');
            } else {
                return compute_regrets_along_history(I_1, I, true_board, player_br_policy, player_cumulative_strategy, br_player, t, forward_reach, regret_list, markers, current_history, q_z, reward, traversal_index, 'x');
            }
        }
        else {
            I.simulate_sense(action, true_board);

            if (current_player == 'x') {
                return compute_regrets_along_history(I, I_2, true_board, player_br_policy, player_cumulative_strategy, br_player, t, forward_reach, regret_list, markers, current_history, q_z, reward, traversal_index, 'x');
            } else {
                return compute_regrets_along_history(I_1, I, true_board, player_br_policy, player_cumulative_strategy, br_player, t, forward_reach, regret_list, markers, current_history, q_z, reward, traversal_index, 'o');
            }
        }
    }
}

void compute_regrets_along_history_wrapper(PolicyVec& player_br_policy, PolicyVec& player_cumulative_strategy, char br_player, 
                                            long int t, std::vector<std::vector<double>>& regret_list, 
                                            std::vector<long int>& markers, History& start_history, double q_z, double reward){
    std::string board = "000000000";
    TicTacToeBoard true_board = TicTacToeBoard(board);
    std::string hash_1 = "";
    std::string hash_2 = "";
    InformationSet I_1 = InformationSet('x', true, hash_1);
    InformationSet I_2 = InformationSet('o', false, hash_2);
    compute_regrets_along_history(I_1, I_2, true_board, player_br_policy, player_cumulative_strategy, br_player, t, 1.0, regret_list, markers, start_history, q_z, reward, 0, 'x');        

} 

void mccfr_outcome_sampling_best_response(PolicyVec& opponent_policy, PolicyVec& player_br_policy, char br_player, std::vector<std::string>& player_information_sets, long int T, double eps, long int step_size) {
    std::vector<std::vector<double>> regret_list;
    std::vector<long int> markers;
    PolicyVec cumulative_strategy;
    cumulative_strategy.player = br_player;
    
    for (long int i = 0; i < player_information_sets.size(); i++) {
        regret_list.push_back(std::vector<double>(13, 0.0));

        std::vector<double> probability_dist(13, 0.0);
        cumulative_strategy.policy_dict.push_back(probability_dist);
        markers.push_back(0);
    }

    for (int t = 0; t < T; t++) {
        std::vector<int> h = {};
        TerminalHistory start_history = TerminalHistory(h);
        double q_z = 0.0;
        double reward = 0;

        if (br_player == 'x') {
            q_z = sample_terminal_history_wrapper(player_br_policy, opponent_policy, start_history, reward, br_player, eps);
        } else {
            q_z = sample_terminal_history_wrapper(opponent_policy, player_br_policy, start_history, reward, br_player, eps);
        }

        // traverse history and update regrets
        compute_regrets_along_history_wrapper(player_br_policy, cumulative_strategy, br_player, t, regret_list, markers, start_history, q_z, reward);
        
        if (t % step_size == 0 && t != 0) {
            // overridde eps based on step size.
            // eps = 1.0/(((t*1.0)/(step_size*1.0))+1.0);

            PolicyVec average_strategy = cumulative_strategy;
            // normalize the cumulative strategy
            #pragma omp parallel for num_threads(NUM_THREADS)
            for (long int i = 0; i < player_information_sets.size(); i++) {
                std::vector<double>& cumulative_prob_table = average_strategy.policy_dict[i];
                double sum = 0.0;

                for (int j = 0; j < 13; j++) {
                    sum += cumulative_prob_table[j];
                }

                if (sum > 0) {
                    for (int j = 0; j < 13; j++) {
                        cumulative_prob_table[j] /= sum;
                    }
                }
            }

            // build br policy
            PolicyVec br_policy = player_br_policy;
            #pragma omp parallel for num_threads(NUM_THREADS)
            for (long int i = 0; i < player_information_sets.size(); i++) {
                std::vector<double>& br_prob_dist = br_policy.policy_dict[i];
                InformationSet I(br_player, get_move_flag(player_information_sets[i], br_player), player_information_sets[i]);
                std::vector<double>& regret_I = regret_list[I.get_index()];
                double max_regret = -1.0;
                int max_regret_action = -1;
                std::vector<int> actions;
                I.get_actions(actions);

                for (int a: actions) {
                    if (regret_I[a] > max_regret) {
                        max_regret = regret_I[a];
                        max_regret_action = a;
                    }
                }

                for (int a: actions) {
                    if (a == max_regret_action) {
                        br_prob_dist[a] = 1.0;
                    } else {
                        br_prob_dist[a] = 0.0;
                    }
                }
            }

            double expected_utility = 0.0;
            if (br_player == 'x'){
                expected_utility = get_expected_utility_wrapper(player_br_policy, opponent_policy);
                std::cout << "Expected utility after iteration " << t << ": " << expected_utility << std::endl;
                expected_utility = get_expected_utility_wrapper(average_strategy, opponent_policy);
                std::cout << "Expected utility after averaging: " << expected_utility << std::endl;
                expected_utility = get_expected_utility_wrapper(br_policy, opponent_policy);
                std::cout << "Expected utility after deterministic best response: " << expected_utility << std::endl;
            }
            else {
                expected_utility = get_expected_utility_wrapper(opponent_policy, player_br_policy);
                std::cout << "Expected utility after iteration " << t << ": " << expected_utility << std::endl;
                expected_utility = get_expected_utility_wrapper(opponent_policy, average_strategy);
                std::cout << "Expected utility after averaging: " << expected_utility << std::endl;
                expected_utility = get_expected_utility_wrapper(opponent_policy, br_policy);
                std::cout << "Expected utility after deterministic best response: " << expected_utility << std::endl;
            }
        }
    }
}

int main(int argc, char* argv[]) {
    std::cout.precision(17);
    std::string file_path_1 = argv[1];
    std::string file_path_2 = argv[2];
    NUM_THREADS = std::stoi(argv[3]); //96;
    std::vector<std::string> P1_information_sets;
    std::vector<std::string> P2_information_sets;
    std::string P1_information_sets_file = "data/P1_information_sets_V2.txt";
    std::string P2_information_sets_file = "data/P2_information_sets_V2.txt";

    // read the P1 information sets
    std::ifstream P1_f_is(P1_information_sets_file);
    std::string P1_line_is;
    while (std::getline(P1_f_is, P1_line_is)) {
        P1_information_sets.push_back(P1_line_is);
    }
    P1_f_is.close();
    // read the P2 information sets
    std::ifstream P2_f_is(P2_information_sets_file);
    std::string P2_line_is;
    while (std::getline(P2_f_is, P2_line_is)) {
        P2_information_sets.push_back(P2_line_is);
    }
    P2_f_is.close();
    // create hash to int maps (for performance reasons)
    for (long int i = 0; i < P1_information_sets.size(); i++) {
        InformationSet::P1_hash_to_int_map[P1_information_sets[i]] = i;
    }
    for (long int i = 0; i < P2_information_sets.size(); i++) {
        InformationSet::P2_hash_to_int_map[P2_information_sets[i]] = i;
    }

    std::cout << "Loading start policies..." << std::endl;
    PolicyVec policy_obj_x('x', file_path_1, true);
    PolicyVec policy_obj_o('o', file_path_2, true);
    std::cout << "Start policies loaded." << std::endl;

    char continue_exp = 'y';
    while (continue_exp == 'y') {
        double eps = 0.0;
        long int num_iterations = 0;
        long int step_size = 0;
        char player;

        std::cout << "Enter number of iterations: ";
        std::cin >> num_iterations;
        std::cout << "Enter the number of iterations after which progress is to be checked: ";
        std::cin >> step_size;
        std::cout << "Enter the player for whom the best response is to be computed (x/o):";
        std::cin >> player;
        std::cout << "Enter value of epsilon:";
        std::cin >> eps; 

        if (player == 'x'){
            PolicyVec player_br_policy = policy_obj_x;
            mccfr_outcome_sampling_best_response(policy_obj_o, player_br_policy, 'x', P1_information_sets,  num_iterations, eps, step_size);
        }
        else if (player == 'o'){
            PolicyVec player_br_policy = policy_obj_o;
            mccfr_outcome_sampling_best_response(policy_obj_x, player_br_policy, 'o', P2_information_sets, num_iterations, eps, step_size);
        }
       
        std::cout << "Continue experiments? (y/n): ";
        std::cin >> continue_exp;
    }
    
}