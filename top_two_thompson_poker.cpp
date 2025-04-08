#include "cpp_headers/poker_classes.hpp"
#include "cpp_headers/poker_utilities.hpp"
#include <random>
#include <cmath>
int NUMBER_THREADS = 4;

static std::random_device rd;
static std::mt19937 generator(rd());

int sampleIndex(const std::vector<double>& probabilities) {
    std::discrete_distribution<int> distribution(probabilities.begin(), probabilities.end());
    return distribution(generator);
}

// Function to sample from Dirichlet distribution
std::vector<double> sample_dirichlet(const std::vector<double>& alpha, std::mt19937& gen) {
    std::vector<double> gamma_samples(alpha.size());
    
    // Sample from Gamma distribution
    for (size_t i = 0; i < alpha.size(); ++i) {
        std::gamma_distribution<double> gamma_dist(alpha[i], 1.0);
        gamma_samples[i] = gamma_dist(gen);
    }

    // Normalize
    double sum = std::accumulate(gamma_samples.begin(), gamma_samples.end(), 0.0);
    for (double& value : gamma_samples) {
        value /= sum;
    }

    return gamma_samples;
}

// Function to sample from Beta distribution
double sample_beta(double alpha, double beta, std::mt19937& gen) {
    std::gamma_distribution<double> gamma_alpha(alpha, 1.0);
    std::gamma_distribution<double> gamma_beta(beta, 1.0);
    double x = gamma_alpha(gen);
    double y = gamma_beta(gen);
    return x / (x + y);
}


bool areEqual(PolicyVec& a, PolicyVec& b){
    for (int i = 0; i < a.policy_dict.size(); i++){
        for (int j = 0; j < a.policy_dict[i].size(); j++){
            if (fabs(a.policy_dict[i][j] - b.policy_dict[i][j]) > 1e-6){
                return false;
            }
        }
    }
    return true;
}


void sample_game(InformationSet& I_1, InformationSet& I_2, InformationSet previous_opponent_I, int prev_opponent_action, PokerTable& true_cards, History& current_history, char br_player, 
            PolicyVec& policy_obj, PolicyVec& opponent_policy, std::vector<int>& infoset_reach_count, std::vector<std::vector<std::unordered_map<int, int>>>& empirical_action_reward, 
            std::vector<std::vector<int>>& action_terminal_reach_count) {
    InformationSet I = true_cards.player_to_move == 'x' ? I_1 : I_2;
    int action = 0;
    
    if (br_player == I.player){
        infoset_reach_count[I.get_index()] += 1;
        std::vector<double>& prob_dist = policy_obj.policy_dict[I.get_index()];
        action = sampleIndex(prob_dist);
    }
    else {
        std::vector<double>& prob_dist = opponent_policy.policy_dict[I.get_index()];
        action = sampleIndex(prob_dist);
    }

    if (I.move_flag) {
        PokerTable new_board = true_cards;
        bool success = new_board.update_move(action);
        current_history.history.push_back(action);

        char winner;
        if (success && !new_board.is_win(winner) && !new_board.is_over()) {
            InformationSet new_I = I;
            new_I.update_move(action);

            if (I.player == 'x') {
                if (new_board.player_to_move == 'x') {
                    sample_game(new_I, I_2, previous_opponent_I, prev_opponent_action, new_board, current_history, br_player, policy_obj, opponent_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count);
                }
                else {
                    sample_game(new_I, I_2, I, action, new_board, current_history, br_player, policy_obj, opponent_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count);
                }
            } else {
                if (new_board.player_to_move == 'o') {
                    sample_game(I_1, new_I, previous_opponent_I, prev_opponent_action, new_board, current_history, br_player, policy_obj, opponent_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count);
                }
                else {
                    sample_game(I_1, new_I, I, action, new_board, current_history, br_player, policy_obj, opponent_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count);
                }
            }
        } else {
            TerminalHistory H_T = TerminalHistory(current_history.history);
            H_T.set_reward(I.game);
            int r = br_player == 'x' ? H_T.reward[0] : H_T.reward[1];

            // update action pull count and empirical action reward only when action leads to terminal state
            if (I.player == br_player){
                action_terminal_reach_count[I.get_index()][action] += 1;        
                std::unordered_map<int, int>& action_reward = empirical_action_reward[I.get_index()][action];
                action_reward[r] += 1;
            }
            else {
                action_terminal_reach_count[previous_opponent_I.get_index()][prev_opponent_action] += 1;
                std::unordered_map<int, int>& action_reward = empirical_action_reward[previous_opponent_I.get_index()][prev_opponent_action];
                action_reward[r] += 1;
            }
        }
    }
    else {
        InformationSet new_I = I;
        PokerTable new_board = true_cards;
        new_I.simulate_sense(action, new_board);
        current_history.history.push_back(action);

        if (I.player == 'x') {
            sample_game(new_I, I_2, previous_opponent_I, prev_opponent_action, new_board, current_history, br_player, policy_obj, opponent_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count);
        } else {
            sample_game(I_1, new_I, previous_opponent_I, prev_opponent_action, new_board, current_history, br_player, policy_obj, opponent_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count);
        }
    }
}


