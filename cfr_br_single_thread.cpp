// computes the best response against a policy using cfr

// Compile: g++-13 -O3 cfr_br.cpp rbt_classes.cpp rbt_utilities.cpp -o cfr_br -fopenmp -I /Users/anvay/Downloads/boost_1_84_0

#include "cpp_headers/rbt_classes.hpp"
#include "cpp_headers/rbt_utilities.hpp"

int main(int argc, char* argv[]) {
    std::cout.precision(17);
    std::string P1_information_sets_file = "data/P1_information_sets.txt";
    std::string P2_information_sets_file = "data/P2_information_sets.txt";
    std::vector<std::string> information_sets;
    std::vector<std::vector<double>> regret_list;
    std::string line;
    Policy policy_obj_x;
    Policy policy_obj_o;
    
    if (argc != 6) {
        std::cout << "Usage: ./cfr_br <player> <x_policy_file> <o_policy_file> <iterations> <threads>" << std::endl;
        return 1;
    }

    std::string player = argv[1];
    std::string P1_policy_file = argv[2];
    std::string P2_policy_file = argv[3];
    int num_iterations = std::stoi(argv[4]);
    int num_threads = std::stoi(argv[5]);

    policy_obj_x.load_policy('x', P1_policy_file);
    policy_obj_o.load_policy('o', P2_policy_file);
    std::string policy_name;

    if (player != "x" && player != "o") {
        std::cout << "Invalid player argument. Must be 'x' or 'o'." << std::endl;
        return 1;
    }

    std::ifstream f1;
    if (player == "x") {
        f1.open(P1_information_sets_file);

        std::string::size_type pos = P1_policy_file.find_last_of("/");
        if (pos != std::string::npos) {
            policy_name = P1_policy_file.substr(pos + 1);
        }
        else {
            policy_name = P1_policy_file;
        }
    }
    else {
        f1.open(P2_information_sets_file);

        std::string::size_type pos = P2_policy_file.find_last_of("/");
        if (pos != std::string::npos) {
            policy_name = P2_policy_file.substr(pos + 1);
        }
        else {
            policy_name = P2_policy_file;
        }
    }

    while (std::getline(f1, line)) {
        information_sets.push_back(line);
        std::vector<double> regret_vector;
        for (int i = 0; i < 13; i++) {
            regret_vector.push_back(0.0);
        }
        regret_list.push_back(regret_vector);
    }
    f1.close();

    double expected_utility = get_expected_utility_wrapper(policy_obj_x, policy_obj_o);
    std::cout << "Expected utility: " << expected_utility << std::endl; 

    std::ofstream f_out_policy;
    for (int T = 1; T <= num_iterations; T++) {
        std::cout << "Starting iteration " << T << "..." << std::endl;
        auto start = std::chrono::system_clock::now();

        for (int i = 0; i < information_sets.size(); i++) {
            std::string I_hash = information_sets[i];
            bool move_flag = I_hash[I_hash.size()-1] == 'm' ? true : false;
            
            I_hash.pop_back();
            InformationSet I(player[0], move_flag, I_hash);
            
            calc_cfr_policy_given_I(I, policy_obj_x, policy_obj_o, T, regret_list[i]);
            std::cout << "Finished iteration " << i << ", information set " << I_hash << std::endl;
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

            for (int action : actions) {
                total_regret += regret_vector[action];
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
                        prob_dist[action] = 1.0 / double(actions.size());
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
                        prob_dist[action] = 1.0 / double(actions.size());
                    }
                }
            }
        }

        double expected_utility = get_expected_utility_wrapper(policy_obj_x, policy_obj_o);
        std::cout << "Expected utility: " << expected_utility << std::endl; 
    
        std::string out_policy_file = "data/best_response/iteration_" + std::to_string(T) + "_best_response_against_" + policy_name;
        f_out_policy.open(out_policy_file, std::ios::trunc);

        if (player == "x"){
            json jx;
            for (auto& it: policy_obj_x.policy_dict) {
                for (int i = 0; i < 13; i++) {
                    jx[it.first][std::to_string(i)] = it.second[i];
                }
            }
            f_out_policy << jx.dump() << std::endl;
        }
        else {
            json jo;
            for (auto& it: policy_obj_o.policy_dict) {
                for (int i = 0; i < 13; i++) {
                    jo[it.first][std::to_string(i)] = it.second[i];
                }
            }
            f_out_policy << jo.dump() << std::endl;
        }
        f_out_policy.close();
    }
}