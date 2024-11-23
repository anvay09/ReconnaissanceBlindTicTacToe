#include "cpp_headers/rbt_classes.hpp"
#include "cpp_headers/rbt_utilities.hpp"
#include <random>
#include <cmath>
int NUMBER_THREADS = 4;


void pretty_print(std::chrono::time_point<std::chrono::system_clock> start, std::chrono::time_point<std::chrono::system_clock> end, std::string msg, int flag) {
    if (flag) {
    std::chrono::duration<double> elapsed_seconds = end-start;
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);
    std::cout << "finished " << msg << " in " << elapsed_seconds.count() << "s" << std::endl;
    }
}


void print_histogram(std::vector<long int>& visited_infosets) {
    std::vector<long int> buckets(10, 0);
    // index 0: visited 0 times, index 1: visited 1 times, index 2: visited 2 times, index 3: visited 2-10 times, index 4: visited 11-100 times, index 5: visited 101-1000 times, 
    //index 6: visited 1001-10000 times, index 7: visited 10001-100000 times, index 8: visited 100001-1000000 times, index 9: visited 1000001+ times

    for (long int i = 0; i < visited_infosets.size(); i++) {
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
        std::cout << buckets[i] << "\t\t";
    }
    std::cout << std::endl;

    std::cout << "0\t\t1\t\t2\t\t3-10\t\t11-100\t\t101-1k\t\t1k-10k\t\t10k-100k\t\t100k-1M\t\t1M+" << std::endl;
}


int sampleIndex(const std::vector<double>& probabilities) {
    std::random_device rd;
    std::mt19937 generator(rd());
    std::discrete_distribution<int> distribution(probabilities.begin(), probabilities.end());
    return distribution(generator);
}


double sample_game_given_policies(InformationSet& I_1, InformationSet& I_2, TicTacToeBoard& true_board, PolicyVec& policy_obj_x, 
                                  PolicyVec& policy_obj_o, History& current_history, char player, double& reward, char br_player) {
    InformationSet& I = player == 'x' ? I_1 : I_2;
    PolicyVec& policy_obj = player == 'x' ? policy_obj_x : policy_obj_o;
    std::vector<double> prob_dist = policy_obj.policy_dict[I.get_index()];

    int action = sampleIndex(prob_dist);

    if (I.move_flag) {
        bool success = true_board.update_move(action, player);
        current_history.history.push_back(action);

        char winner;
        if (success && !true_board.is_win(winner) && !true_board.is_over()) {
            InformationSet new_I = I;
            new_I.update_move(action, player);
            new_I.reset_zeros();

            if (player == 'x') {
                return sample_game_given_policies(new_I, I_2, true_board, policy_obj_x, policy_obj_o, current_history, 'o', reward, br_player);
            } else {
                return sample_game_given_policies(I_1, new_I, true_board, policy_obj_x, policy_obj_o, current_history, 'x', reward, br_player);
            }
        } else {
            TerminalHistory H_T = TerminalHistory(current_history.history);
            H_T.set_reward();
            reward = br_player == 'x' ? (double) H_T.reward[0] : (double) H_T.reward[1];
            return reward;
        }
    }
    else {
        InformationSet new_I = I;
        new_I.simulate_sense(action, true_board);
        current_history.history.push_back(action);

        if (player == 'x') {
            return sample_game_given_policies(new_I, I_2, true_board, policy_obj_x, policy_obj_o, current_history, 'x', reward, br_player);
        } else {
            return sample_game_given_policies(I_1, new_I, true_board, policy_obj_x, policy_obj_o, current_history, 'o', reward, br_player);
        }
    }
}


double sample_game_given_policies_wrapper(PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, History& current_history, double& reward, char br_player) {
    std::string board = "000000000";
    TicTacToeBoard true_board = TicTacToeBoard(board);
    std::string hash_1 = "";
    std::string hash_2 = "";
    InformationSet I_1 = InformationSet('x', true, hash_1);
    InformationSet I_2 = InformationSet('o', false, hash_2);
    return sample_game_given_policies(I_1, I_2, true_board, policy_obj_x, policy_obj_o, current_history, 'x', reward, br_player);
}


