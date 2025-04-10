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


void precompute_cohorts(std::vector<std::vector<std::unordered_set<std::string>>>& cohorts, std::vector<std::string>& player_information_sets, char br_player, char game) {
    std::cout << "Building cohorts" << std::endl;
    // precompute cohorts
    
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
}


double sample_game(InformationSet& I_1, InformationSet& I_2, PokerTable& true_cards, History& current_history, char br_player, 
                   PolicyVec& policy_obj, PolicyVec& opponent_policy, Sequence& trajectory) {
    InformationSet I = true_cards.player_to_move == 'x' ? I_1 : I_2;
    int action = 0;
    
    if (br_player == I.player){
        std::vector<double>& prob_dist = policy_obj.policy_dict[I.get_index()];
        action = sampleIndex(prob_dist);
        trajectory.extend(I, action);
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
                    return sample_game(new_I, I_2, new_board, current_history, br_player, policy_obj, opponent_policy, trajectory);
                }
                else {
                    return sample_game(new_I, I_2, new_board, current_history, br_player, policy_obj, opponent_policy, trajectory);
                }
            } else {
                if (new_board.player_to_move == 'o') {
                    return sample_game(I_1, new_I, new_board, current_history, br_player, policy_obj, opponent_policy, trajectory);
                }
                else {
                    return sample_game(I_1, new_I, new_board, current_history, br_player, policy_obj, opponent_policy, trajectory);
                }
            }
        } else {
            TerminalHistory H_T = TerminalHistory(current_history.history);
            H_T.set_reward(I.game);
            double r = br_player == 'x' ? (double) H_T.reward[0] : (double) H_T.reward[1];
            // scale reward between -1 to 1
            double MAX_UTILITY = I.game == 'L' ? LEDUC_MAX_UTILITY : KUHN_MAX_UTILITY;
            r = r / (2.0 * MAX_UTILITY);
            return r; 
        }
    }
    else {
        InformationSet new_I = I;
        PokerTable new_board = true_cards;
        new_I.simulate_sense(action, new_board);
        current_history.history.push_back(action);

        if (I.player == 'x') {
            return sample_game(new_I, I_2, new_board, current_history, br_player, policy_obj, opponent_policy, trajectory);
        } else {
            return sample_game(I_1, new_I, new_board, current_history, br_player, policy_obj, opponent_policy, trajectory);
        }
    }
}


double build_max_policy(PolicyVec& policy_obj, InformationSet& I, Sequence& trajectory, std::vector<Sequence>& terminal_sequences,
                        std::unordered_map<std::string, int>& sequence_hash_to_index_map, char game, bool UCB_flag, 
                        std::vector<double>& global_infoset_values, std::vector<std::vector<double>>& global_action_values, std::vector<std::vector<std::unordered_set<std::string>>>& cohorts) {
    std::vector<int> legal_actions;
    I.get_actions(legal_actions);
    std::vector<double> legal_action_values(6, 0.0);
    double infoset_value = -1.0;

    for (int a : legal_actions){
        std::unordered_set<std::string>& cohort = cohorts[I.get_index()][a];

        Sequence new_trajectory = trajectory;
        new_trajectory.extend(I, a);

        for (std::string I_prime_hash : cohort){
            InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash, game);
            legal_action_values[a] += build_max_policy(policy_obj, I_prime, new_trajectory, terminal_sequences, sequence_hash_to_index_map, game, UCB_flag, global_infoset_values, global_action_values, cohorts);
        }

        int s_index = sequence_hash_to_index_map[new_trajectory.hash];
        double p_hat, r_hat = 0.0;

        if (UCB_flag){
            p_hat = terminal_sequences[s_index].ucb_p;
            r_hat = terminal_sequences[s_index].ucb_r;
        }
        else{
            p_hat = terminal_sequences[s_index].p;
            if (terminal_sequences[s_index].n != 0){
                r_hat = terminal_sequences[s_index].r / (double) terminal_sequences[s_index].n;
            }
        }

        legal_action_values[a] += p_hat * r_hat;

        if (!UCB_flag){
            if (cohort.size() == 0 && terminal_sequences[s_index].n == 0){
                legal_action_values[a] = 0.0;
            }
        }

        // make sure action values lie between [-1, 1]
        legal_action_values[a] = std::min(1.0, legal_action_values[a]);
        legal_action_values[a] = std::max(-1.0, legal_action_values[a]);
    }

    for (int a : legal_actions){
        if (legal_action_values[a] > infoset_value){
            infoset_value = legal_action_values[a];
        }
    }

    std::vector<int> candidate_actions;
    for (int a : legal_actions){
        if (fabs(legal_action_values[a] - infoset_value) < 1e-6){
            candidate_actions.push_back(a);
        }
    }

    // sample from candidate actions
    int action = candidate_actions[sampleIndex(std::vector<double>(candidate_actions.size(), 1.0 / candidate_actions.size()))];
    std::vector<double>& prob_dist = policy_obj.policy_dict[I.get_index()];

    for (int a : legal_actions){
        if (a == action){
            prob_dist[a] = 1.0;
        }
        else{
            prob_dist[a] = 0.0;
        }
    }

    for (int a : legal_actions){
        global_action_values[I.get_index()][a] = legal_action_values[a];
    }
    global_infoset_values[I.get_index()] = infoset_value;

    return infoset_value;
}


