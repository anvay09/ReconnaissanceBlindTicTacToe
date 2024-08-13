#include "cpp_headers/rbt_classes.hpp"

// g++-13 -O3 read_infoset_policy.cpp rbt_classes.cpp -o read_i -fopenmp

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

int main(int argc, char** argv) {
    std::string file_path_1 = argv[1];
    std::string file_path_2 = argv[2];
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

    std::cout.precision(17);
    std::cout << "Loading policies..." << std::endl;

    PolicyVec policy_obj_x('x', file_path_1);
    PolicyVec policy_obj_o('o', file_path_2);
    std::cout << "Policies loaded." << std::endl;
    
    int flag = 1;

    while (flag) {
        std::string infoset;
        char player;
        std::cout << "Enter player: ";
        std::cin >> player;
        std::cout << "Enter information set: ";
        std::cin >> infoset;
        bool move_flag = get_move_flag(infoset, player);
        InformationSet I(player, move_flag, infoset);
        if (player == 'x') {
            std::cout << "Player 1 policy for infoset: " << infoset << std::endl;
            std::cout << "Player 1 policy: " << std::endl;
            for (int i = 0; i < 13 ; i++) {
                std::cout << policy_obj_x.policy_dict[I.get_index()][i] << " ";
            }
           std::cout<< std::endl;
        } 
        else {
            std::cout << "Player 2 policy for infoset: " << infoset << std::endl;
            std::cout << "Player 2 policy: " << std::endl;
            for (int i = 0; i < 13 ; i++) {
                std::cout << policy_obj_o.policy_dict[I.get_index()][i] << " ";
            }
           std::cout<< std::endl;
        }
        std::cout << "Continue? (1/0): ";
        std::cin >> flag;
    }
    return 0;
}