double sample_terminal_history(InformationSet& I_1, InformationSet& I_2, TicTacToeBoard& true_board, std::vector<std::vector<double>>& infoset_ucb_values, std::vector<std::vector<double>>& infoset_empirical_reward, std::vector<std::vector<long int>>& infoset_pull_count, long int timestep, long int C, PolicyVec& opponent_policy, History& current_history, char player, char br_player) {
    InformationSet& I = player == 'x' ? I_1 : I_2;
    int action = 0;
    if (player == br_player){ // choose action with max UCB value
        std::vector<double>& action_ucbs = infoset_ucb_values[I.get_index()];
        std::vector<double>& emp_rewards = infoset_empirical_reward[I.get_index()];
        std::vector<long int>& pull_counts = infoset_pull_count[I.get_index()];

        double max_ucb = -std::numeric_limits<double>::infinity();
        std::vector<int> legal_actions;
        I.get_actions(legal_actions);

        // updating global timestep
        for (int a : legal_actions){
            if (pull_counts[a] > 0){
                action_ucbs[a] = emp_rewards[a] + sqrt(C*log(timestep) / pull_counts[a]);
            }
        }

        for (int a : legal_actions){
            if (action_ucbs[a] >= max_ucb){
                max_ucb = action_ucbs[a];
                action = a;
            }
        }

        std::vector<double> best_arms(13, 0.0);
        double sum = 0.0;

        for (int a : legal_actions){
            if (std::fabs(action_ucbs[a] - max_ucb) < std::numeric_limits<double>::epsilon()){
                best_arms[a] = 1.0;
                sum += 1.0;
            }
        }

        if (sum > 0){
            for (int a : legal_actions){
                best_arms[a] /= sum;
            }
            action = sampleIndex(best_arms);
        }
    }
    
    else {
        std::vector<double> prob_dist = opponent_policy.policy_dict[I.get_index()];
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
                return sample_terminal_history(new_I, I_2, true_board, infoset_ucb_values, infoset_empirical_reward, infoset_pull_count, timestep, C, opponent_policy, current_history, 'o', br_player);
            } else {
                return sample_terminal_history(I_1, new_I, true_board, infoset_ucb_values, infoset_empirical_reward, infoset_pull_count, timestep, C, opponent_policy, current_history, 'x', br_player);
            }
        } else {
            TerminalHistory H_T = TerminalHistory(current_history.history);
            H_T.set_reward();
            double reward = br_player == 'x' ? (double) H_T.reward[0] : (double) H_T.reward[1];
            return reward;
        }
    }
    else {
        InformationSet new_I = I;
        new_I.simulate_sense(action, true_board);
        current_history.history.push_back(action);

        if (player == 'x') {
            return sample_terminal_history(new_I, I_2, true_board, infoset_ucb_values, infoset_empirical_reward, infoset_pull_count, timestep, C, opponent_policy, current_history, 'x', br_player);
        } else {
            return sample_terminal_history(I_1, new_I, true_board, infoset_ucb_values, infoset_empirical_reward, infoset_pull_count, timestep, C, opponent_policy, current_history, 'o', br_player);
        }
    }
}


double sample_terminal_history_wrapper(std::vector<std::vector<double>>& infoset_ucb_values, std::vector<std::vector<double>>& infoset_empirical_reward, std::vector<std::vector<long int>>& infoset_pull_count, long int timestep, long int C, PolicyVec& opponent_policy, History& current_history, char br_player) {
    std::string board = "000000000";
    TicTacToeBoard true_board = TicTacToeBoard(board);
    std::string hash_1 = "";
    std::string hash_2 = "";
    InformationSet I_1 = InformationSet('x', true, hash_1);
    InformationSet I_2 = InformationSet('o', false, hash_2);
    return sample_terminal_history(I_1, I_2, true_board, infoset_ucb_values, infoset_empirical_reward, infoset_pull_count, timestep, C, opponent_policy, current_history, 'x', br_player);
}


void build_policy(std::vector<std::vector<double>>& ucb_values, PolicyVec& policy_obj, std::vector<std::string>& information_sets){
    #pragma omp parallel for num_threads(NUMBER_THREADS)
    for (long int i = 0; i < ucb_values.size(); i++){
        std::vector<double>& action_ucbs = ucb_values[i];
        double max_reward = -100.0;
        std::vector<int> legal_actions;
        std::string I_hash = information_sets[i];
        InformationSet I(policy_obj.player, get_move_flag(I_hash, policy_obj.player), I_hash);
        I.get_actions(legal_actions);
        int action = 0;

        for (int a : legal_actions){
            if (action_ucbs[a] >= max_reward){
                max_reward = action_ucbs[a];
                action = a;
            }
        }

        std::vector<double> best_arms(13, 0.0);
        best_arms[action] = 1.0;

        double sum = 0.0;

        for (int a : legal_actions){
            if (std::fabs(action_ucbs[a] - max_reward) < std::numeric_limits<double>::epsilon()){
                best_arms[a] = 1.0;
                sum += 1.0;
            }
        }

        if (sum > 0) {
            for (int a : legal_actions){
                best_arms[a] /= sum;
            }
        }   
        else {
            best_arms[action] = 1.0;
        }

        policy_obj.policy_dict[i] = best_arms;
    }
}


