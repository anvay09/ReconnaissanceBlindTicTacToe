#include "cpp_headers/rbt_classes.hpp"
#include "cpp_headers/rbt_utilities.hpp"
#include <random>
#include <cmath>
int NUMBER_THREADS = 96;


void pretty_print(std::chrono::time_point<std::chrono::system_clock> start, std::chrono::time_point<std::chrono::system_clock> end, std::string msg, int flag) {
    if (flag) {
    std::chrono::duration<double> elapsed_seconds = end-start;
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);
    std::cout << "finished " << msg << " in " << elapsed_seconds.count() << "s" << std::endl;
    }
}


void print_histogram(std::vector<int>& visited_infosets) {
    std::vector<int> buckets(10, 0);

    for (int i = 0; i < visited_infosets.size(); i++) {
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
        std::cout << buckets[i] << "\t";
    }
    std::cout << std::endl;

    std::cout << "0\t1\t2\t3-10\t11-100\t101-1k\t1k-10k\t10k-100k\t100k-1M\t1M+" << std::endl;
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


int sampleIndex(const std::vector<double>& probabilities) {
    std::random_device rd;
    std::mt19937 generator(rd());
    std::discrete_distribution<int> distribution(probabilities.begin(), probabilities.end());
    return distribution(generator);
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
                if ((r == 1 && br_player == 'x') || (r == -1 && br_player == 'o')){
                    empirical_action_reward[I.get_index()][action][0] += 1;
                }
                else if (r == 0){
                    empirical_action_reward[I.get_index()][action][1] += 1;
                }
                else if ((r == -1 && br_player == 'x') || (r == 1 && br_player == 'o')){
                    empirical_action_reward[I.get_index()][action][2] += 1;
                }
            }
            else {
                // find third last action in history
                int third_last_action = current_history.history[current_history.history.size() - 3];
                action_terminal_reach_count[previous_opponent_I.get_index()][third_last_action] += 1;

                if ((r == 1 && br_player == 'x') || (r == -1 && br_player == 'o')){
                    empirical_action_reward[previous_opponent_I.get_index()][third_last_action][0] += 1;
                }
                else if (r == 0){
                    empirical_action_reward[previous_opponent_I.get_index()][third_last_action][1] += 1;
                }
                else if ((r == -1 && br_player == 'x') || (r == 1 && br_player == 'o')){
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
            std::vector<std::vector<int>>& action_terminal_reach_count, std::vector<int>& I_tickmark, std::vector<std::vector<int>>& I_a_tickmark, std::vector<std::vector<int>>& action_explore_count) {
    InformationSet I = curr_player == 'x' ? I_1 : I_2;
    int action = 0;
    int terminal_flag = 0;
    int is_child_infoset_ticked = 0;
    
    if (br_player == curr_player){
        infoset_reach_count[I.get_index()] += 1;
        std::vector<double>& prob_dist = policy_obj.policy_dict[I.get_index()];
        action = sampleIndex(prob_dist);
        action_explore_count[I.get_index()][action] += 1;
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
                is_child_infoset_ticked = exploit(new_I, I_2, I, new_board, current_history, 'o', br_player, policy_obj, opponent_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, I_tickmark, I_a_tickmark, action_explore_count);
            } else {
                is_child_infoset_ticked = exploit(I_1, new_I, I, new_board, current_history, 'x', br_player, policy_obj, opponent_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, I_tickmark, I_a_tickmark, action_explore_count);
            }
        } else {
            TerminalHistory H_T = TerminalHistory(current_history.history);
            H_T.set_reward();
            int r = br_player == 'x' ? H_T.reward[0] : H_T.reward[1];
            terminal_flag = 1;

            // update action pull count and empirical action reward only when action leads to terminal state
            if (curr_player == br_player){
                action_terminal_reach_count[I.get_index()][action] += 1;        
                if ((r == 1 && br_player == 'x') || (r == -1 && br_player == 'o')){
                    empirical_action_reward[I.get_index()][action][0] += 1;
                }
                else if (r == 0){
                    empirical_action_reward[I.get_index()][action][1] += 1;
                }
                else if ((r == -1 && br_player == 'x') || (r == 1 && br_player == 'o')){
                    empirical_action_reward[I.get_index()][action][2] += 1;
                }
            }
            else {
                // find third last action in history
                int third_last_action = current_history.history[current_history.history.size() - 3];
                action_terminal_reach_count[previous_opponent_I.get_index()][third_last_action] += 1;
                
                if ((r == 1 && br_player == 'x') || (r == -1 && br_player == 'o')){
                    empirical_action_reward[previous_opponent_I.get_index()][third_last_action][0] += 1;
                }
                else if (r == 0){
                    empirical_action_reward[previous_opponent_I.get_index()][third_last_action][1] += 1;
                }
                else if ((r == -1 && br_player == 'x') || (r == 1 && br_player == 'o')){
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
            is_child_infoset_ticked = exploit(new_I, I_2, previous_opponent_I, new_board, current_history, 'x', br_player, policy_obj, opponent_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, I_tickmark, I_a_tickmark, action_explore_count);
        } else {
            is_child_infoset_ticked = exploit(I_1, new_I, previous_opponent_I, new_board, current_history, 'o', br_player, policy_obj, opponent_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, I_tickmark, I_a_tickmark, action_explore_count);
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
                     std::vector<std::vector<int>>& action_terminal_reach_count, std::vector<int>& I_tickmark, std::vector<std::vector<int>>& I_a_tickmark, std::vector<std::vector<int>>& action_explore_count) {
    std::string board = "000000000";
    TicTacToeBoard true_board = TicTacToeBoard(board);
    std::string hash_1 = "";
    std::string hash_2 = "";
    InformationSet I_1 = InformationSet('x', true, hash_1);
    InformationSet I_2 = InformationSet('o', false, hash_2);
    exploit(I_1, I_2, I_2, true_board, current_history, 'x', br_player, policy_obj, opponent_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, I_tickmark, I_a_tickmark, action_explore_count);
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


void update_max_reward_policy_given_history(InformationSet& I, TicTacToeBoard& true_board, InformationSet& opponent_I, History& game, PolicyVec& max_reward_policy, 
                                            std::vector<int>& infoset_reach_count, std::vector<std::vector<std::vector<int>>>& empirical_action_reward, 
                                            std::vector<std::vector<int>>& action_terminal_reach_count, std::vector<double>& infoset_values, char curr_player, char br_player, int traversal_index) {
    if (traversal_index == game.history.size()){
        return;
    }
    
    int action = game.history[traversal_index];

    if (curr_player == br_player){
        if (I.move_flag) {
            InformationSet new_I = I;
            TicTacToeBoard new_board = true_board;
            new_I.update_move(action, I.player);
            new_I.reset_zeros();
            new_board.update_move(action, I.player);
            update_max_reward_policy_given_history(new_I, new_board, opponent_I, game, max_reward_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_values, toggle_player(curr_player), br_player, traversal_index + 1);
        }
        else {
            InformationSet new_I = I;
            new_I.simulate_sense(action, true_board);
            I.reset_zeros();
            update_max_reward_policy_given_history(new_I, true_board, opponent_I, game, max_reward_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_values, curr_player, br_player, traversal_index + 1);
        }

        std::vector<int> legal_actions;
        I.get_actions(legal_actions);
        std::vector<double> action_values(13, 0.0);
        double infoset_value = -1.0;

        for (int a : legal_actions){
            std::unordered_set<std::string> cohort;
            std::unordered_map<std::string, double> cohort_values;
            get_cohort(I, a, cohort);
            int norm = 0;
            int terminal_reach_count = action_terminal_reach_count[I.get_index()][a];

            for (std::string I_prime_hash : cohort){
                InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash);
                cohort_values[I_prime_hash] = infoset_values[I_prime.get_index()];

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
            std::vector<double>& prob_dist = max_reward_policy.policy_dict[I.get_index()];

            for (int a : legal_actions){
                if (a == action){
                    prob_dist[a] = 1.0;
                }
                else{
                    prob_dist[a] = 0.0;
                }
            }

            infoset_values[I.get_index()] = 0.0;
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
            std::vector<double>& prob_dist = max_reward_policy.policy_dict[I.get_index()];

            for (int a : legal_actions){
                if (a == action){
                    prob_dist[a] = 1.0;
                }
                else{
                    prob_dist[a] = 0.0;
                }
            }

            infoset_values[I.get_index()] = infoset_value;
        }
    }
    else {
        if (opponent_I.move_flag){
            opponent_I.update_move(action, opponent_I.player);
            opponent_I.reset_zeros();
            true_board.update_move(action, opponent_I.player);
            update_max_reward_policy_given_history(I, true_board, opponent_I, game, max_reward_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_values, toggle_player(curr_player), br_player, traversal_index + 1);
        }
        else {
            opponent_I.simulate_sense(action, true_board);
            update_max_reward_policy_given_history(I, true_board, opponent_I, game, max_reward_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_values, curr_player, br_player, traversal_index + 1);
        }
    }
}


double build_max_UCB_policy(PolicyVec& policy_obj, InformationSet& I, std::vector<int>& infoset_reach_count, std::vector<int>& I_tickmark, 
                            std::vector<std::vector<std::vector<int>>>& empirical_action_reward, std::vector<std::vector<int>>& action_terminal_reach_count, 
                            std::vector<int>& infoset_time_step, int C, std::vector<int>& success_metrics, std::vector<double>& infoset_ucb_values, std::vector<std::vector<int>>& success_metrics_pi_hat, std::vector<std::vector<int>>& action_explore_count){
    std::vector<int> legal_actions;
    I.get_actions(legal_actions);
    std::vector<double> action_ucb_values(13, 0.0);
    double max_ucb = -1.0;

    for (int a : legal_actions){
        std::unordered_set<std::string> cohort;
        std::unordered_map<std::string, double> cohort_ucb_values;
        get_cohort(I, a, cohort);
        int u = 0;
        int norm = 0;
        int terminal_reach_count = action_terminal_reach_count[I.get_index()][a]; // number of times action led to terminal state

        for (std::string I_prime_hash : cohort){
            InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash);
            std::vector<int> success_metrics_prime{0, 0, 0}; // wins, draws, losses

            cohort_ucb_values[I_prime_hash] = build_max_UCB_policy(policy_obj, I_prime, infoset_reach_count, I_tickmark, empirical_action_reward, action_terminal_reach_count, infoset_time_step, C, success_metrics_prime, infoset_ucb_values, success_metrics_pi_hat, action_explore_count);


            if (I_tickmark[I_prime.get_index()] == 0){
                u += infoset_reach_count[I_prime.get_index()];
                action_ucb_values[a] += infoset_reach_count[I_prime.get_index()];
            }
            else {
                int denom = success_metrics_prime[0] + success_metrics_prime[1] + success_metrics_prime[2];
                if (denom != 0){
                    u += denom;
                    action_ucb_values[a] += infoset_reach_count[I_prime.get_index()] * (success_metrics_prime[0] - success_metrics_prime[2]) / denom;
                    norm += infoset_reach_count[I_prime.get_index()];

                    success_metrics[0] += success_metrics_prime[0];
                    success_metrics[1] += success_metrics_prime[1];
                    success_metrics[2] += success_metrics_prime[2];
                }
            }
        }
        // if infoset reach count is zero then the action and pull count is zero then the action has not been taken
        // in that case ignore the action

        if (terminal_reach_count != 0){
            u += terminal_reach_count;
            action_ucb_values[a] += (empirical_action_reward[I.get_index()][a][0] - empirical_action_reward[I.get_index()][a][2]) / terminal_reach_count;

            success_metrics[0] += empirical_action_reward[I.get_index()][a][0];
            success_metrics[1] += empirical_action_reward[I.get_index()][a][1];
            success_metrics[2] += empirical_action_reward[I.get_index()][a][2];
        }

        if (u != 0){
            action_ucb_values[a] /= norm;
            // action_ucb_values[a] += sqrt(C * log(infoset_time_step[I.get_index()])/u);
            action_ucb_values[a] += sqrt(C * log(infoset_time_step[I.get_index()])/action_explore_count[I.get_index()][a]);
            infoset_time_step[I.get_index()] += 1;
        }
        else {
            action_ucb_values[a] = 1.0;
        }
    }

    for (int a : legal_actions){
        if (action_ucb_values[a] > max_ucb){
            max_ucb = action_ucb_values[a];
        }
    }

    if (I_tickmark[I.get_index()] == 0){
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

        infoset_ucb_values[I.get_index()] = 0.0;
        success_metrics_pi_hat[I.get_index()] = success_metrics;
        return 0.0;
    }
    else {
        std::vector<int> candidate_actions;
        for (int a : legal_actions){
            if (fabs(action_ucb_values[a] - max_ucb) < 1e-6){
                candidate_actions.push_back(a);
            }
        }

        // sample from candidate actions
        if (candidate_actions.size() != 0){
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
        }
        else {
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
        }

        infoset_ucb_values[I.get_index()] = max_ucb;
        success_metrics_pi_hat[I.get_index()] = success_metrics;
        return max_ucb;
    }
}


double build_max_UCB_policy_parallel(PolicyVec& policy_obj, InformationSet& I, std::vector<int>& infoset_reach_count, std::vector<int>& I_tickmark, 
                                     std::vector<std::vector<std::vector<int>>>& empirical_action_reward, std::vector<std::vector<int>>& action_terminal_reach_count, 
                                     std::vector<int>& infoset_time_step, int C, std::vector<int>& success_metrics, std::vector<double>& infoset_ucb_values, std::vector<std::vector<int>>& success_metrics_pi_hat, std::vector<std::vector<int>>& action_explore_count){
    std::vector<int> legal_actions;
    I.get_actions(legal_actions);
    std::vector<double> action_ucb_values(13, 0.0);
    double max_ucb = -1.0;

    #pragma omp parallel for num_threads(NUMBER_THREADS)
    for (int a : legal_actions){
        std::unordered_set<std::string> cohort;
        std::unordered_map<std::string, double> cohort_ucb_values;
        get_cohort(I, a, cohort);
        int u = 0;
        int norm = 0;
        int terminal_reach_count = action_terminal_reach_count[I.get_index()][a]; // number of times action led to terminal state

        for (std::string I_prime_hash : cohort){
            InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash);
            std::vector<int> success_metrics_prime{0, 0, 0}; // wins, draws, losses

            cohort_ucb_values[I_prime_hash] = build_max_UCB_policy(policy_obj, I_prime, infoset_reach_count, I_tickmark, empirical_action_reward, action_terminal_reach_count, infoset_time_step, C, success_metrics_prime, infoset_ucb_values, success_metrics_pi_hat, action_explore_count);

            if (I_tickmark[I_prime.get_index()] == 0){
                u += infoset_reach_count[I_prime.get_index()];
                action_ucb_values[a] += infoset_reach_count[I_prime.get_index()];
            }
            else {
                int denom = success_metrics_prime[0] + success_metrics_prime[1] + success_metrics_prime[2];
                if (denom != 0){
                    u += denom;
                    action_ucb_values[a] += infoset_reach_count[I_prime.get_index()] * (success_metrics_prime[0] - success_metrics_prime[2]) / denom;
                    norm += infoset_reach_count[I_prime.get_index()];

                    success_metrics[0] += success_metrics_prime[0];
                    success_metrics[1] += success_metrics_prime[1];
                    success_metrics[2] += success_metrics_prime[2];
                }
            }
        }
        // if infoset reach count is zero then the action and pull count is zero then the action has not been taken
        // in that case ignore the action

        if (terminal_reach_count != 0){
            u += terminal_reach_count;
            action_ucb_values[a] += (empirical_action_reward[I.get_index()][a][0] - empirical_action_reward[I.get_index()][a][2]) / terminal_reach_count;

            success_metrics[0] += empirical_action_reward[I.get_index()][a][0];
            success_metrics[1] += empirical_action_reward[I.get_index()][a][1];
            success_metrics[2] += empirical_action_reward[I.get_index()][a][2];
        }

        if (u != 0){
            action_ucb_values[a] /= norm;
            // action_ucb_values[a] += sqrt(C * log(infoset_time_step[I.get_index()])/u);
            action_ucb_values[a] += sqrt(C * log(infoset_time_step[I.get_index()])/action_explore_count[I.get_index()][a]);
            infoset_time_step[I.get_index()] += 1;
        }
        else {
            action_ucb_values[a] = 1.0;
        }
    }

    // std::cout << "Action UCB values: ";
    for (int a : legal_actions){
        // std::cout << action_ucb_values[a] << " ";
        if (action_ucb_values[a] > max_ucb){
            max_ucb = action_ucb_values[a];
        }
    }
    // std::cout << std::endl;

    if (I_tickmark[I.get_index()] == 0){
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

        infoset_ucb_values[I.get_index()] = 0.0;
        success_metrics_pi_hat[I.get_index()] = success_metrics;
        return 0.0;
    }
    else {
        std::vector<int> candidate_actions;
        for (int a : legal_actions){
            if (fabs(action_ucb_values[a] - max_ucb) < 1e-6){
                candidate_actions.push_back(a);
            }
        }

        // sample from candidate actions
                // sample from candidate actions
        if (candidate_actions.size() != 0){
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
        }
        else {
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
        }

        infoset_ucb_values[I.get_index()] = max_ucb;
        success_metrics_pi_hat[I.get_index()] = success_metrics;
        return max_ucb;
    }
}


void update_max_UCB_policy_given_history(InformationSet& I, TicTacToeBoard& true_board, InformationSet& opponent_I, History& game, PolicyVec& max_UCB_policy, std::vector<int>& infoset_reach_count, 
                                         std::vector<std::vector<std::vector<int>>>& empirical_action_reward, std::vector<std::vector<int>>& action_terminal_reach_count, 
                                         std::vector<double>& infoset_ucb_values, char curr_player, char br_player, int traversal_index, std::vector<int>& infoset_time_step, 
                                         int C, std::vector<int>& I_tickmark, std::vector<std::vector<int>>& success_metrics_pi_hat, std::vector<std::vector<int>>& action_explore_count) {
    if (traversal_index == game.history.size()){
        return;
    }
    
    int action = game.history[traversal_index];

    if (curr_player == br_player){
        if (I.move_flag) {
            InformationSet new_I = I;
            TicTacToeBoard new_board = true_board;
            new_I.update_move(action, I.player);
            new_I.reset_zeros();
            new_board.update_move(action, I.player);
            update_max_UCB_policy_given_history(new_I, new_board, opponent_I, game, max_UCB_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_ucb_values, toggle_player(curr_player), br_player, traversal_index + 1, infoset_time_step, C, I_tickmark, success_metrics_pi_hat, action_explore_count);
        }
        else {
            InformationSet new_I = I;
            new_I.simulate_sense(action, true_board);
            I.reset_zeros();
            update_max_UCB_policy_given_history(new_I, true_board, opponent_I, game, max_UCB_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_ucb_values, curr_player, br_player, traversal_index + 1, infoset_time_step, C, I_tickmark, success_metrics_pi_hat, action_explore_count);
        }   

        std::vector<int> legal_actions;
        I.get_actions(legal_actions);
        std::vector<double> action_ucb_values(13, 0.0);
        double max_ucb = -1.0;
        std::vector<int> success_metrics{0, 0, 0};

        for (int a : legal_actions){
            std::unordered_set<std::string> cohort;
            std::unordered_map<std::string, double> cohort_ucb_values;
            get_cohort(I, a, cohort);
            int u = 0;
            int norm = 0;
            int terminal_reach_count = action_terminal_reach_count[I.get_index()][a]; // number of times action led to terminal state

            for (std::string I_prime_hash : cohort){
                InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash);

                std::vector<int> success_metrics_prime = success_metrics_pi_hat[I_prime.get_index()];
                cohort_ucb_values[I_prime_hash] = infoset_ucb_values[I_prime.get_index()];

                if (I_tickmark[I_prime.get_index()] == 0){
                    u += infoset_reach_count[I_prime.get_index()];
                    action_ucb_values[a] += infoset_reach_count[I_prime.get_index()];
                }
                else {
                    int denom = success_metrics_prime[0] + success_metrics_prime[1] + success_metrics_prime[2];
                    if (denom != 0){
                        u += denom;
                        action_ucb_values[a] += infoset_reach_count[I_prime.get_index()] * (success_metrics_prime[0] - success_metrics_prime[2]) / denom;
                        norm += infoset_reach_count[I_prime.get_index()];

                        success_metrics[0] += success_metrics_prime[0];
                        success_metrics[1] += success_metrics_prime[1];
                        success_metrics[2] += success_metrics_prime[2];
                    }
                }
            }

            if (terminal_reach_count != 0){
                u += terminal_reach_count;
                action_ucb_values[a] += (empirical_action_reward[I.get_index()][a][0] - empirical_action_reward[I.get_index()][a][2]) / terminal_reach_count;

                success_metrics[0] += empirical_action_reward[I.get_index()][a][0];
                success_metrics[1] += empirical_action_reward[I.get_index()][a][1];
                success_metrics[2] += empirical_action_reward[I.get_index()][a][2];
            }

            if (u != 0){
                action_ucb_values[a] /= norm;
                // action_ucb_values[a] += sqrt(C * log(infoset_time_step[I.get_index()])/u);
                action_ucb_values[a] += sqrt(C * log(infoset_time_step[I.get_index()])/action_explore_count[I.get_index()][a]);
                infoset_time_step[I.get_index()] += 1;
            }
            else {
                action_ucb_values[a] = 1.0;
            }
        }

        // if (I.get_hash() == "" && infoset_time_step[I.get_index()]%10000 == 0){
        //     std::cout << "Action UCB values: ";
        // }
        for (int a : legal_actions){
            // if (I.get_hash() == "" && infoset_time_step[I.get_index()]%10000 == 0){
            //     std::cout << action_ucb_values[a] << " ";
            // }
            if (action_ucb_values[a] > max_ucb){
                max_ucb = action_ucb_values[a];
            }
        }
        // if (I.get_hash() == "" && infoset_time_step[I.get_index()]%10000 == 0){
        //     std::cout << std::endl;
        // }

        if (I_tickmark[I.get_index()] == 0){
            // sample from legal actions
            int action = legal_actions[std::rand() % legal_actions.size()];
            std::vector<double>& prob_dist = max_UCB_policy.policy_dict[I.get_index()];

            for (int a : legal_actions){
                if (a == action){
                    prob_dist[a] = 1.0;
                }
                else{
                    prob_dist[a] = 0.0;
                }
            }

            success_metrics_pi_hat[I.get_index()] = success_metrics;
            infoset_ucb_values[I.get_index()] = 0.0;
        }
        else {
            std::vector<int> candidate_actions;
            for (int a : legal_actions){
                if (fabs(action_ucb_values[a] - max_ucb) < 1e-6){
                    candidate_actions.push_back(a);
                }
            }

            // sample from candidate actions
            if (candidate_actions.size() != 0){
                int action = candidate_actions[std::rand() % candidate_actions.size()];
                std::vector<double>& prob_dist = max_UCB_policy.policy_dict[I.get_index()];

                for (int a : legal_actions){
                    if (a == action){
                        prob_dist[a] = 1.0;
                    }
                    else{
                        prob_dist[a] = 0.0;
                    }
                }
            }
            else {
                int action = legal_actions[std::rand() % legal_actions.size()];
                std::vector<double>& prob_dist = max_UCB_policy.policy_dict[I.get_index()];

                for (int a : legal_actions){
                    if (a == action){
                        prob_dist[a] = 1.0;
                    }
                    else{
                        prob_dist[a] = 0.0;
                    }
                }
            }

            success_metrics_pi_hat[I.get_index()] = success_metrics;
            infoset_ucb_values[I.get_index()] = max_ucb;
        }
    }
    else {
        if (opponent_I.move_flag){
            opponent_I.update_move(action, opponent_I.player);
            opponent_I.reset_zeros();
            true_board.update_move(action, opponent_I.player);
            update_max_UCB_policy_given_history(I, true_board, opponent_I, game, max_UCB_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_ucb_values, toggle_player(curr_player), br_player, traversal_index + 1, infoset_time_step, C, I_tickmark, success_metrics_pi_hat, action_explore_count);
        }
        else {
            opponent_I.simulate_sense(action, true_board);
            update_max_UCB_policy_given_history(I, true_board, opponent_I, game, max_UCB_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_ucb_values, curr_player, br_player, traversal_index + 1, infoset_time_step, C, I_tickmark, success_metrics_pi_hat, action_explore_count);
        }
    }
}


void calc_br(PolicyVec& opponent_policy, char br_player, std::vector<std::string>& player_information_sets, int log_frequency, int m, PolicyVec& player_br, int experiment_number) {
    std::vector<std::vector<int>> I_a_tickmark(player_information_sets.size(), std::vector<int>(13, 0));
    std::vector<int> I_tickmark(player_information_sets.size(), 0);
    std::vector<int> infoset_reach_count(player_information_sets.size(), 0);
    std::vector<int> infoset_time_step(player_information_sets.size(), 1);
    std::vector<double> infoset_values(player_information_sets.size(), 0.0);
    std::vector<double> infoset_ucb_values(player_information_sets.size(), 0.0);
    std::vector<std::vector<int>> success_metrics_pi_hat(player_information_sets.size(), std::vector<int>(3, 0));
    std::vector<std::vector<std::vector<int>>> empirical_action_reward(player_information_sets.size(), std::vector<std::vector<int>>(13, std::vector<int>(3, 0)));
    std::vector<std::vector<int>> action_terminal_reach_count(player_information_sets.size(), std::vector<int>(13, 0));
    std::vector<std::vector<int>> action_explore_count(player_information_sets.size(), std::vector<int>(13, 0));
    PolicyVec player_max_ucb_policy(br_player, player_information_sets);
    std::vector<std::pair<int, double>> exploitability_log; 

    int flag = 1;
    int t = 0;
    int k = 1;

    PolicyVec exact_br(br_player, player_information_sets);
    double exact_br_value = compute_best_response_wrapper(opponent_policy, exact_br, br_player);

    while (flag){ 
        std::vector<int> h = {};
        TerminalHistory start_history = TerminalHistory(h);
        double reward = 0.0;
        explore_wrapper(I_a_tickmark, I_tickmark, reward, opponent_policy, start_history, br_player, k, infoset_reach_count, empirical_action_reward, action_terminal_reach_count);

        t += 1;

        std::string hash = "";
        InformationSet I = br_player == 'x' ? InformationSet('x', true, hash) : InformationSet('o', false, hash);
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
    int iterations = 200000;
    int samples = 1;
    int C = 16;

    // std::cout << "Enter number of iterations: ";
    // std::cin >> iterations;
    // std::cout << "Enter number of samples per iteration: ";
    // std::cin >> samples;
    // std::cout << "Enter value of C: ";
    // std::cin >> C;

    std::string hash = "";
    InformationSet root = br_player == 'x' ? InformationSet('x', true, hash) : InformationSet('o', false, hash);
    double root_val = build_max_reward_policy_parallel(player_br, root, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_values);
    std::cout << "Best response policy computed" << std::endl;

    root = br_player == 'x' ? InformationSet('x', true, hash) : InformationSet('o', false, hash);
    std::vector<int> success_metrics{0, 0, 0};
    double max_UCB = build_max_UCB_policy_parallel(player_max_ucb_policy, root, infoset_reach_count, I_tickmark, empirical_action_reward, action_terminal_reach_count, infoset_time_step, C, success_metrics, infoset_ucb_values, success_metrics_pi_hat, action_explore_count);
    std::cout << "Max UCB policy computed" << std::endl;

    for (int j = 0; j <= iterations; j++) {
        if (j % 1000 == 0 && j != 0) { 
            // root = br_player == 'x' ? InformationSet('x', true, hash) : InformationSet('o', false, hash);
            // std::vector<int> success_metrics{0, 0, 0};
            // double max_UCB = build_max_UCB_policy_parallel(player_max_ucb_policy, root, infoset_reach_count, I_tickmark, empirical_action_reward, action_terminal_reach_count, infoset_time_step, C, success_metrics, infoset_ucb_values, success_metrics_pi_hat, action_explore_count);

            double expected_utility = 0.0;
            double exploitability = 0.0;
            int sample_count = (j + 1) * samples * 2 + t;

            if (br_player == 'x') {
                expected_utility = get_expected_utility_wrapper(player_br, opponent_policy);
                exploitability_log.push_back(std::make_pair(sample_count, exact_br_value - expected_utility));
            } 
            else {
                expected_utility = get_expected_utility_wrapper(opponent_policy, player_br);
                exploitability_log.push_back(std::make_pair(sample_count, exact_br_value - expected_utility));
            }
            std::cout << "Expected utility of best response policy: " << expected_utility << std::endl;
            
            // number of information sets visited
            int count = 0;
            for (int i = 0; i < infoset_reach_count.size(); i++) {
                if (infoset_reach_count[i] > 0) {
                    count += 1;
                }
            }
            std::cout << "Iteration: " << j << std::endl;
            std::cout << "Number of information sets visited: " << count << std::endl;
            std::cout << "Number of games sampled so far: " << sample_count << std::endl;
        }

        for (int i = 0; i < samples; i++){
            std::vector<int> h = {};
            TerminalHistory start_history = TerminalHistory(h);
            exploit_wrapper(player_br, opponent_policy, start_history, br_player, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, I_tickmark, I_a_tickmark, action_explore_count);

            std::string board = "000000000";
            TicTacToeBoard true_board = TicTacToeBoard(board);
            std::string hash_1 = "";
            std::string hash_2 = "";
            InformationSet I_1 = InformationSet('x', true, hash_1);
            InformationSet I_2 = InformationSet('o', false, hash_2);
            update_max_reward_policy_given_history(I_1, true_board, I_2, start_history, player_br, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_values, 'x', br_player, 0);

            board = "000000000";
            true_board = TicTacToeBoard(board);
            hash_1 = "";
            hash_2 = "";
            I_1 = InformationSet('x', true, hash_1);
            I_2 = InformationSet('o', false, hash_2);
            update_max_UCB_policy_given_history(I_1, true_board, I_2, start_history, player_max_ucb_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_ucb_values, 'x', br_player, 0, infoset_time_step, C, I_tickmark, success_metrics_pi_hat, action_explore_count);

            h = {};
            start_history = TerminalHistory(h);
            exploit_wrapper(player_max_ucb_policy, opponent_policy, start_history, br_player, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, I_tickmark, I_a_tickmark, action_explore_count);

            board = "000000000";
            true_board = TicTacToeBoard(board);
            hash_1 = "";
            hash_2 = "";
            I_1 = InformationSet('x', true, hash_1);
            I_2 = InformationSet('o', false, hash_2);
            update_max_reward_policy_given_history(I_1, true_board, I_2, start_history, player_max_ucb_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_values, 'x', br_player, 0);

            board = "000000000";
            true_board = TicTacToeBoard(board);
            hash_1 = "";
            hash_2 = "";
            I_1 = InformationSet('x', true, hash_1);
            I_2 = InformationSet('o', false, hash_2);
            update_max_UCB_policy_given_history(I_1, true_board, I_2, start_history, player_max_ucb_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_ucb_values, 'x', br_player, 0, infoset_time_step, C, I_tickmark, success_metrics_pi_hat, action_explore_count);
        } 
    }

    std::cout << "Saving exploitability log" << std::endl;
    std::string file_name = "data/exploitability_log_" + std::to_string(experiment_number) + ".txt";

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

    // compute epsilon best response
    char continue_exp = 'y';
    int experiment_num = 1;

    // while (continue_exp == 'y') {
    while (experiment_num <= 20) {
        int log_frequency = 10000;
        char player = 'x';
        int m = 1;
        // std::cout << "Enter log frequency: ";
        // std::cin >> log_frequency;
        // std::cout << "Enter player for pull arms: ";
        // std::cin >> player;
        // std::cout << "Enter value of m: ";
        // std::cin >> m;

        if (player == 'x') {
            PolicyVec uniform_x('x', P1_information_sets);
            calc_br(policy_obj_o, 'x', P1_information_sets, log_frequency, m, uniform_x, experiment_num);
        }
        else {
            PolicyVec uniform_o('o', P2_information_sets);
            calc_br(policy_obj_x, 'o', P2_information_sets, log_frequency, m, uniform_o, experiment_num);
        }

        std::cout << "Continue experiments? (" << experiment_num << " experiments done) (y/n): ";
        // std::cin >> continue_exp;
        experiment_num += 1;
    }
    
}
