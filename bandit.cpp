#include "cpp_headers/rbt_classes.hpp"
#include "cpp_headers/rbt_utilities.hpp"
#include <random>
#include <cmath>
int NUMBER_THREADS = 4;


void pretty_print(std::chrono::time_point<std::chrono::system_clock> start, std::chrono::time_point<std::chrono::system_clock> end, std::string msg, int flag) {
    if (flag) {
    std::chrono::duration<double> elapsed_seconds = end-start;
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);
    std::cout << "finished " << msg << " in " << elapsed_seconds.count() << "s" << std::endl;
    }
}

int sampleIndex(const std::vector<double>& probabilities) {
    std::random_device rd;
    std::mt19937 generator(rd());
    std::discrete_distribution<int> distribution(probabilities.begin(), probabilities.end());
    return distribution(generator);
}


void explore(InformationSet& I_1, InformationSet& I_2, TicTacToeBoard& true_board, History& current_history, char curr_player, char br_player, PolicyVec& opponent_policy, std::vector<std::vector<int>>& I_a_tickmark, std::vector<int>& I_tickmark, double& reward){
    InformationSet I = br_player == 'x' ? I_1 : I_2;
    int action = 0;
    int terminal_flag = 0;
    
    if (br_player == curr_player){
        std::vector<int> legal_actions;
        I.get_actions(legal_actions);
        std::vector<int> A;
        std::vector<double> prob_dist(13, 0.0); 

        for (int a : legal_actions){
            if (I_a_tickmark[I.get_index()][a] == 0){
                A.push_back(a);
            }
            else {
                I_a_tickmark[I.get_index()][a] = 1;
            }
        }

        for (int a : A){
            prob_dist[a] = 1.0/A.size();
        }

        action = sampleIndex(prob_dist);
    }
    else {
        std::vector<double> prob_dist = opponent_policy.policy_dict[I.get_index()];
        action = sampleIndex(prob_dist);
    }

    if (I.move_flag) {
        bool success = true_board.update_move(action, curr_player);
        current_history.history.push_back(action);

        char winner;
        if (success && !true_board.is_win(winner) && !true_board.is_over()) {
            InformationSet new_I = I;
            new_I.update_move(action, curr_player);
            new_I.reset_zeros();

            if (curr_player == 'x') {
                return explore(new_I, I_2, true_board, current_history, 'o', br_player, opponent_policy, I_a_tickmark, I_tickmark, reward);
            } else {
                return explore(I_1, new_I, true_board, current_history, 'x', br_player, opponent_policy, I_a_tickmark, I_tickmark, reward);
            }
        } else {
            TerminalHistory H_T = TerminalHistory(current_history.history);
            H_T.set_reward();
            reward = br_player == 'x' ? (double) H_T.reward[0] : (double) H_T.reward[1];
            terminal_flag = 1;
        }
    }
    else {
        InformationSet new_I = I;
        new_I.simulate_sense(action, true_board);
        current_history.history.push_back(action);

        if (curr_player == 'x') {
            return explore(new_I, I_2, true_board, current_history, 'x', br_player, opponent_policy, I_a_tickmark, I_tickmark, reward);
        } else {
            return explore(I_1, new_I, true_board, current_history, 'o', br_player, opponent_policy, I_a_tickmark, I_tickmark, reward);
        }
    }


    if (terminal_flag == 1){
        I_a_tickmark[I.get_index()][action] = 1;
    }
    else {
        if (I.move_flag) {
            InformationSet new_I = I;
            new_I.update_move(action, curr_player);
            new_I.reset_zeros();

            if (I_tickmark[new_I.get_index()] == 1){
                I_a_tickmark[I.get_index()][action] = 1;
            }
        }
        else {
            InformationSet new_I = I;
            new_I.simulate_sense(action, true_board);
            if (I_tickmark[new_I.get_index()] == 1){
                I_a_tickmark[I.get_index()][action] = 1;
            }
        }
    }
    std::vector<int> legal_actions;
    I.get_actions(legal_actions);
    int count = 0;
    for (int a : legal_actions){
        if (I_a_tickmark[I.get_index()][a] == 1){
            count += 1;
        }
    }
    if (count == legal_actions.size()){
        I_tickmark[I.get_index()] = 1;
    }
}


void explore_wrapper(std::vector<std::vector<int>>& I_a_tickmark, std::vector<int>& I_tickmark, double& reward, PolicyVec& opponent_policy, History& current_history, char br_player) {
    std::string board = "000000000";
    TicTacToeBoard true_board = TicTacToeBoard(board);
    std::string hash_1 = "";
    std::string hash_2 = "";
    InformationSet I_1 = InformationSet('x', true, hash_1);
    InformationSet I_2 = InformationSet('o', false, hash_2);
    explore(I_1, I_2, true_board, current_history, 'x', br_player, opponent_policy, I_a_tickmark, I_tickmark, reward);
}


void calc_br(PolicyVec& opponent_policy, char br_player, std::vector<std::string>& player_information_sets, long int log_frequency) {
    std::vector<std::vector<int>> I_a_tickmark(player_information_sets.size(), std::vector<int>(13, 0));
    std::vector<int> I_tickmark(player_information_sets.size(), 0);
    int flag = 1;
    long int t = 0;

    while (flag)
    {   std::vector<int> h = {};
        TerminalHistory start_history = TerminalHistory(h);
        double reward = 0.0;
        explore_wrapper(I_a_tickmark, I_tickmark, reward, opponent_policy, start_history, br_player);
        t += 1;

        std::string hash = "";
        InformationSet I = br_player == 'x' ? InformationSet('x', true, hash) : InformationSet('o', false, hash);
        if (I_tickmark[I.get_index()] == 1){
            flag = 0;
        }

        if (t % log_frequency == 0){
            std:: cout << "Number of histories sampled so far" << t << std::endl;
        }
    }

    std::cout << "Total Number of histories sampled for pull each policy once: " << t << std::endl;
}


int main(int argc, char* argv[]) {
    std::cout.precision(17);
    std::string file_path_1 = argv[1];
    std::string file_path_2 = argv[2];

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
    for (long int i = 0; i < P1_information_sets.size(); i++) {
        InformationSet::P1_hash_to_int_map[P1_information_sets[i]] = i;
    }
    for (long int i = 0; i < P2_information_sets.size(); i++) {
        InformationSet::P2_hash_to_int_map[P2_information_sets[i]] = i;
    }

    // load policies
    std::cout << "Loading policies" << std::endl;
    PolicyVec policy_obj_x('x', file_path_1);
    PolicyVec policy_obj_o('o', file_path_2);

    // compute epsilon best response
    char continue_exp = 'y';
    while (continue_exp == 'y') {
        long int log_frequency = 10000;
        char player = 'x';
        std::cout << "Enter log frequency: ";
        std::cin >> log_frequency;
        std::cout << "Enter player for pull arms: ";
        std::cin >> player;

        if (player == 'x') {
            calc_br(policy_obj_o, 'x', P1_information_sets, log_frequency);
        }
        else {
            calc_br(policy_obj_x, 'o', P2_information_sets, log_frequency);
        }

        std::cout << "Continue experiments? (y/n): ";
        std::cin >> continue_exp;
    }
    
}
