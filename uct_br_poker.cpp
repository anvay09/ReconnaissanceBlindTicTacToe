#include "cpp_headers/poker_classes.hpp"
#include "cpp_headers/poker_utilities.hpp"
#include <random>
#include <cmath>
int NUM_THREADS = 4;

int sampleIndex(const std::vector<double> &probabilities)
{
    std::random_device rd;
    std::mt19937 generator(rd());
    std::discrete_distribution<int> distribution(probabilities.begin(), probabilities.end());
    return distribution(generator);
}


double sample_terminal_history(InformationSet &I_1, InformationSet &I_2, PokerTable &true_cards, std::vector<std::vector<double>> &infoset_ucb, PolicyVec &opponent_policy, History &current_history, char br_player, char game)
{
    InformationSet I = true_cards.player_to_move == 'x' ? I_1 : I_2;
    int action = 0;
    if (I.player == br_player) {   // choose action with max UCB value
        std::vector<double> &action_ucbs = infoset_ucb[I.get_index()];

        double max_ucb = -std::numeric_limits<double>::infinity();
        std::vector<int> legal_actions;
        I.get_actions(legal_actions);
        for (int a : legal_actions)
        {
            if (action_ucbs[a] >= max_ucb)
            {
                max_ucb = action_ucbs[a];
                action = a;
            }
        }
    }
    else {
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
                return sample_terminal_history(new_I, I_2, true_cards, infoset_ucb, opponent_policy, current_history, br_player, game);
            } else {
                return sample_terminal_history(I_1, new_I, true_cards, infoset_ucb, opponent_policy, current_history, br_player, game);
            }
        }
        else {
            TerminalHistory H_T = TerminalHistory(current_history.history);
            H_T.set_reward(game);
            double reward = br_player == 'x' ? (double)H_T.reward[0] : (double)H_T.reward[1];
            return reward;
        }
    }
    else {
        InformationSet new_I = I;
        new_I.simulate_sense(action, true_cards);
        current_history.history.push_back(action);

        if (I.player == 'x') {
            return sample_terminal_history(new_I, I_2, true_cards, infoset_ucb, opponent_policy, current_history, br_player, game);
        }
        else {
            return sample_terminal_history(I_1, new_I, true_cards, infoset_ucb, opponent_policy, current_history, br_player, game);
        }
    }
}


double sample_terminal_history_wrapper(std::vector<std::vector<double>> &infoset_ucb, PolicyVec &opponent_policy, History &current_history, char br_player, char game)
{
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

    current_history.history.push_back(cards[0]);
    current_history.history.push_back(cards[1]);
    current_history.history.push_back(cards[2]);

    return sample_terminal_history(I_1, I_2, true_cards, infoset_ucb, opponent_policy, current_history, br_player, game);
}


void build_policy(std::vector<std::vector<double>> &ucb_values, PolicyVec &policy_obj, std::vector<std::string> &information_sets, char game) {
    #pragma omp parallel for num_threads(NUM_THREADS)
    for (long int i = 0; i < ucb_values.size(); i++) {
        std::vector<double> &action_ucbs = ucb_values[i];
        double max_reward = game == 'L'? LEDUC_MIN_UTILITY : KUHN_MIN_UTILITY;
        
        std::string I_hash = information_sets[i];
        InformationSet I(policy_obj.player, get_move_flag(I_hash, policy_obj.player), I_hash, game);
        
        std::vector<int> legal_actions;
        I.get_actions(legal_actions);
        int action = 0;

        for (int a : legal_actions) {
            if (action_ucbs[a] >= max_reward) {
                max_reward = action_ucbs[a];
                action = a;
            }
        }

        std::vector<double> best_arms(6, 0.0);
        best_arms[action] = 1.0;
        policy_obj.policy_dict[i] = best_arms;
    }
}


void update_ucb(std::vector<std::vector<double>> &infoset_ucb, std::vector<std::vector<double>> &infoset_q, std::vector<double> &infoset_u, std::vector<std::vector<double>> &infoset_action_u, double reward, TerminalHistory &history, char br_player, long int C, char game) {
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

    double total_reward = 0.0;
    long int total_pull = 0;
    
    for (int a = 3; a < history.history.size(); a++) {
        InformationSet I = true_cards.player_to_move == 'x' ? I_1 : I_2;
        int action = history.history[a];

        if (I.player == br_player) {
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
        }

        if (action < 5) {
            I.update_move(action);
            true_cards.update_move(action);
        } else {
            I.simulate_sense(action, true_cards);
        }
    }
}