void update_max_reward_policy_given_trajectory(PolicyVec& policy_obj, Sequence& trajectory, std::vector<Sequence>& terminal_sequences, std::unordered_map<std::string, int>& sequence_hash_to_index_map,
                                               std::vector<double>& global_infoset_values, std::vector<std::vector<double>>& global_action_values, char game, char br_player, std::vector<std::vector<std::unordered_set<std::string>>>& cohorts) {
    for (int j = trajectory.seq.size() - 1; j >= 0; j--){
        std::string I_hash = trajectory.seq[j].first;
        int played_action = trajectory.seq[j].second;
        bool move_flag = get_move_flag(I_hash, br_player);
        InformationSet I(br_player, move_flag, I_hash, game);

        std::string seq_hash = I_hash + "-" + std::to_string(played_action);
        int s_index = sequence_hash_to_index_map[seq_hash];
        std::vector<double>& prob_dist = policy_obj.policy_dict[I.get_index()];

        // compute value of played action
        std::unordered_set<std::string>& cohort = cohorts[I.get_index()][played_action];

        double action_value = 0.0;
        for (std::string I_prime_hash : cohort){
            InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash, game);
            action_value += global_infoset_values[I_prime.get_index()];
        }

        double p_hat, r_hat = 0.0;
        p_hat = terminal_sequences[s_index].p;
        if (terminal_sequences[s_index].n != 0){
            r_hat = terminal_sequences[s_index].r / (double) terminal_sequences[s_index].n;
        }
        action_value += p_hat * r_hat;

        if (cohort.size() == 0 && terminal_sequences[s_index].n == 0){
            action_value = 0.0;
        }
        
        // make sure action value lies between [-1, 1]
        action_value = std::min(1.0, action_value);
        action_value = std::max(-1.0, action_value);

        // update played action value
        global_action_values[I.get_index()][played_action] = action_value;

        // compare action values to find best action
        std::vector<int> legal_actions;
        I.get_actions(legal_actions);
        double infoset_value = -1.0;
        for (int a : legal_actions){
            if (global_action_values[I.get_index()][a] > infoset_value){
                infoset_value = global_action_values[I.get_index()][a];
            }
        }

        std::vector<int> candidate_actions;
        for (int a: legal_actions){
            if (fabs(global_action_values[I.get_index()][a] - infoset_value) < 1e-6){
                candidate_actions.push_back(a);
            }
        }

        // sample from candidate actions
        int best_action = candidate_actions[sampleIndex(std::vector<double>(candidate_actions.size(), 1.0 / candidate_actions.size()))];
        for (int a : legal_actions){
            if (a == best_action){
                prob_dist[a] = 1.0;
            }
            else{
                prob_dist[a] = 0.0;
            }
        }
        global_infoset_values[I.get_index()] = infoset_value;
    }
}