void sample_game_wrapper(InformationSet& I_1, PokerTable& true_cards, InformationSet& I_2, PolicyVec& policy_obj, PolicyVec& opponent_policy, History& current_history, char br_player, 
                     std::vector<int>& infoset_reach_count, std::vector<std::vector<std::unordered_map<int, int>>>& empirical_action_reward, 
                     std::vector<std::vector<int>>& action_terminal_reach_count) {
    if (br_player == 'x'){
        sample_game(I_1, I_2, I_2, 0, true_cards, current_history, br_player, policy_obj, opponent_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count);
    }
    else {
        sample_game(I_1, I_2, I_1, 0, true_cards, current_history, br_player, policy_obj, opponent_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count);
    }
}


double build_max_reward_policy(PolicyVec& policy_obj, InformationSet& I, std::vector<int>& infoset_reach_count, std::vector<std::vector<std::unordered_map<int, int>>>& empirical_action_reward, 
                               std::vector<std::vector<int>>& action_terminal_reach_count, std::vector<double>& infoset_values, char game, std::vector<std::vector<std::unordered_set<std::string>>>& cohorts) {
    std::vector<int> legal_actions;
    I.get_actions(legal_actions);
    std::vector<double> action_values(6, 0.0);
    double infoset_value = I.game == 'L'? LEDUC_MIN_UTILITY : KUHN_MIN_UTILITY;

    for (int a : legal_actions){
        std::unordered_set<std::string>& cohort = cohorts[I.get_index()][a];
        std::unordered_map<std::string, double> cohort_values;
 
        int norm = 0;
        int terminal_reach_count = action_terminal_reach_count[I.get_index()][a];

        for (std::string I_prime_hash : cohort){
            InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash, game);
            cohort_values[I_prime_hash] = build_max_reward_policy(policy_obj, I_prime, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_values, game, cohorts);

            norm += infoset_reach_count[I_prime.get_index()];
            action_values[a] += cohort_values[I_prime_hash] * infoset_reach_count[I_prime.get_index()]; 
        }

        norm += terminal_reach_count;
        std::unordered_map<int, int>& action_reward = empirical_action_reward[I.get_index()][a];
        double action_value = 0.0;
        for (auto& [r, count] : action_reward){
            action_value += r * count;
        }

        action_values[a] += action_value;
        if (norm != 0){
            action_values[a] /= norm;
        }
        else {
            action_values[a] = I.game == 'L'? LEDUC_MIN_UTILITY : KUHN_MIN_UTILITY;
        }
    }

    for (int a : legal_actions){
        if (action_values[a] > infoset_value){
            infoset_value = action_values[a];
        }
    }

    if (infoset_reach_count[I.get_index()] == 0){
        // sample from legal actions
        int action = legal_actions[std::rand() % legal_actions.size()];
        std::vector<double>& prob_dist = policy_obj.policy_dict[I.get_index()];

        for (int a : legal_actions){
            if (a == action){
                prob_dist[a] = 1.0;
            }
            else{
                prob_dist[a] = 0.0;
            }
        }

        infoset_values[I.get_index()] = 0.0;
        return 0.0;
    }
    else {
        std::vector<int> candidate_actions;
        for (int a : legal_actions){
            if (fabs(action_values[a] - infoset_value) < 1e-6){
                candidate_actions.push_back(a);
            }
        }

        // sample from candidate actions
        int action = candidate_actions[std::rand() % candidate_actions.size()];
        std::vector<double>& prob_dist = policy_obj.policy_dict[I.get_index()];

        for (int a : legal_actions){
            if (a == action){
                prob_dist[a] = 1.0;
            }
            else{
                prob_dist[a] = 0.0;
            }
        }

        infoset_values[I.get_index()] = infoset_value;
        return infoset_value;
    }
}


