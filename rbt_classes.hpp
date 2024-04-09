#ifndef RBT_CLASSES_HPP
#define RBT_CLASSES_HPP

#include <iostream>
#include <unordered_map>
#include <map>
#include <vector>
#include <algorithm>
#include <string>
#include <set>
#include <iterator>
#include <utility>
#include <tuple>


static std::string EMPTY_BOARD = "000000000";
static std::unordered_map<int, std::vector<int> > sense_square_dict = {{9, {0, 1, 3, 4}}, {10, {1, 2, 4, 5}}, {11, {3, 4, 6, 7}}, {12, {4, 5, 7, 8}}};
class Policy;

class TicTacToeBoard
{
public:
    std::string board;
    TicTacToeBoard(std::string& board = EMPTY_BOARD);
    char operator[](int key) const;
    char & operator[](int key);
    void operator=(const TicTacToeBoard &other);
    bool operator==(const TicTacToeBoard &other);
    TicTacToeBoard copy();
    bool is_win(char& winner);
    bool is_over();
    bool is_draw();
    bool is_valid_move(int square);
    bool update_move(int square, char player);
    void print_board();
};

class InformationSet : public TicTacToeBoard
{
public:
    char player;
    bool move_flag;
    InformationSet();
    InformationSet(char player, bool move_flag, std::string& board = EMPTY_BOARD);
    bool operator==(const InformationSet &other);
    char other_player();
    InformationSet copy();
    std::string get_hash();
    void get_states(std::vector<TicTacToeBoard> &states);
    void get_actions(std::vector<int> &actions);
    void get_actions_given_policy(std::vector<int>& actions, Policy& policy_obj);
    void get_valid_moves(std::vector<int> &actions);
    void get_played_actions(std::vector<int> &actions);
    void get_useful_senses(std::vector<int> &actions);
    int get_number_of_unknown_opponent_moves();
    void get_uncertain_squares(std::vector<int> &squares);
    void simulate_sense(int action, TicTacToeBoard& true_board);
    void reset_zeros();
    bool is_valid_move(int square);
    bool update_move(int square, char player);
    bool is_win_for_player();
    int win_exists();
    int draw_exists();
    bool is_over();
    int num_self_moves();
};

class History
{
public:
    std::vector<int> history;
    int track_traversal_index;
    History(std::vector<int> history);
    char other_player(char player);
    bool get_board(TicTacToeBoard &board, char& curr_player);
    std::pair<InformationSet, InformationSet> get_information_sets();
};

class TerminalHistory : public History
{
public:
    std::vector<int> reward;
    TerminalHistory(std::vector<int> history, std::vector<int> reward = {0, 0});
    TerminalHistory copy();
    void set_reward();
};

class NonTerminalHistory : public History
{
public:
    NonTerminalHistory(std::vector<int> history);
    NonTerminalHistory copy();
};

class Policy
{
public:
    char player;
    std::unordered_map<std::string, std::vector<double> > policy_dict;
    Policy();
    Policy(char player, std::string& file_path);
    Policy(char player, std::unordered_map<std::string, std::vector<double> >& policy_dict);
    Policy copy();
    void update_policy_for_given_information_set(InformationSet& information_set, std::vector<double>& prob_distribution);
    std::unordered_map<std::string, std::vector<double> > read_policy_from_json(std::string& file_path);
};

#endif // RBT_CLASSES_HPP_