void update_ucb_new(std::vector<std::vector<double>> &infoset_ucb, std::vector<std::vector<double>> &infoset_q, std::vector<double> &infoset_u, std::vector<std::vector<double>> &infoset_action_u, double reward, TerminalHistory &history, char br_player, long int C, PolicyVec& player_br_policy, char game) {
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
        InformationSet I = true_cards.player_to_move == 'x' ? I_1 : I_2;
        int action = history.history[i];

        if (I.player == br_player) {
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

            double max_reward = -std::numeric_limits<double>::infinity();
            int action_new = 0;

            for (int a : legal_actions) {
                if (infoset_ucb[I.get_index()][a] >= max_reward) {
                    max_reward = infoset_ucb[I.get_index()][a];
                    action_new = a;
                }
            }

            std::vector<double> best_arms(6, 0.0);
            best_arms[action_new] = 1.0;
            player_br_policy.policy_dict[I.get_index()] = best_arms;
        }

        if (action < 5) {
            I.update_move(action);
            true_cards.update_move(action);
        } else {
            I.simulate_sense(action, true_cards);
        }
    }
}


void uct_best_response(PolicyVec &opponent_policy, PolicyVec &player_br_policy, char br_player, std::vector<std::string> &player_information_sets, long int T, double exact_br_value, int experiment_number, long int log_size, long int C, char game)
{
    std::vector<double> infoset_u(player_information_sets.size(), 0.0);
    std::vector<std::vector<double>> infoset_ucb(player_information_sets.size(), std::vector<double>(6, std::numeric_limits<double>::infinity()));
    std::vector<std::vector<double>> infoset_q(player_information_sets.size(), std::vector<double>(6, 0.0));
    std::vector<std::vector<double>> infoset_action_u(player_information_sets.size(), std::vector<double>(6, 0.0));
    std::vector<std::pair<int, double>> exploitability_log;

    for (long int t = 0; t <= T; t++)
    {
        // sample terminal history
        std::vector<int> h = {};
        TerminalHistory start_history = TerminalHistory(h);
        double reward = 0.0;
        reward = sample_terminal_history_wrapper(infoset_ucb, opponent_policy, start_history, br_player, game);
        // update ucb values
        update_ucb_new(infoset_ucb, infoset_q, infoset_u, infoset_action_u, reward, start_history, br_player, C, player_br_policy, game);

        if (t % log_size == 0 && t != 0) {
            double expected_utility = 0.0;
            std::cout << "############################################################" << std::endl;
            if (br_player == 'x') {
                double expected_utility = get_expected_utility_wrapper(player_br_policy, opponent_policy, game);
                std::cout << "Expected utility after " << t << " iterations: " << expected_utility << std::endl;
                exploitability_log.push_back(std::make_pair(t, exact_br_value - expected_utility));
            }
            else
            {
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
    std::string file_name = "data/C=" + std::to_string(C) + "_" + std::string(1, br_player) + "_" + std::string(1, game) + "poker_uct_exploitability_log_" + std::to_string(experiment_number) + ".txt";
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
        long int num_iterations = 0;
        char player;
        int experiment_number = 1;
        int num_experiments = 0;
        long int log_size = 1;
        long int C = 1;

        std::cout << "Enter number of iterations: ";
        std::cin >> num_iterations;
        std::cout << "Enter the number of iterations after which progress is to be checked: ";
        std::cin >> log_size;
        std::cout << "Enter the player for whom the best response is to be computed (x/o):";
        std::cin >> player;
        std::cout << "Enter number of experiments: ";
        std::cin >> num_experiments;
        std::cout << "Enter C value: ";
        std::cin >> C;

        double expected_utility = 0.0;
        if (player == 'x')
        {
            expected_utility = compute_best_response_wrapper(policy_obj_o, br_x, 'x', game);
        }
        else if (player == 'o')
        {
            expected_utility = compute_best_response_wrapper(policy_obj_x, br_o, 'o', game);
        }

        while (experiment_number <= num_experiments)
        {
            if (player == 'x')
            {
                PolicyVec uniform_x('x', P1_information_sets, game);
                uct_best_response(policy_obj_o, uniform_x, 'x', P1_information_sets, num_iterations, expected_utility, experiment_number, log_size, C, game);
            }
            else if (player == 'o')
            {
                PolicyVec uniform_o('o', P2_information_sets, game);
                uct_best_response(policy_obj_x, uniform_o, 'o', P2_information_sets, num_iterations, expected_utility, experiment_number, log_size, C, game);
            }
            experiment_number += 1;
        }

        std::cout << "Continue experiments? (y/n): ";
        std::cin >> continue_exp;
    }
}