double build_max_reward_policy_dirichlet(PolicyVec& policy_obj, InformationSet& I, std::vector<int>& infoset_reach_count, std::vector<std::vector<std::unordered_map<int, int>>>& empirical_action_reward, 
                                         std::vector<std::vector<int>>& action_terminal_reach_count, std::vector<double>& infoset_values, char game, std::vector<std::vector<std::unordered_set<std::string>>>& cohorts) {
    std::vector<int> legal_actions;
    I.get_actions(legal_actions);
    std::vector<double> action_values(6, 0.0);
    double infoset_value = I.game == 'L'? LEDUC_MIN_UTILITY : KUHN_MIN_UTILITY;

    for (int a : legal_actions){
        std::unordered_set<std::string>& cohort = cohorts[I.get_index()][a];
        std::unordered_map<std::string, double> cohort_values;
 
        double norm = 0.0;
        std::vector<double> alpha(cohort.size()+1, 0.0);
        int terminal_reach_count = action_terminal_reach_count[I.get_index()][a];

        for (std::string I_prime_hash : cohort){
            InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash, game);
            cohort_values[I_prime_hash] = build_max_reward_policy_dirichlet(policy_obj, I_prime, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_values, game, cohorts);

            norm += infoset_reach_count[I_prime.get_index()];
        }
        norm += terminal_reach_count;

        if (norm != 0){
            int index = 0;
            for (std::string I_prime_hash : cohort){
                InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash, game);
                alpha[index++] = infoset_reach_count[I_prime.get_index()];
            }
            alpha[index] = terminal_reach_count;

            std::vector<double> samples = sample_dirichlet(alpha, generator);

            index = 0;
            for (std::string I_prime_hash : cohort){
                InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash, game);
                action_values[a] += cohort_values[I_prime_hash] * samples[index++];
            }
            
            if (terminal_reach_count != 0){
                std::vector<double> reward_values;
                std::vector<double> reward_frequency;
                for (auto& [r, count] : empirical_action_reward[I.get_index()][a]){
                    reward_values.push_back(r);
                    reward_frequency.push_back(((double) count));
                }

                std::vector<double> reward_samples = sample_dirichlet(reward_frequency, generator);
                double terminal_value_sample = 0.0;

                for (int i = 0; i < reward_values.size(); i++){
                    terminal_value_sample += reward_values[i] * reward_samples[i];
                }

                double terminal_value = terminal_value_sample * samples[index];
                action_values[a] += terminal_value;
            }
        }
        else {
            action_values[a] = I.game == 'L'? LEDUC_MAX_UTILITY : KUHN_MAX_UTILITY;
        }
    }

    for (int a : legal_actions){
        if (action_values[a] > infoset_value){
            infoset_value = action_values[a];
        }
    }

    if (infoset_reach_count[I.get_index()] == 0){
        // sample from legal actions
        int action = legal_actions[std::rand() % legal_actions.size()];
        std::vector<double>& prob_dist = policy_obj.policy_dict[I.get_index()];

        for (int a : legal_actions){
            if (a == action){
                prob_dist[a] = 1.0;
            }
            else{
                prob_dist[a] = 0.0;
            }
        }

        infoset_values[I.get_index()] = 0.0;
        return 0.0;
    }
    else {
        std::vector<int> candidate_actions;
        for (int a : legal_actions){
            if (fabs(action_values[a] - infoset_value) < 1e-6){
                candidate_actions.push_back(a);
            }
        }

        // sample from candidate actions
        int action = candidate_actions[std::rand() % candidate_actions.size()];
        std::vector<double>& prob_dist = policy_obj.policy_dict[I.get_index()];

        for (int a : legal_actions){
            if (a == action){
                prob_dist[a] = 1.0;
            }
            else{
                prob_dist[a] = 0.0;
            }
        }

        infoset_values[I.get_index()] = infoset_value;
        return infoset_value;
    }
}


