#include "cpp_headers/rbt_classes.hpp"
#include "cpp_headers/rbt_utilities.hpp"
#include <random>
#include <cmath>
int NUMBER_THREADS = 96;
int NUM_ACTIONS = 13;

// this code is a version of sequence UCB (for regret minimization) which uses the sequence with the minimum n_hat count to decide the UCB of the policy

static std::random_device rd;
static std::mt19937 generator(rd());


int sampleIndex(const std::vector<double>& probabilities) {
    std::discrete_distribution<int> distribution(probabilities.begin(), probabilities.end());
    return distribution(generator);
}


int get_number_of_unknown_opponent_moves(InformationSet& I) {
    std::string B = I.get_board_from_hash();
    int count_x = 0;
    int count_o = 0;
    for (int i = 0; i < 9; i++) {
        if (B[i] == 'x') {
            count_x++;
        }
        if (B[i] == 'o') {
            count_o++;
        }
    }
    if (I.player == 'x') {
        return count_x - count_o;
    } else {
        return count_o - count_x + 1;
    }
}


void get_uncertain_squares(InformationSet& I, std::vector<int> &squares) {
    std::string B = I.get_board_from_hash();
    for (int i = 0; i < 9; i++) {
        if (B[i] == '-') {
            squares.push_back(i);
        }
    }
}


void get_states_in_infoset(InformationSet &I, std::vector<TicTacToeBoard> &states) {
    int num_unknown_opponent_moves = get_number_of_unknown_opponent_moves(I);
    std::string board_copy = I.get_board_from_hash();

    for (int i = 0; i < 9; i++) {
        if (board_copy[i] == '-') {
            board_copy[i] = '0';
        }
    }

    if (num_unknown_opponent_moves == 0) {
        states.push_back(TicTacToeBoard(board_copy));
    } 
    else {
        std::vector<int> uncertain_ind;
        get_uncertain_squares(I, uncertain_ind);

        std::vector<char> base_perm(num_unknown_opponent_moves, I.other_player());
        base_perm.insert(base_perm.end(), uncertain_ind.size() - num_unknown_opponent_moves, '0');
        // sort 
        std::sort(base_perm.begin(), base_perm.end());

        do {
            TicTacToeBoard new_state(board_copy);
            for (int j = 0; j < base_perm.size(); j++) {
                new_state[uncertain_ind[j]] = base_perm[j];
            }
            char winner;
            if (!new_state.is_win(winner) && !new_state.is_over()) {
                states.push_back(new_state);
            }
        } while (std::next_permutation(base_perm.begin(), base_perm.end()));

    }
}


void get_cohort(InformationSet I, int action, std::unordered_set<std::string> &cohort) {
    if (I.move_flag) {
        I.update_move(action, I.player);
        I.reset_zeros();
        if (I.get_index() != -1) {
            cohort.insert(I.get_hash());
        }
        return;
    }
    else {
        std::vector<TicTacToeBoard> states;
        get_states_in_infoset(I, states);
        for (TicTacToeBoard &state : states) {
            InformationSet new_I = I;
            new_I.simulate_sense(action, state);
            if (new_I.get_index() != -1) {
                if (cohort.find(new_I.get_hash()) == cohort.end()) {
                    cohort.insert(new_I.get_hash());
                }
            }
        }
    }
}


void precompute_cohorts(std::vector<std::vector<std::unordered_set<std::string>>>& cohorts, std::vector<std::string>& player_information_sets, char br_player) {
    std::cerr << "Building cohorts" << std::endl;
    // precompute cohorts
    
    #pragma omp parallel for num_threads(NUMBER_THREADS)
    for (int i = 0; i < player_information_sets.size(); i++) {
        InformationSet I = InformationSet(br_player, get_move_flag(player_information_sets[i], br_player), player_information_sets[i]);
        std::vector<int> legal_actions;
        I.get_actions(legal_actions);
        for (int a : legal_actions) {
            std::unordered_set<std::string> cohort;
            get_cohort(I, a, cohort);
            cohorts[I.get_index()][a] = cohort;
        }
    }
    std::cerr << "Cohorts built" << std::endl;
}


