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

void sample_terminal_history(InformationSet& I_1, InformationSet& I_2, TicTacToeBoard& true_board, PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, PolicyVec& player_uniform_policy, History& current_history, char player, double& reward, char update_player, double& eps, double& reach_probability_explore, double& reach_probability_exploit, int& explore_or_exploit_flag) {
    InformationSet& I = player == 'x' ? I_1 : I_2;
    PolicyVec& policy_obj = player == 'x' ? policy_obj_x : policy_obj_o;
    std::vector<double> prob_dist = policy_obj.policy_dict[I.get_index()];
    std::vector<double> uniform_prob_dist = player_uniform_policy.policy_dict[I.get_index()];
    int action = -1;

    if (player == update_player) { // explore with a small epsilon
        std::vector<int> actions;
        I.get_actions(actions);
        if (explore_or_exploit_flag == 1){
            action = sampleIndex(prob_dist);
        }
        else{
            action = sampleIndex(uniform_prob_dist);
        }
        
        reach_probability_exploit *= prob_dist[action];
        reach_probability_explore *= uniform_prob_dist[action];
    }
    else{
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
                sample_terminal_history(new_I, I_2, true_board, policy_obj_x, policy_obj_o, player_uniform_policy, current_history, 'o', reward, update_player, eps, reach_probability_explore, reach_probability_exploit, explore_or_exploit_flag);
            } else {
                sample_terminal_history(I_1, new_I, true_board, policy_obj_x, policy_obj_o, player_uniform_policy, current_history, 'x', reward, update_player, eps, reach_probability_explore, reach_probability_exploit, explore_or_exploit_flag);
            }
        } else {
            TerminalHistory H_T = TerminalHistory(current_history.history);
            H_T.set_reward();
            if (update_player == 'x'){
                reward = (double) H_T.reward[0];
            } else {
                reward = (double) H_T.reward[1];
            }
        }
    }
    else {
        InformationSet new_I = I;
        new_I.simulate_sense(action, true_board);
        current_history.history.push_back(action);

        if (player == 'x') {
            sample_terminal_history(new_I, I_2, true_board, policy_obj_x, policy_obj_o, player_uniform_policy, current_history, 'x', reward, update_player, eps, reach_probability_explore, reach_probability_exploit, explore_or_exploit_flag);
        } else {
            sample_terminal_history(I_1, new_I, true_board, policy_obj_x, policy_obj_o, player_uniform_policy, current_history, 'o', reward, update_player, eps, reach_probability_explore, reach_probability_exploit, explore_or_exploit_flag);
        }
    }
}