double update_max_ucb_policy_given_trajectory(PolicyVec& update_policy_obj, PolicyVec& played_policy_obj, InformationSet& I, Sequence& trajectory, std::vector<Sequence>& terminal_sequences,
                                              std::unordered_map<std::string, int>& sequence_hash_to_index_map, char game, std::vector<double>& global_infoset_values, std::vector<std::vector<double>>& global_action_values, std::vector<std::vector<std::unordered_set<std::string>>>& cohorts) {
    std::vector<int> legal_actions;
    I.get_actions(legal_actions);
    std::vector<int> played_actions;
    I.get_actions_given_policy(played_actions, played_policy_obj);
    double infoset_value = -1.0;

    for (int a : played_actions){
        double played_action_value = 0.0;

        std::unordered_set<std::string>& cohort = cohorts[I.get_index()][a];
  
        Sequence new_trajectory = trajectory;
        new_trajectory.extend(I, a);

        for (std::string I_prime_hash : cohort){
            InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash, game);
            played_action_value += update_max_ucb_policy_given_trajectory(update_policy_obj, played_policy_obj, I_prime, new_trajectory, terminal_sequences, sequence_hash_to_index_map, game, global_infoset_values, global_action_values, cohorts);
        }

        int s_index = sequence_hash_to_index_map[new_trajectory.hash];
        double p_hat, r_hat = 0.0;

        p_hat = terminal_sequences[s_index].ucb_p;
        r_hat = terminal_sequences[s_index].ucb_r;

        played_action_value += p_hat * r_hat;

        // make sure action values lie between [-1, 1]
        played_action_value = std::min(1.0, played_action_value);
        played_action_value = std::max(-1.0, played_action_value);

        global_action_values[I.get_index()][a] = played_action_value;
    }

    for (int a : legal_actions){
        if (global_action_values[I.get_index()][a] > infoset_value){
            infoset_value = global_action_values[I.get_index()][a];
        }
    }

    std::vector<int> candidate_actions;
    for (int a : legal_actions){
        if (fabs(global_action_values[I.get_index()][a] - infoset_value) < 1e-6){
            candidate_actions.push_back(a);
        }
    }

    // sample from candidate actions
    int action = candidate_actions[sampleIndex(std::vector<double>(candidate_actions.size(), 1.0 / candidate_actions.size()))];
    std::vector<double>& prob_dist = update_policy_obj.policy_dict[I.get_index()];

    for (int a : legal_actions){
        if (a == action){
            prob_dist[a] = 1.0;
        }
        else{
            prob_dist[a] = 0.0;
        }
    }

    global_infoset_values[I.get_index()] = infoset_value;

    return infoset_value;
}


void get_sequences(InformationSet& I, Sequence& trajectory, std::vector<Sequence>& terminal_sequences, std::unordered_map<std::string, int>& sequence_hash_to_index_map, char game, std::vector<std::vector<std::unordered_set<std::string>>>& cohorts) {
    std::vector<int> legal_actions;
    I.get_actions(legal_actions);
    
    for (int a : legal_actions){
        Sequence new_trajectory = trajectory;
        new_trajectory.extend(I, a);
        terminal_sequences.push_back(new_trajectory);
        sequence_hash_to_index_map[new_trajectory.hash] = terminal_sequences.size() - 1;

        std::unordered_set<std::string>& cohort = cohorts[I.get_index()][a];

        for (std::string I_prime_hash : cohort){
            InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash, game);
            get_sequences(I_prime, new_trajectory, terminal_sequences, sequence_hash_to_index_map, game, cohorts);
        }
    }
}


void get_sequences_wrapper(std::vector<Sequence>& terminal_sequences, char game, char br_player, std::unordered_map<std::string, int>& sequence_hash_to_index_map, 
                           std::vector<std::vector<std::unordered_set<std::string>>>& cohorts) {
    std::vector<char> player_cards = {'J', 'Q', 'K'};
    for (int card_index = 0; card_index < player_cards.size(); card_index++){
        std::string hash_1 = "a-" + std::string(1, player_cards[card_index]) + "--";
        std::string hash_2 = "o-" + std::string(1, player_cards[card_index]) + "--";
        InformationSet root = br_player == 'x' ? InformationSet('x', true, hash_1, game) : InformationSet('o', false, hash_2, game);
        Sequence empty_sequence = Sequence();
        get_sequences(root, empty_sequence, terminal_sequences, sequence_hash_to_index_map, game, cohorts);
    }
}


