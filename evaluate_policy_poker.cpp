#include "cpp_headers/poker_classes.hpp"
#include "cpp_headers/poker_utilities.hpp"

// g++-13 -O3 evaluate_policy.cpp rbt_classes.cpp -o evaluate_policy -fopenmp

int main(int argc, char* argv[]) {
    std::cout.precision(17);

    std::vector<std::string> P1_information_sets;
    std::vector<std::string> P2_information_sets;
    std::string P1_information_sets_file = "P1_information_sets_Leduc_Poker.txt";
    std::string P2_information_sets_file = "P2_information_sets_Leduc_Poker.txt";

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
    char player = 'x';
    PolicyVec policy_obj_x('x', P1_information_sets);
    PolicyVec policy_obj_o('o', P2_information_sets);

    std::cout << "Policies loaded." << std::endl;
    std::cout << "Getting expected utility..." << std::endl;  

    // save policies to file
    save_map_txt("data/P1_uniform_policy_Leduc_Poker.txt", policy_obj_x.policy_dict, P1_information_sets);
    save_map_txt("data/P2_uniform_policy_Leduc_Poker.txt", policy_obj_o.policy_dict, P2_information_sets);
    
    double expected_utility = get_expected_utility_wrapper(policy_obj_x, policy_obj_o);
    std::cout << "Expected utility: " << expected_utility << std::endl;

    return 0;
}