double sample_game(InformationSet& I_1, InformationSet& I_2, TicTacToeBoard& true_board, History& current_history, char br_player, 
                   PolicyVec& policy_obj, PolicyVec& opponent_policy, Sequence& trajectory, char curr_player, std::vector<int>& infoset_reach_counts, std::vector<std::vector<int>>& action_reach_counts) {
    InformationSet I = curr_player == 'x' ? I_1 : I_2;
    int action = 0;
    
    if (br_player == I.player){
        infoset_reach_counts[I.get_index()] += 1;
        std::vector<double>& prob_dist = policy_obj.policy_dict[I.get_index()];
        action = sampleIndex(prob_dist);
        trajectory.extend(I, action);
        action_reach_counts[I.get_index()][action] += 1;
    }
    else {
        std::vector<double>& prob_dist = opponent_policy.policy_dict[I.get_index()];
        action = sampleIndex(prob_dist);
    }

    if (I.move_flag) {
        TicTacToeBoard new_board = true_board;
        bool success = new_board.update_move(action, curr_player);
        current_history.history.push_back(action);
        
        char winner;
        if (success && !new_board.is_win(winner) && !new_board.is_over()) {
            InformationSet new_I = I;
            new_I.update_move(action, curr_player);

            if (curr_player == 'x') {
                return sample_game(new_I, I_2, new_board, current_history, br_player, policy_obj, opponent_policy, trajectory, 'o', infoset_reach_counts, action_reach_counts);
            }
            else {
                return sample_game(I_1, new_I, new_board, current_history, br_player, policy_obj, opponent_policy, trajectory, 'x', infoset_reach_counts, action_reach_counts);
            }
        } else {
            TerminalHistory H_T = TerminalHistory(current_history.history);
            H_T.set_reward();
            double r = br_player == 'x' ? (double) H_T.reward[0] : (double) H_T.reward[1];
            return r; 
        }
    }
    else {
        InformationSet new_I = I;
        TicTacToeBoard new_board = true_board;
        new_I.simulate_sense(action, new_board);
        current_history.history.push_back(action);

        if (curr_player == 'x') {
            return sample_game(new_I, I_2, new_board, current_history, br_player, policy_obj, opponent_policy, trajectory, 'x', infoset_reach_counts, action_reach_counts);
        } else {
            return sample_game(I_1, new_I, new_board, current_history, br_player, policy_obj, opponent_policy, trajectory, 'o', infoset_reach_counts, action_reach_counts);
        }
    }
}


