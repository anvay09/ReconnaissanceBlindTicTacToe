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


bool LUCB_stopping_condition(std::vector<double>& UCB, std::vector<double>& LCB, double eps, int T, std::vector<PolicyVec>& P1_strategies, PolicyVec& policy_obj_o, char game){
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
        std::cout << "Stopping condition reached" << std::endl;
        std::cout << "T = " << T << std::endl;
        std::cout << "Arm with highest UCB: " << max_UCB_policy_index << std::endl;
        std::cout << "UCB: " << UCB[max_UCB_policy_index] << " LCB: " << LCB[max_UCB_policy_index] << std::endl;
        PolicyVec& P1_strategy = P1_strategies[max_UCB_policy_index];
        double expected_utility_arm = get_expected_utility_wrapper(P1_strategy, policy_obj_o, game);
        std::cout << "Expected utility of highest UCB arm: " << expected_utility_arm << std::endl;
        std::cout << "Second highest UCB: " << second_max_UCB_policy_index << std::endl;
        std::cout << "UCB: " << UCB[second_max_UCB_policy_index] << " LCB: " << LCB[second_max_UCB_policy_index] << std::endl;
        PolicyVec& P1_strategy_second = P1_strategies[second_max_UCB_policy_index];
        double expected_utility_arm_second = get_expected_utility_wrapper(P1_strategy_second, policy_obj_o, game);
        std::cout << "Expected utility of second highest UCB arm: " << expected_utility_arm_second << std::endl;
        return true;
    }
    return false;
}


int main(int argc, char* argv[]) {
    std::cout.precision(17);
    std::string file_path_1 = argv[1]; // start policy P1
    std::string file_path_2 = argv[2]; // start policy P2
    double eps = std::stod(argv[3]); // epsilon for stopping condition
    double delta = std::stod(argv[4]); // delta for mistake probability
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
    // PolicyVec uniform_o('o', P2_information_sets, game);

    std::vector<PolicyVec> P1_strategies = {uniform_x};
    // std::vector<PolicyVec> P2_strategies = {uniform_o};

    enumerate_all_deterministic_strategies(P1_information_sets, P1_strategies, game, 'x', 0);
    int n = P1_strategies.size();
    // enumerate_all_deterministic_strategies(P2_information_sets, P2_strategies, game, 'o', 0);

    std::vector <double> true_expected_utilities(P1_strategies.size(), 0.0);
    for (int i = 0; i < P1_strategies.size(); i++){
        true_expected_utilities[i] = get_expected_utility_wrapper(P1_strategies[i], policy_obj_o, game);
    }

    // initialize UCB, LCB, empirical mean for player 1
    std::vector<double> UCB(P1_strategies.size(), 0.0);
    std::vector<double> LCB(P1_strategies.size(), 0.0);
    std::vector<double> total_empirical_reward(P1_strategies.size(), 0.0);
    std::vector <int> pull_count (P1_strategies.size(), 0);
    int T = 0;
    double k = 4.0 / 5.0;
    int max_UCB_policy_index = 0;
    double max_UCB = 0.0;
    int max_empirical_mean_policy_index = 0;
    double max_empirical_mean = 0.0;

    // pull each arm once
    for (int i = 0; i < P1_strategies.size(); i++) {
        PolicyVec& P1_strategy = P1_strategies[i];
        double reward = sample_terminal_history_wrapper(P1_strategy, policy_obj_o, 'x', game);
        total_empirical_reward[i] += reward;
        pull_count[i] += 1;
        T += 1;
    }

    // update UCB, LCB
    for (int i = 0; i < P1_strategies.size(); i++) {
        UCB[i] = total_empirical_reward[i] / pull_count[i] + std::sqrt(std::log(n * T / delta) / (2 * pull_count[i]));
        LCB[i] = total_empirical_reward[i] / pull_count[i] - std::sqrt(std::log(n * T / delta) / (2 * pull_count[i]));
    }
    
    // main loop
    while (!LUCB_stopping_condition(UCB, LCB, eps, T, P1_strategies, policy_obj_o, game)){
        // print all indices, UCB, LCB, mean rewards, pull counts, true expected utilities
        if (T % 1000 == 0 && T != 0){
            std::cout << "T: " << T << std::endl;
            PolicyVec& P1_strategy = P1_strategies[max_empirical_mean_policy_index];
            double expected_utility_arm = get_expected_utility_wrapper(P1_strategy, policy_obj_o, game);
            std::cout << "Expected utility of highest empirical mean arm: " << expected_utility_arm << std::endl;
            for (int i = 0; i < P1_strategies.size(); i++){
                std::cout << "Index: " << i << " ";
                std::cout << "UCB: " << UCB[i] << " LCB: " << LCB[i] << " ";
                std::cout << "Mean reward: " << total_empirical_reward[i] / pull_count[i] << " ";
                std::cout << "Pull count: " << pull_count[i] << " ";
                std::cout << "True expected utility: " << true_expected_utilities[i] << std::endl;
            }
        }

        // select arm with highest UCB
        max_UCB_policy_index = 0;
        max_UCB = UCB[0];

        for (int i = 1; i < P1_strategies.size(); i++){
            if (UCB[i] > max_UCB){
                max_UCB = UCB[i];
                max_UCB_policy_index = i;
            }
        }

        // sample terminal history
        PolicyVec& P1_strategy_UCB = P1_strategies[max_UCB_policy_index];
        double reward_UCB = sample_terminal_history_wrapper(P1_strategy_UCB, policy_obj_o, 'x', game);

        // update empirical mean, UCB, LCB
        total_empirical_reward[max_UCB_policy_index] += reward_UCB;
        pull_count[max_UCB_policy_index] += 1;
        T += 1;

        for (int i = 0; i < P1_strategies.size(); i++) {
            UCB[i] = total_empirical_reward[i] / pull_count[i] + std::sqrt(std::log(k * n * std::pow(T, 4) / delta) / (2 * pull_count[i]));
            LCB[i] = total_empirical_reward[i] / pull_count[i] - std::sqrt(std::log(k * n * std::pow(T, 4) / delta) / (2 * pull_count[i]));
        }

        // select arm with highest empirical mean
        max_empirical_mean_policy_index = 0;
        max_empirical_mean = total_empirical_reward[0] / pull_count[0];

        for (int i = 1; i < P1_strategies.size(); i++){
            double empirical_mean = total_empirical_reward[i] / pull_count[i];
            if (empirical_mean > max_empirical_mean){
                max_empirical_mean = empirical_mean;
                max_empirical_mean_policy_index = i;
            }
        }

        // sample terminal history
        PolicyVec& P1_strategy = P1_strategies[max_empirical_mean_policy_index];
        double reward = sample_terminal_history_wrapper(P1_strategy, policy_obj_o, 'x', game);

        // update empirical mean, UCB, LCB
        total_empirical_reward[max_empirical_mean_policy_index] += reward;
        pull_count[max_empirical_mean_policy_index] += 1;
        T += 1;

        for (int i = 0; i < P1_strategies.size(); i++) {
            UCB[i] = total_empirical_reward[i] / pull_count[i] + std::sqrt(std::log(k * n * std::pow(T, 4) / delta) / (2 * pull_count[i]));
            LCB[i] = total_empirical_reward[i] / pull_count[i] - std::sqrt(std::log(k * n * std::pow(T, 4) / delta) / (2 * pull_count[i]));
        }
    }
}