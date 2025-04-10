#ifndef POKER_CLASSES_HPP
#define POKER_CLASSES_HPP

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
#include "Eigen/Dense"
#include <functional>

static std::string EMPTY_TABLE = "---"; // Player 1 card {J, Q, K}, Player 2 card {J, Q, K}, Board card {J, Q, K}
static std::string EMPTY_HASH = "";
static double LEDUC_MIN_UTILITY = -13.0;
static double LEDUC_MAX_UTILITY = 13.0;
static double KUHN_MIN_UTILITY = -2.0;
static double KUHN_MAX_UTILITY = 2.0;

static std::vector<std::string> unique_draws_leduc = {"JJQ", "JQJ", "QJJ", "QQJ", "QJQ", "JQQ", 
                                                      "KKJ", "KJK", "JKK", "KKQ", "KQK", "QKK", 
                                                      "QQK", "QKQ", "KQQ", "JJK", "JKJ", "KJJ",
                                                      "JQK", "JKQ", "QJK", "QKJ", "KJQ", "KQJ"};
static double p = 1.0/30.0;
static std::vector<double> draw_probabilities_leduc = {p, p, p, p, p, p,
                                                       p, p, p, p, p, p,
                                                       p, p, p, p, p, p,
                                                       2*p, 2*p, 2*p, 2*p, 2*p, 2*p};

static std::vector<std::string> unique_draws_kuhn = {"JQK", "JKQ", "QJK", "QKJ", "KJQ", "KQJ"};
static std::vector<double> draw_probabilities_kuhn = {1.0/6.0, 1.0/6.0, 1.0/6.0, 1.0/6.0, 1.0/6.0, 1.0/6.0};

class Policy;
class PolicyVec;

class PokerTable
{
public:
    std::string cards;
    std::string bid_sequence;
    char player_to_move;
    char game;
    PokerTable(std::string& cards = EMPTY_TABLE, std::string& bid_sequence = EMPTY_HASH, char player = 'x', char game = 'L');
    char operator[](int key) const;
    char & operator[](int key);
    void operator=(const PokerTable &other);
    bool operator==(const PokerTable &other);
    PokerTable copy();
    bool is_win(char& winner);
    bool is_over();
    bool is_draw();
    bool is_valid_move(int action);
    bool update_move(int action);
    void print_cards();
};

class InformationSet : public PokerTable
{
public:
    static std::unordered_map<std::string, int > P1_hash_to_int_map;
    static std::unordered_map<std::string, int > P2_hash_to_int_map;

    char player;
    bool move_flag;
    std::string hash;
    int index;
    InformationSet();
    InformationSet(char player, bool move_flag, std::string& hash, std::string& cards, char game = 'L');
    InformationSet(char player, bool move_flag, std::string& hash, std::string& cards, int index, char game = 'L');
    InformationSet(char player, bool move_flag, std::string& hash = EMPTY_HASH, char game = 'L');
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
    void simulate_sense(int action, PokerTable& true_cards);
    void get_useful_senses(std::vector<int> &actions);
    bool is_valid_move(int action);
    bool update_move(int action);
    bool is_over();
};

class History
{
public:
    std::vector<int> history;
    int track_traversal_index;
    History(std::vector<int>& history);
    char other_player(char player);
    std::vector<double> update_true_cards_given_history(PokerTable &true_cards);
    void get_information_sets(InformationSet& I_1, InformationSet& I_2);
    void print_history();
};

class TerminalHistory : public History
{
public:
    std::vector<double> reward;
    TerminalHistory(std::vector<int>& history, std::vector<double> reward = {0.0, 0.0});
    TerminalHistory copy();
    void set_reward(char game);
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
    PolicyVec(char player, std::vector<std::string> & information_sets, char game);
    PolicyVec(char player, std::string& file_path, char game);
    PolicyVec(char player, std::string& file_path, char game, bool from_txt);
    PolicyVec(char player, std::vector<std::vector<double> >& policy_dict);
    PolicyVec copy();
    std::vector<std::vector<double> > read_policy_from_json(std::string& file_path, char player, char game);
    std::vector<std::vector<double> > read_policy_from_txt(std::string& file_path, char player, char game);
};


class Sequence
{
    public:
        std::vector<std::pair<std::string, int>> seq;
        double r; // total empirical reward
        double p; // total empirical probability, p = n / n_pi
        double ucb_r; // upper confidence bound on reward
        double ucb_p; // upper confidence bound on probability
        int n; // number of times this sequence has been sampled
        int n_pi; // number of times a policy that could have generated this sequence has been sampled
        std::string hash; // a sequence can be uniquely identified by the last (I, action) pair
        Sequence();
        Sequence(std::vector<std::pair<std::string, int>> seq, double r, double p, double ucb_r, double ucb_p, int n, int n_pi, std::string hash);
        void operator=(const Sequence &other);
        bool operator==(const Sequence &other);
        void update_hash();
        void extend(InformationSet& I, int action);
        void pop_back();
};

#endif // POKER_CLASSES_HPP_