double build_max_reward_policy_dirichlet_parallel(PolicyVec& policy_obj, InformationSet& I, std::vector<int>& infoset_reach_count, std::vector<std::vector<std::unordered_map<int, int>>>& empirical_action_reward,
                                                  std::vector<std::vector<int>>& action_terminal_reach_count, std::vector<double>& infoset_values, char game, std::vector<std::vector<std::unordered_set<std::string>>>& cohorts) {
    std::vector<int> legal_actions;
    I.get_actions(legal_actions);
    std::vector<double> action_values(6, 0.0);
    double infoset_value = I.game == 'L'? LEDUC_MIN_UTILITY : KUHN_MIN_UTILITY;

    #pragma omp parallel for num_threads(NUMBER_THREADS)
    for (int a : legal_actions){
        std::unordered_set<std::string>& cohort = cohorts[I.get_index()][a];
        std::unordered_map<std::string, double> cohort_values;
  
        double norm = 0.0;
        std::vector<double> alpha(cohort.size()+1, 0.0);
        int terminal_reach_count = action_terminal_reach_count[I.get_index()][a];

        for (std::string I_prime_hash : cohort){
            InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash, game);
            cohort_values[I_prime_hash] = build_max_reward_policy_dirichlet(policy_obj, I_prime, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_values, game, cohorts);

            norm += infoset_reach_count[I_prime.get_index()];
        }
        norm += terminal_reach_count;

        if (norm != 0){
            int index = 0;
            for (std::string I_prime_hash : cohort){
                InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash, game);
                alpha[index++] = infoset_reach_count[I_prime.get_index()];
            }
            alpha[index] = terminal_reach_count;

            std::vector<double> samples = sample_dirichlet(alpha, generator);

            index = 0;
            for (std::string I_prime_hash : cohort){
                InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash, game);
                action_values[a] += cohort_values[I_prime_hash] * samples[index++];
            }
            
            if (terminal_reach_count != 0){
                std::vector<double> reward_values;
                std::vector<double> reward_frequency;
                for (auto& [r, count] : empirical_action_reward[I.get_index()][a]){
                    reward_values.push_back(r);
                    reward_frequency.push_back(((double) count));
                }

                std::vector<double> reward_samples = sample_dirichlet(reward_frequency, generator);
                double terminal_value_sample = 0.0;

                for (int i = 0; i < reward_values.size(); i++){
                    terminal_value_sample += reward_values[i] * reward_samples[i];
                }

                double terminal_value = terminal_value_sample * samples[index];
                action_values[a] += terminal_value;
            }
        }
        else {
            action_values[a] = I.game == 'L'? LEDUC_MAX_UTILITY : KUHN_MAX_UTILITY;
        }
    }

    for (int a : legal_actions){
        if (action_values[a] > infoset_value){
            infoset_value = action_values[a];
        }
    }

    if (infoset_reach_count[I.get_index()] == 0){
        // sample from legal actions
        int action = legal_actions[std::rand() % legal_actions.size()];
        std::vector<double>& prob_dist = policy_obj.policy_dict[I.get_index()];

        for (int a : legal_actions){
            if (a == action){
                prob_dist[a] = 1.0;
            }
            else{
                prob_dist[a] = 0.0;
            }
        }

        infoset_values[I.get_index()] = 0.0;
        return 0.0;
    }
    else {
        std::vector<int> candidate_actions;
        for (int a : legal_actions){
            if (fabs(action_values[a] - infoset_value) < 1e-6){
                candidate_actions.push_back(a);
            }
        }

        // sample from candidate actions
        int action = candidate_actions[std::rand() % candidate_actions.size()];
        std::vector<double>& prob_dist = policy_obj.policy_dict[I.get_index()];

        for (int a : legal_actions){
            if (a == action){
                prob_dist[a] = 1.0;
            }
            else{
                prob_dist[a] = 0.0;
            }
        }

        infoset_values[I.get_index()] = infoset_value;
        return infoset_value;
    }
}


