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
            std::vector<int>& infoset_reach_count, std::vector<std::vector<double>>& empirical_action_reward, std::vector<std::vector<int>>& action_pull_count) {
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
                is_child_infoset_ticked = explore(new_I, I_2, I, new_board, current_history, 'o', br_player, opponent_policy, I_a_tickmark, I_tickmark, reward, m, infoset_reach_count, empirical_action_reward, action_pull_count);
            } else {
                is_child_infoset_ticked = explore(I_1, new_I, I, new_board, current_history, 'x', br_player, opponent_policy, I_a_tickmark, I_tickmark, reward, m, infoset_reach_count, empirical_action_reward, action_pull_count);
            }
        } else {
            TerminalHistory H_T = TerminalHistory(current_history.history);
            H_T.set_reward();
            reward = br_player == 'x' ? (double) H_T.reward[0] : (double) H_T.reward[1];
            terminal_flag = 1;

            // update action pull count and empirical action reward only when action leads to terminal state
            if (curr_player == br_player){
                action_pull_count[I.get_index()][action] += 1;
                empirical_action_reward[I.get_index()][action] += reward;
            }
            else {
                // find third last action in history
                int third_last_action = current_history.history[current_history.history.size() - 3];
                action_pull_count[previous_opponent_I.get_index()][third_last_action] += 1;
                empirical_action_reward[previous_opponent_I.get_index()][third_last_action] -= reward;
            }
        }
    }
    else {
        InformationSet new_I = I;
        TicTacToeBoard new_board = true_board;
        new_I.simulate_sense(action, new_board);
        current_history.history.push_back(action);

        if (curr_player == 'x') {
            is_child_infoset_ticked = explore(new_I, I_2, previous_opponent_I, new_board, current_history, 'x', br_player, opponent_policy, I_a_tickmark, I_tickmark, reward, m, infoset_reach_count, empirical_action_reward, action_pull_count);
        } else {
            is_child_infoset_ticked = explore(I_1, new_I, previous_opponent_I, new_board, current_history, 'o', br_player, opponent_policy, I_a_tickmark, I_tickmark, reward, m, infoset_reach_count, empirical_action_reward, action_pull_count);
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


void explore_wrapper(std::vector<std::vector<int>>& I_a_tickmark, std::vector<int>& I_tickmark, double& reward, PolicyVec& opponent_policy, History& current_history, char br_player, int m, std::vector<int>& infoset_reach_count, std::vector<std::vector<double>>& empirical_action_reward, std::vector<std::vector<int>>& action_pull_count) {
    std::string board = "000000000";
    TicTacToeBoard true_board = TicTacToeBoard(board);
    std::string hash_1 = "";
    std::string hash_2 = "";
    InformationSet I_1 = InformationSet('x', true, hash_1);
    InformationSet I_2 = InformationSet('o', false, hash_2);
    explore(I_1, I_2, I_2, true_board, current_history, 'x', br_player, opponent_policy, I_a_tickmark, I_tickmark, reward, m, infoset_reach_count, empirical_action_reward, action_pull_count);
}


double build_max_policy(PolicyVec& policy_obj, InformationSet& I, std::vector<int>& infoset_reach_count, std::vector<std::vector<double>>& empirical_action_reward, std::vector<std::vector<int>>& action_pull_count){
    std::vector<int> legal_actions;
    I.get_actions(legal_actions);
    std::vector<double> action_values(13, 0.0);
    double infoset_value = - std::numeric_limits<double>::infinity();
 
    // std::cout << "Building max policy for infoset " << I.get_hash() << std::endl;

    for (int a : legal_actions){
        std::unordered_set<std::string> cohort;
        get_cohort(I, a, cohort);

        if (cohort.size() == 0){
            int pull_count = action_pull_count[I.get_index()][a];
            if (pull_count == 0){
                action_values[a] = 0.0;
            }
            else{
                action_values[a] = empirical_action_reward[I.get_index()][a] / pull_count;
            }
        }
        else {
            std::unordered_map<std::string, double> cohort_values;
            int norm = 0;

            for (std::string I_prime_hash : cohort){
                InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash);
                cohort_values[I_prime_hash] = build_max_policy(policy_obj, I_prime, infoset_reach_count, empirical_action_reward, action_pull_count);

                norm += infoset_reach_count[I_prime.get_index()];
                action_values[a] += cohort_values[I_prime_hash] * infoset_reach_count[I_prime.get_index()];
            }

            int pull_count = action_pull_count[I.get_index()][a];
            norm += pull_count;
            action_values[a] += empirical_action_reward[I.get_index()][a];
            if (norm == 0){
                action_values[a] = 0.0;
            }
            else{
                action_values[a] /= norm;
                std::cout << "Infoset " << I.get_hash() << " Action " << a << " Value " << action_values[a] << std::endl;
            }
        }

        // find max action value

        if (action_values[a] >= infoset_value){
            infoset_value = action_values[a];
        }
    }

    double count = 0.0;
    for (int a : legal_actions){
        if (action_values[a] == infoset_value){
            count += 1.0;
        }
    }

    // update policy
    for (int a : legal_actions){
        if (action_values[a] == infoset_value){
            policy_obj.policy_dict[I.get_index()][a] = 1.0/count;
        }
        else{
            policy_obj.policy_dict[I.get_index()][a] = 0.0;
        }
    }

    return infoset_value;
}


void calc_br(PolicyVec& opponent_policy, char br_player, std::vector<std::string>& player_information_sets, long int log_frequency, int m, PolicyVec& player_br) {
    std::vector<std::vector<int>> I_a_tickmark(player_information_sets.size(), std::vector<int>(13, 0));
    std::vector<int> I_tickmark(player_information_sets.size(), 0);
    std::vector<int> infoset_reach_count(player_information_sets.size(), 0);
    std::vector<std::vector<double>> empirical_action_reward(player_information_sets.size(), std::vector<double>(13, 0.0));
    std::vector<std::vector<int>> action_pull_count(player_information_sets.size(), std::vector<int>(13, 0));

    int flag = 1;
    long int t = 0;
    int k = 1;

    while (flag){ 
        std::vector<int> h = {};
        TerminalHistory start_history = TerminalHistory(h);
        double reward = 0.0;
        explore_wrapper(I_a_tickmark, I_tickmark, reward, opponent_policy, start_history, br_player, k, infoset_reach_count, empirical_action_reward, action_pull_count);

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
    std::string hash = "";
    InformationSet root = br_player == 'x' ? InformationSet('x', true, hash) : InformationSet('o', false, hash);
    double root_value = build_max_policy(player_br, root, infoset_reach_count, empirical_action_reward, action_pull_count);
    std::cout << "Best response policy computed" << std::endl;
    // std::cout << "Value of best response policy: " << root_value << std::endl;
    // std::cout << "Value of actions at root infoset: " << std::endl;
    // for (int i = 0; i < 13; i++) {
    //     std::cout << "Action " << i << ": " << player_br.policy_dict[root.get_index()][i] << std::endl;
    // }

    double expected_utility = 0.0;
    if (br_player == 'x') {
        expected_utility = get_expected_utility_wrapper(player_br, opponent_policy);
    } else {
        expected_utility = get_expected_utility_wrapper(opponent_policy, player_br);
    }
    std::cout << "Expected utility of best response policy: " << expected_utility << std::endl;
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
    for (long int i = 0; i < P1_information_sets.size(); i++) {
        InformationSet::P1_hash_to_int_map[P1_information_sets[i]] = i;
    }
    for (long int i = 0; i < P2_information_sets.size(); i++) {
        InformationSet::P2_hash_to_int_map[P2_information_sets[i]] = i;
    }

    // load policies
    std::cout << "Loading policies" << std::endl;
    PolicyVec policy_obj_x('x', file_path_1);
    PolicyVec policy_obj_o('o', file_path_2);

    // compute epsilon best response
    char continue_exp = 'y';
    while (continue_exp == 'y') {
        long int log_frequency = 10000;
        char player = 'x';
        int m = 1;
        std::cout << "Enter log frequency: ";
        std::cin >> log_frequency;
        std::cout << "Enter player for pull arms: ";
        std::cin >> player;
        std::cout << "Enter value of m: ";
        std::cin >> m;

        if (player == 'x') {
            PolicyVec uniform_x('x', P1_information_sets);
            calc_br(policy_obj_o, 'x', P1_information_sets, log_frequency, m, uniform_x);
        }
        else {
            PolicyVec uniform_o('o', P2_information_sets);
            calc_br(policy_obj_x, 'o', P2_information_sets, log_frequency, m, uniform_o);
        }

        std::cout << "Continue experiments? (y/n): ";
        std::cin >> continue_exp;
    }
    
}
