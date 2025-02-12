#include "cpp_headers/rbt_classes.hpp"
#include "cpp_headers/rbt_utilities.hpp"
#include <random>
#include <cmath>
int NUMBER_THREADS = 96;

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


int explore(InformationSet& I_1, InformationSet& I_2, InformationSet previous_opponent_I, TicTacToeBoard& true_board, History& current_history, char curr_player, char br_player, 
            PolicyVec& opponent_policy, std::vector<std::vector<int>>& I_a_tickmark, std::vector<int>& I_tickmark, double& reward, int m, 
            std::vector<int>& infoset_reach_count, std::vector<std::vector<std::vector<int>>>& empirical_action_reward, std::vector<std::vector<int>>& action_terminal_reach_count) {
    InformationSet I = curr_player == 'x' ? I_1 : I_2;
    int action = 0;
    int terminal_flag = 0;
    int is_child_infoset_ticked = 0;
    
    if (br_player == curr_player){
        infoset_reach_count[I.get_index()] += 1;

        std::vector<int> legal_actions;
        I.get_actions(legal_actions);
        std::vector<int> A;
        std::vector<double> prob_dist(13, 0.0); 

        for (int a : legal_actions){
            if (I_a_tickmark[I.get_index()][a] < m){
                A.push_back(a);
            }
        }

        if (A.size() == 0){
            for (int a : legal_actions){
                prob_dist[a] = 1.0/legal_actions.size();
            }
        }
        else{
            for (int a : A){
                prob_dist[a] = 1.0/A.size();
            }
        }

        action = sampleIndex(prob_dist);
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
            new_I.reset_zeros();

            if (curr_player == 'x') {
                is_child_infoset_ticked = explore(new_I, I_2, I, new_board, current_history, 'o', br_player, opponent_policy, I_a_tickmark, I_tickmark, reward, m, infoset_reach_count, empirical_action_reward, action_terminal_reach_count);
            } else {
                is_child_infoset_ticked = explore(I_1, new_I, I, new_board, current_history, 'x', br_player, opponent_policy, I_a_tickmark, I_tickmark, reward, m, infoset_reach_count, empirical_action_reward, action_terminal_reach_count);
            }
        } else {
            TerminalHistory H_T = TerminalHistory(current_history.history);
            H_T.set_reward();
            int r = br_player == 'x' ? H_T.reward[0] : H_T.reward[1];
            terminal_flag = 1;

            // update action pull count and empirical action reward only when action leads to terminal state
            if (curr_player == br_player){
                action_terminal_reach_count[I.get_index()][action] += 1;
                if (r == 1){
                    empirical_action_reward[I.get_index()][action][0] += 1;
                }
                else if (r == 0){
                    empirical_action_reward[I.get_index()][action][1] += 1;
                }
                else if (r == -1){
                    empirical_action_reward[I.get_index()][action][2] += 1;
                }
            }
            else {
                // find third last action in history
                int third_last_action = current_history.history[current_history.history.size() - 3];
                action_terminal_reach_count[previous_opponent_I.get_index()][third_last_action] += 1;

                if (r == 1){
                    empirical_action_reward[previous_opponent_I.get_index()][third_last_action][0] += 1;
                }
                else if (r == 0){
                    empirical_action_reward[previous_opponent_I.get_index()][third_last_action][1] += 1;
                }
                else if (r == -1){
                    empirical_action_reward[previous_opponent_I.get_index()][third_last_action][2] += 1;
                }
            }

            reward = (double) r;
        }
    }
    else {
        InformationSet new_I = I;
        TicTacToeBoard new_board = true_board;
        new_I.simulate_sense(action, new_board);
        current_history.history.push_back(action);

        if (curr_player == 'x') {
            is_child_infoset_ticked = explore(new_I, I_2, previous_opponent_I, new_board, current_history, 'x', br_player, opponent_policy, I_a_tickmark, I_tickmark, reward, m, infoset_reach_count, empirical_action_reward, action_terminal_reach_count);
        } else {
            is_child_infoset_ticked = explore(I_1, new_I, previous_opponent_I, new_board, current_history, 'o', br_player, opponent_policy, I_a_tickmark, I_tickmark, reward, m, infoset_reach_count, empirical_action_reward, action_terminal_reach_count);
        }
    }

    if (curr_player == br_player){
        if (terminal_flag == 1 || is_child_infoset_ticked == 1){
            I_a_tickmark[I.get_index()][action] += 1;
        }
  
        std::vector<int> legal_actions;
        I.get_actions(legal_actions);

        int count = 0;
        for (int a : legal_actions){
            if (I_a_tickmark[I.get_index()][a] >= m){
                count += 1;
            }
        }

        if (count == legal_actions.size()){
            I_tickmark[I.get_index()] += 1;
            return 1;
        }
        else {
            return 0;
        }
    }
    else {
        if (terminal_flag == 1 || is_child_infoset_ticked == 1){
            return 1;
        }
        else {
            return 0;
        }
    }
}