void update_ucb(std::vector<std::vector<double>>& infoset_ucb_values, std::vector<std::vector<double>>& infoset_empirical_reward, std::vector<std::vector<long int>>& infoset_pull_count, long int timestep, double reward, TerminalHistory& history, char player, long int C) {
    std::string board = "000000000";
    TicTacToeBoard true_board = TicTacToeBoard(board);
    std::string hash_1 = "";
    std::string hash_2 = "";
    InformationSet I_1 = InformationSet('x', true, hash_1);
    InformationSet I_2 = InformationSet('o', false, hash_2);
    char curr_player = 'x';
    double total_reward = 0.0;
    long int total_pull = 0;

    for (int action : history.history) {

        if (curr_player == player) {
            InformationSet I = curr_player == 'x' ? I_1 : I_2;
            total_pull = infoset_pull_count[I.get_index()][action];
            total_reward = infoset_empirical_reward[I.get_index()][action] * total_pull;
            infoset_pull_count[I.get_index()][action] += 1;
            infoset_empirical_reward[I.get_index()][action] =  (total_reward + reward) / (total_pull + 1);

            std::vector<int> legal_actions;
            I.get_actions(legal_actions);
            double exploration_bonus = C;
            for (int a : legal_actions){
                if (infoset_pull_count[I.get_index()][a] > 0){
                    infoset_ucb_values[I.get_index()][a] = infoset_empirical_reward[I.get_index()][a] + sqrt(exploration_bonus*log(timestep) / infoset_pull_count[I.get_index()][a]);
                }
            }
        }

        if (action < 9) {
            if (curr_player == 'x') {
                I_1.update_move(action, curr_player);
                I_1.reset_zeros();
            } else {
                I_2.update_move(action, curr_player);
                I_2.reset_zeros();
            }
            true_board.update_move(action, curr_player);
            curr_player = (curr_player == 'x') ? 'o' : 'x';
        } 
        else {
            if (curr_player == 'x') {
                I_1.simulate_sense(action, true_board);
            } else {
                I_2.simulate_sense(action, true_board);
            }
        }
    }

}


double update_ucb_reverse_recursive(InformationSet& I_1, InformationSet& I_2, TicTacToeBoard& true_board, std::vector<std::vector<double>>& infoset_ucb_values, std::vector<std::vector<double>>& infoset_empirical_reward, std::vector<std::vector<long int>>& infoset_pull_count, long int timestep, double reward, TerminalHistory& history, int traversal_index, char player, char curr_player, long int C){
    if (traversal_index == history.history.size()){
        return reward;
    }
    
    int played_action = history.history[traversal_index];
    double percolated_reward = 0.0;

    if (played_action < 9) {
        if (curr_player == 'x') {
            InformationSet new_I = I_1;
            new_I.update_move(played_action, curr_player);
            new_I.reset_zeros();
            TicTacToeBoard new_board = true_board;
            new_board.update_move(played_action, curr_player);

            percolated_reward = update_ucb_reverse_recursive(new_I, I_2, new_board, infoset_ucb_values, infoset_empirical_reward, infoset_pull_count, timestep, reward, history, traversal_index + 1, player, 'o', C);
        } else {
            InformationSet new_I = I_2;
            new_I.update_move(played_action, curr_player);
            new_I.reset_zeros();
            TicTacToeBoard new_board = true_board;
            new_board.update_move(played_action, curr_player);

            percolated_reward = update_ucb_reverse_recursive(I_1, new_I, new_board, infoset_ucb_values, infoset_empirical_reward, infoset_pull_count, timestep, reward, history, traversal_index + 1, player, 'x', C);
        }
    } 
    else {
        if (curr_player == 'x') {
            InformationSet new_I = I_1;
            new_I.simulate_sense(played_action, true_board);
            TicTacToeBoard new_board = true_board;

            percolated_reward = update_ucb_reverse_recursive(new_I, I_2, new_board, infoset_ucb_values, infoset_empirical_reward, infoset_pull_count, timestep, reward, history, traversal_index + 1, player, 'x', C);
        } else {
            InformationSet new_I = I_2;
            new_I.simulate_sense(played_action, true_board);
            TicTacToeBoard new_board = true_board;

            percolated_reward = update_ucb_reverse_recursive(I_1, new_I, new_board, infoset_ucb_values, infoset_empirical_reward, infoset_pull_count, timestep, reward, history, traversal_index + 1, player, 'o', C);
        }
    }

    if (curr_player == player) {
        InformationSet I = curr_player == 'x' ? I_1 : I_2;
        long int total_pull = infoset_pull_count[I.get_index()][played_action];
        double total_reward = infoset_empirical_reward[I.get_index()][played_action] * total_pull;
        
        // check if percolated reward is not infinity
        if (percolated_reward != std::numeric_limits<double>::infinity()){
            infoset_pull_count[I.get_index()][played_action] += 1;
            infoset_empirical_reward[I.get_index()][played_action] =  (total_reward + percolated_reward) / (total_pull + 1);
        }
        else {
            infoset_pull_count[I.get_index()][played_action] += 1;
            infoset_empirical_reward[I.get_index()][played_action] =  (total_reward + reward) / (total_pull + 1);
        }
        
        std::vector<int> legal_actions;
        I.get_actions(legal_actions);
        double exploration_bonus = C;
        double max_reward = -std::numeric_limits<double>::infinity();
        for (int a : legal_actions){
            if (infoset_empirical_reward[I.get_index()][a] >= max_reward){
                max_reward = infoset_empirical_reward[I.get_index()][a];
            }

            if (infoset_pull_count[I.get_index()][a] > 0){
                infoset_ucb_values[I.get_index()][a] = infoset_empirical_reward[I.get_index()][a] + sqrt(exploration_bonus*log(timestep) / infoset_pull_count[I.get_index()][a]);
            }
        }

        return max_reward;
    }
    else {
        return percolated_reward;
    }
}