void update_max_reward_policy_given_trajectory(PolicyVec& policy_obj, Sequence& trajectory, std::vector<Sequence>& terminal_sequences, std::unordered_map<std::string, int>& sequence_hash_to_index_map,
                                               std::vector<double>& global_infoset_values, std::vector<std::vector<double>>& global_action_values, char br_player, std::vector<std::vector<std::unordered_set<std::string>>>& cohorts) {
    for (int j = trajectory.seq.size() - 1; j >= 0; j--){
        std::string I_hash = trajectory.seq[j].first;
        int played_action = trajectory.seq[j].second;
        bool move_flag = get_move_flag(I_hash, br_player);
        InformationSet I(br_player, move_flag, I_hash);

        std::string seq_hash = I_hash + "-" + std::to_string(played_action);
        int s_index = sequence_hash_to_index_map[seq_hash];
        std::vector<double>& prob_dist = policy_obj.policy_dict[I.get_index()];

        // compute value of played action
        std::unordered_set<std::string>& cohort = cohorts[I.get_index()][played_action];

        double action_value = 0.0;
        for (std::string I_prime_hash : cohort){
            InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash);
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

        // std::cout << "Infoset: " << I.get_hash() << " ";
        std::vector<int> candidate_actions;
        for (int a: legal_actions){
            if (fabs(global_action_values[I.get_index()][a] - infoset_value) < 1e-6){
                candidate_actions.push_back(a);
            }
            // std::cout << "Action " << a << ": " << global_action_values[I.get_index()][a] << " ";
        }
        // std::cout << std::endl;

        // sample from candidate actions
        int best_action = candidate_actions[sampleIndex(std::vector<double>(candidate_actions.size(), 1.0 / candidate_actions.size()))];
        // int best_action = candidate_actions[0];
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


double find_maximizing_sequence(PolicyVec& played_policy_obj, InformationSet& I, Sequence& trajectory, Sequence& maximising_trajectory, std::vector<Sequence>& terminal_sequences, 
                                std::unordered_map<std::string, int>& sequence_hash_to_index_map, std::vector<double>& global_infoset_values, std::vector<std::vector<double>>& global_action_values,
                                std::vector<std::vector<std::unordered_set<std::string>>>& cohorts, std::vector<int>& infoset_maximising_trajectories, 
                                std::vector<std::vector<int>>& action_maximising_trajectories, double C, int t, double delta){
    std::vector<int> legal_actions;
    I.get_actions(legal_actions);

    std::vector<int> played_actions;
    I.get_actions_given_policy(played_actions, played_policy_obj);
    int played_action = played_actions[0];
    std::vector<double> action_UCBs(NUM_ACTIONS, -1.0);
    double infoset_UCB = -1.0;

    std::unordered_set<std::string>& cohort = cohorts[I.get_index()][played_action];
    std::vector<double> cohort_values(cohort.size() + 1, 0.0);
    std::vector<Sequence> cohort_maximising_trajectories(cohort.size() + 1, Sequence());
    Sequence new_trajectory = trajectory;
    new_trajectory.extend(I, played_action);

    int ind = 0; 
    int n_pi = 0;
    int s_index = sequence_hash_to_index_map[new_trajectory.hash];
    n_pi = terminal_sequences[s_index].n_pi;
    if (n_pi > 0){
        cohort_values[ind] = std::min(1.0, terminal_sequences[s_index].p * terminal_sequences[s_index].ucb_r + std::sqrt(C * std::log((double) t / delta) / (double) n_pi));
    }
    else {
        cohort_values[ind] = 1.0;
    }
    
    cohort_maximising_trajectories[ind] = terminal_sequences[s_index];                      
    
    for (std::string I_prime_hash : cohort){
        ind++;
        InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash);
        cohort_values[ind] = find_maximizing_sequence(played_policy_obj, I_prime, new_trajectory, cohort_maximising_trajectories[ind],
                                                      terminal_sequences, sequence_hash_to_index_map, global_infoset_values, 
                                                      global_action_values, cohorts, infoset_maximising_trajectories, action_maximising_trajectories,
                                                      C, t, delta);
        
        cohort_values[0] += global_infoset_values[I_prime.get_index()];
        cohort_values[0] = std::min(1.0, cohort_values[0]);

        for (std::string I_prime_hash_j : cohort){
            if (I_prime_hash_j == I_prime_hash) continue;
            InformationSet I_prime_j(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash);
            cohort_values[ind] += global_infoset_values[I_prime_j.get_index()];
        }
        cohort_values[ind] += terminal_sequences[s_index].p * terminal_sequences[s_index].ucb_r;
        
        cohort_values[ind] = std::min(1.0, cohort_values[ind]);
        cohort_maximising_trajectories[ind] = terminal_sequences[infoset_maximising_trajectories[I_prime.get_index()]];
        
        if (cohort_values[ind] > action_UCBs[played_action]){
            action_UCBs[played_action] = cohort_values[ind];
        }
    }

    std::vector<int> candidate_cohort_indices;
    if (cohort_values[0] > action_UCBs[played_action]){
        action_UCBs[played_action] = cohort_values[0];
        candidate_cohort_indices.push_back(0);
    }
    
    for (ind = 1; ind < cohort.size() + 1; ind++){
        if (fabs(cohort_values[ind] - action_UCBs[played_action]) < 1e-6){
            candidate_cohort_indices.push_back(ind);
        }
    }

    ind = candidate_cohort_indices[sampleIndex(std::vector<double>(candidate_cohort_indices.size(), 1.0 / candidate_cohort_indices.size()))];
    maximising_trajectory = cohort_maximising_trajectories[ind];
    action_maximising_trajectories[I.get_index()][played_action] = sequence_hash_to_index_map[maximising_trajectory.hash];

    for (int a : legal_actions){
        if (a != played_action){
            int n_pi_a = terminal_sequences[action_maximising_trajectories[I.get_index()][a]].n_pi;
            if (n_pi_a == 0){
                action_UCBs[a] = 1.0;
            }
            else {
                action_UCBs[a] = std::min(1.0, global_action_values[I.get_index()][a] + std::sqrt(C * std::log((double) t / delta) / (double) n_pi_a));
            }
        }
    }

    std::vector<int> candidate_actions;
    for (int a : legal_actions){
        infoset_UCB = std::max(infoset_UCB, action_UCBs[a]);
        if (fabs(action_UCBs[a] - infoset_UCB) < 1e-6){
            candidate_actions.push_back(a);
        }
    }

    // sample from candidate actions
    int action = candidate_actions[sampleIndex(std::vector<double>(candidate_actions.size(), 1.0 / candidate_actions.size()))];
    maximising_trajectory = terminal_sequences[action_maximising_trajectories[I.get_index()][action]];
    infoset_maximising_trajectories[I.get_index()] = sequence_hash_to_index_map[maximising_trajectory.hash];
    return infoset_UCB;
}


void build_max_UCB_policy_given_maximising_sequence(PolicyVec& policy_obj, Sequence& maximising_trajectory, char br_player, std::unordered_map<std::string, int>& backup){
    for (int j = maximising_trajectory.seq.size() - 1; j >= 0; j--){
        std::string I_hash = maximising_trajectory.seq[j].first;
        int action = maximising_trajectory.seq[j].second;
        bool move_flag = get_move_flag(I_hash, br_player);
        InformationSet I(br_player, move_flag, I_hash);
        std::vector<double>& prob_dist = policy_obj.policy_dict[I.get_index()];

        // set action to 1.0 and all other actions to 0.0
        std::vector<int> legal_actions;
        I.get_actions(legal_actions);
        for (int a : legal_actions){
            if (prob_dist[a] == 1.0){
                backup[I_hash] = a;
            }

            if (a == action){
                prob_dist[a] = 1.0;
            }
            else{
                prob_dist[a] = 0.0;
            }
        }
    }
}


void restore_backup(PolicyVec& policy_obj, char br_player, std::unordered_map<std::string, int>& backup){
    for (auto& it: backup){
        std::string I_hash = it.first;
        int action = it.second;

        bool move_flag = get_move_flag(I_hash, br_player);
        InformationSet I(br_player, move_flag, I_hash);
        std::vector<double>& prob_dist = policy_obj.policy_dict[I.get_index()];

        std::vector<int> legal_actions;
        I.get_actions(legal_actions);
        for (int a : legal_actions){
            if (a == action){
                prob_dist[a] = 1.0;
            }
            else{
                prob_dist[a] = 0.0;
            }
        }
    }
}


void get_sequences(InformationSet& I, Sequence& trajectory, std::vector<Sequence>& terminal_sequences, std::unordered_map<std::string, int>& sequence_hash_to_index_map, std::vector<std::vector<std::unordered_set<std::string>>>& cohorts) {
    std::vector<int> legal_actions;
    I.get_actions(legal_actions);
    
    for (int a : legal_actions){
        Sequence new_trajectory = trajectory;
        new_trajectory.extend(I, a);
        terminal_sequences.push_back(new_trajectory);
        sequence_hash_to_index_map[new_trajectory.hash] = terminal_sequences.size() - 1;

        std::unordered_set<std::string>& cohort = cohorts[I.get_index()][a];

        for (std::string I_prime_hash : cohort){
            InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash);
            get_sequences(I_prime, new_trajectory, terminal_sequences, sequence_hash_to_index_map, cohorts);
        }
    }
}


