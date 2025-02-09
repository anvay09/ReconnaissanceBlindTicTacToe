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


void WALK_TREES(InformationSet& I_1, InformationSet& I_2, InformationSet prev_opponent_I, int prev_opponent_action, PokerTable& true_cards, PolicyVec& opponent_policy, 
                History& current_history, char br_player, std::vector<std::vector<std::vector<double>>>& R, std::vector<double>& infoset_reach_probability, 
                std::vector<std::vector<double>>& terminal_reach_probability, char game, double opp_reach) {
    InformationSet& I = true_cards.player_to_move == 'x' ? I_1 : I_2;
    std::vector<int> legal_actions;

    if (I.player == br_player) { 
        I.get_actions(legal_actions);
        infoset_reach_probability[I.get_index()] += opp_reach; 
        // std::cout << "------ Infoset: " << I.get_hash() << " ------ Reach: " << infoset_reach_probability[I.get_index()] << std::endl;
    }
    else {
        I.get_actions_given_policy(legal_actions, opponent_policy);
    }

    if (I.move_flag) {
        for (int action : legal_actions){
            PokerTable new_cards = true_cards;
            History new_history = current_history;

            bool success = new_cards.update_move(action);
            new_history.history.push_back(action);

            double probability = opp_reach;
            if (I.player != br_player){ 
                probability *= opponent_policy.policy_dict[I.get_index()][action];
            }

            char winner;
            if (success && !new_cards.is_win(winner) && !new_cards.is_over()) {
                InformationSet new_I = I;
                new_I.update_move(action);

                if (I.player == 'x') {
                    if (new_cards.player_to_move == 'x'){
                        WALK_TREES(new_I, I_2, prev_opponent_I, prev_opponent_action, new_cards, opponent_policy, new_history, br_player, R, infoset_reach_probability, terminal_reach_probability, game, probability);
                    }
                    else {
                        WALK_TREES(new_I, I_2, I, action, new_cards, opponent_policy, new_history, br_player, R, infoset_reach_probability, terminal_reach_probability, game, probability);
                    }
                } else {
                    if (new_cards.player_to_move == 'o'){
                        WALK_TREES(I_1, new_I, prev_opponent_I, prev_opponent_action, new_cards, opponent_policy, new_history, br_player, R, infoset_reach_probability, terminal_reach_probability, game, probability);
                    }
                    else {
                        WALK_TREES(I_1, new_I, I, action, new_cards, opponent_policy, new_history, br_player, R, infoset_reach_probability, terminal_reach_probability, game, probability);
                    }
                }
            } else {
                TerminalHistory H_T = TerminalHistory(new_history.history);
                H_T.set_reward(game);
                double reward = br_player == 'x' ? H_T.reward[0] : H_T.reward[1];
                double MAX_UTIL = game == 'L' ? LEDUC_MAX_UTILITY : KUHN_MAX_UTILITY;
    
                if (I.player == br_player){
                    terminal_reach_probability[I.get_index()][action] += probability;  
                    R[I.get_index()][action][reward + MAX_UTIL] += probability;
                }
                else {
                    terminal_reach_probability[prev_opponent_I.get_index()][prev_opponent_action] += probability;
                    R[prev_opponent_I.get_index()][prev_opponent_action][reward + MAX_UTIL] += probability;
                }
            }
        }
    }
    else {
        for (int action : legal_actions){
            InformationSet new_I = I;
            new_I.simulate_sense(action, true_cards);

            History new_history = current_history;
            new_history.history.push_back(action);

            double probability = opp_reach;
            if (I.player != br_player){ 
                probability *= opponent_policy.policy_dict[I.get_index()][action];
            }
    
            if (I.player == 'x') {
                WALK_TREES(new_I, I_2, prev_opponent_I, prev_opponent_action, true_cards, opponent_policy, new_history, br_player, R, infoset_reach_probability, terminal_reach_probability, game, probability);
            } else {
                WALK_TREES(I_1, new_I, prev_opponent_I, prev_opponent_action, true_cards, opponent_policy, new_history, br_player, R, infoset_reach_probability, terminal_reach_probability, game, probability);
            }
        }
    }
}


void init_T_and_R(std::vector<std::vector<std::vector<double>>>& T, std::vector<std::vector<std::vector<double>>>& R, InformationSet& I, 
                  std::vector<double>& infoset_reach_probability, std::vector<std::vector<double>>& terminal_reach_probability,
                  std::vector<std::string>& information_sets, char game) {
    std::vector<int> legal_actions;
    I.get_actions(legal_actions);

    double parent_reach = infoset_reach_probability[I.get_index()];
    if (parent_reach == 0.0) {
        return;
    }

    for (int a : legal_actions){
        double sum = 0.0;
        std::unordered_set<std::string> cohort;
        std::unordered_map<std::string, double> cohort_values;
        get_cohort(I, a, cohort);

        for (std::string I_prime_hash : cohort){
            InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash, game);
            double child_reach = infoset_reach_probability[I_prime.get_index()] / parent_reach;
            T[I.get_index()][a][I_prime.get_index()] = child_reach;
            sum += child_reach;

            init_T_and_R(T, R, I_prime, infoset_reach_probability, terminal_reach_probability, information_sets, game);
        }

        double terminal_reach = terminal_reach_probability[I.get_index()][a] / parent_reach;
        T[I.get_index()][a][information_sets.size() - 1] = terminal_reach;
        sum += terminal_reach;

        // Normalize the transition probabilities (because of floating point errors)
        for (std::string I_prime_hash : cohort){
            InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash, game);
            T[I.get_index()][a][I_prime.get_index()] /= sum;
        }
        T[I.get_index()][a][information_sets.size() - 1] /= sum;


        double MAX_UTIL = game == 'L' ? LEDUC_MAX_UTILITY : KUHN_MAX_UTILITY;
        double r_sum = 0.0;

        for (int i = 0; i < 2 * MAX_UTIL + 1; i++){
            if (R[I.get_index()][a][i] != 0.0){
                R[I.get_index()][a][i] /= terminal_reach_probability[I.get_index()][a];
                r_sum += R[I.get_index()][a][i];
            }
        }

        // Normalize the reward probabilities (because of floating point errors)

        for (int i = 0; i < 2 * MAX_UTIL + 1; i++){
            if (R[I.get_index()][a][i] != 0.0){
                R[I.get_index()][a][i] /= r_sum;
            }
        }
    }
}


