#include "cpp_headers/poker_classes.hpp"
#include "cpp_headers/poker_utilities.hpp"

// g++-13 -O3 evaluate_policy_poker.cpp poker_classes.cpp poker_utilities.cpp -o eval_poker -fopenmp

int main(int argc, char* argv[]) {
    std::cout.precision(17);

    std::vector<std::string> P1_information_sets;
    std::vector<std::string> P2_information_sets;
    std::unordered_set<std::string> P1_hashes;
    std::unordered_set<std::string> P2_hashes;
    std::string P1_information_sets_file = "P1_information_sets_Leduc_Poker.txt";
    std::string P2_information_sets_file = "P2_information_sets_Leduc_Poker.txt";

    std::ifstream P1_f_is(P1_information_sets_file);
    std::string P1_line_is;
    while (std::getline(P1_f_is, P1_line_is)) {
        P1_information_sets.push_back(P1_line_is);
        P1_hashes.insert(P1_line_is);
    }
    P1_f_is.close();

    std::ifstream P2_f_is(P2_information_sets_file);
    std::string P2_line_is;
    while (std::getline(P2_f_is, P2_line_is)) {
        P2_information_sets.push_back(P2_line_is);
        P2_hashes.insert(P2_line_is);
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
    // std::string P1_policy_file = "data/Iterative_1/average/P1_iteration_5000_average_cfr_policy_cpp.json";
    // std::string P2_policy_file = "data/Iterative_1/average/P2_iteration_5000_average_cfr_policy_cpp.json";
    std::string P1_policy_file = "data/P1_nash_Leduc_Poker.txt";
    std::string P2_policy_file = "data/P2_nash_Leduc_Poker.txt";

    PolicyVec policy_obj_x('x', P1_policy_file, true);
    PolicyVec policy_obj_o('o', P2_policy_file, true);

    std::cout << "Policies loaded." << std::endl;
    std::cout << "Getting expected utility..." << std::endl;  

    double exploitability = 0.0;
    double expected_utility = get_expected_utility_wrapper(policy_obj_x, policy_obj_o);
    std::cout << "Expected utility: " << expected_utility << std::endl;

    PolicyVec br_x('x', P1_information_sets);
    PolicyVec br_o('o', P2_information_sets);

    std::cout << "Computing best response for player x" << std::endl;
    expected_utility = compute_best_response_wrapper(policy_obj_o, br_x, 'x');
    
    std::cout << "Expected utility output of BR function: " << expected_utility << std::endl;
    expected_utility = get_expected_utility_wrapper(br_x, policy_obj_o);
    exploitability += expected_utility;
    std::cout << "Expected utility: " << expected_utility << std::endl;

    std::cout << "Computing best response for player o" << std::endl;
    expected_utility = compute_best_response_wrapper(policy_obj_x, br_o, 'o');

    std::cout << "Expected utility output of BR function: " << expected_utility << std::endl;
    expected_utility = get_expected_utility_wrapper(policy_obj_x, br_o);
    exploitability -= expected_utility;
    std::cout << "Expected utility: " << expected_utility << std::endl;

    std::cout << "Exploitability: " << exploitability << std::endl;

    return 0;
}


    // std::string nash_file = "LeducNashValues.txt";
    // std::unordered_map<char, int> action_to_index = {{'x', 0}, {'b', 1}, {'c', 2}, {'r', 3}, {'f', 4}};
    // // parse file
    // std::ifstream nash_f(nash_file);
    // std::string nash_line;

    // while (std::getline(nash_f, nash_line)) {
    //     std::vector<std::string> nash_line_split;
    //     if (nash_line[0] == '#'){
    //         continue;
    //     }
    //     // remove commas and colons
    //     nash_line.erase(std::remove(nash_line.begin(), nash_line.end(), ','), nash_line.end());
    //     nash_line.erase(std::remove(nash_line.begin(), nash_line.end(), ':'), nash_line.end());

    //     split(nash_line, nash_line_split, ' ');
    //     std::string cards = nash_line_split[0];
    //     if (cards.size() == 1){
    //         cards += "-";
    //     }
    //     std::string bid_sequence = nash_line_split[1];
    //     if (bid_sequence == "*"){
    //         bid_sequence = "";
    //     }
    //     std::vector<double> prob_dist(6, 0.0);
    //     int i = 2;

    //     while (i < nash_line_split.size()) {
    //         char action = nash_line_split[i++][0];
    //         if (action_to_index.find(action) == action_to_index.end()) {
    //             continue;
    //         }
    //         double prob = std::stod(nash_line_split[i++]);
    //         prob_dist[action_to_index[action]] = prob;
    //     }

    //     std::string hash = "a-" + cards + "-" + bid_sequence;
    //     char player = 'x';
    //     if (P2_hashes.find(hash) != P2_hashes.end()){
    //         player = 'o';
    //     }

    //     InformationSet I(player, get_move_flag(hash, player), hash);
    //     if (player == 'x') {
    //         policy_obj_x.policy_dict[I.get_index()] = prob_dist;
    //     }
    //     else {
    //         policy_obj_o.policy_dict[I.get_index()] = prob_dist;
    //     }
    // }

    // expected_utility = get_expected_utility_wrapper(policy_obj_x, policy_obj_o);
    // std::cout << "Expected utility: " << expected_utility << std::endl;

    // save policies to file
    // save_map_txt("data/P1_nash_Leduc_Poker.txt", policy_obj_x.policy_dict, P1_information_sets);
    // save_map_txt("data/P2_nash_Leduc_Poker.txt", policy_obj_o.policy_dict, P2_information_sets);