void get_policy_sequences(PolicyVec& policy_obj, InformationSet& I, Sequence& trajectory, std::vector<int>& policy_sequences, char game, std::unordered_map<std::string, int>& sequence_hash_to_index_map, 
                          std::vector<std::vector<std::unordered_set<std::string>>>& cohorts) {
    std::vector<int> legal_actions;
    I.get_actions_given_policy(legal_actions, policy_obj);
    
    for (int a : legal_actions){
        Sequence new_trajectory = trajectory;
        new_trajectory.extend(I, a);
        int index = sequence_hash_to_index_map[new_trajectory.hash];
        policy_sequences.push_back(index);

        std::unordered_set<std::string>& cohort = cohorts[I.get_index()][a];

        for (std::string I_prime_hash : cohort){
            InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash, game);
            get_policy_sequences(policy_obj, I_prime, new_trajectory, policy_sequences, game, sequence_hash_to_index_map, cohorts);
        }
    }
}


void get_policy_sequences_wrapper(PolicyVec& policy_obj, std::vector<int>& policy_sequences, char game, std::unordered_map<std::string, int>& sequence_hash_to_index_map, char br_player, 
                                  std::vector<std::vector<std::unordered_set<std::string>>>& cohorts) {
    std::vector<char> player_cards = {'J', 'Q', 'K'};
    for (int card_index = 0; card_index < player_cards.size(); card_index++){
        std::string hash_1 = "a-" + std::string(1, player_cards[card_index]) + "--";
        std::string hash_2 = "o-" + std::string(1, player_cards[card_index]) + "--";
        InformationSet root = br_player == 'x' ? InformationSet('x', true, hash_1, game) : InformationSet('o', false, hash_2, game);
        Sequence empty_sequence = Sequence();
        get_policy_sequences(policy_obj, root, empty_sequence, policy_sequences, game, sequence_hash_to_index_map, cohorts);
    }
}


void update_sequence_data(std::vector<int>& policy_sequences, std::vector<Sequence>& terminal_sequences, double reward_game, std::string terminal_hash_game, double C_r, double C_p) {
    for (int i = 0; i < policy_sequences.size(); i++){
        int s_index = policy_sequences[i];

        if (terminal_sequences[s_index].hash == terminal_hash_game){
            terminal_sequences[s_index].r += reward_game;
            terminal_sequences[s_index].n += 1;
            terminal_sequences[s_index].n_pi += 1;
            terminal_sequences[s_index].p = (double) terminal_sequences[s_index].n / (double) terminal_sequences[s_index].n_pi;
            terminal_sequences[s_index].ucb_r = std::min(1.0, terminal_sequences[s_index].r / (double) terminal_sequences[s_index].n + C_r * std::sqrt(1.0 / (double) terminal_sequences[s_index].n));
            terminal_sequences[s_index].ucb_p = std::min(1.0, terminal_sequences[s_index].p + C_p * std::sqrt(1.0 / (double) terminal_sequences[s_index].n_pi));
        }
        else{
            terminal_sequences[s_index].n_pi += 1;
            terminal_sequences[s_index].p = (double) terminal_sequences[s_index].n / (double) terminal_sequences[s_index].n_pi;
            terminal_sequences[s_index].ucb_p = std::min(1.0, terminal_sequences[s_index].p + C_p * std::sqrt(1.0 / (double) terminal_sequences[s_index].n_pi));
        }
    }
}


