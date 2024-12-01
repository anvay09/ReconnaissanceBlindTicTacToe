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
            if (Information_sets[j] == "") {
                f_out << "* ";
            }
            else {
                f_out << Information_sets[j] << " ";
            }

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


void save_output(std::string output_policy_file, char player, std::vector<std::string>& information_sets, PolicyVec& policy_obj, bool txt_flag = true) {
    std::cout << "Saving policy for player " << player << "..." << std::endl;
    if (txt_flag) {
        save_map_txt(output_policy_file, policy_obj.policy_dict, information_sets);
    }
    else {
        save_map_json(output_policy_file, policy_obj.policy_dict, information_sets);
    }
}


int main(int argc, char** argv) {
    std::string file_path = argv[1];
    char player = argv[2][0];
    char input_type = argv[3][0]; // 'j' for json, 't' for txt
    char output_type = argv[4][0]; // 'j' for json, 't' for txt
    int normalise_flag = argv[5][0] - '0'; // 1 for normalise, 0 for not normalise

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
    bool txt_flag = input_type == 't' ? true : false;
    PolicyVec policy_obj = PolicyVec(player, file_path, txt_flag);
    std::cout << "Policies loaded." << std::endl;


    if (normalise_flag == 0) {
        std::cout << "Saving policies..." << std::endl;
        if (output_type == 't') {
            file_path = file_path.substr(0, file_path.find_last_of('.')) + ".txt";
            save_output(file_path, player, player == 'x' ? P1_information_sets : P2_information_sets, policy_obj);
        }
        else {
            file_path = file_path.substr(0, file_path.find_last_of('.')) + ".json";
            save_output(file_path, player, player == 'x' ? P1_information_sets : P2_information_sets, policy_obj, false);
        }   
    }
    else {
        std::cout << "Normalising policies..." << std::endl;
    
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
        if (output_type == 't') {
            file_path = file_path.substr(0, file_path.find_last_of('.')) + "_normalised.txt";
            save_output(file_path, player, player == 'x' ? P1_information_sets : P2_information_sets, policy_obj);
        }
        else {
            file_path = file_path.substr(0, file_path.find_last_of('.')) + "_normalised.json";
            save_output(file_path, player, player == 'x' ? P1_information_sets : P2_information_sets, policy_obj, false);
        }    
    }
}