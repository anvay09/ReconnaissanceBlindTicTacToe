#include "cpp_headers/rbt_classes.hpp"
#include <random>


int sampleIndex(const std::vector<double>& probabilities) {
    std::random_device rd;
    std::mt19937 generator(rd());
    std::discrete_distribution<int> distribution(probabilities.begin(), probabilities.end());
    return distribution(generator);
}


double sample_terminal_history(InformationSet& I_1, InformationSet& I_2, TicTacToeBoard& true_board, PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, History& current_history, char player, double probability) {
    InformationSet& I = player == 'x' ? I_1 : I_2;
    PolicyVec& policy_obj = player == 'x' ? policy_obj_x : policy_obj_o;

    std::vector<double> prob_dist = policy_obj.policy_dict[I.get_index()];
    int action = sampleIndex(prob_dist);

    if (I.move_flag) {
        bool success = true_board.update_move(action, player);

        double probability_new = probability * policy_obj.policy_dict[I.get_index()][action];
        current_history.history.push_back(action);

        char winner;
        if (success && !true_board.is_win(winner) && !true_board.is_over()) {
            InformationSet new_I = I;
            new_I.update_move(action, player);
            new_I.reset_zeros();

            if (player == 'x') {
                return sample_terminal_history(new_I, I_2, true_board, policy_obj_x, policy_obj_o, current_history, 'o', probability_new);
            } else {
                return sample_terminal_history(I_1, new_I, true_board, policy_obj_x, policy_obj_o, current_history, 'x', probability_new);
            }
        } else {
            TerminalHistory H_T = TerminalHistory(current_history.history);
            return probability_new;
        }
    }
    else {
        InformationSet new_I = I;
        new_I.simulate_sense(action, true_board);

        double probability_new = probability * policy_obj.policy_dict[I.get_index()][action];
        current_history.history.push_back(action);

        if (player == 'x') {
            return sample_terminal_history(new_I, I_2, true_board, policy_obj_x, policy_obj_o, current_history, 'x', probability_new);
        } else {
            return sample_terminal_history(I_1, new_I, true_board, policy_obj_x, policy_obj_o, current_history, 'o', probability_new);
        }
    }
}



int main(int argc, char* argv[]) {
    std::cout.precision(17);
    std::string file_path_1 = argv[1];
    std::string file_path_2 = argv[2];

    // int num_iterations = std::stoi(argv[4]);

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

    for (int i = 0; i < 10; i++) {
        std::string board = "000000000";
        TicTacToeBoard true_board = TicTacToeBoard(board);
        std::string hash_1 = "";
        std::string hash_2 = "";
        InformationSet I_1 = InformationSet('x', true, hash_1);
        InformationSet I_2 = InformationSet('o', false, hash_2);
        std::vector<int> h = {};
        TerminalHistory start_history = TerminalHistory(h);
        double q_z = sample_terminal_history(I_1, I_2, true_board, policy_obj_x, policy_obj_o, start_history, 'x', 1.0);

        std::cout << "Sampled terminal history with probability " << q_z << " : ";
        start_history.print_history();
    }

}