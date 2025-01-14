#include "cpp_headers/poker_classes.hpp"
#include "cpp_headers/poker_utilities.hpp"
#include <random>
int NUM_THREADS = 4;
// g++-13 poker_exact_LUCB.cpp poker_classes.cpp poker_utilities.cpp -O3 -o poker_exact_LUCB -fopenmp

int sampleIndex(const std::vector<double>& probabilities) {
    std::random_device rd;
    std::mt19937 generator(rd());
    std::discrete_distribution<int> distribution(probabilities.begin(), probabilities.end());
    return distribution(generator);
}


void enumerate_all_deterministic_strategies(std::vector<std::string>& information_sets, std::vector<PolicyVec>& strategies, char game, char player, int index){
    if (index == information_sets.size()){
        return;
    }

    std::string I_hash = information_sets[index];
    InformationSet I(player, get_move_flag(I_hash, player), I_hash, game);
    std::vector<int> legal_actions;
    I.get_actions(legal_actions);

    std::vector<PolicyVec> new_strategies;
    for (int a : legal_actions){
        for (PolicyVec& strategy : strategies){
            PolicyVec new_strategy = strategy;

            for (int b : legal_actions){
                if (b == a){
                    new_strategy.policy_dict[I.get_index()][b] = 1.0;
                }
                else{
                    new_strategy.policy_dict[I.get_index()][b] = 0.0;
                }
            }

            new_strategies.push_back(new_strategy);
        }
    }

    strategies = new_strategies;
    enumerate_all_deterministic_strategies(information_sets, strategies, game, player, index + 1);
}


double sample_terminal_history(InformationSet& I_1, InformationSet& I_2, PokerTable& true_cards, PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, History& current_history, char update_player) {
    InformationSet I = true_cards.player_to_move == 'x' ? I_1 : I_2;
    PolicyVec& policy_obj = I.player == 'x' ? policy_obj_x : policy_obj_o;
    std::vector<double> prob_dist = policy_obj.policy_dict[I.get_index()];

    int action = sampleIndex(prob_dist);

    if (I.move_flag) {
        bool success = true_cards.update_move(action);
        current_history.history.push_back(action);

        char winner;
        if (success && !true_cards.is_win(winner) && !true_cards.is_over()) {
            InformationSet new_I = I;
            new_I.update_move(action);

            if (I.player == 'x') {
                return sample_terminal_history(new_I, I_2, true_cards, policy_obj_x, policy_obj_o, current_history, update_player);
            } else {
                return sample_terminal_history(I_1, new_I, true_cards, policy_obj_x, policy_obj_o, current_history, update_player);
            }
        } else {
            TerminalHistory H_T = TerminalHistory(current_history.history);
            H_T.set_reward(I.game);
            double reward;
            if (update_player == 'x'){
                reward = (double) H_T.reward[0];
            } else {
                reward = (double) H_T.reward[1];
            }
            return reward;
        }
    }
    else {
        InformationSet new_I = I;
        new_I.simulate_sense(action, true_cards);
        current_history.history.push_back(action);

        if (I.player == 'x') {
            return sample_terminal_history(new_I, I_2, true_cards, policy_obj_x, policy_obj_o, current_history, update_player);
        } else {
            return sample_terminal_history(I_1, new_I, true_cards, policy_obj_x, policy_obj_o, current_history, update_player);
        }
    }
}


double sample_terminal_history_wrapper(PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, char update_player, char game) {
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

    std::vector<int> h = {};
    TerminalHistory current_history = TerminalHistory(h);
    current_history.history.push_back(cards[0]);
    current_history.history.push_back(cards[1]);
    current_history.history.push_back(cards[2]);
  
    return sample_terminal_history(I_1, I_2, true_cards, policy_obj_x, policy_obj_o, current_history, update_player);
}


bool LUCB_stopping_condition(std::vector<double>& UCB, std::vector<double>& LCB, double eps){
    int max_UCB_policy_index = 0;
    int second_max_UCB_policy_index = 0;

    for (int i = 0; i < UCB.size(); i++){
        if (UCB[i] > UCB[max_UCB_policy_index]){
            second_max_UCB_policy_index = max_UCB_policy_index;
            max_UCB_policy_index = i;
        }
        else if (UCB[i] > UCB[second_max_UCB_policy_index]){
            second_max_UCB_policy_index = i;
        }
    }

    if (LCB[max_UCB_policy_index] > UCB[second_max_UCB_policy_index] - eps){
        return true;
    }
    return false;
}


int get_arm_with_highest_empirical_mean(std::vector<double>& total_empirical_reward, std::vector<long int>& pull_count){
    int max_empirical_mean_policy_index = 0;
    double max_empirical_mean = total_empirical_reward[0] / pull_count[0];

    for (int i = 1; i < total_empirical_reward.size(); i++){
        double empirical_mean = total_empirical_reward[i] / pull_count[i];
        if (empirical_mean > max_empirical_mean){
            max_empirical_mean = empirical_mean;
            max_empirical_mean_policy_index = i;
        }
    }

    return max_empirical_mean_policy_index;
}