void calc_br_sequence_LUCB(PolicyVec& opponent_policy, char br_player, std::vector<std::string>& player_information_sets, int log_frequency, PolicyVec& player_br, 
    int experiment_number, char game, int iterations, double C_r, double C_p, std::string& exp_name, std::unordered_map<std::string, int>& sequence_hash_to_index_map, 
    std::vector<Sequence>& terminal_sequences, std::vector<std::vector<std::unordered_set<std::string>>>& cohorts) 
{  
    // store infoset and action values to avoid recomputation for parts of the tree that don't change
    std::vector<double> infoset_empirical_values(player_information_sets.size(), 0.0);
    std::vector<std::vector<double>> action_empirical_values(player_information_sets.size(), std::vector<double>(6, 0.0));

    std::vector<double> infoset_upper_bound_values(player_information_sets.size(), 0.0);
    std::vector<std::vector<double>> action_upper_bound_values(player_information_sets.size(), std::vector<double>(6, 0.0));
    
    PolicyVec player_max_ucb_policy(br_player, player_information_sets, game);
    std::vector<std::pair<int, double>> exploitability_log; 

    std::vector<std::string>& unique_draws = game == 'L' ? unique_draws_leduc : unique_draws_kuhn;
    std::vector<double>& draw_probabilities = game == 'L' ? draw_probabilities_leduc : draw_probabilities_kuhn;

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

    std::vector<char> player_cards = {'J', 'Q', 'K'};
    for (int card_index = 0; card_index < player_cards.size(); card_index++){
        std::string hash_1 = "a-" + std::string(1, player_cards[card_index]) + "--";
        std::string hash_2 = "o-" + std::string(1, player_cards[card_index]) + "--";
        InformationSet root = br_player == 'x' ? InformationSet('x', true, hash_1, game) : InformationSet('o', false, hash_2, game);
        Sequence empty_sequence = Sequence();
        double root_val = build_max_policy(player_br, root, empty_sequence, terminal_sequences, sequence_hash_to_index_map, game, false, infoset_empirical_values, action_empirical_values, cohorts);  
    }

    std::cout << "Max Reward policy computed" << std::endl;

    for (int card_index = 0; card_index < player_cards.size(); card_index++){
        std::string hash_1 = "a-" + std::string(1, player_cards[card_index]) + "--";
        std::string hash_2 = "o-" + std::string(1, player_cards[card_index]) + "--";
        InformationSet root = br_player == 'x' ? InformationSet('x', true, hash_1, game) : InformationSet('o', false, hash_2, game);
        Sequence empty_sequence = Sequence();
        double max_UCB = build_max_policy(player_max_ucb_policy, root, empty_sequence, terminal_sequences, sequence_hash_to_index_map, game, true, infoset_upper_bound_values, action_upper_bound_values, cohorts);
    }

    std::cout << "Max UCB policy computed" << std::endl;
    bool max_UCB_flag = true;

    for (int t = 1; t <= iterations; t += 1) {
        if (t % log_frequency == 0) { 
            double expected_utility = 0.0;
            double exploitability = 0.0;
        
            if (br_player == 'x') {
                expected_utility = get_expected_utility_wrapper(player_br, opponent_policy, game);
                exploitability_log.push_back(std::make_pair(t, exact_br_value - expected_utility));
            } 
            else {
                expected_utility = get_expected_utility_wrapper(opponent_policy, player_br, game);
                exploitability_log.push_back(std::make_pair(t, exact_br_value - expected_utility));
            }
            std::cout << "Expected utility of best response policy: " << expected_utility << std::endl;
            std::cout << "Number of games sampled so far: " << t << std::endl;
        }
  
        if (max_UCB_flag) {
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
            Sequence trajectory = Sequence();
            double reward = sample_game(I_1, I_2, true_cards, start_history, br_player, player_max_ucb_policy, opponent_policy, trajectory);

            std::vector<int> policy_sequences;
            get_policy_sequences_wrapper(player_max_ucb_policy, policy_sequences, game, sequence_hash_to_index_map, br_player, cohorts);

            // update sequence data for policy sequences
            update_sequence_data(policy_sequences, terminal_sequences, reward, trajectory.hash, C_r, C_p);

            // update policies 
            update_max_reward_policy_given_trajectory(player_br, trajectory, terminal_sequences, sequence_hash_to_index_map, infoset_empirical_values, action_empirical_values, game, br_player, cohorts);

            for (int card_index = 0; card_index < player_cards.size(); card_index++){
                std::string hash_1 = "a-" + std::string(1, player_cards[card_index]) + "--";
                std::string hash_2 = "o-" + std::string(1, player_cards[card_index]) + "--";
                InformationSet root = br_player == 'x' ? InformationSet('x', true, hash_1, game) : InformationSet('o', false, hash_2, game);
                Sequence empty_sequence = Sequence();
                double max_UCB = update_max_ucb_policy_given_trajectory(player_max_ucb_policy, player_max_ucb_policy, root, empty_sequence, terminal_sequences, sequence_hash_to_index_map, game, infoset_upper_bound_values, action_upper_bound_values, cohorts);
            }

            max_UCB_flag = false;
        }
        else {
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
            Sequence trajectory = Sequence();
            double reward = sample_game(I_1, I_2, true_cards, start_history, br_player, player_br, opponent_policy, trajectory);

            std::vector<int> policy_sequences;
            get_policy_sequences_wrapper(player_br, policy_sequences, game, sequence_hash_to_index_map, br_player, cohorts);
            // update sequence data for policy sequences
            update_sequence_data(policy_sequences, terminal_sequences, reward, trajectory.hash, C_r, C_p);

            // update policies
            update_max_reward_policy_given_trajectory(player_br, trajectory, terminal_sequences, sequence_hash_to_index_map, infoset_empirical_values, action_empirical_values, game, br_player, cohorts);

            for (int card_index = 0; card_index < player_cards.size(); card_index++){
                std::string hash_1 = "a-" + std::string(1, player_cards[card_index]) + "--";
                std::string hash_2 = "o-" + std::string(1, player_cards[card_index]) + "--";
                InformationSet root = br_player == 'x' ? InformationSet('x', true, hash_1, game) : InformationSet('o', false, hash_2, game);
                Sequence empty_sequence = Sequence();
                double max_UCB = update_max_ucb_policy_given_trajectory(player_max_ucb_policy, player_br, root, empty_sequence, terminal_sequences, sequence_hash_to_index_map, game, infoset_upper_bound_values, action_upper_bound_values, cohorts);
            }

            max_UCB_flag = true;
        }
    }

    std::cout << "Saving exploitability log" << std::endl;
    std::string file_name = "data/sequence/" + exp_name + "_" + std::string(1, game) + "_poker_" + std::string(1, br_player) + "_C_r=" + std::to_string(C_r) + "_C_p=" + std::to_string(C_p) + "_seq-LUCB_exploitability_log_" + std::to_string(experiment_number) + ".txt";

    std::ofstream f(file_name);
    for (int i = 0; i < exploitability_log.size(); i++) {
        f << exploitability_log[i].first << " " << exploitability_log[i].second << std::endl;
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
    double C_r = std::stod(argv[8]);
    double C_p = std::stod(argv[9]);
    std::string exp_name = argv[10];
    int start_index = std::stoi(argv[11]);
    
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

    std::vector<std::string>& player_information_sets = player == 'x' ? P1_information_sets : P2_information_sets;
    std::vector<std::vector<std::unordered_set<std::string>>> cohorts(player_information_sets.size(), std::vector<std::unordered_set<std::string>>(6));
    precompute_cohorts(cohorts, player_information_sets, player, game);
    // initialize sequences for player
    std::vector<Sequence> terminal_sequences;
    std::unordered_map<std::string, int> sequence_hash_to_index_map;
    get_sequences_wrapper(terminal_sequences, game, player, sequence_hash_to_index_map, cohorts);
    std::cout << "Number of sequences: " << terminal_sequences.size() << std::endl;

    // compute epsilon best response
    int experiment_num = start_index;

    while (experiment_num < experiments + start_index) {
        if (player == 'x') {
            PolicyVec uniform_x('x', P1_information_sets, game);
            calc_br_sequence_LUCB(policy_obj_o, 'x', P1_information_sets, log_frequency, uniform_x, experiment_num, game, iterations, C_r, C_p, exp_name, sequence_hash_to_index_map, terminal_sequences, cohorts);
        }
        else {
            PolicyVec uniform_o('o', P2_information_sets, game);
            calc_br_sequence_LUCB(policy_obj_x, 'o', P2_information_sets, log_frequency, uniform_o, experiment_num, game, iterations, C_r, C_p, exp_name, sequence_hash_to_index_map, terminal_sequences, cohorts);
        }

        std::cout << "(" << experiment_num << " experiments done)" << std::endl;
        experiment_num += 1;
    }
}