#include "cpp_headers/poker_classes.hpp"
#include "cpp_headers/poker_utilities.hpp"
#include <random>
#include <cmath>

int NUMBER_THREADS = 4;
const double e = 2.718281828459;

static std::random_device rd;
static std::mt19937 generator(rd());


int sampleIndex(const std::vector<double>& probabilities) {
    std::discrete_distribution<int> distribution(probabilities.begin(), probabilities.end());
    return distribution(generator);
}


double sample_terminal_history(InformationSet& I_1, InformationSet& I_2, InformationSet prev_opponent_I, int prev_opponent_action,
                               PokerTable& true_cards, PolicyVec& player_policy, PolicyVec& opponent_policy, History& current_history, 
                               std::vector<std::pair<std::string, int>>& trajectory, char br_player, std::vector<std::vector<double>>& Q, 
                               std::vector<double>& V, std::vector<std::vector<double>>& theta, std::vector<std::vector<double>>& kappa,
                               std::vector<int>& infoset_reach_count, std::vector<std::vector<int>>& terminal_reach_count, char game) {
    InformationSet I = true_cards.player_to_move == 'x' ? I_1 : I_2;
    int action;

    if (I.player == br_player) { 
        std::vector<int> legal_actions;
        I.get_actions(legal_actions);
        action = legal_actions[0];

        // choose the action with the highest UCB
        double max_UCB = 0.0;
        for (int a : legal_actions) {
            if (Q[I.get_index()][a] >= max_UCB) {
                max_UCB = Q[I.get_index()][a];
            }
        }

        std::vector<int> candidate_actions;
        for (int a : legal_actions) {
            if (fabs(Q[I.get_index()][a] - max_UCB) < 1e-6) {
                candidate_actions.push_back(a);
            }
        }

        action = candidate_actions[sampleIndex(std::vector<double>(candidate_actions.size(), 1.0 / candidate_actions.size()))];

        // update player policy to max UCB action
        std::vector<double>& prob_dist = player_policy.policy_dict[I.get_index()];
        for (int i = 0; i < prob_dist.size(); i++) { prob_dist[i] = 0.0; }
        prob_dist[action] = 1.0;
        
        // update reach count
        infoset_reach_count[I.get_index()] += 1;

        // update trajectory
        trajectory.push_back(std::make_pair(I.get_hash(), action));
    }
    else {
        std::vector<double>& prob_dist = opponent_policy.policy_dict[I.get_index()];
        action = sampleIndex(prob_dist);
    }

    if (I.move_flag) {
        bool success = true_cards.update_move(action);
        current_history.history.push_back(action);

        char winner;
        if (success && !true_cards.is_win(winner) && !true_cards.is_over()) {
            InformationSet new_I = I;
            new_I.update_move(action);

            if (I.player == 'x') {   
                if (true_cards.player_to_move == 'x'){
                    return sample_terminal_history(new_I, I_2, prev_opponent_I, prev_opponent_action, true_cards, player_policy, opponent_policy, current_history, trajectory, br_player, Q, V, theta, kappa, infoset_reach_count, terminal_reach_count, game);
                }
                else {
                    return sample_terminal_history(new_I, I_2, I, action, true_cards, player_policy, opponent_policy, current_history, trajectory, br_player, Q, V, theta, kappa, infoset_reach_count, terminal_reach_count, game);
                }
            } else {
                if (true_cards.player_to_move == 'o'){
                    return sample_terminal_history(I_1, new_I, prev_opponent_I, prev_opponent_action, true_cards, player_policy, opponent_policy, current_history, trajectory, br_player, Q, V, theta, kappa, infoset_reach_count, terminal_reach_count, game);
                }
                else {
                    return sample_terminal_history(I_1, new_I, I, action, true_cards, player_policy, opponent_policy, current_history, trajectory, br_player, Q, V, theta, kappa, infoset_reach_count, terminal_reach_count, game);
                }
            }
        } else {
            TerminalHistory H_T = TerminalHistory(current_history.history);
            H_T.set_reward(game);
            double reward = br_player == 'x' ? H_T.reward[0] : H_T.reward[1];
            double max_reward = game == 'L' ? LEDUC_MAX_UTILITY : KUHN_MAX_UTILITY;

            // scale reward between 0 and 1
            reward = (reward + max_reward) / (2.0 * max_reward);
   
            if (I.player == br_player){
                theta[I.get_index()][action] += reward;
                kappa[I.get_index()][action] += reward * reward;
                terminal_reach_count[I.get_index()][action] += 1;        
            }
            else {
                theta[prev_opponent_I.get_index()][prev_opponent_action] += reward;
                kappa[prev_opponent_I.get_index()][prev_opponent_action] += reward * reward;
                terminal_reach_count[prev_opponent_I.get_index()][prev_opponent_action] += 1;
            }
            
            return reward;
        }
    }
    else {
        InformationSet new_I = I;
        new_I.simulate_sense(action, true_cards);
        current_history.history.push_back(action);

        if (I.player == 'x') {       
            return sample_terminal_history(new_I, I_2, prev_opponent_I, prev_opponent_action, true_cards, player_policy, opponent_policy, current_history, trajectory, br_player, Q, V, theta, kappa, infoset_reach_count, terminal_reach_count, game);
        } else {
            return sample_terminal_history(I_1, new_I, prev_opponent_I, prev_opponent_action, true_cards, player_policy, opponent_policy, current_history, trajectory, br_player, Q, V, theta, kappa, infoset_reach_count, terminal_reach_count, game);
        }
    }
}


