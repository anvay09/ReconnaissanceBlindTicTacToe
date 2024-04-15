// Compile: g++-13 -O3 cfr.cpp rbt_classes.cpp rbt_utilities.cpp -o cfr -fopenmp -I /Users/anvay/Downloads/boost_1_84_0

#include "cpp_headers/rbt_classes.hpp"
#include "cpp_headers/rbt_utilities.hpp"

int main() {
    std::string policy_file_x = "data/P1_deterministic_policy.json";
    std::string policy_file_o = "data/P2_deterministic_policy.json";

    Policy policy_obj_x('x', policy_file_x);
    Policy policy_obj_o('o', policy_file_o);

    std::string P1_information_sets_file = "data/P1_information_sets.txt";
    std::string P2_information_sets_file = "data/P2_information_sets.txt";
    
    std::vector<std::string> P1_information_sets;
    std::vector<std::string> P2_information_sets;

    std::vector<std::vector<double>> regret_list_x;
    std::vector<std::vector<double>> regret_list_o;

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

    std::ofstream f_out_policy;
    for (int T = 1; T <= 1000; T++) {
        std::string next_policy_file_x = "data/P1_iteration_" + std::to_string(T) + "_cfr_policy_cpp.json";
        std::string next_policy_file_o = "data/P2_iteration_" + std::to_string(T) + "_cfr_policy_cpp.json";

        std::cout << "Starting iteration " << T << " for player x..." << std::endl;

        #pragma omp parallel for 
        for (int i = 0; i < P1_information_sets.size(); i++) {
            std::string I_hash = P1_information_sets[i];
            bool move_flag = I_hash[I_hash.size()-1] == 'm' ? true : false;
            I_hash.pop_back();
            InformationSet I('x', move_flag, I_hash);

            calc_cfr_policy_given_I(I, policy_obj_x, policy_obj_o, T, regret_list_x[i]);
        }

        std::cout << "Starting iteration " << T << " for player o..." << std::endl;

        #pragma omp parallel for
        for (int i = 0; i < P2_information_sets.size(); i++) {
            std::string I_hash = P2_information_sets[i];
            bool move_flag = I_hash[I_hash.size()-1] == 'm' ? true : false;
            I_hash.pop_back();
            InformationSet I('o', move_flag, I_hash);

            calc_cfr_policy_given_I(I, policy_obj_x, policy_obj_o, T, regret_list_o[i]);
        }
        
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
                total_regret += regret_vector[j];
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
                total_regret += regret_vector[j];
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