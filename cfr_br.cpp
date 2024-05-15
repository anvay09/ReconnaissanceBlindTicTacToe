// computes the best response against a policy using cfr

// Compile: g++-13 -O3 cfr_br.cpp rbt_classes.cpp rbt_utilities.cpp -o cfr_br -fopenmp -I /Users/anvay/Downloads/boost_1_84_0

#include "cpp_headers/rbt_classes.hpp"
#include "cpp_headers/rbt_utilities.hpp"

int main(int argc, char* argv[]) {
    std::string P1_information_sets_file = "data/P1_information_sets.txt";
    std::string P2_information_sets_file = "data/P2_information_sets.txt";
    std::string P1_policy_file = "data/Iterative_1/average/P1_average_overall_policy_after_100_rounds_normalised.json";
    std::string P2_policy_file = "data/Iterative_1/average/P2_average_overall_policy_after_100_rounds_normalised.json";
    Policy policy_obj_x;
    Policy policy_obj_o;
    
    if (argc != 4) {
        std::cout << "Usage: ./cfr_br <player> <other_player_policy_file> <iterations>" << std::endl;
        return 1;
    }

    std::string player = argv[1];
    std::string policy_file = argv[2];
    int num_iterations = std::stoi(argv[3]);

    std::string policy_name;
    std::string::size_type pos = policy_file.find_last_of("/");
    if (pos != std::string::npos) {
        policy_name = policy_file.substr(pos + 1);
    }
    else {
        policy_name = policy_file;
    }

    if (player != "x" && player != "o") {
        std::cout << "Invalid player argument. Must be 'x' or 'o'." << std::endl;
        return 1;
    }

    std::ifstream f1;
    if (player == "o") {
        policy_obj_x.load_policy('x', policy_file);
        policy_obj_o.load_policy('o', P2_policy_file);
        f1.open(P2_information_sets_file);
    }
    else {
        policy_obj_x.load_policy('x', P1_policy_file);
        policy_obj_o.load_policy('o', policy_file);
        f1.open(P1_information_sets_file);
    }

    // calculate expected utility
    std::string board = "000000000";
    TicTacToeBoard true_board = TicTacToeBoard(board);
    std::string board_1 = "000000000";
    std::string board_2 = "---------";
    InformationSet I_1 = InformationSet('x', true, board_1);
    InformationSet I_2 = InformationSet('o', false, board_2);
    std::vector<int> h = {};
    TerminalHistory start_history = TerminalHistory(h);

    double expected_utility = get_expected_utility_parallel(I_1, I_2, true_board, 'x', policy_obj_x, policy_obj_o, 1, start_history, 'x');
    std::cout << "Expected utility: " << expected_utility << std::endl; 

    std::vector<std::string> information_sets;
    std::vector<std::vector<double>> regret_list;
    std::string line;

    while (std::getline(f1, line)) {
        information_sets.push_back(line);
        std::vector<double> regret_vector;
        for (int i = 0; i < 13; i++) {
            regret_vector.push_back(0.0);
        }
        regret_list.push_back(regret_vector);
    }
    f1.close();

    std::ofstream f_out_policy;
    for (int T = 1; T <= num_iterations; T++) {
        std::cout << "Starting iteration " << T << "..." << std::endl;
        auto start = std::chrono::system_clock::now();

        #pragma omp parallel for num_threads(96)
        for (int i = 0; i < information_sets.size(); i++) {
            std::string I_hash = information_sets[i];
            bool move_flag = I_hash[I_hash.size()-1] == 'm' ? true : false;
            I_hash.pop_back();
            InformationSet I(player[0], move_flag, I_hash);
            
            calc_cfr_policy_given_I(I, policy_obj_x, policy_obj_o, T, regret_list[i]);
        }

        auto end = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = end - start;
        std::time_t end_time = std::chrono::system_clock::to_time_t(end);
    
        std::cout << "finished computation at " << std::ctime(&end_time)
                << "elapsed time: " << elapsed_seconds.count() << "s"
                << std::endl;

        
        std::cout << "Updating policy..." << std::endl;

        for (int i = 0; i < information_sets.size(); i++) {
            std::string I_hash = information_sets[i];
            bool move_flag = I_hash[I_hash.size()-1] == 'm' ? true : false;
            I_hash.pop_back();
            InformationSet I(player[0], move_flag, I_hash);
            I_hash += move_flag ? "m" : "s";
            std::vector<double>& regret_vector = regret_list[i];
            double total_regret = 0.0;
            std::vector<int> actions;
            I.get_actions(actions);

            for (int j = 0; j < actions.size(); j++) {
                total_regret += regret_vector[j];
            }
            
            if (player == "x") {
                std::vector<double>& prob_dist = policy_obj_x.policy_dict[I_hash];
                if (total_regret > 0) {
                    for (int action : actions) {
                        prob_dist[action] = regret_vector[action] / total_regret;
                    }
                }
                else {
                    for (int action : actions) {
                        prob_dist[action] = 1.0 / actions.size();
                    }
                }
            }
            else {
                std::vector<double>& prob_dist = policy_obj_o.policy_dict[I_hash];
                if (total_regret > 0) {
                    for (int action : actions) {
                        prob_dist[action] = regret_vector[action] / total_regret;
                    }
                }
                else {
                    for (int action : actions) {
                        prob_dist[action] = 1.0 / actions.size();
                    }
                }
            }
        }

        // calculate expected utility
        std::string board = "000000000";
        TicTacToeBoard true_board = TicTacToeBoard(board);
        std::string board_1 = "000000000";
        std::string board_2 = "---------";
        InformationSet I_1 = InformationSet('x', true, board_1);
        InformationSet I_2 = InformationSet('o', false, board_2);
        std::vector<int> h = {};
        TerminalHistory start_history = TerminalHistory(h);

        double expected_utility = get_expected_utility_parallel(I_1, I_2, true_board, 'x', policy_obj_x, policy_obj_o, 1, start_history, 'x');
        std::cout << "Expected utility: " << expected_utility << std::endl; 
    }

    std::string out_policy_file = "data/best_response/best_response_against_" + policy_name;
    f_out_policy.open(out_policy_file, std::ios::trunc);

    json jx;
    for (auto& it: policy_obj_x.policy_dict) {
        for (int i = 0; i < 13; i++) {
            jx[it.first][std::to_string(i)] = it.second[i];
        }
    }
    f_out_policy << jx.dump() << std::endl;
    f_out_policy.close();
}