void logging(int t, int log_frequency, PolicyVec& br, PolicyVec& opponent_policy, char br_player, double exact_br_value, std::vector<int>& infoset_reach_count, 
             std::vector<std::pair<int, double>>& sample_gameability_log, char game, std::vector<double>& infoset_values, std::vector<std::vector<std::unordered_map<int, int>>>& empirical_action_reward,
             std::vector<std::vector<int>>& action_terminal_reach_count, std::vector<std::vector<std::unordered_set<std::string>>>& cohorts) {
    if (t % log_frequency == 0 && t != 0) { 
        std::vector<char> player_cards = {'J', 'Q', 'K'};
        for (int card_index = 0; card_index < player_cards.size(); card_index++){
            std::string hash_1 = "a-" + std::string(1, player_cards[card_index]) + "--";
            std::string hash_2 = "o-" + std::string(1, player_cards[card_index]) + "--";
            InformationSet root = br_player == 'x' ? InformationSet('x', true, hash_1, game) : InformationSet('o', false, hash_2, game);
            double root_val = build_max_reward_policy(br, root, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_values, game, cohorts);    
        }

        double expected_utility = 0.0;
        double sample_gameability = 0.0;
    
        if (br_player == 'x') {
            expected_utility = get_expected_utility_wrapper(br, opponent_policy, game);
            sample_gameability_log.push_back(std::make_pair(t, exact_br_value - expected_utility));
        } 
        else {
            expected_utility = get_expected_utility_wrapper(opponent_policy, br, game);
            sample_gameability_log.push_back(std::make_pair(t, exact_br_value - expected_utility));
        }
        std::cout << "Expected utility of best response policy: " << expected_utility << std::endl;
        std::cout << "Number of games sampled so far: " << t << std::endl;
    }
}


