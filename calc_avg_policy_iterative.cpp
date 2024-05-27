#include "cpp_headers/rbt_classes.hpp"
#include "cpp_headers/json.hpp"
using json = nlohmann::json;

void calc_average_policy(std::vector<Policy>& policy_obj_list, InformationSet& I, char initial_player, std::vector<double> pol_prob_dist) {
    std::string I_hash = I.get_hash();
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

    pol_prob_dist = prob_dist;
}

int main(int argc, char* argv[]) {
    int number_threads = std::stoi(argv[1]); //96;
    char current_player = argv[2][0];
    std::string policy_file_base = argv[3];
    int rounds = std::stoi(argv[4]);
    std::string base_path = argv[5];
    
    for (int j=1; j < rounds + 1; j++)
    {
        std::vector<Policy> policy_obj_list;
        std::vector<InformationSet> I_list;
        for (int i = 1; i < j + 1; i++) {
            std::string policy_file = policy_file_base;
            std::replace(policy_file.begin(), policy_file.end(), '{', std::to_string(i)[0]);
            policy_file.erase(std::remove(policy_file.begin(), policy_file.end(), '}'), policy_file.end());
            
            Policy policy_obj(current_player, policy_file);
            policy_obj_list.push_back(policy_obj);
        }

        std::string IS_file_player;
        if (current_player == 'o') {
            IS_file_player = "data/P2_information_sets.txt";
        }
        else {
            IS_file_player = "data/P1_information_sets.txt";
        }
        std::ifstream f1(IS_file_player);
        std::string line;
        std::vector<std::string> P_information_sets;
        while (std::getline(f1, line)) {
            P_information_sets.push_back(line);
        }
        f1.close();
        for (int i = 0; i < P_information_sets.size(); i++) {
            std::string I_hash = P_information_sets[i];
            bool move_flag = I_hash[I_hash.size()-1] == 'm' ? true : false;
            I_hash.pop_back();
            InformationSet I(current_player, move_flag, I_hash);
            I_list.push_back(I);
        }

        std::string policy_file = policy_file_base;
        std::replace(policy_file.begin(), policy_file.end(), '{', std::to_string(1)[0]);
        policy_file.erase(std::remove(policy_file.begin(), policy_file.end(), '}'), policy_file.end());
        Policy average_policy(current_player, policy_file);

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