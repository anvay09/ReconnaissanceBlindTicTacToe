#include "cpp_headers/rbt_classes.hpp"
#include "cpp_headers/json.hpp"
using json = nlohmann::json;

// g++-13 -O3 rbt_classes.cpp count_symmetry.cpp -o count_symmetry

std::string rotate90(std::string& board)
{
    std::string new_board = board;
    new_board[0] = board[6];
    new_board[1] = board[3];
    new_board[2] = board[0];
    new_board[3] = board[7];
    new_board[5] = board[1];
    new_board[6] = board[8];
    new_board[7] = board[5];
    new_board[8] = board[2];

    return new_board;
}

std::string rotate180(std::string& board)
{
    std::string new_board = board;
    new_board[0] = board[8];
    new_board[1] = board[7];
    new_board[2] = board[6];
    new_board[3] = board[5];
    new_board[5] = board[3];
    new_board[6] = board[2];
    new_board[7] = board[1];
    new_board[8] = board[0];

    return new_board;
}

std::string rotate270(std::string& board)
{
    std::string new_board = board;
    new_board[0] = board[2];
    new_board[1] = board[5];
    new_board[2] = board[8];
    new_board[3] = board[1];
    new_board[5] = board[7];
    new_board[6] = board[0];
    new_board[7] = board[3];
    new_board[8] = board[6];

    return new_board;
}

std::string flip_horizontal(std::string& board)
{
    std::string new_board = board;
    new_board[0] = board[2];
    new_board[2] = board[0];
    new_board[3] = board[5];
    new_board[5] = board[3];
    new_board[6] = board[8];
    new_board[8] = board[6];

    return new_board;
}

std::string flip_vertical(std::string& board)
{
    std::string new_board = board;
    new_board[0] = board[6];
    new_board[1] = board[7];
    new_board[2] = board[8];
    new_board[6] = board[0];
    new_board[7] = board[1];
    new_board[8] = board[2];

    return new_board;
}

std::string flip_diagonal(std::string& board)
{
    std::string new_board = board;
    new_board[1] = board[3];
    new_board[3] = board[1];
    new_board[2] = board[6];
    new_board[6] = board[2];
    new_board[5] = board[7];
    new_board[7] = board[5];

    return new_board;
}

std::string flip_antidiagonal(std::string& board)
{
    std::string new_board = board;
    new_board[0] = board[8];
    new_board[8] = board[0];
    new_board[1] = board[5];
    new_board[5] = board[1];
    new_board[2] = board[3];
    new_board[3] = board[2];

    return new_board;
}


std::string maximise_information(std::string& board, char player)
{
    std::string new_board = board;
    if (board[9] == 'm') {
        int countx = 0;
        int counto = 0;
        int countdash = 0;
        for (int i = 0; i < 9; i++) {
            if (board[i] == 'x') {
                countx++;
            } else if (board[i] == 'o') {
                counto++;
            }
            else if (board[i] == '-') {
                countdash++;
            }
        }

        if ((player == 'x' && countx == counto) || (player == 'o' && countx == counto + 1)) {
            for (int i = 0; i < 9; i++) {
                if (board[i] == '-') {
                    new_board[i] = '0';
                }
            }
        } else if (player == 'x' && countx == counto + countdash) {
            for (int i = 0; i < 9; i++) {
                if (board[i] == '-') {
                    new_board[i] = 'o';
                }
            }
        } else if (player == 'o' && countx + countdash == counto + 1) {
            for (int i = 0; i < 9; i++) {
                if (board[i] == '-') {
                    new_board[i] = 'x';
                }
            }
        }
    }
    return new_board;
}


