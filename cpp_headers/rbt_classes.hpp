#ifndef RBT_CLASSES_HPP
#define RBT_CLASSES_HPP

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <vector>
#include <algorithm>
#include <string>
#include <set>
#include <iterator>
#include <utility>
#include <tuple>
#include <cassert>
#include <chrono>
#include <ctime>

static std::string EMPTY_BOARD = "000000000";
static std::string EMPTY_HASH = "";
class Policy;
class PolicyVec;

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
    static std::unordered_map<std::string, long int > P1_hash_to_int_map;
    static std::unordered_map<std::string, long int > P2_hash_to_int_map;
    // static std::unordered_map<int, std::vector<int> > sense_square_dict;
    char player;
    bool move_flag;
    std::string hash;
    long int index;
    InformationSet();
    InformationSet(char player, bool move_flag, std::string& hash, std::string& board);
    InformationSet(char player, bool move_flag, std::string& hash, std::string& board, long int index);
    InformationSet(char player, bool move_flag, std::string& hash = EMPTY_HASH);
    bool operator==(const InformationSet &other);
    char other_player();
    InformationSet copy();
    std::string get_hash();
    std::string get_v1_hash();
    std::string get_board_from_hash();
    long int get_index();
    void get_states(std::vector<TicTacToeBoard> &states);
    void get_actions(std::vector<int> &actions);
    void get_actions_given_policy(std::vector<int>& actions, PolicyVec& policy_obj);
    void get_actions_given_policy(std::vector<int>& actions, Policy& policy_obj);
    void get_valid_moves(std::vector<int> &actions);
    void get_played_actions(std::vector<int> &actions);
    void simulate_sense(int action, TicTacToeBoard& true_board);
    void reset_zeros();
    void reset_zeros(std::string& board);
    void get_useful_senses(std::vector<int> &actions);
    bool is_valid_move(int square);
    bool update_move(int square, char player);
    bool is_win_for_player();
    int win_exists();
    int draw_exists();
    bool is_over();
};

class History
{
public:
    std::vector<int> history;
    int track_traversal_index;
    History(std::vector<int>& history);
    char other_player(char player);
    bool get_board(TicTacToeBoard &board, char& curr_player);
    void get_information_sets(InformationSet& I_1, InformationSet& I_2);
};

class TerminalHistory : public History
{
public:
    std::vector<double> reward;
    TerminalHistory(std::vector<int>& history, std::vector<double> reward = {0.0, 0.0});
    TerminalHistory copy();
    void set_reward();
};

class NonTerminalHistory : public History
{
public:
    NonTerminalHistory(std::vector<int>& history);
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
    std::unordered_map<std::string, std::vector<double> > read_policy_from_json(std::string& file_path);
};

// similar to python's split method
void split(std::string str, std::string splitBy, std::vector<std::string>& tokens);
// intersection of two sets
std::vector<int> intersection(std::vector<int> const& left_vector, std::vector<int> const& right_vector);


class PolicyVec
{
public:
    char player;
    std::vector<std::vector<double> > policy_dict;
    PolicyVec();
    PolicyVec(char player, std::string& file_path);
    PolicyVec(char player, std::vector<std::vector<double> >& policy_dict);
    PolicyVec copy();
    std::vector<std::vector<double> > read_policy_from_json(std::string& file_path, char player);
};

#endif // RBT_CLASSES_HPP_