void get_sequences_wrapper(std::vector<Sequence>& terminal_sequences, char br_player, std::unordered_map<std::string, int>& sequence_hash_to_index_map, 
                           std::vector<std::vector<std::unordered_set<std::string>>>& cohorts) {
    std::string board = "000000000";
    TicTacToeBoard true_board = TicTacToeBoard(board);
    std::string hash_1 = "";
    std::string hash_2 = "";
    InformationSet I_1 = InformationSet('x', true, hash_1);
    InformationSet I_2 = InformationSet('o', false, hash_2);
    Sequence empty_sequence = Sequence();
    InformationSet root = br_player == 'x' ? I_1 : I_2;
    get_sequences(root, empty_sequence, terminal_sequences, sequence_hash_to_index_map, cohorts);
}


void init_maximising_seq(InformationSet& I, Sequence& trajectory, std::string& maximising_sequence_hash, std::vector<std::vector<std::unordered_set<std::string>>>& cohorts, 
                         std::vector<int>& infoset_maximising_sequences, std::vector<std::vector<int>>& action_maximising_sequences,
                         std::unordered_map<std::string, int>& sequence_hash_to_index_map, std::vector<Sequence>& terminal_sequences) {
    std::vector<int> legal_actions;
    I.get_actions(legal_actions);
    
    for (int a : legal_actions){
        Sequence new_trajectory = trajectory;
        new_trajectory.extend(I, a);

        std::unordered_set<std::string>& cohort = cohorts[I.get_index()][a];

        for (std::string I_prime_hash : cohort){
            InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash);
            init_maximising_seq(I_prime, new_trajectory, maximising_sequence_hash, cohorts, infoset_maximising_sequences, action_maximising_sequences, sequence_hash_to_index_map, terminal_sequences);
        }

        if (cohort.size() == 0){
            action_maximising_sequences[I.get_index()][a] = sequence_hash_to_index_map[new_trajectory.hash];
        }
        else {
            action_maximising_sequences[I.get_index()][a] = sequence_hash_to_index_map[maximising_sequence_hash];
        }
    }

    infoset_maximising_sequences[I.get_index()] = action_maximising_sequences[I.get_index()][legal_actions[0]];
    maximising_sequence_hash = terminal_sequences[infoset_maximising_sequences[I.get_index()]].hash;
}


