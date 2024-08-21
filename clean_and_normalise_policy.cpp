#include "cpp_headers/rbt_classes.hpp"
#include "cpp_headers/rbt_utilities.hpp"

void save_map_json(std::string output_file, std::vector<std::vector<double>>& map, std::vector<std::string>& information_sets){
    std::ofstream f_out;
    f_out.open(output_file, std::ios::trunc);
    json jx;
    for (long int j = 0; j < map.size(); j++) {
        for (int i = 0; i < 13; i++) {
            jx[information_sets[j]][std::to_string(i)] = map[j][i];
        }
    }
    f_out << jx.dump() << std::endl;
    f_out.close();
}

void save_map_txt(std::string output_file, std::vector<std::vector<double>>& map, std::vector<std::string>& Information_sets){
    std::ofstream f_out;
    f_out.open(output_file, std::ios::trunc);
    for (long int j = 0; j < map.size(); j++) {
        // if all actions have zero probability, do not save information set
        bool all_zero = true;
        for (int i = 0; i < 13; i++) {
            if (map[j][i] > 0.0){
                all_zero = false;
                break;
            }
        }

        if (all_zero) {
            continue;
        }
        else {
            f_out << Information_sets[j] << " ";
            for (int i = 0; i < 13; i++) {
                if (map[j][i] > 0.0){
                    f_out << i << " " << map[j][i] << " ";
                }
            }
            f_out << std::endl;
        }
    }
    f_out.close();
}

void save_output(std::string output_policy_file, char player, std::vector<std::string>& information_sets, PolicyVec& policy_obj) {
    std::cout << "Saving policy for player " << player << "..." << std::endl;
    // save_map_json(output_policy_file, policy_obj.policy_dict, information_sets);
    save_map_txt(output_policy_file, policy_obj.policy_dict, information_sets);
}

int main(int argc, char** argv) {
    std::string file_path = argv[1];
    char player = argv[2][0];
    bool txt_flag = std::stoi(argv[3]) == 1 ? true : false;
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
    PolicyVec policy_obj = PolicyVec(player, file_path, txt_flag);
    std::cout << "Policies loaded." << std::endl;

    for (long int i = 0; i < policy_obj.policy_dict.size(); i++) {
        std::vector<double>& prob_dist = policy_obj.policy_dict[i];

        for (int j = 0; j < prob_dist.size(); j++) {
            if (prob_dist[j] < 1e-3) {
                prob_dist[j] = 0.0;
            }
        }

        double sum = 0.0;

        for (int j = 0; j < prob_dist.size(); j++) {
            sum += prob_dist[j];
        }

        if (sum == 0.0) {
            continue;
        }
        else {
            for (int j = 0; j < prob_dist.size(); j++) {
                prob_dist[j] /= sum;
            }
        }
        
    }

    std::cout << "Saving policies..." << std::endl;
    file_path = file_path.substr(0, file_path.find_last_of('.')) + "_normalised.txt";
    save_output(file_path, player, player == 'x' ? P1_information_sets : P2_information_sets, policy_obj);
}