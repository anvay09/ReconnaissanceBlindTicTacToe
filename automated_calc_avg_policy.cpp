#include "rbt_classes.hpp"
#include "json.hpp"
using json = nlohmann::json;

void calc_average_policy(std::vector<Policy>& policy_obj_list, InformationSet& I, char initial_player, Policy& average_policy) {
    std::string I_hash = I.get_hash();
    std::cout << "Calculating average policy for " << I_hash << "..." << std::endl;
    average_policy.player = I.player;

    std::vector<double> prob_dist;
    for (int i = 0; i < 13; i++) {
        prob_dist.push_back(0);
    }
    
    std::vector<int> actions;
    I.get_actions(actions);

    for (int action: actions) {
        double numerator = 0;
        double denominator = 0;

        for (int p = 0; p < policy_obj_list.size(); p++) {
            std::vector<double>& policy = policy_obj_list[p].policy_dict[I_hash];
            numerator += policy[action];
            for (int act: actions) {
                denominator += policy[act];
            }
        }

        if (denominator == 0) {
            prob_dist[action] = 0;
        } else {
            prob_dist[action] = numerator / denominator;
        }
    }

    average_policy.policy_dict[I_hash] = prob_dist;
}

void get_average_policy(std::vector<Policy>& policy_obj_list, std::vector<InformationSet>& I_list, char initial_player, Policy& average_policy) {
    for (InformationSet I: I_list) {
        calc_average_policy(policy_obj_list, I, initial_player, average_policy);
    }
}

int main(int argc, char* argv[]) {
    char current_player = argv[1][0];
    std::string policy_file_base = argv[2];
    int rounds = std::stoi(argv[3]);
    std::string base_file = argv[4];

    std::vector<Policy> policy_obj_list;
    std::vector<InformationSet> I_list;

    for (int i = 1; i < rounds + 1; i++) {
        std::string policy_file = policy_file_base;
        std::replace(policy_file.begin(), policy_file.end(), '{', std::to_string(i)[0]);
        policy_file.erase(std::remove(policy_file.begin(), policy_file.end(), '}'), policy_file.end());
        
        Policy policy_obj(current_player, policy_file);
        policy_obj_list.push_back(policy_obj);
    }

    std::string IS_file_player;
    if (current_player == 'o') {
        IS_file_player = "data/P2_information_sets.json";
    }
    else {
        IS_file_player = "data/P1_information_sets.json";
    }

    std::ifstream i(IS_file_player);
    json I_set;
    i >> I_set;
    
    for (json::iterator it = I_set.begin(); it != I_set.end(); ++it) {
        std::string I_hash = it.key();
        bool move_flag = I_hash[I_hash.size() - 1] == 'm';
        I_hash.pop_back();
        InformationSet I(current_player, move_flag, I_hash);
        I_list.push_back(I);
    }

    std::replace(policy_file_base.begin(), policy_file_base.end(), '{', std::to_string(1)[0]);
    policy_file_base.erase(std::remove(policy_file_base.begin(), policy_file_base.end(), '}'), policy_file_base.end());
    Policy average_policy(current_player, policy_file_base);

    std::cout << "Starting average run..." << std::endl;
    get_average_policy(policy_obj_list, I_list, current_player, average_policy);

    std::string outfile_name;
    if (current_player == 'o') {
        outfile_name = base_file + "/average/P2_average_overall_policy_after_" + std::to_string(rounds) + "_rounds.json";
    }
    else {
        outfile_name = base_file + "/average/P1_average_overall_policy_after_" + std::to_string(rounds) + "_rounds.json";
    }

    std::ofstream out(outfile_name);

    json j;
    for (auto& it: average_policy.policy_dict) {
        for (int i = 0; i < 13; i++) {
            j[it.first][std::to_string(i)] = it.second[i];
        }
    }
    out << j.dump() << std::endl;
    out.close();
    return 0;
}