void update_reach_and_sequence_data(PolicyVec& policy_obj, InformationSet& I, Sequence& trajectory, std::unordered_map<std::string, int>& sequence_hash_to_index_map, 
                                    std::vector<std::vector<std::unordered_set<std::string>>>& cohorts, std::vector<int>& infoset_reach_counts,
                                    double reach, std::vector<Sequence>& terminal_sequences, double reward_game, 
                                    std::string terminal_hash_game, double C_r, std::vector<std::vector<int>>& action_reach_counts) {
    std::vector<int> policy_actions;
    I.get_actions_given_policy(policy_actions, policy_obj);
    
    for (int a : policy_actions){
        Sequence new_trajectory = trajectory;
        new_trajectory.extend(I, a);
        int index = sequence_hash_to_index_map[new_trajectory.hash];
        int reach_sum = action_reach_counts[I.get_index()][a];

        // update data for the sequence
        if (terminal_sequences[index].hash == terminal_hash_game){
            terminal_sequences[index].r += reward_game;
            terminal_sequences[index].n += 1;
            terminal_sequences[index].n_pi += 1;
            if (reach_sum == 0){
                terminal_sequences[index].p = reach;
            }
            else {
                terminal_sequences[index].p = reach * (double) terminal_sequences[index].n / (double) reach_sum;
            }
            
            terminal_sequences[index].ucb_r = ((double) terminal_sequences[index].r / (double) terminal_sequences[index].n) + C_r * std::sqrt(1.0 / (double) terminal_sequences[index].n);
        }
        else{
            terminal_sequences[index].n_pi += 1;
            if (reach_sum == 0){
                terminal_sequences[index].p = reach;
            }
            else {
                terminal_sequences[index].p = reach * (double) terminal_sequences[index].n / (double) reach_sum;
            } 
        }
        
        std::unordered_set<std::string>& cohort = cohorts[I.get_index()][a];

        for (std::string I_prime_hash : cohort){
            InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash);
            double new_reach = 0.0;
            if (reach_sum != 0) {
                new_reach = reach * ((double) infoset_reach_counts[I_prime.get_index()] / (double) reach_sum);
            }
            update_reach_and_sequence_data(policy_obj, I_prime, new_trajectory, sequence_hash_to_index_map, cohorts, infoset_reach_counts, new_reach, terminal_sequences, reward_game, terminal_hash_game, C_r, action_reach_counts);
        }
    }
}


