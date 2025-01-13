#include "cpp_headers/poker_classes.hpp"
#include "cpp_headers/poker_utilities.hpp"
// g++-13 count_terminal_infosets_poker.cpp poker_classes.cpp poker_utilities.cpp -O3 -o count_terminal_infosets_poker 


int build_balanced_exploration_policy(PolicyVec& policy_obj, InformationSet& I) {
    std::vector<int> legal_actions;
    I.get_actions(legal_actions);
    std::vector<int> action_tree_size(6, 0);

    for (int a : legal_actions){
        std::unordered_set<std::string> cohort;
        std::unordered_map<std::string, int> cohort_values;
        get_cohort(I, a, cohort);

        if (cohort.size() == 0){
            action_tree_size[a] = 1;
            continue;
        }

        for (std::string I_prime_hash : cohort){
            InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash);
            cohort_values[I_prime_hash] = build_balanced_exploration_policy(policy_obj, I_prime);

            action_tree_size[a] += cohort_values[I_prime_hash];
        }
    }

    int total = 0;
    for (int a : legal_actions){
        total += action_tree_size[a];
    }

    for (int a : legal_actions){
        policy_obj.policy_dict[I.get_index()][a] = (double)action_tree_size[a] / total;
    }

    return total;
}


void build_balanced_exploration_policy_wrapper(PolicyVec& policy_obj, char player, char game){
    std::vector<char> player_cards = {'J', 'Q', 'K'};
    for (int card_index = 0; card_index < player_cards.size(); card_index++){
        std::string hash_1 = "a-" + std::string(1, player_cards[card_index]) + "--";
        std::string hash_2 = "o-" + std::string(1, player_cards[card_index]) + "--";
        InformationSet root = player == 'x' ? InformationSet('x', true, hash_1, game) : InformationSet('o', false, hash_2, game);
        int root_val = build_balanced_exploration_policy(policy_obj, root);
    }
}


int main(int argc, char* argv[]) {
    std::cout.precision(17);
    char game = argv[1][0];
    char player = argv[2][0];
    
    // load information sets
    std::vector<std::string> P1_information_sets;
    std::vector<std::string> P2_information_sets;
    std::string P1_information_sets_file = game == 'L'? "P1_information_sets_Leduc_Poker.txt" : "P1_information_sets_Kuhn_Poker.txt";
    std::string P2_information_sets_file = game == 'L'? "P2_information_sets_Leduc_Poker.txt" : "P2_information_sets_Kuhn_Poker.txt";
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

    // create hash to int maps
    for (int i = 0; i < P1_information_sets.size(); i++) {
        InformationSet::P1_hash_to_int_map[P1_information_sets[i]] = i;
    }
    for (int i = 0; i < P2_information_sets.size(); i++) {
        InformationSet::P2_hash_to_int_map[P2_information_sets[i]] = i;
    }


    PolicyVec uniform_policy(player, player == 'x'? P1_information_sets : P2_information_sets, game);
    std::cout << "Initialized uniform policy for player " << player << std::endl;
    build_balanced_exploration_policy_wrapper(uniform_policy, player, game);
    std::cout << "Built balanced exploration policy for player " << player << std::endl;

    if (player == 'x') {
        save_map_txt("P1_balanced_exploration_policy_" + std::string(1, game) + ".txt", uniform_policy.policy_dict, P1_information_sets);
    }
    else {
        save_map_txt("P2_balanced_exploration_policy_" + std::string(1, game) + ".txt", uniform_policy.policy_dict, P2_information_sets);
    }

    return 0;
}