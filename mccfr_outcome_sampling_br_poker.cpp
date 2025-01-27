#include "cpp_headers/poker_classes.hpp"
#include "cpp_headers/poker_utilities.hpp"
#include <random>
int NUM_THREADS = 4;

int sampleIndex(const std::vector<double>& probabilities) {
    std::random_device rd;
    std::mt19937 generator(rd());
    std::discrete_distribution<int> distribution(probabilities.begin(), probabilities.end());
    return distribution(generator);
}

double sample_terminal_history(InformationSet& I_1, InformationSet& I_2, PokerTable& true_cards, PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, History& current_history, double probability, double& reward, char update_player, double eps) {
    InformationSet I = true_cards.player_to_move == 'x' ? I_1 : I_2;
    PolicyVec& policy_obj = I.player == 'x' ? policy_obj_x : policy_obj_o;
    std::vector<double> prob_dist = policy_obj.policy_dict[I.get_index()];

    if (I.player == update_player) { // explore with a small epsilon
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
        bool success = true_cards.update_move(action);

        if (I.player == update_player) { // update the probability only if the player is the one we are updating
            probability = probability * prob_dist[action];
        }

        current_history.history.push_back(action);

        char winner;
        if (success && !true_cards.is_win(winner) && !true_cards.is_over()) {
            InformationSet new_I = I;
            new_I.update_move(action);

            if (I.player == 'x') {
                return sample_terminal_history(new_I, I_2, true_cards, policy_obj_x, policy_obj_o, current_history, probability, reward, update_player, eps);
            } else {
                return sample_terminal_history(I_1, new_I, true_cards, policy_obj_x, policy_obj_o, current_history, probability, reward, update_player, eps);
            }
        } else {
            TerminalHistory H_T = TerminalHistory(current_history.history);
            H_T.set_reward(I.game);
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
        new_I.simulate_sense(action, true_cards);
        
        if (I.player == update_player) { // update the probability only if the player is the one we are updating
            probability = probability * prob_dist[action];
        }
        current_history.history.push_back(action);

        if (I.player == 'x') {
            return sample_terminal_history(new_I, I_2, true_cards, policy_obj_x, policy_obj_o, current_history, probability, reward, update_player, eps);
        } else {
            return sample_terminal_history(I_1, new_I, true_cards, policy_obj_x, policy_obj_o, current_history, probability, reward, update_player, eps);
        }
    }
}


double sample_terminal_history_wrapper(PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, History& current_history, double& reward, char update_player, double eps, char game) {
    std::vector<std::string>& unique_draws = game == 'L' ? unique_draws_leduc : unique_draws_kuhn;
    std::vector<double>& draw_probabilities = game == 'L' ? draw_probabilities_leduc : draw_probabilities_kuhn;
    
    std::random_device rd;
    std::mt19937 generator(rd());
    std::discrete_distribution<int> distribution(draw_probabilities.begin(), draw_probabilities.end());
    int draw_index = distribution(generator);

    std::string cards = unique_draws[draw_index];
    PokerTable true_cards = PokerTable(cards);
    true_cards.game = game;
    std::string hash_1 = "a-" + std::string(1, cards[0]) + "--";
    std::string hash_2 = "o-" + std::string(1, cards[1]) + "--";
    InformationSet I_1 = InformationSet('x', true, hash_1, game);
    InformationSet I_2 = InformationSet('o', false, hash_2, game);

    current_history.history.push_back(cards[0]);
    current_history.history.push_back(cards[1]);
    current_history.history.push_back(cards[2]);
  
    return sample_terminal_history(I_1, I_2, true_cards, policy_obj_x, policy_obj_o, current_history, draw_probabilities[draw_index], reward, update_player, eps);
}


double compute_regrets_along_history(InformationSet& I_1, InformationSet& I_2, PokerTable& true_cards, 
                                        PolicyVec& player_br_policy, PolicyVec& player_cumulative_strategy, char br_player, 
                                        long int t, double forward_reach, std::vector<std::vector<double>>& regret_list, 
                                        std::vector<long int>& markers, History& current_history, double q_z, double reward, 
                                        int traversal_index) {
    if (traversal_index == current_history.history.size()) {
        return 1.0;
    }

    InformationSet& I = true_cards.player_to_move == 'x' ? I_1 : I_2;
    int action = current_history.history[traversal_index]; 
    traversal_index += 1;

    if (I.player == br_player){
        std::vector<int> actions;
        I.get_actions(actions);
        std::vector<double>& br_prob_dist = player_br_policy.policy_dict[I.get_index()];
        std::vector<double>& cumulative_prob_table = player_cumulative_strategy.policy_dict[I.get_index()];
        std::vector<double>& regret_I = regret_list[I.get_index()];
        double played_action_prob = br_prob_dist[action];
        double reach_prob = 0.0;

        if (I.move_flag) {
            true_cards.update_move(action);
            InformationSet new_I = I;
            new_I.update_move(action);

            if (I.player == 'x') {
                reach_prob = compute_regrets_along_history(new_I, I_2, true_cards, player_br_policy, player_cumulative_strategy, br_player, t, forward_reach * played_action_prob, regret_list, markers, current_history, q_z, reward, traversal_index);
            } else {
                reach_prob = compute_regrets_along_history(I_1, new_I, true_cards, player_br_policy, player_cumulative_strategy, br_player, t, forward_reach * played_action_prob, regret_list, markers, current_history, q_z, reward, traversal_index);
            }
        }
        else {
            InformationSet new_I = I;
            new_I.simulate_sense(action, true_cards);

            if (I.player == 'x') {
                reach_prob = compute_regrets_along_history(new_I, I_2, true_cards, player_br_policy, player_cumulative_strategy, br_player, t, forward_reach * played_action_prob, regret_list, markers, current_history, q_z, reward, traversal_index);
            } else {
                reach_prob = compute_regrets_along_history(I_1, new_I, true_cards, player_br_policy, player_cumulative_strategy, br_player, t, forward_reach * played_action_prob, regret_list, markers, current_history, q_z, reward, traversal_index);
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

        return reach_prob * played_action_prob;
    }
    else {
        if (I.move_flag) {
            true_cards.update_move(action);
            I.update_move(action);

            if (I.player == 'x') {
                return compute_regrets_along_history(I, I_2, true_cards, player_br_policy, player_cumulative_strategy, br_player, t, forward_reach, regret_list, markers, current_history, q_z, reward, traversal_index);
            } else {
                return compute_regrets_along_history(I_1, I, true_cards, player_br_policy, player_cumulative_strategy, br_player, t, forward_reach, regret_list, markers, current_history, q_z, reward, traversal_index);
            }
        }
        else {
            I.simulate_sense(action, true_cards);

            if (I.player == 'x') {
                return compute_regrets_along_history(I, I_2, true_cards, player_br_policy, player_cumulative_strategy, br_player, t, forward_reach, regret_list, markers, current_history, q_z, reward, traversal_index);
            } else {
                return compute_regrets_along_history(I_1, I, true_cards, player_br_policy, player_cumulative_strategy, br_player, t, forward_reach, regret_list, markers, current_history, q_z, reward, traversal_index);
            }
        }
    }
}

void compute_regrets_along_history_wrapper(PolicyVec& player_br_policy, PolicyVec& player_cumulative_strategy, char br_player, 
                                            long int t, std::vector<std::vector<double>>& regret_list, 
                                            std::vector<long int>& markers, History& start_history, double q_z, double reward, char game){
    std::string cards = "---";
    cards[0] = start_history.history[0];
    cards[1] = start_history.history[1];
    cards[2] = start_history.history[2];
    double draw_prob = 0.0;
    std::vector<std::string>& unique_draws = game == 'L' ? unique_draws_leduc : unique_draws_kuhn;
    std::vector<double>& draw_probabilities = game == 'L' ? draw_probabilities_leduc : draw_probabilities_kuhn;

    for (int i = 0; i < unique_draws.size(); i++) {
        if (unique_draws[i] == cards) {
            draw_prob = draw_probabilities[i];
            break;
        }
    }

    PokerTable true_cards = PokerTable(cards);
    true_cards.game = game;
    std::string hash_1 = "a-" + std::string(1, cards[0]) + "--";
    std::string hash_2 = "o-" + std::string(1, cards[1]) + "--";
    InformationSet I_1 = InformationSet('x', true, hash_1, game);
    InformationSet I_2 = InformationSet('o', false, hash_2, game);

    compute_regrets_along_history(I_1, I_2, true_cards, player_br_policy, player_cumulative_strategy, br_player, t, draw_prob, regret_list, markers, start_history, q_z, reward, 3);        

} 

void mccfr_outcome_sampling_best_response(PolicyVec& opponent_policy, PolicyVec& player_br_policy, char br_player, std::vector<std::string>& player_information_sets, long int T, double eps, long int step_size, int experiment_number, char game) {
    std::vector<std::vector<double>> regret_list;
    std::vector<long int> markers;
    PolicyVec cumulative_strategy;
    cumulative_strategy.player = br_player;

    PolicyVec exact_br(br_player, player_information_sets, game);
    double exact_br_value = compute_best_response_wrapper(opponent_policy, exact_br, br_player, game);
    if (br_player == 'x'){
        exact_br_value = get_expected_utility_wrapper(exact_br, opponent_policy, game);
    }
    else {
        exact_br_value = get_expected_utility_wrapper(opponent_policy, exact_br, game);
    }

    std::cout << "Exact best response value: " << exact_br_value << std::endl;
    std::vector<std::pair<int, double>> exploitability_log; 
    
    for (long int i = 0; i < player_information_sets.size(); i++) {
        regret_list.push_back(std::vector<double>(6, 0.0));

        std::vector<double> probability_dist(6, 0.0);
        cumulative_strategy.policy_dict.push_back(probability_dist);
        markers.push_back(0);
    }

    for (int t = 0; t <= T; t++) {
        std::vector<int> h = {};
        TerminalHistory start_history = TerminalHistory(h);
        double q_z = 0.0;
        double reward = 0;

        if (br_player == 'x') {
            q_z = sample_terminal_history_wrapper(player_br_policy, opponent_policy, start_history, reward, br_player, eps, game);
        } else {
            q_z = sample_terminal_history_wrapper(opponent_policy, player_br_policy, start_history, reward, br_player, eps, game);
        }

        // traverse history and update regrets
        compute_regrets_along_history_wrapper(player_br_policy, cumulative_strategy, br_player, t, regret_list, markers, start_history, q_z, reward, game);
        
        if (t % step_size == 0 && t != 0) {
            // overridde eps based on step size.
            // eps = 1.0/(((t*1.0)/(step_size*1.0))+1.0);

            // PolicyVec average_strategy = cumulative_strategy;
            // // normalize the cumulative strategy
            // #pragma omp parallel for num_threads(NUM_THREADS)
            // for (long int i = 0; i < player_information_sets.size(); i++) {
            //     std::vector<double>& cumulative_prob_table = average_strategy.policy_dict[i];
            //     double sum = 0.0;

            //     for (int j = 0; j < 6; j++) {
            //         sum += cumulative_prob_table[j];
            //     }

            //     if (sum > 0) {
            //         for (int j = 0; j < 6; j++) {
            //             cumulative_prob_table[j] /= sum;
            //         }
            //     }
            // }

            double expected_utility = 0.0;
            if (br_player == 'x'){
                expected_utility = get_expected_utility_wrapper(player_br_policy, opponent_policy, game);
                std::cout << "Expected utility after iteration " << t << ": " << expected_utility << std::endl;
                exploitability_log.push_back(std::make_pair(t, exact_br_value - expected_utility));

                // expected_utility = get_expected_utility_wrapper(average_strategy, opponent_policy, game);
                // std::cout << "Expected utility after averaging: " << expected_utility << std::endl;
            }
            else {
                expected_utility = get_expected_utility_wrapper(opponent_policy, player_br_policy, game);
                std::cout << "Expected utility after iteration " << t << ": " << expected_utility << std::endl;
                exploitability_log.push_back(std::make_pair(t, exact_br_value - expected_utility));

                // expected_utility = get_expected_utility_wrapper(opponent_policy, average_strategy, game);
                // std::cout << "Expected utility after averaging: " << expected_utility << std::endl;
            }
        }
    }

    std::cout << "Saving exploitability log" << std::endl;
    std::string file_name = "data/MCCFR/" + std::string(1, game) + "_poker_" + std::string(1, br_player) + "_MCCFR_OS_exploitability_log_" + std::to_string(experiment_number) + ".txt";

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
    char game = argv[3][0];
    char player = argv[4][0];
    int iterations = std::stoi(argv[5]);
    int log_frequency = std::stoi(argv[6]);
    int experiments = std::stoi(argv[7]);
    double eps = std::stod(argv[8]);
    int start_index = std::stoi(argv[9]);

    std::vector<std::string> P1_information_sets;
    std::vector<std::string> P2_information_sets;
    std::string P1_information_sets_file = game == 'L'? "P1_information_sets_Leduc_Poker.txt" : "P1_information_sets_Kuhn_Poker.txt";
    std::string P2_information_sets_file = game == 'L'? "P2_information_sets_Leduc_Poker.txt" : "P2_information_sets_Kuhn_Poker.txt";

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
    PolicyVec policy_obj_x('x', file_path_1, game, true);
    PolicyVec policy_obj_o('o', file_path_2, game, true);
    std::cout << "Start policies loaded." << std::endl;

    int experiment_num = start_index;
    while (experiment_num < experiments + start_index)
    {
        if (player == 'x'){
            PolicyVec uniform_policy_obj_x('x', P1_information_sets, game);
            PolicyVec player_br_policy = uniform_policy_obj_x;
            mccfr_outcome_sampling_best_response(policy_obj_o, player_br_policy, 'x', P1_information_sets, iterations, eps, log_frequency, experiment_num, game);
        }
        else if (player == 'o'){
            PolicyVec uniform_policy_obj_o('o', P2_information_sets, game);
            PolicyVec player_br_policy = uniform_policy_obj_o;
            mccfr_outcome_sampling_best_response(policy_obj_x, player_br_policy, 'o', P2_information_sets, iterations, eps, log_frequency, experiment_num, game);
        }
       
        std::cout << "(" << experiment_num << " experiments done)" << std::endl;
        experiment_num += 1;
    }
}