void updateBounds(std::vector<std::vector<double>>& theta, std::vector<std::vector<double>>& kappa, std::vector<int>& infoset_reach_count, 
                  std::vector<std::vector<int>>& terminal_reach_count, std::vector<std::vector<double>>& Q, 
                  std::vector<double>& V, std::vector<std::pair<std::string, int>>& trajectory, char br_player, double eps, double delta, int S, int H, int t, char game, 
                  double c1, double c2, double c3, double iota){
    int _H = trajectory.size();

    for (int h = _H-1; h >=0; h--){
        std::string I_hash = trajectory[h].first;
        InformationSet I = InformationSet(br_player, get_move_flag(I_hash, br_player), I_hash, game);
        int a = trajectory[h].second;
        double n_t = terminal_reach_count[I.get_index()][a];
        
        std::unordered_set<std::string> cohort;
        get_cohort(I, a, cohort);

        for (std::string I_prime_hash : cohort){
            InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash, game);
            n_t += infoset_reach_count[I_prime.get_index()];
        }
        
        double r_hat = theta[I.get_index()][a] / n_t;
        double sigma_hat = kappa[I.get_index()][a] / n_t;

        std::vector<double> p_hat(cohort.size() + 1, 0.0);
        std::vector<double> V_next(cohort.size() + 1, 0.0);
        double E_x = 0.0;
        double V_x = 0.0;
        
        int i = 0;
        p_hat[i] = terminal_reach_count[I.get_index()][a] / n_t;
        V_next[i] = r_hat;
        E_x += p_hat[i] * V_next[i];
        i++;

        for (std::string I_prime_hash : cohort){
            InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash, game);
            p_hat[i] = infoset_reach_count[I_prime.get_index()] / n_t;
            V_next[i] = V[I_prime.get_index()];
            E_x += p_hat[i] * V_next[i];
            i++;
        }
        
        i = 0;
        while(i < cohort.size() + 1){
            V_x += p_hat[i] * (V_next[i] - E_x) * (V_next[i] - E_x);
            i++;
        }

        double x = iota / n_t;
        // omitting second term: c2 * std::sqrt(x * (sigma_hat - r_hat * r_hat))
        double bonus = c1 * std::sqrt(x * V_x) + c3 * H * x;
        // std::cout << "Infoset: " << I.get_hash() << " Action: " << a << " Bonus: " << bonus << " E_x: " << E_x << " V_x: " << V_x << std::endl;

        // update Q, omitting r_hat
        Q[I.get_index()][a] = std::min((double) h, E_x + bonus);
        // update V
        std::vector<int> legal_actions;
        I.get_actions(legal_actions);
        for (int _a : legal_actions){
            if (Q[I.get_index()][_a] > V[I.get_index()]){
                V[I.get_index()] = Q[I.get_index()][_a];
            }
        }
    }
}


