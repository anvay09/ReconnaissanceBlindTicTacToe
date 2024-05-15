// Compile: g++-13 -O3 cfr.cpp rbt_classes.cpp rbt_utilities.cpp -o cfr -fopenmp -I /Users/anvay/Downloads/boost_1_84_0

#include "cpp_headers/rbt_classes.hpp"
#include "cpp_headers/rbt_utilities.hpp"

int main(int argc, char* argv[])  {
    int number_threads = std::stoi(argv[1]); //96;
    std::string base_path = argv[2]; //"data/Iterative_1";
    int start_iter = std::stoi(argv[3]); //1;
    int end_iter = std::stoi(argv[4]); //1000;
    std::string policy_file_x = argv[5]; //"data/P1_uniform_policy.json";
    std::string policy_file_o = argv[6]; //"data/P2_uniform_policy.json";

    Policy policy_obj_x('x', policy_file_x);
    Policy policy_obj_o('o', policy_file_o);

    std::string P1_information_sets_file = "data/P1_information_sets.txt";
    std::string P2_information_sets_file = "data/P2_information_sets.txt";
    
    std::vector<std::string> P1_information_sets;
    std::vector<std::string> P2_information_sets;

    std::vector<std::vector<double>> regret_list_x;
    std::vector<std::vector<double>> regret_list_o;
    std::unordered_map<std::string, std::vector<double> > regret_map_x;
    std::unordered_map<std::string, std::vector<double> > regret_map_o;

    if (start_iter == 1) {
        std::ifstream f1(P1_information_sets_file);
        std::string line;
        
        while (std::getline(f1, line)) {
            P1_information_sets.push_back(line);
            std::vector<double> regret_vector;
            for (int i = 0; i < 13; i++) {
                regret_vector.push_back(0.0);
            }
            regret_list_x.push_back(regret_vector);
        }
        f1.close();

        std::ifstream f2(P2_information_sets_file);
        while (std::getline(f2, line)) {
            P2_information_sets.push_back(line);
            std::vector<double> regret_vector;
            for (int i = 0; i < 13; i++) {
                regret_vector.push_back(0.0);
            }
            regret_list_o.push_back(regret_vector);
        }
        f2.close();
    }

    else {
        std::string prev_regret_file_x = base_path + "/regret/P1_iteration_" + std::to_string(start_iter-1) + "_regret_cpp.json";
        std::string prev_regret_file_o = base_path + "/regret/P2_iteration_" + std::to_string(start_iter-1) + "_regret_cpp.json";
        regret_map_x = get_prev_regrets(prev_regret_file_x);
        regret_map_o = get_prev_regrets(prev_regret_file_o);

        std::ifstream f1(P1_information_sets_file);
        std::string line;
        
        while (std::getline(f1, line)) {
            P1_information_sets.push_back(line);
            std::vector<double> regret_vector;
            for (int i = 0; i < 13; i++) {
                regret_vector.push_back(regret_map_x[line][i]);
            }
            regret_list_x.push_back(regret_vector);
        }
        f1.close();

        std::ifstream f2(P2_information_sets_file);
        while (std::getline(f2, line)) {
            P2_information_sets.push_back(line);
            std::vector<double> regret_vector;
            for (int i = 0; i < 13; i++) {
                regret_vector.push_back(regret_map_o[line][i]);
            }
            regret_list_o.push_back(regret_vector);
        }
        f2.close();
    }

    std::ofstream f_out_policy;

    for (int T = start_iter; T <= end_iter; T++) {
        std::string next_policy_file_x = base_path + "/cfr" + "/P1_iteration_" + std::to_string(T) + "_cfr_policy_cpp.json";
        std::string next_policy_file_o = base_path + "/cfr" + "/P2_iteration_" + std::to_string(T) + "_cfr_policy_cpp.json";
        std::string next_regret_file_x = base_path + "/regret/P1_iteration_" + std::to_string(T) + "_regret_cpp.json";
        std::string next_regret_file_o = base_path + "/regret/P2_iteration_" + std::to_string(T) + "_regret_cpp.json";

        std::cout << "Starting iteration " << T << " for player x..." << std::endl;
        auto start = std::chrono::system_clock::now();

        #pragma omp parallel for num_threads(number_threads)
        for (int i = 0; i < P1_information_sets.size(); i++) {
            std::string I_hash = P1_information_sets[i];
            bool move_flag = I_hash[I_hash.size()-1] == 'm' ? true : false;
            I_hash.pop_back();
            InformationSet I('x', move_flag, I_hash);

            calc_cfr_policy_given_I(I, policy_obj_x, policy_obj_o, T, regret_list_x[i]);
        }

        auto end = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = end - start;
        std::time_t end_time = std::chrono::system_clock::to_time_t(end);
    
        std::cout << "finished computation at " << std::ctime(&end_time)
                << "elapsed time: " << elapsed_seconds.count() << "s"
                << std::endl;

        std::cout << "Saving regrets for player x..." << std::endl;
        for (int i = 0; i < P1_information_sets.size(); i++) {
            std::string I_hash = P1_information_sets[i];
            std::vector<double>& regret_vector = regret_list_x[i];
            regret_map_x[I_hash] = regret_vector;
        }

        f_out_policy.open(next_regret_file_x, std::ios::trunc);

        json jx_regret;
        for (auto& it: regret_map_x) {
            for (int i = 0; i < 13; i++) {
                jx_regret[it.first][std::to_string(i)] = it.second[i];
            }
        }
        f_out_policy << jx_regret.dump() << std::endl;
        f_out_policy.close();


        std::cout << "Updating policy for player x..." << std::endl;

        for (int i = 0; i < P1_information_sets.size(); i++) {
            std::string I_hash = P1_information_sets[i];
            bool move_flag = I_hash[I_hash.size()-1] == 'm' ? true : false;
            I_hash.pop_back();
            InformationSet I('x', move_flag, I_hash);
            I_hash += move_flag ? "m" : "s";
            std::vector<double>& regret_vector = regret_list_x[i];
            double total_regret = 0.0;
            std::vector<int> actions;
            I.get_actions(actions);

            for (int j = 0; j < actions.size(); j++) {
                total_regret += regret_vector[actions[j]];
            }

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

        
        f_out_policy.open(next_policy_file_x, std::ios::trunc);

        json jx;
        for (auto& it: policy_obj_x.policy_dict) {
            for (int i = 0; i < 13; i++) {
                jx[it.first][std::to_string(i)] = it.second[i];
            }
        }
        f_out_policy << jx.dump() << std::endl;
        f_out_policy.close();

        std::cout << "Starting iteration " << T << " for player o..." << std::endl;
        start = std::chrono::system_clock::now();

        #pragma omp parallel for num_threads(number_threads)
        for (int i = 0; i < P2_information_sets.size(); i++) {
            std::string I_hash = P2_information_sets[i];
            bool move_flag = I_hash[I_hash.size()-1] == 'm' ? true : false;
            I_hash.pop_back();
            InformationSet I('o', move_flag, I_hash);

            calc_cfr_policy_given_I(I, policy_obj_x, policy_obj_o, T, regret_list_o[i]);
        }

        end = std::chrono::system_clock::now();
        elapsed_seconds = end - start;
        end_time = std::chrono::system_clock::to_time_t(end);

        std::cout << "finished computation at " << std::ctime(&end_time)
                << "elapsed time: " << elapsed_seconds.count() << "s"
                << std::endl;

        std::cout << "Saving regrets for player o..." << std::endl;
        for (int i = 0; i < P2_information_sets.size(); i++) {
            std::string I_hash = P2_information_sets[i];
            std::vector<double>& regret_vector = regret_list_o[i];
            regret_map_o[I_hash] = regret_vector;
        }

        f_out_policy.open(next_regret_file_o, std::ios::trunc);

        json jo_regret;
        for (auto& it: regret_map_o) {
            for (int i = 0; i < 13; i++) {
                jo_regret[it.first][std::to_string(i)] = it.second[i];
            }
        }
        f_out_policy << jo_regret.dump() << std::endl;
        f_out_policy.close();

        std::cout << "Updating policy for player o..." << std::endl;

        for (int i = 0; i < P2_information_sets.size(); i++) {
            std::string I_hash = P2_information_sets[i];
            bool move_flag = I_hash[I_hash.size()-1] == 'm' ? true : false;
            I_hash.pop_back();
            InformationSet I('o', move_flag, I_hash);
            I_hash += move_flag ? "m" : "s";
            std::vector<double>& regret_vector = regret_list_o[i];
            double total_regret = 0.0;
            std::vector<int> actions;
            I.get_actions(actions);

            for (int j = 0; j < actions.size(); j++) {
                total_regret += regret_vector[actions[j]];
            }

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

        f_out_policy.open(next_policy_file_o, std::ios::trunc);

        json jo;
        for (auto& it: policy_obj_o.policy_dict) {
            for (int i = 0; i < 13; i++) {
                jo[it.first][std::to_string(i)] = it.second[i];
            }
        }
        f_out_policy << jo.dump() << std::endl;
        f_out_policy.close();
    }
}