int get_arm_with_highest_UCB(std::vector<double>& UCB){
    int max_UCB_policy_index = 0;
    double max_UCB = UCB[0];

    for (int i = 1; i < UCB.size(); i++){
        if (UCB[i] > max_UCB){
            max_UCB = UCB[i];
            max_UCB_policy_index = i;
        }
    }

    return max_UCB_policy_index;
}


void logging(int num_samples, std::vector<PolicyVec>& strategies, std::vector<double>& total_empirical_reward, std::vector<long int>& pull_count, PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, char player, char game, std::string output_file, double best_true_expected_utility){
    std::cout << "--------------- Num Samples: " << num_samples << " ---------------" << std::endl;
    int max_empirical_mean_policy_index = get_arm_with_highest_empirical_mean(total_empirical_reward, pull_count);
    double exploitability = 0.0;
    PolicyVec& strategy = strategies[max_empirical_mean_policy_index];
    if (player == 'x'){
        double expected_utility_arm = get_expected_utility_wrapper(strategy, policy_obj_o, game);
        std::cout << "Expected utility of highest empirical mean arm: " << expected_utility_arm << std::endl;
        exploitability = best_true_expected_utility - expected_utility_arm;
    } else {
        double expected_utility_arm = get_expected_utility_wrapper(policy_obj_x, strategy, game);
        std::cout << "Expected utility of highest empirical mean arm: " << expected_utility_arm << std::endl;
        exploitability = best_true_expected_utility - expected_utility_arm;
    }

    // append to outfile
    std::ofstream outfile;
    outfile.open(output_file, std::ios_base::app);
    outfile << num_samples << " " << exploitability << std::endl;
    outfile.close();
}


