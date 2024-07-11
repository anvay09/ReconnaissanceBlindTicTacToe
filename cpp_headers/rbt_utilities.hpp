#include "rbt_classes.hpp"
#include "json.hpp"
using json = nlohmann::json;

char toggle_player(char player);

void valid_histories_play(InformationSet& I_1, InformationSet& I_2, TicTacToeBoard& true_board, char player, History& current_history, InformationSet& end_I, std::vector<int>& played_actions, Policy& policy_obj_x, Policy& policy_obj_o, std::vector<std::vector<int>>& valid_histories_list);

void upgraded_get_histories_given_I(InformationSet& I, Policy& policy_obj_x, Policy& policy_obj_o, std::vector<std::vector<int>>& valid_histories_list);

double get_expected_utility(InformationSet &I_1, InformationSet &I_2, TicTacToeBoard &true_board, char player, Policy &policy_obj_x, Policy &policy_obj_o, double probability, History& current_history, char initial_player);

double get_expected_utility_parallel(InformationSet &I_1, InformationSet &I_2, TicTacToeBoard &true_board, char player, Policy &policy_obj_x, Policy &policy_obj_o, double probability, History& current_history, char initial_player);

double get_expected_utility_wrapper(Policy& policy_obj_x, Policy& policy_obj_o);

double get_prob_h_given_policy(InformationSet& I_1, InformationSet& I_2, TicTacToeBoard& true_board, char player, int next_action, Policy& policy_obj_x, Policy& policy_obj_o, double probability, History history_obj, char initial_player);
    
double get_prob_h_given_policy_wrapper(InformationSet& I_1, InformationSet& I_2, TicTacToeBoard& true_board, char player, int next_action, Policy& policy_obj_x, Policy& policy_obj_o, double probability, History history_obj, InformationSet& curr_I_1, char initial_player);
    
double get_counter_factual_utility(InformationSet& I, Policy& policy_obj_x, Policy& policy_obj_o, std::vector<std::vector<int>>& starting_histories, std::vector<double>& prob_reaching_h_list);

void get_probability_of_reaching_all_h(InformationSet& I, Policy& policy_obj_x, Policy& policy_obj_o, std::vector<std::vector<int>>& starting_histories, char initial_player, std::vector<double>& prob_reaching_h_list_all);

double calc_util_a_given_I_and_action(InformationSet& I, int action, Policy& policy_obj_x, Policy& policy_obj_o, std::vector<std::vector<int>>& starting_histories, std::vector<double>& prob_reaching_h_list);

void calc_cfr_policy_given_I(InformationSet& I, Policy& policy_obj_x, Policy& policy_obj_o, int T, std::vector<double>& regret_list);

std::unordered_map<std::string, std::vector<double> > get_prev_regrets(std::string& file_path, char player);