int main(int argc, char* argv[]) {
    std::cout.precision(17);
    std::string file_path_1 = argv[1];
    std::string file_path_2 = argv[2];
    char player = argv[3][0];
    char game = argv[4][0];

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
    std::vector<std::string>& information_sets = player == 'x' ? P1_information_sets : P2_information_sets;
    PolicyVec& opp_policy_obj = player == 'x' ? policy_obj_o : policy_obj_x;

    // Reward Function R(s, a) -> distribution over [-MAX_UTIL, MAX_UTIL], size of range = 2 * MAX_UTIL + 1
    double MAX_UTIL = game == 'L' ? LEDUC_MAX_UTILITY : KUHN_MAX_UTILITY;
    std::vector<std::vector<std::vector<double>>> R(information_sets.size(), std::vector<std::vector<double>>(6, std::vector<double>(2 * MAX_UTIL + 1, 0.0)));
    std::vector<double> infoset_reach_probability(information_sets.size(), 0.0);
    std::vector<std::vector<double>> terminal_reach_probability(information_sets.size(), std::vector<double>(6, 0.0));

    std::vector<std::string> &unique_draws = game == 'L' ? unique_draws_leduc : unique_draws_kuhn;
    std::vector<double> &draw_probabilities = game == 'L' ? draw_probabilities_leduc : draw_probabilities_kuhn;

    for (int i = 0; i < unique_draws.size(); i++){
        PokerTable true_cards = PokerTable(unique_draws[i]);
        true_cards.game = game;
        std::string hash_1 = "a-" + std::string(1, true_cards.cards[0]) + "--";
        std::string hash_2 = "o-" + std::string(1, true_cards.cards[1]) + "--";
        InformationSet I_1('x', true, hash_1, game);
        InformationSet I_2('o', false, hash_2, game);
        std::vector<int> h = {};
        h.push_back(true_cards.cards[0]);
        h.push_back(true_cards.cards[1]);
        h.push_back(true_cards.cards[2]);
        TerminalHistory start_history = TerminalHistory(h);

        WALK_TREES(I_1, I_2, I_2, -1, true_cards, opp_policy_obj, start_history, player, R, infoset_reach_probability, terminal_reach_probability, game, draw_probabilities[i]);
    }

    std::cout << "Finished walking trees." << std::endl;

    // Add an extra infoset for the terminal state
    information_sets.push_back("-");
    // Transition Function T(s, a, s')
    std::vector<std::vector<std::vector<double>>> T(information_sets.size(), std::vector<std::vector<double>>(6, std::vector<double>(information_sets.size(), 0.0)));
    
    std::vector<char> cards = {'J', 'Q', 'K'};
    for (char c : cards){
        std::string root_hash = player == 'x' ? "a-" + std::string(1, c) + "--" : "o-" + std::string(1, c) + "--";
        InformationSet root = player == 'x' ? InformationSet('x', true, root_hash, game) : InformationSet('o', false, root_hash, game);
        init_T_and_R(T, R, root, infoset_reach_probability, terminal_reach_probability, information_sets, game);
    }

    std::cout << "Finished initializing transition and reward functions." << std::endl;

    // save T to file
    std::ofstream T_file;
    std::string T_file_name = player == 'x' ? "Leduc_Poker_T_x.txt" : "Leduc_Poker_T_o.txt";
    T_file.open(T_file_name);
    for (int i = 0; i < information_sets.size(); i++){
        for (int a = 0; a < 6; a++){
            for (int j = 0; j < information_sets.size(); j++){
                if (T[i][a][j] != 0.0){
                    // T_file << information_sets[i] << " " << a << " " << information_sets[j] << " " << T[i][a][j] << std::endl;
                    T_file << i << " " << a << " " << j << " " << T[i][a][j] << std::endl;
                }
            }
        }
    }

    T_file.close();
    std::cout << "Finished saving transition function." << std::endl;
    information_sets.pop_back();

    // save R to file

    std::ofstream R_file;
    std::string R_file_name = player == 'x' ? "Leduc_Poker_R_x.txt" : "Leduc_Poker_R_o.txt";
    R_file.open(R_file_name);
    for (int i = 0; i < information_sets.size(); i++){
        for (int a = 0; a < 6; a++){
            for (int j = 0; j < 2 * MAX_UTIL + 1; j++){
                if (R[i][a][j] != 0.0){
                    // R_file << information_sets[i] << " " << a << " " << j - MAX_UTIL << " " << R[i][a][j] << std::endl;
                    R_file << i << " " << a << " " << j - MAX_UTIL << " " << R[i][a][j] << std::endl;
                }
            }
        }
    }

    R_file.close();
    std::cout << "Finished saving reward function." << std::endl;
}