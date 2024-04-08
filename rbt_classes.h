#include <iostream>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <string>
#include <set>
#include <iterator>
#include <utility>
#include <tuple>

using namespace std;

static string EMPTY_BOARD = "000000000";
static unordered_map<int, vector<int> > sense_square_dict = {{9, {0, 1, 3, 4}}, {10, {1, 2, 4, 5}}, {11, {3, 4, 6, 7}}, {12, {4, 5, 7, 8}}};
class Policy;

class TicTacToeBoard
{
public:
    string board;
    TicTacToeBoard(string& board = EMPTY_BOARD);
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
    InformationSet(char player, bool move_flag, string& board = EMPTY_BOARD);
    bool operator==(const InformationSet &other);
    char other_player();
    InformationSet copy();
    string get_hash();
    void get_states(vector<TicTacToeBoard> &states);
    void get_actions(vector<int> &actions);
    void get_actions_given_policy(vector<int>& actions, Policy& policy_obj);
    void get_valid_moves(vector<int> &actions);
    void get_played_actions(vector<int> &actions);
    void get_useful_senses(vector<int> &actions);
    int get_number_of_unknown_opponent_moves();
    void get_uncertain_squares(vector<int> &squares);
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
    vector<int> history;
    int track_traversal_index;
    History(vector<int> history);
    char other_player(char player);
    bool get_board(TicTacToeBoard &board, char& curr_player);
    pair<InformationSet, InformationSet> get_information_sets();
};

class TerminalHistory : public History
{
public:
    vector<int> reward;
    TerminalHistory(vector<int> history, vector<int> reward = {0, 0});
    TerminalHistory copy();
    void set_reward();
};

class NonTerminalHistory : public History
{
public:
    NonTerminalHistory(vector<int> history);
    NonTerminalHistory copy();
};

class Policy
{
public:
    char player;
    unordered_map<string, unordered_map<int, double> > policy_dict;
    Policy(char player, string& file_path);
    Policy(char player, unordered_map<string, unordered_map<int, double> >& policy_dict);
    Policy copy();
    void update_policy_for_given_information_set(InformationSet& information_set, vector<double>& prob_distribution);
    unordered_map<string, unordered_map<int, double> > read_policy_from_json(string& file_path);
};
