#include "cpp_headers/poker_classes.hpp"
#include "cpp_headers/poker_utilities.hpp"
#include <random>
#include <cmath>
int NUM_THREADS = 4;

static std::random_device rd;
static std::mt19937 generator(rd());

int sampleIndex(const std::vector<double> &probabilities)
{
    std::discrete_distribution<int> distribution(probabilities.begin(), probabilities.end());
    return distribution(generator);
}


double sample_terminal_history(InformationSet &I_1, InformationSet &I_2, PokerTable &true_cards, std::vector<std::vector<double>> &infoset_ucb, PolicyVec &opponent_policy, History &current_history, char br_player, double eps, double C, double n_0, std::vector<double> &infoset_u, std::vector<std::vector<double>> &infoset_action_u, double d, char game) {
    InformationSet& I = true_cards.player_to_move == 'x' ? I_1 : I_2;
    int action = 0;
    if (I.player == br_player) {    
        double term = n_0 * (1.0/ (1.0 + d * sqrt(infoset_u[I.get_index()])));
        double eta = (term < eps) ? eps : term;
        std::vector<double> selection_prob = {eta, 1.0 - eta};
        if (sampleIndex(selection_prob) == 0){
            std::vector<double> &action_ucbs = infoset_ucb[I.get_index()];
            double max_ucb = -std::numeric_limits<double>::infinity();
            std::vector<int> legal_actions;
            I.get_actions(legal_actions);

            for (int a : legal_actions) {
                if (action_ucbs[a] >= max_ucb) {
                    max_ucb = action_ucbs[a];
                    action = a;
                }
            }
        } else {
            std::vector<int> legal_actions;
            I.get_actions(legal_actions);
            std::vector<double> prob_dist(6, 0.0);
            for (int a : legal_actions) {   
                if (infoset_u[I.get_index()] == 0) {
                    prob_dist[a] = 1.0/legal_actions.size();
                } else {
                    prob_dist[a] = infoset_action_u[I.get_index()][a]/ infoset_u[I.get_index()];
                }
            }
            action = sampleIndex(prob_dist);
        }
    } else {
        std::vector<double> prob_dist = opponent_policy.policy_dict[I.get_index()];
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
                return sample_terminal_history(new_I, I_2, true_cards, infoset_ucb, opponent_policy, current_history, br_player, eps, C, n_0, infoset_u, infoset_action_u, d, game);
            } else {
                return sample_terminal_history(I_1, new_I, true_cards, infoset_ucb, opponent_policy, current_history, br_player, eps, C, n_0, infoset_u, infoset_action_u, d, game);
            }
        } else {
            TerminalHistory H_T = TerminalHistory(current_history.history);
            H_T.set_reward(game);
            double reward = br_player == 'x' ? (double)H_T.reward[0] : (double)H_T.reward[1];
            return reward;
        }
    } else {
        InformationSet new_I = I;
        new_I.simulate_sense(action, true_cards);
        current_history.history.push_back(action);

        if (I.player == 'x') {
            return sample_terminal_history(new_I, I_2, true_cards, infoset_ucb, opponent_policy, current_history, br_player, eps, C, n_0, infoset_u, infoset_action_u, d, game);
        } else {
            return sample_terminal_history(I_1, new_I, true_cards, infoset_ucb, opponent_policy, current_history, br_player, eps, C, n_0, infoset_u, infoset_action_u, d, game);
        }
    }
}


