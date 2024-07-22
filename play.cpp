#include "cpp_headers/rbt_classes.hpp"

int play(InformationSet &I_1, InformationSet &I_2, TicTacToeBoard &true_board, std::unordered_set<std::string>& I_1_set, std::unordered_set<std::string>& I_2_set, char player) {
    long int num_histories = 0;
    
    I_1_set.insert(I_1.get_hash());
    I_2_set.insert(I_2.get_hash());

    InformationSet& I = player == 'x' ? I_1 : I_2;
    
    std::vector<int> actions;
    I.get_actions(actions);
    
    if (I.move_flag) {
        for (int action : actions) {
            TicTacToeBoard new_true_board = true_board;
            bool success = new_true_board.update_move(action, player);

            char winner;
            if (success && !new_true_board.is_win(winner) && !new_true_board.is_over()) {
                InformationSet new_I(I);
                new_I.update_move(action, player);
                new_I.reset_zeros();
                
                if (player == 'x') {
                    num_histories += play(new_I, I_2, new_true_board, I_1_set, I_2_set, 'o');
                } else {
                    num_histories += play(I_1, new_I, new_true_board, I_1_set, I_2_set, 'x');
                }
            } else {
                num_histories += 1;
            }
        }
    } else {
        for (int action : actions) {
            InformationSet new_I(I);
            new_I.simulate_sense(action, true_board);
            TicTacToeBoard new_true_board = true_board;

            if (player == 'x') {
                num_histories += play(new_I, I_2, new_true_board, I_1_set, I_2_set, 'x');
            } else {
                num_histories += play(I_1, new_I, new_true_board, I_1_set, I_2_set, 'o');
            }
        }
    }

    return num_histories;
}


int main() {
    std::string board = "000000000";
    TicTacToeBoard true_board = TicTacToeBoard(board);
    std::string hash_1 = "";
    std::string hash_2 = "";
    InformationSet I_1 = InformationSet('x', true, hash_1);
    InformationSet I_2 = InformationSet('o', false, hash_2);
    std::unordered_set<std::string> I_1_set;
    std::unordered_set<std::string> I_2_set;
    char player = 'x';

    auto start = std::chrono::system_clock::now();   
    long int output = play(I_1, I_2, true_board, I_1_set, I_2_set, player);
    std::cout << output << std::endl;
    std::cout << I_1_set.size() << std::endl;
    std::cout << I_2_set.size() << std::endl;

    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end-start;
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);
 
    std::cout << "finished computation at " << std::ctime(&end_time)
              << "elapsed time: " << elapsed_seconds.count() << "s"
              << std::endl;

    // write I_1_set and I_2_set to file
    std::ofstream I_1_file;
    I_1_file.open("data/P1_information_sets_V2.txt");
    for (std::string hash : I_1_set) {
        I_1_file << hash << std::endl;
    }
    I_1_file.close();

    std::ofstream I_2_file;
    I_2_file.open("data/P2_information_sets_V2.txt");
    for (std::string hash : I_2_set) {
        I_2_file << hash << std::endl;
    }
    I_2_file.close();
    return 0;
}
