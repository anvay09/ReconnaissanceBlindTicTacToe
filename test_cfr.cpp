// Compile: g++-13 -O3 test_cfr.cpp rbt_classes.cpp rbt_utilities.cpp -o test_cfr -fopenmp -I /Users/anvay/Downloads/boost_1_84_0

#include "cpp_headers/rbt_classes.hpp"
#include "cpp_headers/rbt_utilities.hpp"

int main(int argc, char* argv[]) {
    std::string P1_information_sets_file = "data/P1_information_sets.txt";
    std::string P2_information_sets_file = "data/P2_information_sets.txt";
    std::string P1_policy_file = "data/P1_uniform_policy.json";
    std::string P2_policy_file = "data/P2_uniform_policy.json";
    Policy policy_obj_x;
    Policy policy_obj_o;
    std::string player = "x";
    std::string policy_file = P1_policy_file;
    
    int index = argv[1][0] - '0';

    std::ifstream f1;
    if (player == "x") {
        policy_obj_x.load_policy('x', policy_file);
        policy_obj_o.load_policy('o', P2_policy_file);
        f1.open(P1_information_sets_file);
    }
    else {
        policy_obj_x.load_policy('x', P1_policy_file);
        policy_obj_o.load_policy('o', policy_file);
        f1.open(P2_information_sets_file);
    }

    std::vector<std::string> information_sets;
    std::vector<std::vector<double>> regret_list;
    std::string line;

    while (std::getline(f1, line)) {
        information_sets.push_back(line);
        std::vector<double> regret_vector;
        for (int i = 0; i < 13; i++) {
            regret_vector.push_back(0.0);
        }
        regret_list.push_back(regret_vector);
    }
    f1.close();
    

    std::string I_hash = information_sets[index];
    std::cout << I_hash << std::endl;
    bool move_flag = I_hash[I_hash.size()-1] == 'm' ? true : false;
    I_hash.pop_back();
    InformationSet I(player[0], move_flag, I_hash);
    
    calc_cfr_policy_given_I(I, policy_obj_x, policy_obj_o, 1, regret_list[index]);

    for (int i = 0; i < regret_list[index].size(); i++) {
        std::cout << regret_list[index][i] << " ";
    }
    std::cout << std::endl;
}