double update_ucb_reverse_recursive_wrapper(std::vector<std::vector<double>>& infoset_ucb_values, std::vector<std::vector<double>>& infoset_empirical_reward, std::vector<std::vector<long int>>& infoset_pull_count, long int timestep, double reward, TerminalHistory& history, char player, long int C){
    std::string board = "000000000";
    TicTacToeBoard true_board = TicTacToeBoard(board);
    std::string hash_1 = "";
    std::string hash_2 = "";
    InformationSet I_1 = InformationSet('x', true, hash_1);
    InformationSet I_2 = InformationSet('o', false, hash_2);
    char curr_player = 'x';

    return update_ucb_reverse_recursive(I_1, I_2, true_board, infoset_ucb_values, infoset_empirical_reward, infoset_pull_count, timestep, reward, history, 0, player, curr_player, C);
}


void calc_br_ucb(PolicyVec& opponent_policy, long int num_iterations, char br_player, std::vector<std::string>& player_information_sets, long int log_flag, long int log_frequency, long int C, PolicyVec& br_policy) {
    std::vector<long int> infoset_time_steps(player_information_sets.size(), 0);
    std::vector<std::vector<double>> infoset_ucb_values(player_information_sets.size(), std::vector<double>(13, std::numeric_limits<double>::infinity()));
    std::vector<std::vector<double>> infoset_empirical_reward(player_information_sets.size(), std::vector<double>(13, 0.0));
    std::vector<std::vector<long int>> infoset_pull_count(player_information_sets.size(), std::vector<long int>(13, 0));

    for (long int t = 0; t <= num_iterations; t++) {
        // sample terminal history
        std::vector<int> h = {};
        TerminalHistory start_history = TerminalHistory(h);
        double reward = 0.0;

        reward = sample_terminal_history_wrapper(infoset_ucb_values, infoset_empirical_reward, infoset_pull_count, t, C, opponent_policy, start_history, br_player);
        // update ucb values
        // update_ucb(infoset_ucb_values, infoset_empirical_reward, infoset_pull_count, t, reward, start_history, br_player, C);
        update_ucb_reverse_recursive_wrapper(infoset_ucb_values, infoset_empirical_reward, infoset_pull_count, t, reward, start_history, br_player, C);
        
        if (t % log_frequency == 0 && t != 0){
            PolicyVec policy_obj(br_player, player_information_sets);
            std::cout << "Build policy" << std::endl;
            build_policy(infoset_ucb_values, policy_obj, player_information_sets);
            if (br_player == 'x'){
                double expected_utility = get_expected_utility_wrapper(policy_obj, opponent_policy);
                std::cout << "Expected utility after " << t << " iterations: " << expected_utility << std::endl;

                // sample 10 games using best response policy and then sample 10 games using the policy just built

                // std::cout << "Sample 10 games using the policy just built..." << std::endl;
                // for (int i = 0; i < 10; i++){
                //     std::vector<int> empty_h = {};
                //     TerminalHistory start_h = TerminalHistory(empty_h);
                //     double reward = 0.0;

                //     reward = sample_game_given_policies_wrapper(policy_obj, opponent_policy, start_h, reward, br_player);
                //     std::cout << "Reward: " << reward << " History: ";
                //     start_h.print_history();
                // }

                std::string start_infoset_hash = "";
                InformationSet start_infoset = InformationSet('x', true, start_infoset_hash);
                std::cout << "Index of starting information set: " << start_infoset.get_index() << std::endl;
                // print ucbs for the starting information set
                for (int a = 0; a < 13; a++){
                    std::cout << "Action: " << a << " UCB: " << infoset_ucb_values[start_infoset.get_index()][a] << " Pull count: " << infoset_pull_count[start_infoset.get_index()][a] << " Empirical reward: " << infoset_empirical_reward[start_infoset.get_index()][a] << std::endl;
                }

            }
            else {
                double expected_utility = get_expected_utility_wrapper(opponent_policy, policy_obj);
                std::cout << "Expected utility after " << t << " iterations: " << expected_utility << std::endl;

                // sample 10 games using best response policy and then sample 10 games using the policy just built
                // std::cout << "Sampling 10 games using best response policy..." << std::endl;
                // for (int i = 0; i < 10; i++){
                //     std::vector<int> empty_h = {};
                //     TerminalHistory start_h = TerminalHistory(empty_h);
                //     double reward = 0.0;

                //     reward = sample_game_given_policies_wrapper(opponent_policy, br_policy, start_h, reward, br_player);
                //     std::cout << "Reward: " << reward << "History: ";
                //     start_h.print_history();
                // }

                // std::cout << "Sampling 10 games using the policy just built..." << std::endl;
                // for (int i = 0; i < 10; i++){
                //     std::vector<int> empty_h = {};
                //     TerminalHistory start_h = TerminalHistory(empty_h);
                //     double reward = 0.0;

                //     reward = sample_game_given_policies_wrapper(opponent_policy, policy_obj, start_h, reward, br_player);
                //     std::cout << "Reward: " << reward << " History: ";
                //     start_h.print_history();
                // }
            }
        }
    } 
}