void update_reach_and_sequence_data_wrapper(PolicyVec& policy_obj, std::unordered_map<std::string, int>& sequence_hash_to_index_map, char br_player, 
                                            std::vector<std::vector<std::unordered_set<std::string>>>& cohorts, std::vector<int>& infoset_reach_counts, 
                                            std::vector<Sequence>& terminal_sequences, double reward_game, std::string terminal_hash_game, double C_r, std::vector<std::vector<int>>& action_reach_counts) {
    std::string board = "000000000";
    TicTacToeBoard true_board = TicTacToeBoard(board);
    std::string hash_1 = "";
    std::string hash_2 = "";
    InformationSet I_1 = InformationSet('x', true, hash_1);
    InformationSet I_2 = InformationSet('o', false, hash_2);
    Sequence empty_sequence = Sequence();
    InformationSet root = br_player == 'x' ? I_1 : I_2;
    update_reach_and_sequence_data(policy_obj, root, empty_sequence, sequence_hash_to_index_map, cohorts, infoset_reach_counts, 1.0, terminal_sequences, reward_game, terminal_hash_game, C_r, action_reach_counts);
}


void calc_br_sequence_UCB(PolicyVec& opponent_policy, char br_player, std::vector<std::string>& player_information_sets, int log_frequency, PolicyVec& player_br, 
                           int experiment_number, int iterations, double C, double C_r, std::string& exp_name, std::unordered_map<std::string, int>& sequence_hash_to_index_map, 
                           std::vector<Sequence>& terminal_sequences, std::vector<std::vector<std::unordered_set<std::string>>>& cohorts, double delta) {  
    // store infoset and action values to avoid recomputation for parts of the tree that don't change
    std::vector<double> infoset_empirical_values(player_information_sets.size(), 0.0);
    std::vector<std::vector<double>> action_empirical_values(player_information_sets.size(), std::vector<double>(NUM_ACTIONS, 0.0));

    std::vector<int> infoset_maximising_trajectories(player_information_sets.size(), -1);
    std::vector<std::vector<int>> action_maximising_trajectories(player_information_sets.size(), std::vector<int>(NUM_ACTIONS, -1));
    
    std::string board = "000000000";
    TicTacToeBoard true_board = TicTacToeBoard(board);
    std::string hash_1 = "";
    std::string hash_2 = "";
    InformationSet I_1 = InformationSet('x', true, hash_1);
    InformationSet I_2 = InformationSet('o', false, hash_2);
    Sequence empty_sequence = Sequence();
    std::string maximising_trajectory_hash = "";
    InformationSet root = br_player == 'x' ? I_1 : I_2;
    init_maximising_seq(root, empty_sequence, maximising_trajectory_hash, cohorts, infoset_maximising_trajectories, action_maximising_trajectories, sequence_hash_to_index_map, terminal_sequences);

    std::vector<std::vector<int>> action_reach_counts(player_information_sets.size(), std::vector<int>(NUM_ACTIONS, 0));
    std::vector<int> infoset_reach_counts(player_information_sets.size(), 0);

    std::vector<std::pair<int, double>> exploitability_log; 
    std::vector<std::pair<int, double>> reward_log; 
    double cumulative_reward = 0.0;

    // compute exact best response for comparison
    PolicyVec exact_br(br_player, player_information_sets, false);
    compute_best_response_wrapper(opponent_policy, exact_br, br_player);
    double exact_br_value = 0.0;

    if (br_player == 'x') {
        exact_br_value = get_expected_utility_wrapper(exact_br, opponent_policy);
    } 
    else {
        exact_br_value = get_expected_utility_wrapper(opponent_policy, exact_br);
    }
    std::cerr << "Exact best response value: " << exact_br_value << std::endl;

    board = "000000000";
    true_board = TicTacToeBoard(board);
    hash_1 = "";
    hash_2 = "";
    I_1 = InformationSet('x', true, hash_1);
    I_2 = InformationSet('o', false, hash_2);
    empty_sequence = Sequence();
    root = br_player == 'x' ? I_1 : I_2;
    std::unordered_map<std::string, int> backup;

    for (int t = 1; t <= iterations; t += 1) {
        board = "000000000";
        true_board = TicTacToeBoard(board);
        std::string hash_1 = "";
        std::string hash_2 = "";
        I_1 = InformationSet('x', true, hash_1);
        I_2 = InformationSet('o', false, hash_2);
            
        std::vector<int> h = {};
        TerminalHistory start_history = TerminalHistory(h);
        Sequence trajectory = Sequence();
        double reward = sample_game(I_1, I_2, true_board, start_history, br_player, player_br, opponent_policy, trajectory, 'x', infoset_reach_counts, action_reach_counts);
        cumulative_reward += reward;

        update_reach_and_sequence_data_wrapper(player_br, sequence_hash_to_index_map, br_player, cohorts, infoset_reach_counts, terminal_sequences, reward, trajectory.hash, C_r, action_reach_counts);

        // undo changes to player br using backup
        restore_backup(player_br, br_player, backup);

        // update policies
        update_max_reward_policy_given_trajectory(player_br, trajectory, terminal_sequences, sequence_hash_to_index_map, infoset_empirical_values, action_empirical_values, br_player, cohorts);

        if (t % log_frequency == 0) { 
            double expected_utility = 0.0;
            double exploitability = 0.0;
        
            if (br_player == 'x') {
                expected_utility = get_expected_utility_wrapper(player_br, opponent_policy);
                exploitability_log.push_back(std::make_pair(t, exact_br_value - expected_utility));
            } 
            else {
                expected_utility = get_expected_utility_wrapper(opponent_policy, player_br);
                exploitability_log.push_back(std::make_pair(t, exact_br_value - expected_utility));
            }
            reward_log.push_back(std::make_pair(t, cumulative_reward / (double) t));
            std::cout << "Cumulative Average Reward: " << cumulative_reward / (double) t << std::endl;
            std::cerr << "Expected utility of best response policy: " << expected_utility << std::endl;
            std::cerr << "Number of games sampled so far: " << t << std::endl;
        }

        hash_1 = "";
        hash_2 = "";
        I_1 = InformationSet('x', true, hash_1);
        I_2 = InformationSet('o', false, hash_2);
        empty_sequence = Sequence();
        Sequence maximising_trajectory = Sequence();
        root = br_player == 'x' ? I_1 : I_2;

        find_maximizing_sequence(player_br, root, empty_sequence, maximising_trajectory, terminal_sequences, 
                                    sequence_hash_to_index_map, infoset_empirical_values, action_empirical_values, cohorts, 
                                    infoset_maximising_trajectories, action_maximising_trajectories, C, t, delta);
        
        backup = std::unordered_map<std::string, int>();
        build_max_UCB_policy_given_maximising_sequence(player_br, maximising_trajectory, br_player, backup);
    }

    std::cerr << "Saving exploitability log" << std::endl;
    std::string file_name = "data/sequence/" + exp_name + "_rbt_" + std::string(1, br_player) + "_C=" + std::to_string(C) + "_C_r=" + std::to_string(C_r) + "_seq-UCB_exploitability_log_" + std::to_string(experiment_number) + ".txt";

    std::ofstream f1(file_name);
    for (int i = 0; i < exploitability_log.size(); i++) {
        f1 << exploitability_log[i].first << " " << exploitability_log[i].second << std::endl;
    }
    f1.close();

    file_name = "data/sequence/" + exp_name + "_rbt_" + std::string(1, br_player) + "_C=" + std::to_string(C) + "_C_r=" + std::to_string(C_r) + "_seq-UCB_reward_log_" + std::to_string(experiment_number) + ".txt";

    std::ofstream f2(file_name);
    for (int i = 0; i < reward_log.size(); i++) {
        f2 << reward_log[i].first << " " << reward_log[i].second << std::endl;
    }
    f2.close();
}