void explore_wrapper(std::vector<std::vector<int>>& I_a_tickmark, std::vector<int>& I_tickmark, double& reward, PolicyVec& opponent_policy, History& current_history, char br_player, int m, 
                     std::vector<int>& infoset_reach_count, std::vector<std::vector<std::vector<int>>>& empirical_action_reward, std::vector<std::vector<int>>& action_terminal_reach_count) {
    std::string board = "000000000";
    TicTacToeBoard true_board = TicTacToeBoard(board);
    std::string hash_1 = "";
    std::string hash_2 = "";
    InformationSet I_1 = InformationSet('x', true, hash_1);
    InformationSet I_2 = InformationSet('o', false, hash_2);
    explore(I_1, I_2, I_2, true_board, current_history, 'x', br_player, opponent_policy, I_a_tickmark, I_tickmark, reward, m, infoset_reach_count, empirical_action_reward, action_terminal_reach_count);
}


int exploit(InformationSet& I_1, InformationSet& I_2, InformationSet previous_opponent_I, TicTacToeBoard& true_board, History& current_history, char curr_player, char br_player, 
            PolicyVec& policy_obj, PolicyVec& opponent_policy, std::vector<int>& infoset_reach_count, std::vector<std::vector<std::vector<int>>>& empirical_action_reward, 
            std::vector<std::vector<int>>& action_terminal_reach_count, std::vector<int>& I_tickmark, std::vector<std::vector<int>>& I_a_tickmark) {
    InformationSet I = curr_player == 'x' ? I_1 : I_2;
    int action = 0;
    int terminal_flag = 0;
    int is_child_infoset_ticked = 0;
    
    if (br_player == curr_player){
        infoset_reach_count[I.get_index()] += 1;
        std::vector<double>& prob_dist = policy_obj.policy_dict[I.get_index()];
        action = sampleIndex(prob_dist);
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
            new_I.reset_zeros();

            if (curr_player == 'x') {
                is_child_infoset_ticked = exploit(new_I, I_2, I, new_board, current_history, 'o', br_player, policy_obj, opponent_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, I_tickmark, I_a_tickmark);
            } else {
                is_child_infoset_ticked = exploit(I_1, new_I, I, new_board, current_history, 'x', br_player, policy_obj, opponent_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, I_tickmark, I_a_tickmark);
            }
        } else {
            TerminalHistory H_T = TerminalHistory(current_history.history);
            H_T.set_reward();
            int r = br_player == 'x' ? H_T.reward[0] : H_T.reward[1];
            terminal_flag = 1;

            // update action pull count and empirical action reward only when action leads to terminal state
            if (curr_player == br_player){
                action_terminal_reach_count[I.get_index()][action] += 1;        
                if (r == 1){
                    empirical_action_reward[I.get_index()][action][0] += 1;
                }
                else if (r == 0){
                    empirical_action_reward[I.get_index()][action][1] += 1;
                }
                else if (r == -1){
                    empirical_action_reward[I.get_index()][action][2] += 1;
                }
            }
            else {
                // find third last action in history
                int third_last_action = current_history.history[current_history.history.size() - 3];
                action_terminal_reach_count[previous_opponent_I.get_index()][third_last_action] += 1;
                
                if (r == 1){
                    empirical_action_reward[previous_opponent_I.get_index()][third_last_action][0] += 1;
                }
                else if (r == 0){
                    empirical_action_reward[previous_opponent_I.get_index()][third_last_action][1] += 1;
                }
                else if (r == -1){
                    empirical_action_reward[previous_opponent_I.get_index()][third_last_action][2] += 1;
                }
            }
        }
    }
    else {
        InformationSet new_I = I;
        TicTacToeBoard new_board = true_board;
        new_I.simulate_sense(action, new_board);
        current_history.history.push_back(action);

        if (curr_player == 'x') {
            is_child_infoset_ticked = exploit(new_I, I_2, previous_opponent_I, new_board, current_history, 'x', br_player, policy_obj, opponent_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, I_tickmark, I_a_tickmark);
        } else {
            is_child_infoset_ticked = exploit(I_1, new_I, previous_opponent_I, new_board, current_history, 'o', br_player, policy_obj, opponent_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, I_tickmark, I_a_tickmark);
        }
    }

    if (curr_player == br_player){
        if (terminal_flag == 1 || is_child_infoset_ticked == 1){
            I_a_tickmark[I.get_index()][action] += 1;
        }
  
        std::vector<int> legal_actions;
        I.get_actions(legal_actions);

        int count = 0;
        for (int a : legal_actions){
            if (I_a_tickmark[I.get_index()][a] >= 1){
                count += 1;
            }
        }

        if (count == legal_actions.size()){
            I_tickmark[I.get_index()] += 1;
            return 1;
        }
        else {
            return 0;
        }
    }
    else {
        if (terminal_flag == 1 || is_child_infoset_ticked == 1){
            return 1;
        }
        else {
            return 0;
        }
    }
}