void init_action_UCB(InformationSet& I, std::vector<std::vector<double>>& action_UCB, int depth, int H, char game) {
    std::vector<int> legal_actions;
    I.get_actions(legal_actions);

    for (int a : legal_actions){
        action_UCB[I.get_index()][a] = (H - depth);

        std::unordered_set<std::string> cohort;
        std::unordered_map<std::string, double> cohort_values;
        get_cohort(I, a, cohort);

        for (std::string I_prime_hash : cohort){
            InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash, game);
            init_action_UCB(I_prime, action_UCB, depth + 1, H, game);
        }
    }
}


void algorithm(double eps, double delta, double c1, double c2, double c3, int S, int H, char br_player, PolicyVec& player_policy, PolicyVec& opponent_policy, std::vector<std::string>& player_information_sets, int T, int log_freq, char game, double br_value, int experiment_number){
    double iota = std::log(1.0/delta);

    std::vector<std::vector<double>> theta(player_information_sets.size(), std::vector<double>(6, 0.0));
    std::vector<std::vector<double>> kappa(player_information_sets.size(), std::vector<double>(6, 0.0));

    std::vector<int> infoset_reach_count(player_information_sets.size(), 0);
    std::vector<std::vector<int>> terminal_reach_count(player_information_sets.size(), std::vector<int>(6, 0));

    std::vector<std::vector<double>> Q(player_information_sets.size(), std::vector<double>(6, 1.0));
    std::vector<double> V(player_information_sets.size(), 0.0);

    std::vector<std::pair<int, double>> exploitability_log; 

    std::vector<std::string>& unique_draws = game == 'L' ? unique_draws_leduc : unique_draws_kuhn;
    std::vector<double>& draw_probabilities = game == 'L' ? draw_probabilities_leduc : draw_probabilities_kuhn;

    std::vector<char> cards = {'J', 'Q', 'K'};
    for (char c : cards){
        std::string root_hash = br_player == 'x' ? "a-" + std::string(1, c) + "--" : "o-" + std::string(1, c) + "--";
        InformationSet root = br_player == 'x' ? InformationSet('x', true, root_hash, game) : InformationSet('o', false, root_hash, game);
        init_action_UCB(root, Q, 0, H, game);
    }

    for (int t = 1; t <= T; t++){
        if (t % log_freq == 0){
            std::cout << "-------------- Iteration: " << t << " --------------" << std::endl;
            double expected_utility = 0.0;
            if (br_player == 'x'){
                expected_utility = get_expected_utility_wrapper(player_policy, opponent_policy, game);
                exploitability_log.push_back(std::make_pair(t, br_value - expected_utility));
            }
            else if (br_player == 'o'){
                expected_utility = get_expected_utility_wrapper(opponent_policy, player_policy, game);
                exploitability_log.push_back(std::make_pair(t, br_value - expected_utility));
            }
            std::cout << "Expected Utility: " << expected_utility << std::endl;
        }

        std::discrete_distribution<int> distribution(draw_probabilities.begin(), draw_probabilities.end());
        int draw_index = distribution(generator);
       
        std::string cards = unique_draws[draw_index];
        PokerTable true_cards = PokerTable(cards);
        true_cards.game = game;
        std::string hash_1 = "a-" + std::string(1, cards[0]) + "--";
        std::string hash_2 = "o-" + std::string(1, cards[1]) + "--";
        InformationSet I_1 = InformationSet('x', true, hash_1, game);
        InformationSet I_2 = InformationSet('o', false, hash_2, game);
        InformationSet& I = br_player == 'x' ? I_1 : I_2;

        std::vector<int> h = {};
        h.push_back(cards[0]);
        h.push_back(cards[1]);
        h.push_back(cards[2]); 
        TerminalHistory start_history = TerminalHistory(h);
        std::vector<std::pair<std::string, int>> trajectory = {};

        std::vector<int> legal_actions;
        I.get_actions(legal_actions);

        // sample game
        double reward = sample_terminal_history(I_1, I_2, I_2, 0, true_cards, player_policy, opponent_policy, start_history, trajectory, br_player, Q, V, theta, kappa, infoset_reach_count, terminal_reach_count, game);
        // update bounds
        updateBounds(theta, kappa, infoset_reach_count, terminal_reach_count, Q, V, trajectory, br_player, eps, delta, S, H, t, game, c1, c2, c3, iota);
    }

    std::cout << "Saving exploitability log" << std::endl;
    std::string file_name = "data/MVP/" + std::string(1, br_player) + "_" + std::string(1, game) + "_MVP_exploitability_log_" + std::to_string(experiment_number) + ".txt";

    std::ofstream f(file_name);
    for (int i = 0; i < exploitability_log.size(); i++) {
        f << exploitability_log[i].first << " " << exploitability_log[i].second << std::endl;
    }
    f.close();
}