double sample_terminal_history_wrapper(PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, PolicyVec& player_uniform_policy, History& current_history, double& reward, char update_player, double eps, int explore_or_exploit_flag) {
    std::string board = "000000000";
    TicTacToeBoard true_board = TicTacToeBoard(board);
    std::string hash_1 = "";
    std::string hash_2 = "";
    InformationSet I_1 = InformationSet('x', true, hash_1);
    InformationSet I_2 = InformationSet('o', false, hash_2);
    double reach_probability_explore = 1.0;
    double reach_probability_exploit = 1.0;
    sample_terminal_history(I_1, I_2, true_board, policy_obj_x, policy_obj_o, player_uniform_policy, current_history, 'x', reward, update_player, eps, reach_probability_explore, reach_probability_exploit, explore_or_exploit_flag);
    return (1-eps)*reach_probability_exploit + eps*reach_probability_explore;
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
                regret_I[actions[i]] += (reward * reach_prob * (1 - played_action_prob)) / q_z;
            } 
            else {
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

void upfront_flipping_best_response(PolicyVec& opponent_policy, PolicyVec& player_br_policy, PolicyVec& player_uniform_policy, char br_player, std::vector<std::string>& player_information_sets, long int T, long int step_size, double exact_br_value, int experiment_number, long int log_size) {
    std::vector<std::vector<double>> regret_list;
    std::vector<long int> markers;
    PolicyVec cumulative_strategy;
    cumulative_strategy.player = br_player;
    long int explore_count = 0;
    long int exploit_count = 0;
    std::vector<std::pair<int, double>> exploitability_log;
    
    for (long int i = 0; i < player_information_sets.size(); i++) {
        regret_list.push_back(std::vector<double>(13, 0.0));

        std::vector<double> probability_dist(13, 0.0);
        cumulative_strategy.policy_dict.push_back(probability_dist);
        markers.push_back(0);
    }

    for (int t = 0; t < T; t++) {
        double eps = 1.0/(((t*1.0)/(step_size*1.0))+1.0);
        std::vector<int> h = {};
        TerminalHistory start_history = TerminalHistory(h);
        double q_z = 0.0;
        double reward = 0;

        int explore_or_exploit = 0;
        std::vector<double> eps_prob_dist = {eps, 1-eps};
        if (sampleIndex(eps_prob_dist)){
            explore_or_exploit = 1;
            exploit_count += 1;
        }
        else{
            explore_count += 1;
        }

        if (br_player == 'x') {
            q_z = sample_terminal_history_wrapper(player_br_policy, opponent_policy, player_uniform_policy, start_history, reward, br_player, eps, explore_or_exploit);
        } 
        else {
            q_z = sample_terminal_history_wrapper(opponent_policy, player_br_policy, player_uniform_policy, start_history, reward, br_player, eps, explore_or_exploit);
        }

        // traverse history and update regrets
        compute_regrets_along_history_wrapper(player_br_policy, cumulative_strategy, br_player, t, regret_list, markers, start_history, q_z, reward);

        if (t % log_size == 0 && t != 0) {
            double expected_utility = 0.0;
            std::cout << "############################################################" << std::endl;
            if (br_player == 'x'){
                expected_utility = get_expected_utility_wrapper(player_br_policy, opponent_policy);
                exploitability_log.push_back(std::make_pair(t, exact_br_value - expected_utility));
                std::cout << "Expected utility after iteration " << t << ": " << expected_utility << std::endl;
            }
            else {
                expected_utility = get_expected_utility_wrapper(opponent_policy, player_br_policy);
                exploitability_log.push_back(std::make_pair(t, exact_br_value - expected_utility));
                std::cout << "Expected utility after iteration " << t << ": " << expected_utility << std::endl;
            }
            std::cout << "############################################################" << std::endl;
        }
    }

    std::cout << "Saving exploitability log" << std::endl;
    std::string file_name = "data/" + std::string(1, br_player) + "upfront_flipping_exploitability_log_" + std::to_string(experiment_number) + ".txt";

    std::ofstream f(file_name);
    for (int i = 0; i < exploitability_log.size(); i++) {
        f << exploitability_log[i].first << " " << exploitability_log[i].second << std::endl;
    }
    f.close();
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
    PolicyVec uniform_policy_obj_x('x', P1_information_sets);
    PolicyVec uniform_policy_obj_o('o', P2_information_sets);
    PolicyVec br_x('x', P1_information_sets);
    PolicyVec br_o('o', P2_information_sets);

    char continue_exp = 'y';
    while (continue_exp == 'y') {
        long int num_iterations = 0;
        long int step_size = 0;
        char player;
        int experiment_number = 1;
        int num_experiments = 0;
        long int log_size = 1;

        std::cout << "Enter number of iterations: ";
        std::cin >> num_iterations;
        std:: cout << "Enter step size for eps decay";
        std::cin >> step_size;
        std::cout << "Enter the number of iterations after which progress is to be checked: ";
        std::cin >> log_size;
        std::cout << "Enter the player for whom the best response is to be computed (x/o):";
        std::cin >> player;
        std::cout << "Enter number of experiments: ";
        std::cin >> num_experiments;

        double expected_utility = 0.0;
        if (player == 'x'){
            expected_utility = compute_best_response_wrapper(policy_obj_o, br_x, 'x');
        }
        else if (player == 'o'){
            expected_utility = compute_best_response_wrapper(policy_obj_x, br_o, 'o');
        }

        while (experiment_number <= num_experiments){
            if (player == 'x'){
                PolicyVec player_br_policy = policy_obj_x;
                upfront_flipping_best_response(policy_obj_o, player_br_policy, uniform_policy_obj_x, 'x', P1_information_sets,  num_iterations, step_size, expected_utility, experiment_number, log_size);
            }
            else if (player == 'o'){
                PolicyVec player_br_policy = policy_obj_o;
                upfront_flipping_best_response(policy_obj_x, player_br_policy, uniform_policy_obj_o, 'o', P2_information_sets, num_iterations, step_size, expected_utility, experiment_number, log_size);
            }
            experiment_number += 1;
        }
       
        std::cout << "Continue experiments? (y/n): ";
        std::cin >> continue_exp;
    }
    
}