void exploit_wrapper(PolicyVec& policy_obj, PolicyVec& opponent_policy, History& current_history, char br_player, 
                     std::vector<int>& infoset_reach_count, std::vector<std::vector<std::vector<int>>>& empirical_action_reward, 
                     std::vector<std::vector<int>>& action_terminal_reach_count, std::vector<int>& I_tickmark, std::vector<std::vector<int>>& I_a_tickmark) {
    std::string board = "000000000";
    TicTacToeBoard true_board = TicTacToeBoard(board);
    std::string hash_1 = "";
    std::string hash_2 = "";
    InformationSet I_1 = InformationSet('x', true, hash_1);
    InformationSet I_2 = InformationSet('o', false, hash_2);
    if (br_player == 'x'){
        exploit(I_1, I_2, I_2, true_board, current_history, 'x', br_player, policy_obj, opponent_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, I_tickmark, I_a_tickmark);
    }
    else {
        exploit(I_1, I_2, I_1, true_board, current_history, 'x', br_player, policy_obj, opponent_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, I_tickmark, I_a_tickmark);
    }
}


double build_max_reward_policy(PolicyVec& policy_obj, InformationSet& I, std::vector<int>& infoset_reach_count, 
                               std::vector<std::vector<std::vector<int>>>& empirical_action_reward, 
                               std::vector<std::vector<int>>& action_terminal_reach_count, std::vector<double>& infoset_values) {
    std::vector<int> legal_actions;
    I.get_actions(legal_actions);
    std::vector<double> action_values(13, 0.0);
    double infoset_value = -1.0;

    for (int a : legal_actions){
        std::unordered_set<std::string> cohort;
        std::unordered_map<std::string, double> cohort_values;
        get_cohort(I, a, cohort);
        int norm = 0;
        int terminal_reach_count = action_terminal_reach_count[I.get_index()][a]; // number of times action led to terminal state

        for (std::string I_prime_hash : cohort){
            InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash);
            cohort_values[I_prime_hash] = build_max_reward_policy(policy_obj, I_prime, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_values);

            norm += infoset_reach_count[I_prime.get_index()];
            action_values[a] += cohort_values[I_prime_hash] * infoset_reach_count[I_prime.get_index()]; 
        }
        // if infoset reach count is zero then the action and pull count is zero then the action has not been taken
        // in that case ignore the action

        norm += terminal_reach_count;
        action_values[a] += empirical_action_reward[I.get_index()][a][0] - empirical_action_reward[I.get_index()][a][2];
        if (norm != 0){
            action_values[a] /= norm;
        }
        else {
            action_values[a] = -1.0;
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


double build_max_reward_policy_parallel(PolicyVec& policy_obj, InformationSet&I, std::vector<int>& infoset_reach_count, 
                                        std::vector<std::vector<std::vector<int>>>& empirical_action_reward, 
                                        std::vector<std::vector<int>>& action_terminal_reach_count, std::vector<double>& infoset_values) {
    std::vector<int> legal_actions;
    I.get_actions(legal_actions);
    std::vector<double> action_values(13, 0.0);
    double infoset_value = 0.0;
 
    #pragma omp parallel for num_threads(NUMBER_THREADS)
    for (int a : legal_actions){
        std::unordered_set<std::string> cohort;
        std::unordered_map<std::string, double> cohort_values;
        get_cohort(I, a, cohort);
        int norm = 0;
        int terminal_reach_count = action_terminal_reach_count[I.get_index()][a];

        for (std::string I_prime_hash : cohort){
            InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash);
            cohort_values[I_prime_hash] = build_max_reward_policy(policy_obj, I_prime, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_values);

            norm += infoset_reach_count[I_prime.get_index()];
            action_values[a] += cohort_values[I_prime_hash] * infoset_reach_count[I_prime.get_index()];
        }

        norm += terminal_reach_count;
        action_values[a] += empirical_action_reward[I.get_index()][a][0] - empirical_action_reward[I.get_index()][a][2];

        if (norm != 0){
            action_values[a] /= norm;
        }
        else {
            action_values[a] = -1.0;
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


double build_max_reward_policy_dirichlet(PolicyVec& policy_obj, InformationSet& I, std::vector<int>& infoset_reach_count, 
                                         std::vector<std::vector<std::vector<int>>>& empirical_action_reward, 
                                         std::vector<std::vector<int>>& action_terminal_reach_count, std::vector<double>& infoset_values) {
    std::vector<int> legal_actions;
    I.get_actions(legal_actions);
    std::vector<double> action_values(13, 0.0);
    double infoset_value = -1.0;

    for (int a : legal_actions){
        std::unordered_set<std::string> cohort;
        std::unordered_map<std::string, double> cohort_values;
        get_cohort(I, a, cohort);
        double norm = 0.0;
        std::vector<double> alpha(cohort.size()+1, 0.0);
        int terminal_reach_count = action_terminal_reach_count[I.get_index()][a];

        for (std::string I_prime_hash : cohort){
            InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash);
            cohort_values[I_prime_hash] = build_max_reward_policy_dirichlet(policy_obj, I_prime, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_values);

            norm += infoset_reach_count[I_prime.get_index()];
        }
        norm += terminal_reach_count;

        if (norm != 0){
            int index = 0;
            for (std::string I_prime_hash : cohort){
                InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash);
                alpha[index++] = infoset_reach_count[I_prime.get_index()] / norm;
            }
            alpha[index] = terminal_reach_count / norm;

            std::vector<double> samples = sample_dirichlet(alpha, generator);

            index = 0;
            for (std::string I_prime_hash : cohort){
                InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash);
                action_values[a] += cohort_values[I_prime_hash] * samples[index++];
            }
            
            double terminal_value = (empirical_action_reward[I.get_index()][a][0] - empirical_action_reward[I.get_index()][a][2]) * samples[index];
            if (terminal_reach_count != 0){
                action_values[a] += terminal_value / terminal_reach_count;
            }
        }
        else {
            action_values[a] = -1.0;
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



double build_max_reward_policy_dirichlet_parallel(PolicyVec& policy_obj, InformationSet& I, std::vector<int>& infoset_reach_count, 
                                                  std::vector<std::vector<std::vector<int>>>& empirical_action_reward, 
                                                  std::vector<std::vector<int>>& action_terminal_reach_count, std::vector<double>& infoset_values) {
    std::vector<int> legal_actions;
    I.get_actions(legal_actions);
    std::vector<double> action_values(13, 0.0);
    double infoset_value = -1.0;

    #pragma omp parallel for num_threads(NUMBER_THREADS)
    for (int a : legal_actions){
        std::unordered_set<std::string> cohort;
        std::unordered_map<std::string, double> cohort_values;
        get_cohort(I, a, cohort);
        double norm = 0.0;
        std::vector<double> alpha(cohort.size()+1, 0.0);
        int terminal_reach_count = action_terminal_reach_count[I.get_index()][a];

        for (std::string I_prime_hash : cohort){
            InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash);
            cohort_values[I_prime_hash] = build_max_reward_policy_dirichlet(policy_obj, I_prime, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_values);

            norm += infoset_reach_count[I_prime.get_index()];
        }
        norm += terminal_reach_count;

        if (norm != 0){
            int index = 0;
            for (std::string I_prime_hash : cohort){
                InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash);
                alpha[index++] = infoset_reach_count[I_prime.get_index()] / norm;
            }
            alpha[index] = terminal_reach_count / norm;

            std::vector<double> samples = sample_dirichlet(alpha, generator);

            index = 0;
            for (std::string I_prime_hash : cohort){
                InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash);
                action_values[a] += cohort_values[I_prime_hash] * samples[index++];
            }
            
            double terminal_value = (empirical_action_reward[I.get_index()][a][0] - empirical_action_reward[I.get_index()][a][2]) * samples[index];
            if (terminal_reach_count != 0){
                action_values[a] += terminal_value / terminal_reach_count;
            }
        }
        else {
            action_values[a] = -1.0;
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
             std::vector<std::pair<int, double>>& exploitability_log, std::vector<double>& infoset_values, std::vector<std::vector<std::vector<int>>>& empirical_action_reward,
             std::vector<std::vector<int>>& action_terminal_reach_count) {
    if (t % log_frequency == 0 && t != 0) { 
        std::string hash = "";
        InformationSet root = br_player == 'x' ? InformationSet('x', true, hash) : InformationSet('o', false, hash);
        double root_val = build_max_reward_policy_parallel(br, root, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_values);
    
        double expected_utility = 0.0;
        double exploitability = 0.0;
    
        if (br_player == 'x') {
            expected_utility = get_expected_utility_wrapper(br, opponent_policy);
            exploitability_log.push_back(std::make_pair(t, exact_br_value - expected_utility));
        } 
        else {
            expected_utility = get_expected_utility_wrapper(opponent_policy, br);
            exploitability_log.push_back(std::make_pair(t, exact_br_value - expected_utility));
        }
        std::cout << "Expected utility of best response policy: " << expected_utility << std::endl;

        int count = 0;
        for (int i = 0; i < infoset_reach_count.size(); i++) {
            if (infoset_reach_count[i] > 0) {
                count += 1;
            }
        }

        std::cout << "Number of information sets visited: " << count << std::endl;
        std::cout << "Number of games sampled so far: " << t << std::endl;
    }
}


void calc_br(PolicyVec& opponent_policy, char br_player, std::vector<std::string>& player_information_sets, int log_frequency, int m, PolicyVec& br, int experiment_number, int iterations, std::string& exp_name) {    
    std::vector<std::vector<int>> I_a_tickmark(player_information_sets.size(), std::vector<int>(13, 0));
    std::vector<int> I_tickmark(player_information_sets.size(), 0);

    std::vector<int> infoset_reach_count(player_information_sets.size(), 0);
    std::vector<double> infoset_values(player_information_sets.size(), 0.0);

    std::vector<std::vector<std::vector<int>>> empirical_action_reward(player_information_sets.size(), std::vector<std::vector<int>>(13, std::vector<int>(3, 0)));
    std::vector<std::vector<int>> action_terminal_reach_count(player_information_sets.size(), std::vector<int>(13, 0));

    PolicyVec candidate_br(br_player, player_information_sets);
    std::vector<std::pair<int, double>> exploitability_log; 

    int flag = 1;
    int t = 0;
    int k = 1;

    PolicyVec exact_br(br_player, player_information_sets);
    compute_best_response_wrapper(opponent_policy, exact_br, br_player);
    double exact_br_value = 0.0;
    if (br_player == 'x') {
        exact_br_value = get_expected_utility_wrapper(exact_br, opponent_policy);
    } 
    else {
        exact_br_value = get_expected_utility_wrapper(opponent_policy, exact_br);
    }
    std::cout << "Exact best response value: " << exact_br_value << std::endl;

    while (flag){ 
        std::string board = "000000000";
        TicTacToeBoard true_board = TicTacToeBoard(board);
        std::string hash_1 = "";
        std::string hash_2 = "";
        InformationSet I_1 = InformationSet('x', true, hash_1);
        InformationSet I_2 = InformationSet('o', false, hash_2);

        std::vector<int> h = {};
        TerminalHistory start_history = TerminalHistory(h);
        double reward = 0.0;
        explore_wrapper(I_a_tickmark, I_tickmark, reward, opponent_policy, start_history, br_player, k, infoset_reach_count, empirical_action_reward, action_terminal_reach_count);

        t += 1;

        InformationSet I = br_player == 'x' ? InformationSet('x', true, hash_1) : InformationSet('o', false, hash_2);
        if (I_tickmark[I.get_index()] == k){
            k += 1;
            if (k > m){
                flag = 0;
            }
        }

        if (t % log_frequency == 0){
            std:: cout << "Number of games sampled so far: " << t << std::endl;
        }
    }

    std::cout << "Total number of games sampled for pulling each policy " << m << " times: " << t << std::endl;
    
    while (t <= iterations) {
        // logging
        logging(t, log_frequency, br, opponent_policy, br_player, exact_br_value, infoset_reach_count, exploitability_log, infoset_values, empirical_action_reward, action_terminal_reach_count);

        std::string hash = "";
        InformationSet root = br_player == 'x' ? InformationSet('x', true, hash) : InformationSet('o', false, hash);
        double root_val = build_max_reward_policy_parallel(br, root, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_values);    
        
        while (areEqual(br, candidate_br)){
            std::string hash = "";
            InformationSet root = br_player == 'x' ? InformationSet('x', true, hash) : InformationSet('o', false, hash);
            double root_val = build_max_reward_policy_parallel(candidate_br, root, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_values);        
        }

        // toss a coin
        std::vector<double> dist = {0.5, 0.5};
        int head = sampleIndex(dist);

        PolicyVec& policy_to_sample = head == 0 ? br : candidate_br;

        std::string board = "000000000";
        TicTacToeBoard true_board = TicTacToeBoard(board);
        std::string hash_1 = "";
        std::string hash_2 = "";
        InformationSet I_1 = InformationSet('x', true, hash_1);
        InformationSet I_2 = InformationSet('o', false, hash_2);

        std::vector<int> h = {};
        TerminalHistory start_history = TerminalHistory(h);
        double reward = 0.0;

        exploit_wrapper(policy_to_sample, opponent_policy, start_history, br_player, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, I_tickmark, I_a_tickmark);
        t += 1;
    }

    std::cout << "Saving exploitability log" << std::endl;
    std::string file_name = "data/bandit/" + exp_name + "_" + "_RBT_" + std::string(1, br_player) +  "_top_two_thompson_exploitability_log_" + std::to_string(experiment_number) + ".txt";

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
    char player = argv[3][0];
    int iterations = std::stoi(argv[4]);
    int log_frequency = std::stoi(argv[5]);
    int experiments = std::stoi(argv[6]);
    std::string exp_name = argv[7];
    int start_index = std::stoi(argv[8]);
    
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
    std::cout << "Loading policies" << std::endl;
    PolicyVec policy_obj_x('x', file_path_1, true);
    PolicyVec policy_obj_o('o', file_path_2, true);
    double expected_utility = get_expected_utility_wrapper(policy_obj_x, policy_obj_o);
    std::cout << "Expected utility of initial policies: " << expected_utility << std::endl;

    // compute epsilon best response
    int experiment_num = start_index;

    while (experiment_num < experiments + start_index) {
        if (player == 'x') {
            PolicyVec uniform_x('x', P1_information_sets);
            calc_br(policy_obj_o, 'x', P1_information_sets, log_frequency, 1, uniform_x, experiment_num, iterations, exp_name);
        }
        else {
            PolicyVec uniform_o('o', P2_information_sets);
            calc_br(policy_obj_x, 'o', P2_information_sets, log_frequency, 1, uniform_o, experiment_num, iterations, exp_name);
        }

        std::cout << "(" << experiment_num << " experiments done)" << std::endl;
        experiment_num += 1;
    }
}