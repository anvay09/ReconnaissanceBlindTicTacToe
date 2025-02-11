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


int explore(InformationSet& I_1, InformationSet& I_2, InformationSet previous_opponent_I, int prev_opponent_action, PokerTable& true_cards, History& current_history, char br_player, 
            PolicyVec& opponent_policy, std::vector<std::vector<int>>& I_a_tickmark, std::vector<int>& I_tickmark, double& reward, int m, 
            std::vector<int>& infoset_reach_count, std::vector<std::vector<std::vector<int>>>& empirical_action_reward, std::vector<std::vector<int>>& action_terminal_reach_count) {
    InformationSet I = true_cards.player_to_move == 'x' ? I_1 : I_2;
    int action = 0;
    int terminal_flag = 0;
    int is_child_infoset_ticked = 0;
    
    if (br_player == I.player){
        infoset_reach_count[I.get_index()] += 1;

        std::vector<int> legal_actions;
        I.get_actions(legal_actions);
        std::vector<int> A;
        std::vector<double> prob_dist(6, 0.0); 

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
        PokerTable new_cards = true_cards;
        bool success = new_cards.update_move(action);
        current_history.history.push_back(action);

        char winner;
        if (success && !new_cards.is_win(winner) && !new_cards.is_over()) {
            InformationSet new_I = I;
            new_I.update_move(action);

            if (I.player == 'x') {
                if (new_cards.player_to_move == 'x') {
                    is_child_infoset_ticked = explore(new_I, I_2, previous_opponent_I, prev_opponent_action, new_cards, current_history, br_player, opponent_policy, I_a_tickmark, I_tickmark, reward, m, infoset_reach_count, empirical_action_reward, action_terminal_reach_count);
                }
                else {
                    is_child_infoset_ticked = explore(new_I, I_2, I, action, new_cards, current_history, br_player, opponent_policy, I_a_tickmark, I_tickmark, reward, m, infoset_reach_count, empirical_action_reward, action_terminal_reach_count);
                }

                
            } else {
                if (new_cards.player_to_move == 'o') {
                    is_child_infoset_ticked = explore(I_1, new_I, previous_opponent_I, prev_opponent_action, new_cards, current_history, br_player, opponent_policy, I_a_tickmark, I_tickmark, reward, m, infoset_reach_count, empirical_action_reward, action_terminal_reach_count);
                }
                else {
                    is_child_infoset_ticked = explore(I_1, new_I, I, action, new_cards, current_history, br_player, opponent_policy, I_a_tickmark, I_tickmark, reward, m, infoset_reach_count, empirical_action_reward, action_terminal_reach_count);
                }
            }
        } else {
            TerminalHistory H_T = TerminalHistory(current_history.history);
            H_T.set_reward(I.game);
            int r = br_player == 'x' ? H_T.reward[0] : H_T.reward[1];
            terminal_flag = 1;

            // update action pull count and empirical action reward only when action leads to terminal state
            if (I.player == br_player){
                action_terminal_reach_count[I.get_index()][action] += 1;
                if (r > 0){
                    empirical_action_reward[I.get_index()][action][0] += r;
                }
                else if (r == 0){
                    empirical_action_reward[I.get_index()][action][1] += 1;
                }
                else if (r < 0){
                    empirical_action_reward[I.get_index()][action][2] += (-1 * r);
                }
            }
            else {
                action_terminal_reach_count[previous_opponent_I.get_index()][prev_opponent_action] += 1;

                if (r > 0){
                    empirical_action_reward[previous_opponent_I.get_index()][prev_opponent_action][0] += r;
                }
                else if (r == 0){
                    empirical_action_reward[previous_opponent_I.get_index()][prev_opponent_action][1] += 1;
                }
                else if (r < 0){
                    empirical_action_reward[previous_opponent_I.get_index()][prev_opponent_action][2] += (-1 * r);
                }
            }

            reward = (double) r;
        }
    }
    else {
        InformationSet new_I = I;
        PokerTable new_board = true_cards;
        new_I.simulate_sense(action, new_board);
        current_history.history.push_back(action);

        if (I.player == 'x') {
            is_child_infoset_ticked = explore(new_I, I_2, previous_opponent_I, prev_opponent_action, new_board, current_history, br_player, opponent_policy, I_a_tickmark, I_tickmark, reward, m, infoset_reach_count, empirical_action_reward, action_terminal_reach_count);
        } else {
            is_child_infoset_ticked = explore(I_1, new_I, previous_opponent_I, prev_opponent_action, new_board, current_history, br_player, opponent_policy, I_a_tickmark, I_tickmark, reward, m, infoset_reach_count, empirical_action_reward, action_terminal_reach_count);
        }
    }

    if (I.player == br_player){
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


void explore_wrapper(InformationSet& I_1, PokerTable& true_cards, InformationSet& I_2, std::vector<std::vector<int>>& I_a_tickmark, std::vector<int>& I_tickmark, double& reward, PolicyVec& opponent_policy, History& current_history, char br_player, int m, 
                     std::vector<int>& infoset_reach_count, std::vector<std::vector<std::vector<int>>>& empirical_action_reward, std::vector<std::vector<int>>& action_terminal_reach_count) {
    explore(I_1, I_2, I_2, 0, true_cards, current_history, br_player, opponent_policy, I_a_tickmark, I_tickmark, reward, m, infoset_reach_count, empirical_action_reward, action_terminal_reach_count);
}


int exploit(InformationSet& I_1, InformationSet& I_2, InformationSet previous_opponent_I, int prev_opponent_action, PokerTable& true_cards, History& current_history, char br_player, 
            PolicyVec& policy_obj, PolicyVec& opponent_policy, std::vector<int>& infoset_reach_count, std::vector<std::vector<std::vector<int>>>& empirical_action_reward, 
            std::vector<std::vector<int>>& action_terminal_reach_count, std::vector<int>& I_tickmark, std::vector<std::vector<int>>& I_a_tickmark, std::vector<std::vector<int>>& action_explore_count) {
    InformationSet I = true_cards.player_to_move == 'x' ? I_1 : I_2;
    int action = 0;
    int terminal_flag = 0;
    int is_child_infoset_ticked = 0;
    
    if (br_player == I.player){
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
        PokerTable new_board = true_cards;
        bool success = new_board.update_move(action);
        current_history.history.push_back(action);

        char winner;
        if (success && !new_board.is_win(winner) && !new_board.is_over()) {
            InformationSet new_I = I;
            new_I.update_move(action);

            if (I.player == 'x') {
                if (new_board.player_to_move == 'x') {
                    is_child_infoset_ticked = exploit(new_I, I_2, previous_opponent_I, prev_opponent_action, new_board, current_history, br_player, policy_obj, opponent_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, I_tickmark, I_a_tickmark, action_explore_count);
                }
                else {
                    is_child_infoset_ticked = exploit(new_I, I_2, I, action, new_board, current_history, br_player, policy_obj, opponent_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, I_tickmark, I_a_tickmark, action_explore_count);
                }
            } else {
                if (new_board.player_to_move == 'o') {
                    is_child_infoset_ticked = exploit(I_1, new_I, previous_opponent_I, prev_opponent_action, new_board, current_history, br_player, policy_obj, opponent_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, I_tickmark, I_a_tickmark, action_explore_count);
                }
                else {
                    is_child_infoset_ticked = exploit(I_1, new_I, I, action, new_board, current_history, br_player, policy_obj, opponent_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, I_tickmark, I_a_tickmark, action_explore_count);
                }
            }
        } else {
            TerminalHistory H_T = TerminalHistory(current_history.history);
            H_T.set_reward(I.game);
            int r = br_player == 'x' ? H_T.reward[0] : H_T.reward[1];
            terminal_flag = 1;

            // update action pull count and empirical action reward only when action leads to terminal state
            if (I.player == br_player){
                action_terminal_reach_count[I.get_index()][action] += 1;        
                if (r > 0){
                    empirical_action_reward[I.get_index()][action][0] += r;
                }
                else if (r == 0){
                    empirical_action_reward[I.get_index()][action][1] += 1;
                }
                else if (r < 0){
                    empirical_action_reward[I.get_index()][action][2] += (-1 * r);
                }
            }
            else {
                action_terminal_reach_count[previous_opponent_I.get_index()][prev_opponent_action] += 1;
                
                if (r > 0){
                    empirical_action_reward[previous_opponent_I.get_index()][prev_opponent_action][0] += r;
                }
                else if (r == 0){
                    empirical_action_reward[previous_opponent_I.get_index()][prev_opponent_action][1] += 1;
                }
                else if (r < 0){
                    empirical_action_reward[previous_opponent_I.get_index()][prev_opponent_action][2] += (-1 * r);
                }
            }
        }
    }
    else {
        InformationSet new_I = I;
        PokerTable new_board = true_cards;
        new_I.simulate_sense(action, new_board);
        current_history.history.push_back(action);

        if (I.player == 'x') {
            is_child_infoset_ticked = exploit(new_I, I_2, previous_opponent_I, prev_opponent_action, new_board, current_history, br_player, policy_obj, opponent_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, I_tickmark, I_a_tickmark, action_explore_count);
        } else {
            is_child_infoset_ticked = exploit(I_1, new_I, previous_opponent_I, prev_opponent_action, new_board, current_history, br_player, policy_obj, opponent_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, I_tickmark, I_a_tickmark, action_explore_count);
        }
    }

    if (I.player == br_player){
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


void exploit_wrapper(InformationSet& I_1, PokerTable& true_cards, InformationSet& I_2, PolicyVec& policy_obj, PolicyVec& opponent_policy, History& current_history, char br_player, 
                     std::vector<int>& infoset_reach_count, std::vector<std::vector<std::vector<int>>>& empirical_action_reward, 
                     std::vector<std::vector<int>>& action_terminal_reach_count, std::vector<int>& I_tickmark, std::vector<std::vector<int>>& I_a_tickmark, std::vector<std::vector<int>>& action_explore_count) {
    if (br_player == 'x'){
        exploit(I_1, I_2, I_2, 0, true_cards, current_history, br_player, policy_obj, opponent_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, I_tickmark, I_a_tickmark, action_explore_count);
    }
    else {
        exploit(I_1, I_2, I_1, 0, true_cards, current_history, br_player, policy_obj, opponent_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, I_tickmark, I_a_tickmark, action_explore_count);
    }
}


double build_max_reward_policy(PolicyVec& policy_obj, InformationSet& I, std::vector<int>& infoset_reach_count, 
                               std::vector<std::vector<std::vector<int>>>& empirical_action_reward, 
                               std::vector<std::vector<int>>& action_terminal_reach_count, std::vector<double>& infoset_values, char game) {
    std::vector<int> legal_actions;
    I.get_actions(legal_actions);
    std::vector<double> action_values(6, 0.0);
    double infoset_value = I.game == 'L'? LEDUC_MIN_UTILITY : KUHN_MIN_UTILITY;

    for (int a : legal_actions){
        std::unordered_set<std::string> cohort;
        std::unordered_map<std::string, double> cohort_values;
        get_cohort(I, a, cohort);
        int norm = 0;
        int terminal_reach_count = action_terminal_reach_count[I.get_index()][a];

        for (std::string I_prime_hash : cohort){
            InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash, game);
            cohort_values[I_prime_hash] = build_max_reward_policy(policy_obj, I_prime, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_values, game);

            norm += infoset_reach_count[I_prime.get_index()];
            action_values[a] += cohort_values[I_prime_hash] * infoset_reach_count[I_prime.get_index()]; 
        }

        norm += terminal_reach_count;
        action_values[a] += empirical_action_reward[I.get_index()][a][0] - empirical_action_reward[I.get_index()][a][2];
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


double build_max_reward_policy_parallel(PolicyVec& policy_obj, InformationSet&I, std::vector<int>& infoset_reach_count, 
                                        std::vector<std::vector<std::vector<int>>>& empirical_action_reward, 
                                        std::vector<std::vector<int>>& action_terminal_reach_count, std::vector<double>& infoset_values, char game) {
    std::vector<int> legal_actions;
    I.get_actions(legal_actions);
    std::vector<double> action_values(6, 0.0);
    double infoset_value = I.game == 'L'? LEDUC_MIN_UTILITY : KUHN_MIN_UTILITY;
 
    #pragma omp parallel for num_threads(NUMBER_THREADS)
    for (int a : legal_actions){
        std::unordered_set<std::string> cohort;
        std::unordered_map<std::string, double> cohort_values;
        get_cohort(I, a, cohort);
        int norm = 0;
        int terminal_reach_count = action_terminal_reach_count[I.get_index()][a];

        for (std::string I_prime_hash : cohort){
            InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash, game);
            cohort_values[I_prime_hash] = build_max_reward_policy(policy_obj, I_prime, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_values, game);

            norm += infoset_reach_count[I_prime.get_index()];
            action_values[a] += cohort_values[I_prime_hash] * infoset_reach_count[I_prime.get_index()];
        }

        norm += terminal_reach_count;
        action_values[a] += empirical_action_reward[I.get_index()][a][0] - empirical_action_reward[I.get_index()][a][2];

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


void update_max_reward_policy_given_history(InformationSet& I, PokerTable& true_cards, InformationSet& opponent_I, History& current_history, PolicyVec& max_reward_policy, 
                                            std::vector<int>& infoset_reach_count, std::vector<std::vector<std::vector<int>>>& empirical_action_reward, 
                                            std::vector<std::vector<int>>& action_terminal_reach_count, std::vector<double>& infoset_values, char br_player, int traversal_index, char game) {
    if (traversal_index == current_history.history.size()){
        return;
    }
    
    int action = current_history.history[traversal_index];
    char curr_player = true_cards.player_to_move;

    if (curr_player == br_player){
        if (I.move_flag) {
            InformationSet new_I = I;
            PokerTable new_board = true_cards;
            new_I.update_move(action);
            new_board.update_move(action);
            update_max_reward_policy_given_history(new_I, new_board, opponent_I, current_history, max_reward_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_values, br_player, traversal_index + 1, game);
        }
        else {
            InformationSet new_I = I;
            new_I.simulate_sense(action, true_cards);
            update_max_reward_policy_given_history(new_I, true_cards, opponent_I, current_history, max_reward_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_values, br_player, traversal_index + 1, game);
        }

        std::vector<int> legal_actions;
        I.get_actions(legal_actions);
        std::vector<double> action_values(6, 0.0);
        double infoset_value = I.game == 'L'? LEDUC_MIN_UTILITY : KUHN_MIN_UTILITY;

        for (int a : legal_actions){
            std::unordered_set<std::string> cohort;
            std::unordered_map<std::string, double> cohort_values;
            get_cohort(I, a, cohort);

            int norm = 0;
            int terminal_reach_count = action_terminal_reach_count[I.get_index()][a];

            for (std::string I_prime_hash : cohort){
                InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash, game);
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
            opponent_I.update_move(action);
            true_cards.update_move(action);
            update_max_reward_policy_given_history(I, true_cards, opponent_I, current_history, max_reward_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_values, br_player, traversal_index + 1, game);
        }
        else {
            opponent_I.simulate_sense(action, true_cards);
            update_max_reward_policy_given_history(I, true_cards, opponent_I, current_history, max_reward_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_values, br_player, traversal_index + 1, game);
        }
    }
}


double build_max_UCB_policy(PolicyVec& policy_obj, InformationSet& I, std::vector<int>& infoset_reach_count, std::vector<int>& I_tickmark, 
                            std::vector<std::vector<std::vector<int>>>& empirical_action_reward, std::vector<std::vector<int>>& action_terminal_reach_count, 
                            std::vector<int>& infoset_time_step, int C, std::vector<int>& success_metrics, std::vector<double>& infoset_ucb_values, std::vector<std::vector<int>>& success_metrics_pi_hat, std::vector<std::vector<int>>& action_explore_count, char game){
    std::vector<int> legal_actions;
    I.get_actions(legal_actions);
    std::vector<double> action_ucb_values(6, 0.0);
    double max_ucb = I.game == 'L'? LEDUC_MIN_UTILITY : KUHN_MIN_UTILITY;

    for (int a : legal_actions){
        std::unordered_set<std::string> cohort;
        std::unordered_map<std::string, double> cohort_ucb_values;
        get_cohort(I, a, cohort);
        int u = 0;
        int terminal_reach_count = action_terminal_reach_count[I.get_index()][a]; // number of times action led to terminal state

        for (std::string I_prime_hash : cohort){
            InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash, game);
            std::vector<int> success_metrics_prime{0, 0, 0}; // wins, draws, losses

            cohort_ucb_values[I_prime_hash] = build_max_UCB_policy(policy_obj, I_prime, infoset_reach_count, I_tickmark, empirical_action_reward, action_terminal_reach_count, infoset_time_step, C, success_metrics_prime, infoset_ucb_values, success_metrics_pi_hat, action_explore_count, game);


            if (I_tickmark[I_prime.get_index()] == 0){
                u += infoset_reach_count[I_prime.get_index()];
                action_ucb_values[a] += infoset_reach_count[I_prime.get_index()];
            }
            else {
                int denom = success_metrics_prime[0] + success_metrics_prime[1] + success_metrics_prime[2];
                if (denom != 0){
                    u += denom;
                    action_ucb_values[a] += (success_metrics_prime[0] - success_metrics_prime[2]);
                    
                    success_metrics[0] += success_metrics_prime[0];
                    success_metrics[1] += success_metrics_prime[1];
                    success_metrics[2] += success_metrics_prime[2];
                }
            }
        }

        if (terminal_reach_count != 0){
            u += terminal_reach_count;
            action_ucb_values[a] += (empirical_action_reward[I.get_index()][a][0] - empirical_action_reward[I.get_index()][a][2]);

            success_metrics[0] += empirical_action_reward[I.get_index()][a][0];
            success_metrics[1] += empirical_action_reward[I.get_index()][a][1];
            success_metrics[2] += empirical_action_reward[I.get_index()][a][2];
        }

        if (u != 0){
            action_ucb_values[a] /= u;
            action_ucb_values[a] += sqrt(C * log(infoset_time_step[I.get_index()]) / u);
            infoset_time_step[I.get_index()] += 1;
        }
        else {
            action_ucb_values[a] = I.game == 'L'? LEDUC_MAX_UTILITY : KUHN_MAX_UTILITY;
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
                                     std::vector<int>& infoset_time_step, int C, std::vector<int>& success_metrics, std::vector<double>& infoset_ucb_values, std::vector<std::vector<int>>& success_metrics_pi_hat, std::vector<std::vector<int>>& action_explore_count, char game){
    std::vector<int> legal_actions;
    I.get_actions(legal_actions);
    std::vector<double> action_ucb_values(6, 0.0);
    double max_ucb = I.game == 'L'? LEDUC_MIN_UTILITY : KUHN_MIN_UTILITY;

    #pragma omp parallel for num_threads(NUMBER_THREADS)
    for (int a : legal_actions){
        std::unordered_set<std::string> cohort;
        std::unordered_map<std::string, double> cohort_ucb_values;
        get_cohort(I, a, cohort);
        int u = 0;
        int terminal_reach_count = action_terminal_reach_count[I.get_index()][a]; // number of times action led to terminal state

        for (std::string I_prime_hash : cohort){
            InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash, game);
            std::vector<int> success_metrics_prime{0, 0, 0}; // wins, draws, losses

            cohort_ucb_values[I_prime_hash] = build_max_UCB_policy(policy_obj, I_prime, infoset_reach_count, I_tickmark, empirical_action_reward, action_terminal_reach_count, infoset_time_step, C, success_metrics_prime, infoset_ucb_values, success_metrics_pi_hat, action_explore_count, game);

            if (I_tickmark[I_prime.get_index()] == 0){
                u += infoset_reach_count[I_prime.get_index()];
                action_ucb_values[a] += infoset_reach_count[I_prime.get_index()];
            }
            else {
                int denom = success_metrics_prime[0] + success_metrics_prime[1] + success_metrics_prime[2];
                if (denom != 0){
                    u += denom;
                    action_ucb_values[a] += (success_metrics_prime[0] - success_metrics_prime[2]);

                    success_metrics[0] += success_metrics_prime[0];
                    success_metrics[1] += success_metrics_prime[1];
                    success_metrics[2] += success_metrics_prime[2];
                }
            }
        }

        if (terminal_reach_count != 0){
            u += terminal_reach_count;
            action_ucb_values[a] += (empirical_action_reward[I.get_index()][a][0] - empirical_action_reward[I.get_index()][a][2]);

            success_metrics[0] += empirical_action_reward[I.get_index()][a][0];
            success_metrics[1] += empirical_action_reward[I.get_index()][a][1];
            success_metrics[2] += empirical_action_reward[I.get_index()][a][2];
        }

        if (u != 0){
            action_ucb_values[a] /= u;
            action_ucb_values[a] += sqrt(C * log(infoset_time_step[I.get_index()]) / u);
            infoset_time_step[I.get_index()] += 1;
        }
        else {
            action_ucb_values[a] = I.game == 'L'? LEDUC_MAX_UTILITY : KUHN_MAX_UTILITY;
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


void update_max_UCB_policy_given_history(InformationSet& I, PokerTable& true_cards, InformationSet& opponent_I, History& current_history, PolicyVec& max_UCB_policy, std::vector<int>& infoset_reach_count, 
                                         std::vector<std::vector<std::vector<int>>>& empirical_action_reward, std::vector<std::vector<int>>& action_terminal_reach_count, 
                                         std::vector<double>& infoset_ucb_values, char br_player, int traversal_index, std::vector<int>& infoset_time_step, 
                                         int C, std::vector<int>& I_tickmark, std::vector<std::vector<int>>& success_metrics_pi_hat, std::vector<std::vector<int>>& action_explore_count, char game) {
    if (traversal_index == current_history.history.size()){
        return;
    }
    
    int action = current_history.history[traversal_index];
    char curr_player = true_cards.player_to_move;

    if (curr_player == br_player){
        if (I.move_flag) {
            InformationSet new_I = I;
            PokerTable new_board = true_cards;
            new_I.update_move(action);
            new_board.update_move(action);
            update_max_UCB_policy_given_history(new_I, new_board, opponent_I, current_history, max_UCB_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_ucb_values, br_player, traversal_index + 1, infoset_time_step, C, I_tickmark, success_metrics_pi_hat, action_explore_count, game);
        }
        else {
            InformationSet new_I = I;
            new_I.simulate_sense(action, true_cards);
            update_max_UCB_policy_given_history(new_I, true_cards, opponent_I, current_history, max_UCB_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_ucb_values, br_player, traversal_index + 1, infoset_time_step, C, I_tickmark, success_metrics_pi_hat, action_explore_count, game);
        }   

        std::vector<int> legal_actions;
        I.get_actions(legal_actions);
        std::vector<double> action_ucb_values(6, 0.0);
        double max_ucb = true_cards.game == 'L'? LEDUC_MIN_UTILITY : KUHN_MIN_UTILITY;
        std::vector<int> success_metrics{0, 0, 0};

        for (int a : legal_actions){
            std::unordered_set<std::string> cohort;
            std::unordered_map<std::string, double> cohort_ucb_values;
            get_cohort(I, a, cohort);

            int u = 0;
            // int norm = 0;
            int terminal_reach_count = action_terminal_reach_count[I.get_index()][a]; // number of times action led to terminal state

            for (std::string I_prime_hash : cohort){
                InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash, game);

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
                        action_ucb_values[a] += (success_metrics_prime[0] - success_metrics_prime[2]);

                        success_metrics[0] += success_metrics_prime[0];
                        success_metrics[1] += success_metrics_prime[1];
                        success_metrics[2] += success_metrics_prime[2];
                    }
                }
            }

            if (terminal_reach_count != 0){
                u += terminal_reach_count;
                action_ucb_values[a] += (empirical_action_reward[I.get_index()][a][0] - empirical_action_reward[I.get_index()][a][2]);

                success_metrics[0] += empirical_action_reward[I.get_index()][a][0];
                success_metrics[1] += empirical_action_reward[I.get_index()][a][1];
                success_metrics[2] += empirical_action_reward[I.get_index()][a][2];
            }

            if (u != 0){
                action_ucb_values[a] /= u;
                action_ucb_values[a] += sqrt(C * log(infoset_time_step[I.get_index()]) / u);
                infoset_time_step[I.get_index()] += 1;
            }
            else {
                action_ucb_values[a] = true_cards.game == 'L'? LEDUC_MAX_UTILITY : KUHN_MAX_UTILITY;
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
            opponent_I.update_move(action);
            true_cards.update_move(action);
            update_max_UCB_policy_given_history(I, true_cards, opponent_I, current_history, max_UCB_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_ucb_values, br_player, traversal_index + 1, infoset_time_step, C, I_tickmark, success_metrics_pi_hat, action_explore_count, game);
        }
        else {
            opponent_I.simulate_sense(action, true_cards);
            update_max_UCB_policy_given_history(I, true_cards, opponent_I, current_history, max_UCB_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_ucb_values, br_player, traversal_index + 1, infoset_time_step, C, I_tickmark, success_metrics_pi_hat, action_explore_count, game);
        }
    }
}


void calc_br_IS_LUCB(PolicyVec& opponent_policy, char br_player, std::vector<std::string>& player_information_sets, int log_frequency, int m, PolicyVec& player_br, int experiment_number, char game, int iterations, int C, std::string& exp_name) {    
    std::vector<std::vector<int>> I_a_tickmark(player_information_sets.size(), std::vector<int>(6, 0));
    std::vector<int> I_tickmark(player_information_sets.size(), 0);

    std::vector<int> infoset_reach_count(player_information_sets.size(), 0);
    std::vector<int> infoset_time_step(player_information_sets.size(), 1);
    std::vector<double> infoset_values(player_information_sets.size(), 0.0);
    std::vector<double> infoset_ucb_values(player_information_sets.size(), 0.0);

    std::vector<std::vector<int>> success_metrics_pi_hat(player_information_sets.size(), std::vector<int>(3, 0));

    std::vector<std::vector<std::vector<int>>> empirical_action_reward(player_information_sets.size(), std::vector<std::vector<int>>(6, std::vector<int>(3, 0)));
    std::vector<std::vector<int>> action_terminal_reach_count(player_information_sets.size(), std::vector<int>(6, 0));
    std::vector<std::vector<int>> action_explore_count(player_information_sets.size(), std::vector<int>(6, 0));

    PolicyVec player_max_ucb_policy(br_player, player_information_sets, game);
    std::vector<std::pair<int, double>> exploitability_log; 

    std::vector<std::string>& unique_draws = game == 'L' ? unique_draws_leduc : unique_draws_kuhn;
    std::vector<double>& draw_probabilities = game == 'L' ? draw_probabilities_leduc : draw_probabilities_kuhn;

    int flag = 1;
    int t = 0;
    int k = 1;

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

    while (flag){ 
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
        double reward = 0.0;
        explore_wrapper(I_1, true_cards, I_2, I_a_tickmark, I_tickmark, reward, opponent_policy, start_history, br_player, k, infoset_reach_count, empirical_action_reward, action_terminal_reach_count);

        t += 1;

        InformationSet I = br_player == 'x' ? InformationSet('x', true, hash_1, game) : InformationSet('o', false, hash_2, game);
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
    std::cout << "Playing " << iterations << " games with C = " << C << " and log frequency = " << log_frequency << std::endl;

    std::vector<char> player_cards = {'J', 'Q', 'K'};
    for (int card_index = 0; card_index < player_cards.size(); card_index++){
        std::string hash_1 = "a-" + std::string(1, player_cards[card_index]) + "--";
        std::string hash_2 = "o-" + std::string(1, player_cards[card_index]) + "--";
        InformationSet root = br_player == 'x' ? InformationSet('x', true, hash_1, game) : InformationSet('o', false, hash_2, game);
        double root_val = build_max_reward_policy(player_br, root, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_values, game);    
    }

    std::cout << "Best response policy computed" << std::endl;

    for (int card_index = 0; card_index < player_cards.size(); card_index++){
        std::string hash_1 = "a-" + std::string(1, player_cards[card_index]) + "--";
        std::string hash_2 = "o-" + std::string(1, player_cards[card_index]) + "--";
        InformationSet root = br_player == 'x' ? InformationSet('x', true, hash_1, game) : InformationSet('o', false, hash_2, game);
        std::vector<int> success_metrics{0, 0, 0};
        double max_UCB = build_max_UCB_policy(player_max_ucb_policy, root, infoset_reach_count, I_tickmark, empirical_action_reward, action_terminal_reach_count, infoset_time_step, C, success_metrics, infoset_ucb_values, success_metrics_pi_hat, action_explore_count, game);
    }

    std::cout << "Max UCB policy computed" << std::endl;

    bool max_UCB_flag = true;
    for (; t <= iterations; t += 1) {
        if (t % log_frequency == 0 && t != 0) { 
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

            int count = 0;
            for (int i = 0; i < infoset_reach_count.size(); i++) {
                if (infoset_reach_count[i] > 0) {
                    count += 1;
                }
            }
  
            std::cout << "Number of information sets visited: " << count << std::endl;
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

            exploit_wrapper(I_1, true_cards, I_2, player_max_ucb_policy, opponent_policy, start_history, br_player, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, I_tickmark, I_a_tickmark, action_explore_count);

            true_cards = PokerTable(cards);
            true_cards.game = game;
            hash_1 = "a-" + std::string(1, cards[0]) + "--";
            hash_2 = "o-" + std::string(1, cards[1]) + "--";
            I_1 = InformationSet('x', true, hash_1, game);
            I_2 = InformationSet('o', false, hash_2, game);

            if (br_player == 'x') {
                update_max_reward_policy_given_history(I_1, true_cards, I_2, start_history, player_br, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_values, br_player, 3, game);
            }
            else {
                update_max_reward_policy_given_history(I_2, true_cards, I_1, start_history, player_br, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_values, br_player, 3, game);
            }

            true_cards = PokerTable(cards);
            true_cards.game = game;
            hash_1 = "a-" + std::string(1, cards[0]) + "--";
            hash_2 = "o-" + std::string(1, cards[1]) + "--";
            I_1 = InformationSet('x', true, hash_1, game);
            I_2 = InformationSet('o', false, hash_2, game);

            if (br_player == 'x') {
                update_max_UCB_policy_given_history(I_1, true_cards, I_2, start_history, player_max_ucb_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_ucb_values, br_player, 3, infoset_time_step, C, I_tickmark, success_metrics_pi_hat, action_explore_count, game);
            }
            else {
                update_max_UCB_policy_given_history(I_2, true_cards, I_1, start_history, player_max_ucb_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_ucb_values, br_player, 3, infoset_time_step, C, I_tickmark, success_metrics_pi_hat, action_explore_count, game);
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

            exploit_wrapper(I_1, true_cards, I_2, player_br, opponent_policy, start_history, br_player, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, I_tickmark, I_a_tickmark, action_explore_count);

            true_cards = PokerTable(cards);
            true_cards.game = game;
            hash_1 = "a-" + std::string(1, cards[0]) + "--";
            hash_2 = "o-" + std::string(1, cards[1]) + "--";
            I_1 = InformationSet('x', true, hash_1, game);
            I_2 = InformationSet('o', false, hash_2, game);

            if (br_player == 'x') {
                update_max_reward_policy_given_history(I_1, true_cards, I_2, start_history, player_br, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_values, br_player, 3, game);
            }
            else {
                update_max_reward_policy_given_history(I_2, true_cards, I_1, start_history, player_br, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_values, br_player, 3, game);
            }

            true_cards = PokerTable(cards);
            true_cards.game = game;
            hash_1 = "a-" + std::string(1, cards[0]) + "--";
            hash_2 = "o-" + std::string(1, cards[1]) + "--";
            I_1 = InformationSet('x', true, hash_1, game);
            I_2 = InformationSet('o', false, hash_2, game);
  
            if (br_player == 'x') {
                update_max_UCB_policy_given_history(I_1, true_cards, I_2, start_history, player_max_ucb_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_ucb_values, br_player, 3, infoset_time_step, C, I_tickmark, success_metrics_pi_hat, action_explore_count, game);
            }
            else {
                update_max_UCB_policy_given_history(I_2, true_cards, I_1, start_history, player_max_ucb_policy, infoset_reach_count, empirical_action_reward, action_terminal_reach_count, infoset_ucb_values, br_player, 3, infoset_time_step, C, I_tickmark, success_metrics_pi_hat, action_explore_count, game);
            }

            max_UCB_flag = true;
        }
    }

    std::cout << "Saving exploitability log" << std::endl;
    std::string file_name = "data/bandit/" + exp_name + "_" + std::string(1, game) + "_poker_" + std::string(1, br_player) + "_C=" + std::to_string(C) + "_IS-LUCB_exploitability_log_" + std::to_string(experiment_number) + ".txt";

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
    int C = std::stoi(argv[8]);
    std::string exp_name = argv[9];
    std::string algorithm = argv[10];
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

    // compute epsilon best response
    int experiment_num = start_index;

    while (experiment_num < experiments + start_index) {
        if (player == 'x') {
            PolicyVec uniform_x('x', P1_information_sets, game);
            calc_br_IS_LUCB(policy_obj_o, 'x', P1_information_sets, log_frequency, 1, uniform_x, experiment_num, game, iterations, C, exp_name);
        }
        else {
            PolicyVec uniform_o('o', P2_information_sets, game);
            calc_br_IS_LUCB(policy_obj_x, 'o', P2_information_sets, log_frequency, 1, uniform_o, experiment_num, game, iterations, C, exp_name);
        }

        std::cout << "(" << experiment_num << " experiments done)" << std::endl;
        experiment_num += 1;
    }
}