double sample_terminal_history_wrapper(std::vector<std::vector<double>> &infoset_ucb, PolicyVec &opponent_policy, History &current_history, char br_player, double eps, double C, double n_0, std::vector<double> &infoset_u, std::vector<std::vector<double>> &infoset_action_u, double d, char game) {
    std::vector<std::string>& unique_draws = game == 'L' ? unique_draws_leduc : unique_draws_kuhn;
    std::vector<double>& draw_probabilities = game == 'L' ? draw_probabilities_leduc : draw_probabilities_kuhn;

    std::discrete_distribution<int> distribution(draw_probabilities.begin(), draw_probabilities.end());
    int draw_index = distribution(generator);

    std::string cards = unique_draws[draw_index];
    PokerTable true_cards = PokerTable(cards);
    true_cards.game = game;
    std::string hash_1 = "a-" + std::string(1, cards[0]) + "--";
    std::string hash_2 = "o-" + std::string(1, cards[1]) + "--";
    InformationSet I_1 = InformationSet('x', true, hash_1, game);
    InformationSet I_2 = InformationSet('o', false, hash_2, game);

    current_history.history.push_back(cards[0]);
    current_history.history.push_back(cards[1]);
    current_history.history.push_back(cards[2]);

    return sample_terminal_history(I_1, I_2, true_cards, infoset_ucb, opponent_policy, current_history, br_player, eps, C, n_0, infoset_u, infoset_action_u, d, game);
}


void update_ucb_new(std::vector<std::vector<double>> &infoset_ucb, std::vector<std::vector<double>> &infoset_q, std::vector<double> &infoset_u, std::vector<std::vector<double>> &infoset_action_u, double reward, TerminalHistory &history, char br_player, double C, PolicyVec& player_br_policy, char game) {
    std::string cards = "---";
    cards[0] = history.history[0];
    cards[1] = history.history[1];
    cards[2] = history.history[2];

    PokerTable true_cards = PokerTable(cards);
    true_cards.game = game;
    std::string hash_1 = "a-" + std::string(1, cards[0]) + "--";
    std::string hash_2 = "o-" + std::string(1, cards[1]) + "--";
    InformationSet I_1 = InformationSet('x', true, hash_1, game);
    InformationSet I_2 = InformationSet('o', false, hash_2, game);

    for (int i = 3; i < history.history.size(); i++) {  
        InformationSet& I = true_cards.player_to_move == 'x' ? I_1 : I_2;
        int action = history.history[i];

        if (I.player == br_player){
            infoset_u[I.get_index()] += 1;
            infoset_action_u[I.get_index()][action] += 1;

            infoset_q[I.get_index()][action] = infoset_q[I.get_index()][action] + (reward - infoset_q[I.get_index()][action]) / infoset_action_u[I.get_index()][action];
            
            std::vector<int> legal_actions;
            I.get_actions(legal_actions);

            for (int a : legal_actions) {
                if (infoset_action_u[I.get_index()][a] > 0) {
                    infoset_ucb[I.get_index()][a] = infoset_q[I.get_index()][a] + C * sqrt(log(infoset_u[I.get_index()]) / infoset_action_u[I.get_index()][a]);
                }
            }
            for (int a : legal_actions) {
                if (infoset_u[I.get_index()] > 0) {
                    player_br_policy.policy_dict[I.get_index()][a] = infoset_action_u[I.get_index()][a]/infoset_u[I.get_index()];
                }
            }
        }

        if (action < 5) {
            I.update_move(action);
            true_cards.update_move(action);
        } else {
            I.simulate_sense(action, true_cards);
        }
    }
}


