#include "cpp_headers/rbt_classes.hpp"
#include "cpp_headers/rbt_utilities.hpp"


int main(int argc, char** argv) {
    std::cout.precision(17);
    std::string file_path_1 = argv[1];
    std::string file_path_2 = argv[2];
    char input_type = argv[3][0]; // 'j' for json, 't' for txt

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

    auto start = std::chrono::system_clock::now();
    PolicyVec policy_obj_x = PolicyVec('x', file_path_1, txt_flag);
    PolicyVec policy_obj_o = PolicyVec('o', file_path_2, txt_flag);
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end-start;
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);

    std::cout << "Policies loaded." << std::endl;
    std::cout << "Elapsed time: " << elapsed_seconds.count() << "s" << std::endl;
    
    double expected_utility = get_expected_utility_wrapper(policy_obj_x, policy_obj_o);
    std::cout << "Expected utility: " << expected_utility << std::endl;

    PolicyVec br_x = policy_obj_x;
    PolicyVec br_o = policy_obj_o;

    double br_utility = compute_best_response_wrapper(policy_obj_o, br_x, 'x');
    std::cout << "Best response utility of x against o: " << br_utility << std::endl;
    br_utility = compute_best_response_wrapper(policy_obj_x, br_o, 'o');
    std::cout << "Best response utility of o against x: " << br_utility << std::endl;
}