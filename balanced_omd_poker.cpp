#include "cpp_headers/poker_classes.hpp"
#include "cpp_headers/poker_utilities.hpp"
#include <random>
#include <cmath>
int NUMBER_THREADS = 4;


int sampleIndex(const std::vector<double>& probabilities) {
    std::random_device rd;
    std::mt19937 generator(rd());
    std::discrete_distribution<int> distribution(probabilities.begin(), probabilities.end());
    return distribution(generator);
}


double sample_terminal_history(InformationSet& I_1, InformationSet& I_2, PokerTable& true_cards, PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, History& current_history, char update_player, std::vector<std::pair<std::string, int>>& trajectory) {
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

            if (I.player == update_player){
                trajectory.push_back(std::make_pair(I.get_hash(), action));
            }

            if (I.player == 'x') {
                return sample_terminal_history(new_I, I_2, true_cards, policy_obj_x, policy_obj_o, current_history, update_player, trajectory);
            } else {
                return sample_terminal_history(I_1, new_I, true_cards, policy_obj_x, policy_obj_o, current_history, update_player, trajectory);
            }
        } else {
            TerminalHistory H_T = TerminalHistory(current_history.history);
            H_T.set_reward(I.game);

            if (I.player == update_player){
                trajectory.push_back(std::make_pair(I.get_hash(), action));
            }

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

        if (I.player == update_player){
            trajectory.push_back(std::make_pair(I.get_hash(), action));
        }

        if (I.player == 'x') {
            return sample_terminal_history(new_I, I_2, true_cards, policy_obj_x, policy_obj_o, current_history, update_player, trajectory);
        } else {
            return sample_terminal_history(I_1, new_I, true_cards, policy_obj_x, policy_obj_o, current_history, update_player, trajectory);
        }
    }
}


double sample_terminal_history_wrapper(PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, char update_player, char game, std::vector<std::pair<std::string, int>>& trajectory) {
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
  
    return sample_terminal_history(I_1, I_2, true_cards, policy_obj_x, policy_obj_o, current_history, update_player, trajectory);
}


int build_balanced_exploration_policy(PolicyVec& policy_obj, InformationSet& I) {
    std::vector<int> legal_actions;
    I.get_actions(legal_actions);
    std::vector<int> action_tree_size(6, 0);

    for (int a : legal_actions){
        std::unordered_set<std::string> cohort;
        std::unordered_map<std::string, int> cohort_values;
        get_cohort(I, a, cohort);

        if (cohort.size() == 0){
            action_tree_size[a] = 1;
            continue;
        }

        for (std::string I_prime_hash : cohort){
            InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash);
            cohort_values[I_prime_hash] = build_balanced_exploration_policy(policy_obj, I_prime);

            action_tree_size[a] += cohort_values[I_prime_hash];
        }
    }

    int total = 0;
    for (int a : legal_actions){
        total += action_tree_size[a];
    }

    for (int a : legal_actions){
        policy_obj.policy_dict[I.get_index()][a] = (double)action_tree_size[a] / total;
    }

    return total;
}


void build_balanced_exploration_policy_wrapper(PolicyVec& policy_obj, char player, char game){
    std::vector<char> player_cards = {'J', 'Q', 'K'};
    for (int card_index = 0; card_index < player_cards.size(); card_index++){
        std::string hash_1 = "a-" + std::string(1, player_cards[card_index]) + "--";
        std::string hash_2 = "o-" + std::string(1, player_cards[card_index]) + "--";
        InformationSet root = player == 'x' ? InformationSet('x', true, hash_1, game) : InformationSet('o', false, hash_2, game);
        int root_val = build_balanced_exploration_policy(policy_obj, root);
    }
}


void construct_loss_estimators(std::vector<double>& loss_obj, PolicyVec& mu_t, PolicyVec& mu_star, double gamma, double reward, std::vector<std::pair<std::string, int>>& trajectory, char update_player, char game, std::vector<double>& mu_star_reach_list) {
    int H = trajectory.size();

    double mu_star_reach = 1.0;
    double mu_t_reach = 1.0;

    // compute loss estimators
    for (int h = 0; h < H; h++){
        std::string I_hash = trajectory[h].first;
        int action = trajectory[h].second;
        InformationSet I = InformationSet(update_player, get_move_flag(I_hash, update_player), I_hash, game);

        mu_star_reach *= mu_star.policy_dict[I.get_index()][action];
        mu_t_reach *= mu_t.policy_dict[I.get_index()][action];
        mu_star_reach_list[h] = mu_star_reach;

        double r_h = h == H - 1 ? reward : 0.0;
        loss_obj[h] = ((1.0 - r_h) / (mu_t_reach + gamma * mu_star_reach));
    }
}