int main(int argc, char* argv[]) {
    std::cout.precision(17);
    std::string file_path_1 = argv[1]; // start policy P1
    std::string file_path_2 = argv[2]; // start policy P2
    double eps = std::stod(argv[3]); // epsilon for stopping condition
    double delta = std::stod(argv[4]); // delta for mistake probability
    char player = std::string(argv[5])[0]; // player to update
    int number_of_runs = std::stoi(argv[6]); // number of runs
    int log_freq = std::stoi(argv[7]); // logging frequency
    char game = 'K'; // do not run this code for Leduc Poker
    
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

    PolicyVec uniform_x('x', P1_information_sets, game);
    PolicyVec uniform_o('o', P2_information_sets, game);

    std::vector<PolicyVec> P1_strategies = {uniform_x};
    std::vector<PolicyVec> P2_strategies = {uniform_o};

    if (player == 'x'){
        enumerate_all_deterministic_strategies(P1_information_sets, P1_strategies, game, 'x', 0);
    } else {
        enumerate_all_deterministic_strategies(P2_information_sets, P2_strategies, game, 'o', 0);
    }

    std::vector<PolicyVec>& strategies = player == 'x' ? P1_strategies : P2_strategies;
    int n = strategies.size();
    int average_sample_complexity = 0;
    

    std::vector <double> true_expected_utilities(strategies.size(), 0.0);
    double best_true_expected_utility = player == 'x' ? KUHN_MIN_UTILITY : KUHN_MAX_UTILITY;

    for (int i = 0; i < strategies.size(); i++){
        if (player == 'x'){
            true_expected_utilities[i] = get_expected_utility_wrapper(strategies[i], policy_obj_o, game);
            if (true_expected_utilities[i] > best_true_expected_utility){
                best_true_expected_utility = true_expected_utilities[i];
            }
        } else {
            true_expected_utilities[i] = get_expected_utility_wrapper(policy_obj_x, strategies[i], game);
            if (true_expected_utilities[i] < best_true_expected_utility){
                best_true_expected_utility = true_expected_utilities[i];
            }
        }
    }
    std::cout << "Best true expected utility: " << best_true_expected_utility << std::endl;

    for (int j = 1; j <= number_of_runs; j++){
        // initialize UCB, LCB, empirical mean for player 1
        std::vector<double> UCB(strategies.size(), 0.0);
        std::vector<double> LCB(strategies.size(), 0.0);
        std::vector<double> total_empirical_reward(strategies.size(), 0.0);
        std::vector <long int> pull_count (strategies.size(), 0);
        long int T = 0;
        double k = 4.0 / 5.0;
        int max_UCB_policy_index = 0;
        int max_empirical_mean_policy_index = 0;
        int num_samples = 0;
        std::string output_file = "data/exact/Kuhn_Poker_exact_LUCB_" + std::string(1, player) + "_run_" + std::to_string(j) + ".txt";
        // open output file and wipe it clean
        std::ofstream outfile;
        outfile.open(output_file, std::ios::out);
        outfile.close();

        // pull each arm once
        for (int i = 0; i < strategies.size(); i++) {
            PolicyVec& strategy = strategies[i];
            if (player == 'x'){
                double reward = sample_terminal_history_wrapper(strategy, policy_obj_o, 'x', game);
                total_empirical_reward[i] += reward;
            } else {
                double reward = sample_terminal_history_wrapper(policy_obj_x, strategy, 'o', game);
                total_empirical_reward[i] += reward;
            }
            
            pull_count[i] += 1;
            T += 1;
            num_samples += 1;

            if (num_samples % log_freq == 0 && num_samples > 0){
                logging(num_samples, strategies, total_empirical_reward, pull_count, policy_obj_x, policy_obj_o, player, game, output_file, best_true_expected_utility);
            }
        }

        // update UCB, LCB
        for (int i = 0; i < strategies.size(); i++) {
            UCB[i] = total_empirical_reward[i] / pull_count[i] + std::sqrt(std::log(n * T / delta) / (2 * pull_count[i]));
            LCB[i] = total_empirical_reward[i] / pull_count[i] - std::sqrt(std::log(n * T / delta) / (2 * pull_count[i]));
        }
        
        // main loop
        while (!LUCB_stopping_condition(UCB, LCB, eps)){
            // select arm with highest UCB
            max_UCB_policy_index = get_arm_with_highest_UCB(UCB);

            // sample terminal history
            PolicyVec& strategy_UCB = strategies[max_UCB_policy_index];
            if (player == 'x'){
                double reward_UCB = sample_terminal_history_wrapper(strategy_UCB, policy_obj_o, 'x', game);
                total_empirical_reward[max_UCB_policy_index] += reward_UCB;
            } else {
                double reward_UCB = sample_terminal_history_wrapper(policy_obj_x, strategy_UCB, 'o', game);
                total_empirical_reward[max_UCB_policy_index] += reward_UCB;
            }
            
            // update empirical mean, UCB, LCB
            pull_count[max_UCB_policy_index] += 1;
            num_samples += 1;

            if (num_samples % log_freq == 0 && num_samples > 0){
                logging(num_samples, strategies, total_empirical_reward, pull_count, policy_obj_x, policy_obj_o, player, game, output_file, best_true_expected_utility);
            }

            for (int i = 0; i < strategies.size(); i++) {
                UCB[i] = total_empirical_reward[i] / pull_count[i] + std::sqrt(std::log(k * n * std::pow(T, 4) / delta) / (2 * pull_count[i]));
                LCB[i] = total_empirical_reward[i] / pull_count[i] - std::sqrt(std::log(k * n * std::pow(T, 4) / delta) / (2 * pull_count[i]));
            }

            // select arm with highest empirical mean
            max_empirical_mean_policy_index = get_arm_with_highest_empirical_mean(total_empirical_reward, pull_count);

            // sample terminal history
            PolicyVec& strategy = strategies[max_empirical_mean_policy_index];
            if (player == 'x'){
                double reward = sample_terminal_history_wrapper(strategy, policy_obj_o, 'x', game);
                total_empirical_reward[max_empirical_mean_policy_index] += reward;
            } else {
                double reward = sample_terminal_history_wrapper(policy_obj_x, strategy, 'o', game);
                total_empirical_reward[max_empirical_mean_policy_index] += reward;
            }

            // update empirical mean, UCB, LCB
            pull_count[max_empirical_mean_policy_index] += 1;
            num_samples += 1;
            if (num_samples % log_freq == 0 && num_samples > 0){
                logging(num_samples, strategies, total_empirical_reward, pull_count, policy_obj_x, policy_obj_o, player, game, output_file, best_true_expected_utility);
            }

            for (int i = 0; i < strategies.size(); i++) {
                UCB[i] = total_empirical_reward[i] / pull_count[i] + std::sqrt(std::log(k * n * std::pow(T, 4) / delta) / (2 * pull_count[i]));
                LCB[i] = total_empirical_reward[i] / pull_count[i] - std::sqrt(std::log(k * n * std::pow(T, 4) / delta) / (2 * pull_count[i]));
            }

            // update T
            T += 1;
        }

        std::cout << "Stopping condition reached." << std::endl;
        std::cout << "Number of samples: " << num_samples << std::endl;
        PolicyVec& strategy = strategies[max_empirical_mean_policy_index];
        if (player == 'x'){
            double expected_utility_arm = get_expected_utility_wrapper(strategy, policy_obj_o, game);
            std::cout << "Expected utility of highest empirical mean arm: " << expected_utility_arm << std::endl;
        } else {
            double expected_utility_arm = get_expected_utility_wrapper(policy_obj_x, strategy, game);
            std::cout << "Expected utility of highest empirical mean arm: " << expected_utility_arm << std::endl;
        }
        average_sample_complexity += num_samples;
    }

    std::cout << "Average sample complexity: " << (double) average_sample_complexity / number_of_runs << std::endl;
}