void calc_br(PolicyVec& opponent_policy, char br_player, std::vector<std::string>& player_information_sets, int log_frequency, int m, PolicyVec& br, int experiment_number, char game, int iterations, std::string& exp_name) {    
    std::vector<int> infoset_reach_count(player_information_sets.size(), 1);
    std::vector<double> infoset_values(player_information_sets.size(), 0.0);

    std::unordered_map<int, int> reward_map;
    int MIN_UTILITY = game == 'L' ? LEDUC_MIN_UTILITY : KUHN_MIN_UTILITY;
    int MAX_UTILITY = game == 'L' ? LEDUC_MAX_UTILITY : KUHN_MAX_UTILITY;
    for (int i = MIN_UTILITY; i <= MAX_UTILITY; i++) {
        reward_map[i] = 1;
    }

    std::vector<std::vector<std::unordered_map<int, int>>> empirical_action_reward(player_information_sets.size(), std::vector<std::unordered_map<int, int>>(6, reward_map));
    std::vector<std::vector<int>> action_terminal_reach_count(player_information_sets.size(), std::vector<int>(6, 1));

    PolicyVec candidate_br(br_player, player_information_sets, game);
    std::vector<std::pair<int, double>> sample_gameability_log; 

    std::vector<std::string>& unique_draws = game == 'L' ? unique_draws_leduc : unique_draws_kuhn;
    std::vector<double>& draw_probabilities = game == 'L' ? draw_probabilities_leduc : draw_probabilities_kuhn;

    std::cout << "Building cohorts" << std::endl;
    // precompute cohorts
    std::vector<std::vector<std::unordered_set<std::string>>> cohorts(player_information_sets.size(), std::vector<std::unordered_set<std::string>>(13));
    #pragma omp parallel for num_threads(NUMBER_THREADS)
    for (int i = 0; i < player_information_sets.size(); i++) {
        InformationSet I = InformationSet(br_player, get_move_flag(player_information_sets[i], br_player), player_information_sets[i], game);
        std::vector<int> legal_actions;
        I.get_actions(legal_actions);
        for (int a : legal_actions) {
            std::unordered_set<std::string> cohort;
            get_cohort(I, a, cohort);
            cohorts[I.get_index()][a] = cohort;
        }
    }
    std::cout << "Cohorts built" << std::endl;
    int t = 0;

    PolicyVec exact_br(br_player, player_information_sets, game);
    compute_best_response_wrapper(opponent_policy, exact_br, br_player, game);
    double exact_br_value = 0.0;
    if (br_player == 'x') {
        exact_br_value = get_expected_utility_wrapper(exact_br, opponent_policy, game);
    } 
    else {
        exact_br_value = get_expected_utility_wrapper(opponent_policy, exact_br, game);
    }
    std::cout << "Exact best response value: " << exact_br_value << std::endl;

    while (t <= iterations) {
        // logging
        logging(t, log_frequency, br, opponent_policy, br_player, exact_br_value, infoset_reach_count, sample_gameability_log, game, infoset_values, empirical_action_reward, action_terminal_reach_count, cohorts);

        std::vector<char> player_cards = {'J', 'Q', 'K'};
        for (int card_index = 0; card_index < player_cards.size(); card_index++){
            std::string hash_1 = "a-" + std::string(1, player_cards[card_index]) + "--";
            std::string hash_2 = "o-" + std::string(1, player_cards[card_index]) + "--";
            InformationSet root = br_player == 'x' ? InformationSet('x', true, hash_1, game) : InformationSet('o', false, hash_2, game);
            double root_val = build_max_reward_policy_dirichlet(br, root, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_values, game, cohorts);    
        }

        // toss a coin
        std::vector<double> dist = {0.5, 0.5};
        int head = sampleIndex(dist);

        if (head){
            int max_iter = 10;
            int iter = 0;
            do {
                for (int card_index = 0; card_index < player_cards.size(); card_index++){
                    std::string hash_1 = "a-" + std::string(1, player_cards[card_index]) + "--";
                    std::string hash_2 = "o-" + std::string(1, player_cards[card_index]) + "--";
                    InformationSet root = br_player == 'x' ? InformationSet('x', true, hash_1, game) : InformationSet('o', false, hash_2, game);
                    double root_val = build_max_reward_policy_dirichlet(candidate_br, root, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_values, game, cohorts);    
                }
                if (iter == max_iter){
                    break;
                }
                iter += 1;
            } while (areEqual(br, candidate_br));
        }
        
        PolicyVec& policy_to_sample = head == 0 ? br : candidate_br;

        std::discrete_distribution<int> distribution(draw_probabilities.begin(), draw_probabilities.end());
        int draw_index = distribution(generator);
        std::string cards = unique_draws[draw_index];
        PokerTable true_cards = PokerTable(cards);
        true_cards.game = game;
        std::string hash_1 = "a-" + std::string(1, cards[0]) + "--";
        std::string hash_2 = "o-" + std::string(1, cards[1]) + "--";
        InformationSet I_1 = InformationSet('x', true, hash_1, game);
        InformationSet I_2 = InformationSet('o', false, hash_2, game);

        std::vector<int> h = {};
        h.push_back(cards[0]);
        h.push_back(cards[1]);
        h.push_back(cards[2]); 
        TerminalHistory start_history = TerminalHistory(h);

        sample_game_wrapper(I_1, true_cards, I_2, policy_to_sample, opponent_policy, start_history, br_player, infoset_reach_count, empirical_action_reward, action_terminal_reach_count);
        t += 1;
    }

    std::cout << "Saving sample_gameability log" << std::endl;
    std::string file_name = "data/bandit/" + exp_name + "_" + std::string(1, game) + "_poker_" + std::string(1, br_player) +  "_top_two_thompson_sample_gameability_log_" + std::to_string(experiment_number) + ".txt";

    std::ofstream f(file_name);
    for (int i = 0; i < sample_gameability_log.size(); i++) {
        f << sample_gameability_log[i].first << " " << sample_gameability_log[i].second << std::endl;
    }
    f.close();
}