int main(int argc, char *argv[])
{
    std::string P1_information_sets_file = "data/P1_information_sets.txt";
    std::string P2_information_sets_file = "data/P2_information_sets.txt";
    
    std::vector<std::string> P1_information_sets;
    std::vector<std::string> P2_information_sets;

    std::ifstream f1(P1_information_sets_file);
        std::string line;
        
    while (std::getline(f1, line)) {
        P1_information_sets.push_back(line);
        std::vector<double> regret_vector;
        for (int i = 0; i < 13; i++) {
            regret_vector.push_back(0.0);
        }
    }
    f1.close();

    std::ifstream f2(P2_information_sets_file);
    while (std::getline(f2, line)) {
        P2_information_sets.push_back(line);
        std::vector<double> regret_vector;
        for (int i = 0; i < 13; i++) {
            regret_vector.push_back(0.0);
        }
    }
    f2.close();

    std::unordered_set<std::string> P1_unique_information_sets;
    std::unordered_set<std::string> P2_unique_information_sets;

    P1_unique_information_sets.insert(maximise_information(P1_information_sets[0], 'x'));
    P2_unique_information_sets.insert(maximise_information(P2_information_sets[0], 'o'));

    for (std::string& information_set : P1_information_sets) {
        std::vector<std::string> symm_cases(20);

        information_set = maximise_information(information_set, 'x');
        
        symm_cases[0] = information_set;
        symm_cases[1] = rotate90(information_set);
        symm_cases[2] = rotate180(information_set);
        symm_cases[3] = rotate270(information_set);
        symm_cases[4] = flip_horizontal(information_set);
        symm_cases[5] = rotate90(symm_cases[4]);
        symm_cases[6] = rotate180(symm_cases[4]);
        symm_cases[7] = rotate270(symm_cases[4]);
        symm_cases[8] = flip_vertical(information_set);
        symm_cases[9] = rotate90(symm_cases[8]);
        symm_cases[10] = rotate180(symm_cases[8]);
        symm_cases[11] = rotate270(symm_cases[8]);
        symm_cases[12] = flip_diagonal(information_set);
        symm_cases[13] = rotate90(symm_cases[12]);
        symm_cases[14] = rotate180(symm_cases[12]);
        symm_cases[15] = rotate270(symm_cases[12]);
        symm_cases[16] = flip_antidiagonal(information_set);
        symm_cases[17] = rotate90(symm_cases[16]);
        symm_cases[18] = rotate180(symm_cases[16]);
        symm_cases[19] = rotate270(symm_cases[16]);
        
        bool flag = false;
        for (std::string& c : symm_cases) {
            if (P1_unique_information_sets.find(c) != P1_unique_information_sets.end()) {
                flag = true;
                break;
            }
        }
        if (!flag) {
            P1_unique_information_sets.insert(information_set);
        }
    }

    for (std::string& information_set : P2_information_sets) {
        std::vector<std::string> symm_cases(20);

        information_set = maximise_information(information_set, 'o');
        
        symm_cases[0] = information_set;
        symm_cases[1] = rotate90(information_set);
        symm_cases[2] = rotate180(information_set);
        symm_cases[3] = rotate270(information_set);
        symm_cases[4] = flip_horizontal(information_set);
        symm_cases[5] = rotate90(symm_cases[4]);
        symm_cases[6] = rotate180(symm_cases[4]);
        symm_cases[7] = rotate270(symm_cases[4]);
        symm_cases[8] = flip_vertical(information_set);
        symm_cases[9] = rotate90(symm_cases[8]);
        symm_cases[10] = rotate180(symm_cases[8]);
        symm_cases[11] = rotate270(symm_cases[8]);
        symm_cases[12] = flip_diagonal(information_set);
        symm_cases[13] = rotate90(symm_cases[12]);
        symm_cases[14] = rotate180(symm_cases[12]);
        symm_cases[15] = rotate270(symm_cases[12]);
        symm_cases[16] = flip_antidiagonal(information_set);
        symm_cases[17] = rotate90(symm_cases[16]);
        symm_cases[18] = rotate180(symm_cases[16]);
        symm_cases[19] = rotate270(symm_cases[16]);
        
        bool flag = false;
        for (std::string& c : symm_cases) {
            if (P2_unique_information_sets.find(c) != P2_unique_information_sets.end()) {
                flag = true;
                break;
            }
        }
        if (!flag) {
            P2_unique_information_sets.insert(information_set);
        }
    }

    std::cout << "P1 unique information sets: " << P1_unique_information_sets.size() << std::endl;
    std::cout << "P2 unique information sets: " << P2_unique_information_sets.size() << std::endl;

    // write to file

    std::ofstream f3("data/P1_unique_information_sets.txt");
    for (auto i = P1_unique_information_sets.begin(); i != P1_unique_information_sets.end(); ++i) {
        f3 << *i << std::endl;
    }
    f3.close();

    std::ofstream f4("data/P2_unique_information_sets.txt");
    for (auto i = P2_unique_information_sets.begin(); i != P2_unique_information_sets.end(); ++i) {
        f4 << *i << std::endl;
    }

    return 0;
}