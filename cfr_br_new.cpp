// computes the best response against a policy using cfr

// Compile: g++-13 -O3 cfr_br_new.cpp rbt_classes.cpp rbt_utilities.cpp -o cfr_br_new -fopenmp

#include "cpp_headers/rbt_classes.hpp"
#include "cpp_headers/rbt_utilities.hpp"

bool get_move_flag(std::string I_hash, char player){
    bool move_flag;
    if (I_hash.size() != 0){
        move_flag = I_hash[I_hash.size()-1] == '|' ? true : false;
    }
    else {
        move_flag = player == 'x' ? true : false;
    }
    return move_flag;
}

int main(int argc, char* argv[]) {
    std::cout.precision(17);
    std::string P1_information_sets_file = "data/P1_information_sets_V2.txt";
    std::string P2_information_sets_file = "data/P2_information_sets_V2.txt";
    std::vector<std::string> P1_information_sets;
    std::vector<std::string> P2_information_sets;
    std::vector<std::string>& information_sets = P1_information_sets;
    std::vector<std::vector<float>> regret_list;
    std::string line;
    PolicyVec policy_obj_x;
    PolicyVec policy_obj_o;
    
    if (argc != 6) {
        std::cout << "Usage: ./cfr_br <player> <x_policy_file> <o_policy_file> <iterations> <threads>" << std::endl;
        return 1;
    }

    std::string player = argv[1];
    std::string P1_policy_file = argv[2];
    std::string P2_policy_file = argv[3];
    int num_iterations = std::stoi(argv[4]);
    int num_threads = std::stoi(argv[5]);

    std::string policy_name;

    if (player != "x" && player != "o") {
        std::cout << "Invalid player argument. Must be 'x' or 'o'." << std::endl;
        return 1;
    }

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

    if (player == "x") {
        std::string::size_type pos = P2_policy_file.find_last_of("/");
        if (pos != std::string::npos) {
            policy_name = P2_policy_file.substr(pos + 1);
        }
        else {
            policy_name = P2_policy_file;
        }
        information_sets = P1_information_sets;
    }
    else {
        std::string::size_type pos = P1_policy_file.find_last_of("/");
        if (pos != std::string::npos) {
            policy_name = P1_policy_file.substr(pos + 1);
        }
        else {
            policy_name = P1_policy_file;
        }
        information_sets = P2_information_sets;
    }

    std::cout << "Loading policy files..." << std::endl;
    policy_obj_x = PolicyVec('x', P1_policy_file);
    policy_obj_o = PolicyVec('o', P2_policy_file);
    std::cout << "Policy files loaded." << std::endl;
    float expected_utility = get_expected_utility_wrapper(policy_obj_x, policy_obj_o);
    std::cout << "Expected utility: " << expected_utility << std::endl; 

    std::ofstream f_out_policy;
    for (int T = 1; T <= num_iterations; T++) {
        std::cout << "Starting iteration " << T << "..." << std::endl;
        auto start = std::chrono::system_clock::now();

        #pragma omp parallel for num_threads(num_threads) shared(regret_list, policy_obj_x, policy_obj_o)
        for (long int i = 0; i < information_sets.size(); i++) {
            std::string I_hash = information_sets[i];
            bool move_flag = get_move_flag(I_hash, player[0]);
            InformationSet I(player[0], move_flag, I_hash);
            
            calc_cfr_policy_given_I(I, policy_obj_x, policy_obj_o, T, regret_list[i]);
        }

        auto end = std::chrono::system_clock::now();
        std::chrono::duration<float> elapsed_seconds = end - start;
        std::time_t end_time = std::chrono::system_clock::to_time_t(end);
    
        std::cout << "finished computation at " << std::ctime(&end_time)
                << "elapsed time: " << elapsed_seconds.count() << "s"
                << std::endl;

        std::cout << "Updating policy..." << std::endl;

        for (long int i = 0; i < information_sets.size(); i++) {
            std::string I_hash = information_sets[i];
            bool move_flag = get_move_flag(I_hash, player[0]);
            InformationSet I(player[0], move_flag, I_hash);

            std::vector<float>& regret_vector = regret_list[i];
            float total_regret = 0.0;
            std::vector<int> actions;
            I.get_actions(actions);

            for (int action : actions) {
                total_regret += regret_vector[action];
            }
            
            if (player == "x") {
                std::vector<float>& prob_dist = policy_obj_x.policy_dict[I.get_index()];
                if (total_regret > 0) {
                    for (int action : actions) {
                        prob_dist[action] = regret_vector[action] / total_regret;
                    }
                }
                else {
                    for (int action : actions) {
                        prob_dist[action] = 1.0 / float(actions.size());
                    }
                }
            }
            else {
                std::vector<float>& prob_dist = policy_obj_o.policy_dict[I.get_index()];
                if (total_regret > 0) {
                    for (int action : actions) {
                        prob_dist[action] = regret_vector[action] / total_regret;
                    }
                }
                else {
                    for (int action : actions) {
                        prob_dist[action] = 1.0 / float(actions.size());
                    }
                }
            }
        }

        float expected_utility = get_expected_utility_wrapper(policy_obj_x, policy_obj_o);
        std::cout << "Expected utility: " << expected_utility << std::endl; 
    
        // std::string out_policy_file = "data/best_response/iteration_" + std::to_string(T) + "_best_response_against_" + policy_name;
        // f_out_policy.open(out_policy_file, std::ios::trunc);

        // if (player == "x"){
        //     json jx;
        //     for (auto& it: policy_obj_x.policy_dict) {
        //         for (int i = 0; i < 13; i++) {
        //             jx[it.first][std::to_string(i)] = it.second[i];
        //         }
        //     }
        //     f_out_policy << jx.dump() << std::endl;
        // }
        // else {
        //     json jo;
        //     for (auto& it: policy_obj_o.policy_dict) {
        //         for (int i = 0; i < 13; i++) {
        //             jo[it.first][std::to_string(i)] = it.second[i];
        //         }
        //     }
        //     f_out_policy << jo.dump() << std::endl;
        // }
        // f_out_policy.close();
    }
}