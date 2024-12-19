#include "poker_classes.hpp"
#include "json.hpp"
using json = nlohmann::json;

char toggle_player(char player);

void valid_histories_play(InformationSet& I_1, InformationSet& I_2, PokerTable& true_cards, char player, History& current_history, InformationSet& end_I, std::vector<int>& played_actions, int current_action_index, PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, std::vector<std::vector<int>>& valid_histories_list);

void upgraded_get_histories_given_I(InformationSet& I, PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, std::vector<std::vector<int>>& valid_histories_list);

double get_expected_utility(InformationSet &I_1, InformationSet &I_2, PokerTable &true_cards, char player, PolicyVec &policy_obj_x, PolicyVec &policy_obj_o, double probability, History& current_history, char initial_player);

double get_expected_utility_parallel(InformationSet &I_1, InformationSet &I_2, PokerTable &true_cards, char player, PolicyVec &policy_obj_x, PolicyVec &policy_obj_o, double probability, History& current_history, char initial_player);

double get_expected_utility_wrapper(PolicyVec& policy_obj_x, PolicyVec& policy_obj_o);

double get_prob_h_given_policy(InformationSet& I_1, InformationSet& I_2, PokerTable& true_cards, char player, int next_action, PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, double probability, History history_obj, char initial_player, InformationSet& end_I);
    
double get_prob_h_given_policy_wrapper(InformationSet& I_1, InformationSet& I_2, PokerTable& true_cards, char player, int next_action, PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, double probability, History history_obj, InformationSet& curr_I_1, char initial_player);
    
double get_counter_factual_utility(InformationSet& I, PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, std::vector<std::vector<int>>& starting_histories, std::vector<double>& prob_reaching_h_list);

void get_probability_of_reaching_all_h(InformationSet& I, PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, std::vector<std::vector<int>>& starting_histories, char initial_player, std::vector<double>& prob_reaching_h_list_all);

double calc_util_a_given_I_and_action(InformationSet& I, int action, PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, std::vector<std::vector<int>>& starting_histories, std::vector<double>& prob_reaching_h_list);

void calc_cfr_policy_given_I(InformationSet& I, PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, int T, std::vector<double>& regret_list);

std::vector<std::vector<double> > get_prev_regrets(std::string& file_path, char player);

bool get_move_flag(std::string I_hash, char player);

void simulate_opponent_turn(PokerTable& true_cards, History& history, double reach_probability, InformationSet& opponent_I, PolicyVec& policy_obj,  
                            std::vector<PokerTable>& true_cards_list, std::vector<History>& history_list, std::vector<double>& reach_probability_list,  
                            std::vector<InformationSet>& opponent_I_list, std::vector<double>& Q_values, char br_player, int played_action);

double get_max_Q_value_and_update_policy(std::vector<double>& Q_values, std::vector<int>& actions, PolicyVec& br, InformationSet& I);

double get_min_Q_value_and_update_policy(std::vector<double>& Q_values, std::vector<int>& actions, PolicyVec& br, InformationSet& I);

double compute_best_response(InformationSet& I, char br_player, std::vector<PokerTable>& true_cards_list, std::vector<History>& history_list, 
                 std::vector<double>& reach_probability_list, std::vector<InformationSet>& opponent_I_list, PolicyVec& br, PolicyVec& policy_obj);
    
double compute_best_response_parallel(InformationSet& I, char br_player, std::vector<PokerTable>& true_cards_list, std::vector<History>& history_list, 
                 std::vector<double>& reach_probability_list, std::vector<InformationSet>& opponent_I_list, PolicyVec& br, PolicyVec& policy_obj);

double compute_best_response_wrapper(PolicyVec& policy_obj, PolicyVec& br, char br_player);

void save_map_txt(std::string output_file, std::vector<std::vector<double>>& map, std::vector<std::string>& Information_sets);