int main(int argc, char* argv[]) {
    std::cout.precision(17);
    std::string file_path_1 = argv[1];
    std::string file_path_2 = argv[2];
    int log_flag = std::stoi(argv[3]);
    NUMBER_THREADS = std::stoi(argv[4]); //96;

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
    auto start = std::chrono::system_clock::now(); 
    PolicyVec policy_obj_x('x', file_path_1);
    PolicyVec policy_obj_o('o', file_path_2);
    PolicyVec br_x('x', P1_information_sets);
    PolicyVec br_o('o', P2_information_sets);
    auto end = std::chrono::system_clock::now();
    pretty_print(start, end, "loading policies", log_flag);

    // compute epsilon best response
    char continue_exp = 'y';
    while (continue_exp == 'y') {
        long int num_iterations = 10000;
        long int log_frequency = 10000;
        char player = 'x';
        long int C = 1;
        int policy_flag = 0;
        std::cout << "Enter number of iterations: ";
        std::cin >> num_iterations;
        std::cout << "Enter log frequency: ";
        std::cin >> log_frequency;
        std::cout << "Enter player for best response calculation: ";
        std::cin >> player;
        std::cout << "Enter hyperparameter c for UCB:";
        std::cin >> C;

        if (player == 'x') {
            // compute expected utility of the best response (i.e., when opponent policy is known)
            start = std::chrono::system_clock::now();  
            double expected_utility = compute_best_response_wrapper(policy_obj_o, br_x, 'x');
            end = std::chrono::system_clock::now();
            std::cout << "Expected utility of the best response x: " << expected_utility << std::endl;
            pretty_print(start, end, "computing best response x", log_flag);
        }
        else {
            // compute expected utility of the best response (i.e., when opponent policy is known)
            start = std::chrono::system_clock::now(); 
            double expected_utility = compute_best_response_wrapper(policy_obj_x, br_o, 'o');
            end = std::chrono::system_clock::now();
            std::cout << "Expected utility of the best response o: " << expected_utility << std::endl;
            pretty_print(start, end, "computing best response o", log_flag);
        }

        calc_br_ucb(player == 'x' ? policy_obj_o : policy_obj_x, num_iterations, player, player == 'x' ? P1_information_sets : P2_information_sets, log_flag, log_frequency, C, player == 'x' ? br_x : br_o);
        
        std::cout << "Continue experiments? (y/n): ";
        std::cin >> continue_exp;
    }
    
}
