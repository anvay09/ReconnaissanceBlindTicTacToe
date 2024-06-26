#include "rbt_classes.hpp"

std::vector<std::tuple<TerminalHistory, double, int>> terminal_histories;

double get_expected_utility(InformationSet &I_1, InformationSet &I_2, TicTacToeBoard &true_board, char player, Policy &policy_obj_x, Policy &policy_obj_o, double probability, History& current_history, char initial_player, int save_games) {
    double expected_utility_h = 0;
    
    InformationSet& I = player == 'x' ? I_1 : I_2;
    Policy& policy_obj = player == 'x' ? policy_obj_x : policy_obj_o;
    
    std::vector<int> actions;
    I.get_actions_given_policy(actions, policy_obj);
    
    if (I.move_flag) {
        for (int action : actions) {
            TicTacToeBoard new_true_board = true_board;
            bool success = new_true_board.update_move(action, player);

            double probability_new = probability * policy_obj.policy_dict[I.get_hash()][action];
            History new_history = current_history;
            new_history.history.push_back(action);
                
            char winner;
            if (success && !new_true_board.is_win(winner) && !new_true_board.is_over()) {
                InformationSet new_I(I);
                new_I.update_move(action, player);
                new_I.reset_zeros();
                
                if (player == 'x') {
                    expected_utility_h += get_expected_utility(new_I, I_2, new_true_board, 'o', policy_obj_x, policy_obj_o, probability_new, new_history, initial_player, save_games);
                } else {
                    expected_utility_h += get_expected_utility(I_1, new_I, new_true_board, 'x', policy_obj_x, policy_obj_o, probability_new, new_history, initial_player, save_games);
                }
            } else {
                TerminalHistory H_T = TerminalHistory(new_history.history);
                H_T.set_reward();
                if (initial_player == 'x'){
                    if (save_games){
                        terminal_histories.push_back(std::make_tuple(H_T, probability_new, H_T.reward[0]));
                    }
                    expected_utility_h += H_T.reward[0] * probability_new;
                }
                else{
                    if (save_games){
                        terminal_histories.push_back(std::make_tuple(H_T, probability_new, H_T.reward[1]));
                    }
                    expected_utility_h += H_T.reward[1] * probability_new;
                }
            }
        }
    } else {
        for (int action : actions) {
            InformationSet new_I(I);
            new_I.simulate_sense(action, true_board);
            
            double probability_new = probability * policy_obj.policy_dict[I.get_hash()][action];
            History new_history = current_history;
            new_history.history.push_back(action);

            if (player == 'x') {
                expected_utility_h += get_expected_utility(new_I, I_2, true_board, 'x', policy_obj_x, policy_obj_o, probability_new, new_history, initial_player, save_games);
            } else {
                expected_utility_h += get_expected_utility(I_1, new_I, true_board, 'o', policy_obj_x, policy_obj_o, probability_new, new_history, initial_player, save_games);
            }
        }
    }

    return expected_utility_h;
}


int main(int argc, char** argv) {
    std::string board = "000000000";
    TicTacToeBoard true_board = TicTacToeBoard(board);
    std::string board_1 = "000000000";
    std::string board_2 = "---------";
    InformationSet I_1 = InformationSet('x', true, board_1);
    InformationSet I_2 = InformationSet('o', false, board_2);

    std::string file_path_1 = argv[1];
    std::string file_path_2 = argv[2];
    int round = std::stoi(argv[3]);
    int save_games = std::stoi(argv[4]);
    
    char player = 'x';
    Policy policy_obj_x('x', file_path_1);
    Policy policy_obj_o('o', file_path_2);

    std::vector<int> h = {};
    TerminalHistory start_history = TerminalHistory(h);

    std::cout << "Getting expected utility..." << std::endl;
    auto start = std::chrono::system_clock::now();   
    
    double expected_utility = get_expected_utility(I_1, I_2, true_board, player, policy_obj_x, policy_obj_o, 1, start_history, player, save_games);
    std::cout << "Expected utility: " << expected_utility << std::endl;

    // Save expected utility to file
    std::ofstream file;
    
    file.open("round_" + std::to_string(round) + "_expected_utility.txt");
    file << expected_utility;
  
    if (save_games) {
        std::cout << "Number of terminal histories: " << terminal_histories.size() << std::endl;
    }
    
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end-start;
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);
 
    std::cout << "finished computation at " << std::ctime(&end_time)
              << "elapsed time: " << elapsed_seconds.count() << "s"
              << std::endl;
    return 0;
}