void uct_best_response(PolicyVec &opponent_policy, PolicyVec &player_br_policy, char br_player, std::vector<std::string> &player_information_sets, long int T, long int d, double exact_br_value, int experiment_number, long int log_size, double eps, double C, double n_0, char game)
{
    std::vector<double> infoset_u(player_information_sets.size(), 0.0);
    std::vector<std::vector<double>> infoset_ucb(player_information_sets.size(), std::vector<double>(6, std::numeric_limits<double>::infinity()));
    std::vector<std::vector<double>> infoset_q(player_information_sets.size(), std::vector<double>(6, 0.0));
    std::vector<std::vector<double>> infoset_action_u(player_information_sets.size(), std::vector<double>(6, 0.0));
    std::vector<std::pair<int, double>> exploitability_log;

    for (long int t = 0; t <= T; t++) {
        std::vector<int> h = {};
        TerminalHistory start_history = TerminalHistory(h);
        double reward = 0.0;
        reward = sample_terminal_history_wrapper(infoset_ucb, opponent_policy, start_history, br_player, eps, C, n_0, infoset_u, infoset_action_u, d, game);
        // update ucb values
        update_ucb_new(infoset_ucb, infoset_q, infoset_u, infoset_action_u, reward, start_history, br_player, C, player_br_policy, game);

        if (t % log_size == 0 && t != 0)
        {
            double expected_utility = 0.0;
            std::cout << "############################################################" << std::endl;
            if (br_player == 'x') {
                double expected_utility = get_expected_utility_wrapper(player_br_policy, opponent_policy, game);
                std::cout << "Expected utility after " << t << " iterations: " << expected_utility << std::endl;
                exploitability_log.push_back(std::make_pair(t, exact_br_value - expected_utility));
            } else {
                double expected_utility = get_expected_utility_wrapper(opponent_policy, player_br_policy, game);
                std::cout << "Expected utility after " << t << " iterations: " << expected_utility << std::endl;
                exploitability_log.push_back(std::make_pair(t, exact_br_value - expected_utility));
            }
            std::cout << "Checking latest sampled history..." << std::endl;
            for (int i = 0; i < start_history.history.size(); i++)
            {
                std::cout << start_history.history[i] << " ";
            }
            std::cout << std::endl << "############################################################" << std::endl;
        }
    }

    std::cout << "Saving exploitability logs" << std::endl;
    std::string file_name = "data/smooth_uct/d=" + std::to_string(d) + "_" + "eps=" + std::to_string(eps) + "_" + "C=" + std::to_string(C) + "_" + "n0=" + std::to_string(n_0) + std::string(1, br_player) + "_" + std::string(1, game) + "poker_uct_smooth_exploitability_log_" + std::to_string(experiment_number) + ".txt";
    std::ofstream f(file_name);
    for (int i = 0; i < exploitability_log.size(); i++)
    {
        f << exploitability_log[i].first << " " << exploitability_log[i].second << std::endl;
    }
    f.close();
}


int main(int argc, char *argv[])
{
    std::cout.precision(17);
    std::string file_path_1 = argv[1];
    std::string file_path_2 = argv[2];
    char game = argv[3][0];
    int num_iterations = std::stoi(argv[4]);
    int log_size = std::stoi(argv[5]);
    int num_experiments = std::stoi(argv[6]);
    char player = argv[7][0];
    double C = std::stod(argv[8]);
    double n_0 = std::stod(argv[9]);
    double d = std::stod(argv[10]);
    double eps = std::stod(argv[11]);
    int start_index = std::stoi(argv[12]);

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
    PolicyVec br_x('x', P1_information_sets, game);
    PolicyVec br_o('o', P2_information_sets, game);

    char continue_exp = 'y';
    while (continue_exp == 'y')
    {
        int experiment_number = start_index;
        double expected_utility = 0.0;
        if (player == 'x')
        {
            expected_utility = compute_best_response_wrapper(policy_obj_o, br_x, 'x', game);
            std::cout << "Expected utility of best response: " << expected_utility << std::endl;
        }
        else if (player == 'o')
        {
            expected_utility = compute_best_response_wrapper(policy_obj_x, br_o, 'o', game);
            std::cout << "Expected utility of best response: " << expected_utility << std::endl;
        }

        while (experiment_number < num_experiments + start_index)
        {
            if (player == 'x')
            {
                PolicyVec uniform_x('x', P1_information_sets, game);
                uct_best_response(policy_obj_o, uniform_x, 'x', P1_information_sets, num_iterations, d, expected_utility, experiment_number, log_size, eps, C, n_0, game);
            }
            else if (player == 'o')
            {
                PolicyVec uniform_o('o', P2_information_sets, game);
                uct_best_response(policy_obj_x, uniform_o, 'o', P2_information_sets, num_iterations, d, expected_utility, experiment_number, log_size, eps, C, n_0, game);
            }
            experiment_number += 1;
        }

        std::cout << "Continue experiments? (y/n): ";
        std::cin >> continue_exp;
    }
}
