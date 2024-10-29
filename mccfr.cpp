#include "cpp_headers/rbt_classes.hpp"
#include "cpp_headers/rbt_utilities.hpp"
#include <random>


int sampleIndex(const std::vector<double>& probabilities) {
    std::random_device rd;
    std::mt19937 generator(rd());
    std::discrete_distribution<int> distribution(probabilities.begin(), probabilities.end());
    return distribution(generator);
}


double sample_terminal_history(InformationSet& I_1, InformationSet& I_2, TicTacToeBoard& true_board, PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, History& current_history, char player, double probability, double& reward, char update_player) {
    InformationSet& I = player == 'x' ? I_1 : I_2;
    PolicyVec& policy_obj = player == 'x' ? policy_obj_x : policy_obj_o;
    std::vector<double> prob_dist = policy_obj.policy_dict[I.get_index()];

    if (player == update_player) { // explore with a small epsilon
        std::vector<int> actions;
        I.get_actions(actions);
        double eps = 0.01;
        double sum = 1.0;

        for (int i = 0; i < actions.size(); i++) {
            if (prob_dist[actions[i]] == 0.0) {
                prob_dist[actions[i]] = eps;
                sum += eps;
            }
        }
        // renormalize
        for (int i = 0; i < actions.size(); i++) {
            prob_dist[actions[i]] /= sum;
        }
    }

    int action = sampleIndex(prob_dist);

    if (I.move_flag) {
        bool success = true_board.update_move(action, player);

        double probability_new = probability * prob_dist[action];
        current_history.history.push_back(action);

        char winner;
        if (success && !true_board.is_win(winner) && !true_board.is_over()) {
            InformationSet new_I = I;
            new_I.update_move(action, player);
            new_I.reset_zeros();

            if (player == 'x') {
                return sample_terminal_history(new_I, I_2, true_board, policy_obj_x, policy_obj_o, current_history, 'o', probability_new, reward, update_player);
            } else {
                return sample_terminal_history(I_1, new_I, true_board, policy_obj_x, policy_obj_o, current_history, 'x', probability_new, reward, update_player);
            }
        } else {
            TerminalHistory H_T = TerminalHistory(current_history.history);
            H_T.set_reward();
            reward = (double) H_T.reward[0];
            return probability_new;
        }
    }
    else {
        InformationSet new_I = I;
        new_I.simulate_sense(action, true_board);

        double probability_new = probability * prob_dist[action];
        current_history.history.push_back(action);

        if (player == 'x') {
            return sample_terminal_history(new_I, I_2, true_board, policy_obj_x, policy_obj_o, current_history, 'x', probability_new, reward, update_player);
        } else {
            return sample_terminal_history(I_1, new_I, true_board, policy_obj_x, policy_obj_o, current_history, 'o', probability_new, reward, update_player);
        }
    }
}


double sample_terminal_history_wrapper(PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, History& current_history, double& reward, char update_player) {
    std::string board = "000000000";
    TicTacToeBoard true_board = TicTacToeBoard(board);
    std::string hash_1 = "";
    std::string hash_2 = "";
    InformationSet I_1 = InformationSet('x', true, hash_1);
    InformationSet I_2 = InformationSet('o', false, hash_2);
    return sample_terminal_history(I_1, I_2, true_board, policy_obj_x, policy_obj_o, current_history, 'x', 1.0, reward, update_player);
}


void mccfr_outcome_sampling_best_response(PolicyVec& policy_obj, PolicyVec& best_response, char br_player, int T, std::vector<std::string>& information_sets) {
    std::vector<std::vector<double>> regret_list;
    for (long int i = 0; i < information_sets.size(); i++) {
        regret_list.push_back(std::vector<double>(13, 0.0));
    }

    for (int t = 0; t < T; t++) {
        std::vector<int> h = {};
        TerminalHistory start_history = TerminalHistory(h);
        double q_z = 0.0;
        double reward = 0;

        if (br_player == 'x') {
            q_z = sample_terminal_history_wrapper(best_response, policy_obj, start_history, reward, br_player);
        } else {
            q_z = sample_terminal_history_wrapper(policy_obj, best_response, start_history, reward, br_player);
        }

        // traverse history and update regrets
        std::string board = "000000000";
        TicTacToeBoard true_board = TicTacToeBoard(board);
        std::string hash_1 = "";
        std::string hash_2 = "";
        InformationSet I_1 = InformationSet('x', true, hash_1);
        InformationSet I_2 = InformationSet('o', false, hash_2);


        // TO - DO
        
    }
}


int main(int argc, char* argv[]) {
    std::cout.precision(17);
    std::string file_path_1 = argv[1];
    std::string file_path_2 = argv[2];
    int num_iterations = std::stoi(argv[3]);

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

    for (long int i = 0; i < P1_information_sets.size(); i++) {
        InformationSet::P1_hash_to_int_map[P1_information_sets[i]] = i;
    }
    for (long int i = 0; i < P2_information_sets.size(); i++) {
        InformationSet::P2_hash_to_int_map[P2_information_sets[i]] = i;
    }

    std::cout << "Loading policies..." << std::endl;

    PolicyVec policy_obj_x('x', file_path_1);
    PolicyVec policy_obj_o('o', file_path_2);

    std::cout << "Policies loaded." << std::endl;
}