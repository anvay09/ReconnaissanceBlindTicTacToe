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

static std::string EMPTY_TABLE = "000"; // Player 1 card {J, Q, K}, Player 2 card {J, Q, K}, Board card {J, Q, K}
static std::string EMPTY_HASH = "";
class Policy;
class PolicyVec;

class PokerTable
{
public:
    std::string cards;
    std::string bid_sequence;
    PokerTable(std::string& cards = EMPTY_TABLE, std::string& bid_sequence = EMPTY_HASH);
    char operator[](int key) const;
    char & operator[](int key);
    void operator=(const PokerTable &other);
    bool operator==(const PokerTable &other);
    PokerTable copy();
    bool is_win(char& winner);
    bool is_over();
    bool is_draw();
    bool is_valid_move(int action);
    bool update_move(int action, char player);
    void print_cards();
};

class InformationSet : public PokerTable
{
public:
    static std::unordered_map<std::string, int > P1_hash_to_int_map;
    static std::unordered_map<std::string, int > P2_hash_to_int_map;
    // static std::unordered_map<int, std::vector<int> > sense_square_dict;
    char player;
    bool move_flag;
    std::string hash;
    int index;
    InformationSet();
    InformationSet(char player, bool move_flag, std::string& hash, std::string& cards);
    InformationSet(char player, bool move_flag, std::string& hash, std::string& cards, int index);
    InformationSet(char player, bool move_flag, std::string& hash = EMPTY_HASH);
    bool operator==(const InformationSet &other);
    char other_player();
    InformationSet copy();
    std::string get_hash();
    std::string get_cards_from_hash();
    int get_index();
    void get_states(std::vector<PokerTable> &states);
    void get_actions(std::vector<int> &actions);
    void get_actions_given_policy(std::vector<int>& actions, PolicyVec& policy_obj);
    void get_actions_given_policy(std::vector<int>& actions, Policy& policy_obj);
    void get_valid_moves(std::vector<int> &actions);
    void get_played_actions(std::vector<int> &actions);
    void simulate_sense(int action, PokerTable& true_cards);
    void reset_zeros();
    void reset_zeros(std::string& cards);
    void get_useful_senses(std::vector<int> &actions);
    bool is_valid_move(int action);
    bool update_move(int action, char player);
    bool is_win_for_player();
    int win_exists();
    int draw_exists();
    bool is_over();
    double get_number_of_actions();
};

class History
{
public:
    std::vector<int> history;
    int track_traversal_index;
    History(std::vector<int>& history);
    char other_player(char player);
    bool get_cards(PokerTable &cards, char& curr_player);
    void get_information_sets(InformationSet& I_1, InformationSet& I_2);
    void print_history();
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

// similar to python's split method
size_t split(const std::string &txt, std::vector<std::string> &strs, char ch);

class PolicyVec
{
public:
    char player;
    std::vector<std::vector<double> > policy_dict;
    PolicyVec();
    PolicyVec(char player, std::vector<std::string> & information_sets);
    PolicyVec(char player, std::string& file_path);
    PolicyVec(char player, std::string& file_path, bool from_txt);
    PolicyVec(char player, std::vector<std::vector<double> >& policy_dict);
    PolicyVec copy();
    std::vector<std::vector<double> > read_policy_from_json(std::string& file_path, char player);
    std::vector<std::vector<double> > read_policy_from_txt(std::string& file_path, char player);
};

#endif // RBT_CLASSES_HPP_