void update_policy(PolicyVec& mu_t, std::vector<double>& loss_obj, double learning_rate, std::vector<std::pair<std::string, int>>& trajectory, char update_player, char game, std::vector<double>& mu_star_reach_list) {
    int H = trajectory.size();
    double Z_t = 1.0;
    double term = 0.0;
    
    for (int h = H - 1; h >= 0; h--){
        std::string I_hash = trajectory[h].first;
        int action = trajectory[h].second;
        InformationSet I = InformationSet(update_player, get_move_flag(I_hash, update_player), I_hash, game);
        
        if (h == H - 1){
            term = -learning_rate * mu_star_reach_list[h] * loss_obj[h];
        }
        else {
            term = -learning_rate * mu_star_reach_list[h] * loss_obj[h] + mu_star_reach_list[h] * std::log(Z_t) / mu_star_reach_list[h+1];
        }

        Z_t = 1.0 - mu_t.policy_dict[I.get_index()][action] + mu_t.policy_dict[I.get_index()][action] * std::exp(term);

        // Update policy
        double prob_sum = 0.0;
        std::vector<int> legal_actions;
        I.get_actions(legal_actions);
        for (int a : legal_actions){
            if (a == action){
                mu_t.policy_dict[I.get_index()][a] *= std::exp(term - std::log(Z_t));
                prob_sum += mu_t.policy_dict[I.get_index()][a];
            }
            else {
                mu_t.policy_dict[I.get_index()][a] *= std::exp( - std::log(Z_t));
                prob_sum += mu_t.policy_dict[I.get_index()][a];
            }
        }

        // Renormalize
        for (int a : legal_actions){
            mu_t.policy_dict[I.get_index()][a] /= prob_sum;
        }
    }
}


void balanced_OMD(PolicyVec& mu_t, PolicyVec& mu_star, char update_player, char game, double gamma, double learning_rate, int iterations, int log_frequency, PolicyVec& opp_policy){
    for (int t = 0; t <= iterations; t++){
        std::vector<std::pair<std::string, int>> trajectory;
        double reward = 0.0;
        if (update_player == 'x'){
            reward = sample_terminal_history_wrapper(mu_t, opp_policy, update_player, game, trajectory);
        }
        else {
            reward = sample_terminal_history_wrapper(opp_policy, mu_t, update_player, game, trajectory);
        }

        // fit reward between 0 and 1
        double rho_G_i = game == 'L' ? LEDUC_MAX_UTILITY : KUHN_MAX_UTILITY;
        reward = (reward + rho_G_i) / (2 * rho_G_i);

        int H = trajectory.size();
        std::vector<double> loss_obj(H, 0.0);
        std::vector<double> mu_star_reach_list(H, 0.0);

        construct_loss_estimators(loss_obj, mu_t, mu_star, gamma, reward, trajectory, update_player, game, mu_star_reach_list);
        update_policy(mu_t, loss_obj, learning_rate, trajectory, update_player, game, mu_star_reach_list);

        if (t % log_frequency == 0){
            std::cout << "-------------------------------- Iteration " << t << " --------------------------------" << std::endl;
            double expected_utility = 0.0;

            if (update_player == 'x') {
                expected_utility = get_expected_utility_wrapper(mu_t, opp_policy, game);
            } else {
                expected_utility = get_expected_utility_wrapper(opp_policy, mu_t, game);
            }

            std::cout << "Expected utility: " << expected_utility << std::endl;
        }
    }
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
    double gamma = std::stod(argv[8]);
    double learning_rate = std::stod(argv[9]);
    std::string exp_name = argv[10];
    
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
    int experiment_num = 1;

    while (experiment_num <= experiments) {
        if (player == 'x') {
            PolicyVec uniform_x('x', P1_information_sets, game);
            PolicyVec balanced_x('x', P1_information_sets, game);
            build_balanced_exploration_policy_wrapper(balanced_x, 'x', game);  
            std::cout << "Built balanced exploration policy" << std::endl;
            balanced_OMD(uniform_x, balanced_x, 'x', game, gamma, learning_rate, iterations, log_frequency, policy_obj_o);
        }
        else {
            PolicyVec uniform_o('o', P2_information_sets, game);
            PolicyVec balanced_o('o', P2_information_sets, game);
            build_balanced_exploration_policy_wrapper(balanced_o, 'o', game);
            std::cout << "Built balanced exploration policy" << std::endl;
            balanced_OMD(uniform_o, balanced_o, 'o', game, gamma, learning_rate, iterations, log_frequency, policy_obj_x);
        }

        std::cout << "(" << experiment_num << " experiments done)" << std::endl;
        experiment_num += 1;
    }
}