int main(int argc, char* argv[]) {
    std::cout.precision(17);
    std::string file_path_1 = argv[1];
    std::string file_path_2 = argv[2];
    long int num_iterations = std::stoi(argv[3]);
    char player = argv[4][0];
    int num_experiments = std::stoi(argv[5]);
    long int log_freq = std::stoi(argv[6]);
    double eps = std::stod(argv[7]);
    double delta = std::stod(argv[8]);
    std::string base_path = argv[9];
    char game = argv[10][0];

    int experiment_number = 1;

    // constants
    int S = game == 'L'? 200 : 10;
    int H = game == 'L'? 7 : 4; // max depth of the game tree
    double c1 = 0.5;
    double c2 = 0.1;
    double c3 = 0.5;

    std::vector<std::string> P1_information_sets;
    std::vector<std::string> P2_information_sets;
    std::string P1_information_sets_file = game == 'L'? "P1_information_sets_Leduc_Poker.txt" : "P1_information_sets_Kuhn_Poker.txt";
    std::string P2_information_sets_file = game == 'L'? "P2_information_sets_Leduc_Poker.txt" : "P2_information_sets_Kuhn_Poker.txt";

    // read the P1 information sets
    std::ifstream P1_f_is(P1_information_sets_file);
    std::string P1_line_is;
    while (std::getline(P1_f_is, P1_line_is)) {
        P1_information_sets.push_back(P1_line_is);
    }
    P1_f_is.close();
    // read the P2 information sets
    std::ifstream P2_f_is(P2_information_sets_file);
    std::string P2_line_is;
    while (std::getline(P2_f_is, P2_line_is)) {
        P2_information_sets.push_back(P2_line_is);
    }
    P2_f_is.close();
    // create hash to int maps (for performance reasons)
    for (long int i = 0; i < P1_information_sets.size(); i++) {
        InformationSet::P1_hash_to_int_map[P1_information_sets[i]] = i;
    }
    for (long int i = 0; i < P2_information_sets.size(); i++) {
        InformationSet::P2_hash_to_int_map[P2_information_sets[i]] = i;
    }

    std::cout << "Loading start policies..." << std::endl;
    PolicyVec policy_obj_x('x', file_path_1, game, true);
    PolicyVec policy_obj_o('o', file_path_2, game, true);
    std::cout << "Start policies loaded." << std::endl;
    PolicyVec br_x('x', P1_information_sets, game, false);
    PolicyVec br_o('o', P2_information_sets, game, false);

    double expected_utility = 0.0;
    if (player == 'x'){
        expected_utility = compute_best_response_wrapper(policy_obj_o, br_x, 'x', game);
    }
    else if (player == 'o'){
        expected_utility = compute_best_response_wrapper(policy_obj_x, br_o, 'o', game);
    }

    std::cout << "----------- Expected Utility of Best Response: " << expected_utility << " -----------" << std::endl;

    while (experiment_number <= num_experiments){
        std::cout << "----------- Experiment Number: " << experiment_number << " -----------" << std::endl;
        if (player == 'x'){
            PolicyVec uniform_policy_obj_x('x', P1_information_sets, game, false);
            PolicyVec player_br_policy = uniform_policy_obj_x;
            algorithm(eps, delta, c1, c2, c3, S, H, player, player_br_policy, policy_obj_o, P1_information_sets, num_iterations, log_freq, game, expected_utility, experiment_number);
        }
        else if (player == 'o'){
            PolicyVec uniform_policy_obj_o('o', P2_information_sets, game, false);
            PolicyVec player_br_policy = uniform_policy_obj_o;
            algorithm(eps, delta, c1, c2, c3, S, H, player, player_br_policy, policy_obj_x, P2_information_sets, num_iterations, log_freq, game, expected_utility, experiment_number);
        }
        experiment_number += 1;
    }
}
