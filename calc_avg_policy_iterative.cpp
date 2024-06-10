#include "cpp_headers/rbt_classes.hpp"
#include "cpp_headers/json.hpp"
using json = nlohmann::json;

void calc_average_policy(std::vector<Policy>& policy_obj_list, InformationSet& I, char initial_player, std::vector<double>& pol_prob_dist) {
    std::string I_hash = I.get_hash();
    
    std::vector<int> actions;
    I.get_actions(actions);

    for (int action: actions) {
        double numerator = 0;
        double denominator = 0;

        for (int p = 0; p < policy_obj_list.size(); p++) {
            std::vector<double>& policy = policy_obj_list[p].policy_dict[I_hash];
            numerator += double(p+1) * policy[action];
            for (int act: actions) {
                denominator += policy[act];
            }
        }

        if (denominator == 0) {
            pol_prob_dist[action] = 0;
        } else {
            pol_prob_dist[action] = numerator / denominator;
        }
    }
}

int main(int argc, char* argv[]) {
    int number_threads = std::stoi(argv[1]); //96;
    char current_player = argv[2][0];
    std::string policy_file_base = argv[3];
    int rounds = std::stoi(argv[4]);
    std::string base_path = argv[5];

    std::string P1_info_set_file = "data/P1_information_sets.txt";
    std::string P2_info_set_file = "data/P2_information_sets.txt";
    std::vector<std::string> P1_information_sets;
    std::vector<std::string> P2_information_sets;
    std::vector<InformationSet> I1_list;
    std::vector<InformationSet> I2_list;

    std::ifstream f1(P1_info_set_file);
    std::ifstream f2(P2_info_set_file);

    std::string line;
    while (std::getline(f1, line)) {
        P1_information_sets.push_back(line);
    }
    while (std::getline(f2, line)) {
        P2_information_sets.push_back(line);
    }

    f1.close();
    f2.close();

    for (int i = 0; i < P1_information_sets.size(); i++) {
        std::string I_hash = P1_information_sets[i];
        bool move_flag = I_hash[I_hash.size()-1] == 'm' ? true : false;
        I_hash.pop_back();
        InformationSet I(current_player, move_flag, I_hash);
        I1_list.push_back(I);
    }
    
    for (int i = 0; i < P2_information_sets.size(); i++) {
        std::string I_hash = P2_information_sets[i];
        bool move_flag = I_hash[I_hash.size()-1] == 'm' ? true : false;
        I_hash.pop_back();
        InformationSet I(current_player, move_flag, I_hash);
        I2_list.push_back(I);
    }

    std::vector<Policy> policy_obj_list;

    for (int j=1; j < rounds + 1; j++)
    {
        std::vector<InformationSet>& I_list = current_player == 'o' ? I2_list : I1_list;
        
        std::string policy_file = policy_file_base;
        std::replace(policy_file.begin(), policy_file.end(), '{', std::to_string(j)[0]);
        policy_file.erase(std::remove(policy_file.begin(), policy_file.end(), '}'), policy_file.end());
        Policy policy_obj(current_player, policy_file);
        policy_obj_list.push_back(policy_obj);

        std::string avg_policy_file = policy_file_base;
        std::replace(avg_policy_file.begin(), avg_policy_file.end(), '{', std::to_string(1)[0]);
        avg_policy_file.erase(std::remove(avg_policy_file.begin(), avg_policy_file.end(), '}'), avg_policy_file.end());
        Policy average_policy(current_player, avg_policy_file);

        std::cout << "Starting average run upto round " << j << std::endl;
        auto start = std::chrono::system_clock::now();

        #pragma omp parallel for num_threads(number_threads)
        for (InformationSet Iset: I_list) {
            calc_average_policy(policy_obj_list, Iset, current_player, average_policy.policy_dict[Iset.get_hash()]);
        }

        std::cout << "Saving avg policy upto round " << j << std::endl;
        std::string outfile_name;
        if (current_player == 'o') {
            outfile_name = base_path + "/average/P2_average_policy_after_" + std::to_string(j) + "_rounds.json";
        }
        else {
            outfile_name = base_path + "/average/P1_average_policy_after_" + std::to_string(j) + "_rounds.json";
        }

        std::ofstream out(outfile_name);
        json jo;
        for (auto& it: average_policy.policy_dict) {
            for (int i = 0; i < 13; i++) {
                jo[it.first][std::to_string(i)] = it.second[i];
            }
        }
        out << jo.dump() << std::endl;
        out.close();

        auto end = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = end - start;
        std::time_t end_time = std::chrono::system_clock::to_time_t(end);
    
        std::cout << "finished computation at " << std::ctime(&end_time)
                << "elapsed time: " << elapsed_seconds.count() << "s"
                << std::endl;
    }

    return 0;
}