int main(int argc, char* argv[]) {
    std::cerr.precision(17);
    std::string file_path_1 = argv[1]; // start policy P1
    std::string file_path_2 = argv[2]; // start policy P2
    char player = argv[3][0];
    int iterations = std::stoi(argv[4]);
    int log_frequency = std::stoi(argv[5]);
    int experiments = std::stoi(argv[6]);
    double C = std::stod(argv[7]);
    double C_r = std::stod(argv[8]);
    std::string exp_name = argv[9];
    int start_index = std::stoi(argv[10]);
    double delta = 0.05;
    
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
    for (int i = 0; i < P1_information_sets.size(); i++) {
        InformationSet::P1_hash_to_int_map[P1_information_sets[i]] = i;
    }
    for (int i = 0; i < P2_information_sets.size(); i++) {
        InformationSet::P2_hash_to_int_map[P2_information_sets[i]] = i;
    }

    // load policies
    std::cerr << "Loading policies" << std::endl;
    PolicyVec policy_obj_x('x', file_path_1, true);
    PolicyVec policy_obj_o('o', file_path_2, true);
    double expected_utility = get_expected_utility_wrapper(policy_obj_x, policy_obj_o);
    std::cerr << "Expected utility of initial policies: " << expected_utility << std::endl;

    // compute epsilon best response
    int experiment_num = start_index;

    std::vector<std::string>& player_information_sets = player == 'x' ? P1_information_sets : P2_information_sets;
    std::vector<std::vector<std::unordered_set<std::string>>> cohorts(player_information_sets.size(), std::vector<std::unordered_set<std::string>>(NUM_ACTIONS));
    precompute_cohorts(cohorts, player_information_sets, player);

    while (experiment_num < experiments + start_index) {
        // initialize sequences for player
        std::vector<Sequence> terminal_sequences;
        std::unordered_map<std::string, int> sequence_hash_to_index_map;
        get_sequences_wrapper(terminal_sequences, player, sequence_hash_to_index_map, cohorts);
        std::cerr << "Number of sequences: " << terminal_sequences.size() << std::endl;

        if (player == 'x') {
            PolicyVec random_x('x', P1_information_sets, false);
            calc_br_sequence_UCB(policy_obj_o, 'x', P1_information_sets, log_frequency, random_x, experiment_num, iterations, C, C_r, exp_name, sequence_hash_to_index_map, terminal_sequences, cohorts, delta);
        }
        else {
            PolicyVec random_o('o', P2_information_sets, false);
            calc_br_sequence_UCB(policy_obj_x, 'o', P2_information_sets, log_frequency, random_o, experiment_num, iterations, C, C_r, exp_name, sequence_hash_to_index_map, terminal_sequences, cohorts, delta);
        }

        std::cerr << "(" << experiment_num << " experiments done)" << std::endl;
        experiment_num += 1;
    }
}