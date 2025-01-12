#include "cpp_headers/rbt_classes.hpp"
#include "cpp_headers/rbt_utilities.hpp"

// g++-13 -O3 count_terminal_infosets.cpp rbt_classes.cpp rbt_utilities.cpp -o count_terminal_infosets


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


int get_number_of_unknown_opponent_moves(InformationSet& I) {
    std::string B = I.get_board_from_hash();
    int count_x = 0;
    int count_o = 0;
    for (int i = 0; i < 9; i++) {
        if (B[i] == 'x') {
            count_x++;
        }
        if (B[i] == 'o') {
            count_o++;
        }
    }
    if (I.player == 'x') {
        return count_x - count_o;
    } else {
        return count_o - count_x + 1;
    }
}


void get_uncertain_squares(InformationSet& I, std::vector<int> &squares) {
    std::string B = I.get_board_from_hash();
    for (int i = 0; i < 9; i++) {
        if (B[i] == '-') {
            squares.push_back(i);
        }
    }
}


void get_states_in_infoset(InformationSet &I, std::vector<TicTacToeBoard> &states) {
    int num_unknown_opponent_moves = get_number_of_unknown_opponent_moves(I);
    std::string board_copy = I.get_board_from_hash();

    for (int i = 0; i < 9; i++) {
        if (board_copy[i] == '-') {
            board_copy[i] = '0';
        }
    }

    if (num_unknown_opponent_moves == 0) {
        states.push_back(TicTacToeBoard(board_copy));
    } 
    else {
        std::vector<int> uncertain_ind;
        get_uncertain_squares(I, uncertain_ind);

        std::vector<char> base_perm(num_unknown_opponent_moves, I.other_player());
        base_perm.insert(base_perm.end(), uncertain_ind.size() - num_unknown_opponent_moves, '0');
        // sort 
        std::sort(base_perm.begin(), base_perm.end());

        do {
            TicTacToeBoard new_state(board_copy);
            for (int j = 0; j < base_perm.size(); j++) {
                new_state[uncertain_ind[j]] = base_perm[j];
            }
            char winner;
            if (!new_state.is_win(winner) && !new_state.is_over()) {
                states.push_back(new_state);
            }
        } while (std::next_permutation(base_perm.begin(), base_perm.end()));

    }
}


void get_cohort(InformationSet I, int action, std::unordered_set<std::string> &cohort) {
    if (I.move_flag) {
        I.update_move(action, I.player);
        I.reset_zeros();
        if (I.get_index() != -1) {
            cohort.insert(I.get_hash());
        }
        return;
    }
    else {
        std::vector<TicTacToeBoard> states;
        get_states_in_infoset(I, states);
        std::cout << "States in infoset: " << states.size() << std::endl;
        for (TicTacToeBoard &state : states) {
            InformationSet new_I = I;
            new_I.simulate_sense(action, state);
            if (new_I.get_index() != -1) {
                if (cohort.find(new_I.get_hash()) == cohort.end()) {
                    cohort.insert(new_I.get_hash());
                }
            }
        }
    }
}


int build_balanced_exploration_policy(PolicyVec& policy_obj, InformationSet& I) {
    std::vector<int> legal_actions;
    I.get_actions(legal_actions);
    std::vector<int> action_tree_size(13, 0);
    
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


void build_balanced_exploration_policy_wrapper(PolicyVec& policy_obj, char player){
    std::string board = "000000000";
    TicTacToeBoard true_board = TicTacToeBoard(board);
    std::string hash = "";
    InformationSet I = InformationSet(player, player == 'x', hash);
    
    build_balanced_exploration_policy(policy_obj, I);
}


int main(int argc, char** argv) {
    char player = argv[1][0];

    // load information sets
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

    // create hash to int maps
    for (int i = 0; i < P1_information_sets.size(); i++) {
        InformationSet::P1_hash_to_int_map[P1_information_sets[i]] = i;
    }
    for (int i = 0; i < P2_information_sets.size(); i++) {
        InformationSet::P2_hash_to_int_map[P2_information_sets[i]] = i;
    }

    PolicyVec uniform_policy(player, player == 'x'? P1_information_sets : P2_information_sets);
    std::cout << "Initialized uniform policy for player " << player << std::endl;
    build_balanced_exploration_policy_wrapper(uniform_policy, player);
    std::cout << "Built balanced exploration policy for player " << player << std::endl;

    if (player == 'x') {
        save_output("P1_balanced_exploration_policy.txt", player, P1_information_sets, uniform_policy);
    }
    else {
        save_output("P2_balanced_exploration_policy.txt", player, P2_information_sets, uniform_policy);
    }

    return 0;
}