int main(int argc, char* argv[]) {
    std::cout.precision(17);
    std::string file_path_1 = argv[1]; // start policy P1
    std::string file_path_2 = argv[2]; // start policy P2
    char game = argv[3][0];
    char player = argv[4][0];
    int iterations = std::stoi(argv[5]);
    int log_frequency = std::stoi(argv[6]);
    int experiments = std::stoi(argv[7]);
    std::string exp_name = argv[8];
    int start_index = std::stoi(argv[9]);
    NUMBER_THREADS = std::stoi(argv[10]);
    
    // load information sets
    std::vector<std::string> P1_information_sets;
    std::vector<std::string> P2_information_sets;
    std::string P1_information_sets_file = game == 'L'? "P1_information_sets_Leduc_Poker.txt" : "P1_information_sets_Kuhn_Poker.txt";
    std::string P2_information_sets_file = game == 'L'? "P2_information_sets_Leduc_Poker.txt" : "P2_information_sets_Kuhn_Poker.txt";
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
    for (int i = 0; i < P1_information_sets.size(); i++) {
        InformationSet::P1_hash_to_int_map[P1_information_sets[i]] = i;
    }
    for (int i = 0; i < P2_information_sets.size(); i++) {
        InformationSet::P2_hash_to_int_map[P2_information_sets[i]] = i;
    }

    // load policies
    std::cout << "Loading policies" << std::endl;
    PolicyVec policy_obj_x('x', file_path_1, game, true);
    PolicyVec policy_obj_o('o', file_path_2, game, true);
    double expected_utility = get_expected_utility_wrapper(policy_obj_x, policy_obj_o, game);
    std::cout << "Expected utility of initial policies: " << expected_utility << std::endl;

    // compute epsilon best response
    int experiment_num = start_index;

    while (experiment_num < experiments + start_index) {
        if (player == 'x') {
            PolicyVec uniform_x('x', P1_information_sets, game);
            calc_br(policy_obj_o, 'x', P1_information_sets, log_frequency, 1, uniform_x, experiment_num, game, iterations, exp_name);
        }
        else {
            PolicyVec uniform_o('o', P2_information_sets, game);
            calc_br(policy_obj_x, 'o', P2_information_sets, log_frequency, 1, uniform_o, experiment_num, game, iterations, exp_name);
        }

        std::cout << "(" << experiment_num << " experiments done)" << std::endl;
        experiment_num += 1;
    }
}