#include "cpp_headers/poker_classes.hpp"
#include "cpp_headers/poker_utilities.hpp"
#include "cpp_headers/json.hpp"
using json = nlohmann::json;

char toggle_player(char player) {
    return (player == 'x') ? 'o' : 'x';
}


void valid_histories_play(InformationSet& I_1, InformationSet& I_2, PokerTable& true_cards, History& current_history, InformationSet& end_I, 
                          int current_action_index, int other_player_turn_index, PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, std::vector<std::vector<int>>& valid_histories_list){
    InformationSet& I = true_cards.player_to_move == 'x' ? I_1 : I_2;
    std::vector<int> actions;

    if (I == end_I){
        valid_histories_list.push_back(current_history.history);
        return;
    }

    if (I.player == 'x') {
        I.get_actions_given_policy(actions, policy_obj_x);
    } 
    else {
        I.get_actions_given_policy(actions, policy_obj_o);
    }

    if (I.move_flag){
        for (int action : actions) {
            PokerTable new_true_cards = true_cards;
            bool success = new_true_cards.update_move(action);

            History new_history = current_history;
            new_history.history.push_back(action);

            char winner;
            if (success && !new_true_cards.is_win(winner) && !new_true_cards.is_over()) {
                InformationSet new_I = I;
                new_I.update_move(action);

                if (I.player == 'x') {
                    valid_histories_play(new_I, I_2, new_true_cards, new_history, end_I, current_action_index, other_player_turn_index, policy_obj_x, policy_obj_o, valid_histories_list);
                }
                else {
                    valid_histories_play(I_1, new_I, new_true_cards, new_history, end_I, current_action_index, other_player_turn_index, policy_obj_x, policy_obj_o, valid_histories_list);
                }
            }
        }
    }
    else {
        for (int action : actions) {
            InformationSet new_I = I;
            new_I.simulate_sense(action, true_cards);
            PokerTable new_true_cards = true_cards;

            History new_history = current_history;
            new_history.history.push_back(action);

            if (I.player == 'x') {
                valid_histories_play(new_I, I_2, new_true_cards, new_history, end_I, current_action_index, other_player_turn_index, policy_obj_x, policy_obj_o, valid_histories_list);
            }
            else {
                valid_histories_play(I_1, new_I, new_true_cards, new_history, end_I, current_action_index, other_player_turn_index, policy_obj_x, policy_obj_o, valid_histories_list);
            }
        }
    }
}


void upgraded_get_histories_given_I(InformationSet& I, PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, std::vector<std::vector<int>>& valid_histories_list){
    std::vector<std::string> unique_draws = {"JJQ", "JQJ", "QJJ", "QQJ", "QJQ", "JQQ", 
                                             "KKJ", "KJK", "JKK", "KKQ", "KQK", "QKK", 
                                             "QQK", "QKQ", "KQQ", "JJK", "JKJ", "KJJ",
                                             "JQK", "JKQ", "QJK", "QKJ", "KJQ", "KQJ"};
    
    for (std::string draw : unique_draws){
        std::string hash_1 = "a-" + std::string(1, draw[0]) + "--";
        std::string hash_2 = "o-" + std::string(1, draw[1]) + "--";
    
        InformationSet I_1('x', true, hash_1);
        InformationSet I_2('o', false, hash_2);
        PokerTable true_cards = PokerTable(draw);
    
        std::vector<int> h = {};
        h.push_back(draw[0]);
        h.push_back(draw[1]);
        h.push_back(draw[2]);
        NonTerminalHistory current_history(h);

        valid_histories_play(I_1, I_2, true_cards, current_history, I, 0, 0, policy_obj_x, policy_obj_o, valid_histories_list);
    }
    return;
}   


double get_expected_utility(InformationSet &I_1, InformationSet &I_2, PokerTable &true_cards, PolicyVec &policy_obj_x, 
                            PolicyVec &policy_obj_o, double probability, History& current_history, char initial_player) {
    double expected_utility_h = 0.0;
    
    InformationSet& I = true_cards.player_to_move == 'x' ? I_1 : I_2;
    PolicyVec& policy_obj = I.player == 'x' ? policy_obj_x : policy_obj_o;
    
    std::vector<int> actions;
    I.get_actions_given_policy(actions, policy_obj);
    
    if (I.move_flag) {
        for (int action : actions) {
            PokerTable new_true_cards = true_cards;
            bool success = new_true_cards.update_move(action);

            double probability_new = probability * policy_obj.policy_dict[I.get_index()][action];
            History new_history = current_history;
            new_history.history.push_back(action);
            
            char winner;
            if (success && !new_true_cards.is_win(winner) && !new_true_cards.is_over()) {
                InformationSet new_I = I;
                new_I.update_move(action);
                
                if (I.player == 'x') {
                    expected_utility_h += get_expected_utility(new_I, I_2, new_true_cards, policy_obj_x, policy_obj_o, probability_new, new_history, initial_player);
                } else {
                    expected_utility_h += get_expected_utility(I_1, new_I, new_true_cards, policy_obj_x, policy_obj_o, probability_new, new_history, initial_player);
                }
            } else {
                TerminalHistory H_T = TerminalHistory(new_history.history);
                H_T.set_reward();
                if (initial_player == 'x'){
                    expected_utility_h += H_T.reward[0] * probability_new;
                }
                else{
                    expected_utility_h += H_T.reward[1] * probability_new;
                }
            }
        }
    } else {
        for (int action : actions) {
            InformationSet new_I = I;
            new_I.simulate_sense(action, true_cards);
            
            double probability_new = probability * policy_obj.policy_dict[I.get_index()][action];
            History new_history = current_history;
            new_history.history.push_back(action);

            if (I.player == 'x') {
                expected_utility_h += get_expected_utility(new_I, I_2, true_cards, policy_obj_x, policy_obj_o, probability_new, new_history, initial_player);
            } else {
                expected_utility_h += get_expected_utility(I_1, new_I, true_cards, policy_obj_x, policy_obj_o, probability_new, new_history, initial_player);
            }
        }
    }
    
    return expected_utility_h;
}

double get_expected_utility_action_version(InformationSet &I_1, InformationSet &I_2, PokerTable &true_cards, PolicyVec &policy_obj_x, 
                            PolicyVec &policy_obj_o, double probability, History& current_history, char initial_player, int action) {
    double expected_utility_h = 0.0;
    
    InformationSet& I = true_cards.player_to_move == 'x' ? I_1 : I_2;
    PolicyVec& policy_obj = I.player == 'x' ? policy_obj_x : policy_obj_o;
    
    if (I.move_flag) {
        PokerTable new_true_cards = true_cards;
        bool success = new_true_cards.update_move(action);

        double probability_new = probability;
        History new_history = current_history;
        new_history.history.push_back(action);
        
        char winner;
        if (success && !new_true_cards.is_win(winner) && !new_true_cards.is_over()) {
            InformationSet new_I = I;
            new_I.update_move(action);
            
            if (I.player == 'x') {
                expected_utility_h += get_expected_utility(new_I, I_2, new_true_cards, policy_obj_x, policy_obj_o, probability_new, new_history, initial_player);
            } else {
                expected_utility_h += get_expected_utility(I_1, new_I, new_true_cards, policy_obj_x, policy_obj_o, probability_new, new_history, initial_player);
            }
        } else {
            TerminalHistory H_T = TerminalHistory(new_history.history);
            H_T.set_reward();
            if (initial_player == 'x'){
                expected_utility_h += H_T.reward[0] * probability_new;
            }
            else{
                expected_utility_h += H_T.reward[1] * probability_new;
            }
        }
    } 
    else {
        InformationSet new_I = I;
        new_I.simulate_sense(action, true_cards);
        
        double probability_new = probability;
        History new_history = current_history;
        new_history.history.push_back(action);

        if (I.player == 'x') {
            expected_utility_h += get_expected_utility(new_I, I_2, true_cards, policy_obj_x, policy_obj_o, probability_new, new_history, initial_player);
        } else {
            expected_utility_h += get_expected_utility(I_1, new_I, true_cards, policy_obj_x, policy_obj_o, probability_new, new_history, initial_player);
        }
    }

    return expected_utility_h;
}

double get_expected_utility_parallel(InformationSet &I_1, InformationSet &I_2, PokerTable &true_cards, PolicyVec &policy_obj_x, 
                                     PolicyVec &policy_obj_o, double probability, History& current_history, char initial_player) {
    double expected_utility_h = 0.0;
    std::vector<InformationSet> Depth_1_P1_Isets;
    std::vector<InformationSet> Depth_1_P2_Isets;
    std::vector<PokerTable> Depth_1_cards;
    std::vector<char> Depth_1_players;
    std::vector<double> Depth_1_probabilities;
    std::vector<History> Depth_1_histories;
    
    InformationSet& I = true_cards.player_to_move == 'x' ? I_1 : I_2;
    PolicyVec policy_obj = I.player == 'x' ? policy_obj_x : policy_obj_o;

    std::vector<int> actions;
    I.get_actions_given_policy(actions, policy_obj);

    if (I.move_flag) {
        for (int action : actions) {
            PokerTable new_true_cards = true_cards;
            bool success = new_true_cards.update_move(action);

            double probability_new = probability * policy_obj.policy_dict[I.get_index()][action];
            History new_history = current_history;
            new_history.history.push_back(action);

            char winner;
            if (success && !new_true_cards.is_win(winner) && !new_true_cards.is_over()) {
                InformationSet new_I = I;
                new_I.update_move(action);

                if (I.player == 'x') {
                    Depth_1_P1_Isets.push_back(new_I);
                    Depth_1_P2_Isets.push_back(I_2);
                    Depth_1_cards.push_back(new_true_cards);
                    Depth_1_probabilities.push_back(probability_new);
                    Depth_1_histories.push_back(new_history);
                } else {
                    Depth_1_P1_Isets.push_back(I_1);
                    Depth_1_P2_Isets.push_back(new_I);
                    Depth_1_cards.push_back(new_true_cards);
                    Depth_1_probabilities.push_back(probability_new);
                    Depth_1_histories.push_back(new_history);
                }
            } else {
                TerminalHistory H_T = TerminalHistory(new_history.history);
                H_T.set_reward();
                if (initial_player == 'x'){
                    expected_utility_h += H_T.reward[0] * probability_new;
                }
                else{
                    expected_utility_h += H_T.reward[1] * probability_new;
                }
            }
        
        }
    }
    else {
        for (int action : actions) {
            InformationSet new_I = I;
            new_I.simulate_sense(action, true_cards);

            double probability_new = probability * policy_obj.policy_dict[I.get_index()][action];
            History new_history = current_history;
            new_history.history.push_back(action);

            if (I.player == 'x') {
                Depth_1_P1_Isets.push_back(new_I);
                Depth_1_P2_Isets.push_back(I_2);
                Depth_1_cards.push_back(true_cards);
                Depth_1_probabilities.push_back(probability_new);
                Depth_1_histories.push_back(new_history);
            } else {
                Depth_1_P1_Isets.push_back(I_1);
                Depth_1_P2_Isets.push_back(new_I);
                Depth_1_cards.push_back(true_cards);
                Depth_1_probabilities.push_back(probability_new);
                Depth_1_histories.push_back(new_history);
            }   
        }
    }

    # pragma omp parallel for num_threads(96)
    for (int i = 0; i < Depth_1_P1_Isets.size(); i++) {
        expected_utility_h += get_expected_utility(Depth_1_P1_Isets[i], Depth_1_P2_Isets[i], Depth_1_cards[i], policy_obj_x, policy_obj_o, Depth_1_probabilities[i], Depth_1_histories[i], initial_player);
    }

    return expected_utility_h;
}


double get_expected_utility_wrapper(PolicyVec& policy_obj_x, PolicyVec& policy_obj_o){
    std::vector<std::string> unique_draws = {"JJQ", "JQJ", "QJJ", "QQJ", "QJQ", "JQQ", 
                                             "KKJ", "KJK", "JKK", "KKQ", "KQK", "QKK", 
                                             "QQK", "QKQ", "KQQ", "JJK", "JKJ", "KJJ",
                                             "JQK", "JKQ", "QJK", "QKJ", "KJQ", "KQJ"};
    double p = 1.0/30.0;
    std::vector<double> draw_probabilities = {p, p, p, p, p, p,
                                              p, p, p, p, p, p,
                                              p, p, p, p, p, p,
                                              2*p, 2*p, 2*p, 2*p, 2*p, 2*p};

    double expected_utility = 0.0;

    for (int i = 0; i < unique_draws.size(); i++){
        PokerTable true_cards = PokerTable(unique_draws[i]);
        std::string hash_1 = "a-" + std::string(1, true_cards.cards[0]) + "--";
        std::string hash_2 = "o-" + std::string(1, true_cards.cards[1]) + "--";
        InformationSet I_1('x', true, hash_1);
        InformationSet I_2('o', false, hash_2);
        std::vector<int> h = {};
        h.push_back(true_cards.cards[0]);
        h.push_back(true_cards.cards[1]);
        h.push_back(true_cards.cards[2]);
        TerminalHistory start_history = TerminalHistory(h);

        expected_utility += draw_probabilities[i] * get_expected_utility_parallel(I_1, I_2, true_cards, policy_obj_x, policy_obj_o, 1.0, start_history, 'x');
    }

    return expected_utility;
}


double get_prob_h_given_policy(InformationSet& I_1, InformationSet& I_2, PokerTable& true_cards, int next_action, 
                               PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, double probability, History history_obj, char initial_player, InformationSet& end_I){
    InformationSet& I = true_cards.player_to_move == 'x' ? I_1 : I_2;
    PolicyVec& policy_obj = I.player == 'x' ? policy_obj_x : policy_obj_o;

    if (I.move_flag) {
        PokerTable new_true_cards = true_cards;
        bool success = new_true_cards.update_move(next_action);

        if (I.player == toggle_player(initial_player)) {
            probability *= policy_obj.policy_dict[I.get_index()][next_action];
        }
        history_obj.track_traversal_index += 1;
        if (history_obj.track_traversal_index < history_obj.history.size()) {
            int new_next_action = history_obj.history[history_obj.track_traversal_index];

            char winner;
            if (success && !new_true_cards.is_win(winner) && !new_true_cards.is_over()) {
                InformationSet new_I = I;
                new_I.update_move(next_action);

                if (I.player == 'x') {
                    probability = get_prob_h_given_policy(new_I, I_2, new_true_cards, new_next_action, policy_obj_x, policy_obj_o, probability, history_obj, initial_player, end_I);
                }
                else {
                    probability = get_prob_h_given_policy(I_1, new_I, new_true_cards, new_next_action, policy_obj_x, policy_obj_o, probability, history_obj, initial_player, end_I);
                }
            }
        }
    }
    else {
        InformationSet new_I = I;
        new_I.simulate_sense(next_action, true_cards);
        PokerTable new_true_cards = true_cards;

        if (I.player == toggle_player(initial_player)) {
            probability *= policy_obj.policy_dict[I.get_index()][next_action];
        }
        history_obj.track_traversal_index += 1;
        if (history_obj.track_traversal_index < history_obj.history.size()) {
            int new_next_action = history_obj.history[history_obj.track_traversal_index];

            if (I.player == 'x') {
                probability = get_prob_h_given_policy(new_I, I_2, new_true_cards, new_next_action, policy_obj_x, policy_obj_o, probability, history_obj, initial_player, end_I);
            }
            else {
                probability = get_prob_h_given_policy(I_1, new_I, new_true_cards, new_next_action, policy_obj_x, policy_obj_o, probability, history_obj, initial_player, end_I);
            }
        }
    }

    return probability;
}


double get_prob_h_given_policy_wrapper(InformationSet& I_1, InformationSet& I_2, PokerTable& true_cards, int next_action, PolicyVec& policy_obj_x, 
                                       PolicyVec& policy_obj_o, History history_obj, InformationSet& curr_I_1, char initial_player){
    double p = 1.0 / 30.0;
    if (true_cards.cards[0] != true_cards.cards[1] && true_cards.cards[1] != true_cards.cards[2] && true_cards.cards[0] != true_cards.cards[2]){
        p = 2.0 / 30.0;
    }

    if (curr_I_1.get_hash().size() == 5){
        return p;
    }
    else {
        return p * get_prob_h_given_policy(I_1, I_2, true_cards, next_action, policy_obj_x, policy_obj_o, 1.0, history_obj, initial_player, curr_I_1);
    }
}


double get_counter_factual_utility(InformationSet& I, PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, std::vector<std::vector<int>>& starting_histories, std::vector<double>& prob_reaching_h_list, int action) {
    double counter_factual_utility = 0.0;
    int count = 0;
    for (std::vector<int> h : starting_histories) {
        NonTerminalHistory h_object(h);
        std::string cards = "---";

        cards[0] = h_object.history[0];
        cards[1] = h_object.history[1];
        cards[2] = h_object.history[2];

        std::string hash_1 = "a-" + std::string(1, cards[0]) + "--";
        std::string hash_2 = "o-" + std::string(1, cards[1]) + "--";
        InformationSet curr_I_1('x', true, hash_1);
        InformationSet curr_I_2('o', false, hash_2);
        PokerTable true_cards = PokerTable(cards);
        h_object.get_information_sets(curr_I_1, curr_I_2);
        h_object.update_true_cards_given_history(true_cards);

        // std::cout << "True cards bidding sequence: " << true_cards.bid_sequence << std::endl;
        // std::cout << "Cards in hand: " << true_cards.cards << std::endl;
        // std::cout << "Curr I_1: " << curr_I_1.get_hash() << std::endl;
        // std::cout << "Curr I_2: " << curr_I_2.get_hash() << std::endl;
        // std::cout << "I: " << I.get_hash() << std::endl;
        // std::cout << "Action: " << action << std::endl;
        // std::cout << "Prob reaching h: " << prob_reaching_h_list[count] << std::endl;
        // std::cout << "History: " << std::endl;
        // for (int i = 0; i < h.size(); i++) {
        //     std::cout << h[i] << " ";
        // }
        // std::cout << std::endl;
        
        if (prob_reaching_h_list[count] > 0) {
            counter_factual_utility += get_expected_utility_action_version(curr_I_1, curr_I_2, true_cards, policy_obj_x, policy_obj_o, 1.0, h_object, I.player, action) * prob_reaching_h_list[count];
        }
        // std::cout << "Counter factual utility: " << counter_factual_utility << std::endl;

        count += 1;
    }
    return counter_factual_utility;
}


void get_probability_of_reaching_all_h(InformationSet& I, PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, std::vector<std::vector<int>>& starting_histories, 
                                       char initial_player, std::vector<double>& prob_reaching_h_list_all) {
    for (std::vector<int> h : starting_histories) {
        NonTerminalHistory h_object(h);

        std::string cards = "---";
        cards[0] = h_object.history[0];
        cards[1] = h_object.history[1];
        cards[2] = h_object.history[2];
        
        std::string hash_1 = "a-" + std::string(1, cards[0]) + "--";
        std::string hash_2 = "o-" + std::string(1, cards[1]) + "--";
        InformationSet I_1('x', true, hash_1);
        InformationSet I_2('o', false, hash_2);
        PokerTable true_cards = PokerTable(cards);
        double probability_reaching_h = get_prob_h_given_policy_wrapper(I_1, I_2, true_cards, h[3], policy_obj_x, policy_obj_o, h_object, I, initial_player);
        prob_reaching_h_list_all.push_back(probability_reaching_h);
    }
}


double calc_util_a_given_I_and_action(InformationSet& I, int action, PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, 
                                      std::vector<std::vector<int>>& starting_histories, std::vector<double>& prob_reaching_h_list) {
    
    double util_a = 0.0;
    util_a = get_counter_factual_utility(I, policy_obj_x, policy_obj_o, starting_histories, prob_reaching_h_list, action);
    return util_a;
}


void calc_cfr_policy_given_I(InformationSet& I, PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, int T, std::vector<double>& regret_list) {
    auto start = std::chrono::system_clock::now();
    
    double util = 0.0;
    std::vector<int> actions;
    PolicyVec& policy_obj = I.player == 'x' ? policy_obj_x : policy_obj_o;

    std::vector<std::vector<int>> starting_histories;
    std::vector<double> prob_reaching_h_list;
    std::vector<double> util_a_list;
    
    for (int i = 0; i < 6; i++) {
        util_a_list.push_back(0.0);
    }

    std::cout << "Information Set: " << I.get_hash() << std::endl;
    I.get_actions(actions);

    upgraded_get_histories_given_I(I, policy_obj_x, policy_obj_o, starting_histories);
    get_probability_of_reaching_all_h(I, policy_obj_x, policy_obj_o, starting_histories, I.player, prob_reaching_h_list);

    for (int action : actions) {
        double util_a = calc_util_a_given_I_and_action(I, action, policy_obj_x, policy_obj_o, starting_histories, prob_reaching_h_list);
        util += util_a * policy_obj.policy_dict[I.get_index()][action];
        util_a_list[action] = util_a;
    }

    
    for (int action : actions) {
        double regret_T = 0.0;
        if (T == 0) {
            regret_T = util_a_list[action] - util;
        } else {
            regret_T = regret_list[action] + util_a_list[action] - util;
        }

        std::cout << "Action: " << action << " Regret: " << regret_T << std::endl;
        regret_T = regret_T > 0.0 ? regret_T : 0.0;
        regret_list[action] = regret_T;
    }
}


std::vector<std::vector<double> > get_prev_regrets(std::string& file_path, char player){
    std::ifstream i(file_path);
    json regret_obj;
    i >> regret_obj;
    std::vector<std::vector<double> > regret_map;
        
    for (json::iterator it = regret_obj.begin(); it != regret_obj.end(); ++it) {
        std::string I_hash = it.key();
        bool move_flag;
        if (I_hash.size() != 0){
            move_flag = I_hash[0] == 'a' ? true : false;
        }
        else {
            move_flag = player == 'x' ? true : false;
        }

        InformationSet I(player, move_flag, I_hash);

        std::vector <double> probability_distribution(6);
        // initialise all values to zero
        for (int i = 0; i < 6; i++) {
            probability_distribution[i] = 0.0;
        }

        if (move_flag) {
            std::vector<std::string> sense_keys = {"5"};
            for (int i = 0; i < sense_keys.size(); i++) {
                probability_distribution[stoi(sense_keys[i])] = regret_obj[I_hash][sense_keys[i]];
            }
        }
        else if (!move_flag) {
            std::vector<std::string> move_keys = {"0", "1", "2", "3", "4"};
            for (int i = 0; i < move_keys.size(); i++) {
                probability_distribution[stoi(move_keys[i])] = regret_obj[I_hash][move_keys[i]];
            }
        }

        regret_map[I.get_index()] = probability_distribution;
    }

    return regret_map;
}


bool get_move_flag(std::string I_hash, char player){
    bool move_flag;
    if (I_hash.size() != 0){
        move_flag = I_hash[0] == 'a' ? true : false;
    }
    else {
        move_flag = player == 'x' ? true : false;
    }
    return move_flag;
}


void simulate_opponent_turn(PokerTable& true_cards, History& history, double reach_probability, InformationSet& opponent_I, PolicyVec& policy_obj,  
                            std::vector<PokerTable>& true_cards_list, std::vector<History>& history_list, std::vector<double>& reach_probability_list,  
                            std::vector<InformationSet>& opponent_I_list, std::vector<double>& Q_values, char br_player, int played_action) {
    std::vector<int> depth_1_opponent_actions;
    opponent_I.get_actions_given_policy(depth_1_opponent_actions, policy_obj);
    std::vector<History> depth_2_history_list;
    std::vector<double> depth_2_reach_probability_list;
    std::vector<InformationSet> depth_2_opponent_I_list;
    std::vector<PokerTable> depth_2_true_cards_list;

    // skip turn
    if (true_cards.player_to_move == br_player){
        true_cards_list.push_back(true_cards);
        history_list.push_back(history);
        reach_probability_list.push_back(reach_probability);
        opponent_I_list.push_back(opponent_I);
        return;
    }

    // first simulate sense
    for (int opponent_action : depth_1_opponent_actions) {
        InformationSet depth_2_opponent_I = opponent_I;
        depth_2_opponent_I.simulate_sense(opponent_action, true_cards);

        History depth_2_history = history;
        depth_2_history.history.push_back(opponent_action);

        double depth_2_reach_probability = reach_probability * policy_obj.policy_dict[opponent_I.get_index()][opponent_action];
        PokerTable depth_2_true_cards = true_cards;

        depth_2_reach_probability_list.push_back(depth_2_reach_probability);
        depth_2_history_list.push_back(depth_2_history);
        depth_2_opponent_I_list.push_back(depth_2_opponent_I);
        depth_2_true_cards_list.push_back(depth_2_true_cards);
    }

    for (int i = 0; i < depth_2_history_list.size(); i++) {
        InformationSet depth_2_opponent_I = depth_2_opponent_I_list[i];
        std::vector<int> depth_2_opponent_actions;
        depth_2_opponent_I.get_actions_given_policy(depth_2_opponent_actions, policy_obj);

        for (int opponent_action : depth_2_opponent_actions) {
            PokerTable depth_3_true_cards = depth_2_true_cards_list[i];
            History depth_3_history = depth_2_history_list[i];
            double depth_3_reach_probability = depth_2_reach_probability_list[i] * policy_obj.policy_dict[depth_2_opponent_I_list[i].get_index()][opponent_action];
            
            char winner;
            bool success = depth_3_true_cards.update_move(opponent_action);
            depth_3_history.history.push_back(opponent_action);

            if (success && !depth_3_true_cards.is_win(winner) && !depth_3_true_cards.is_over()) {
                InformationSet depth_3_opponent_I = depth_2_opponent_I_list[i];
                depth_3_opponent_I.update_move(opponent_action);

                if (depth_3_true_cards.player_to_move == br_player){
                    true_cards_list.push_back(depth_3_true_cards);
                    history_list.push_back(depth_3_history);
                    reach_probability_list.push_back(depth_3_reach_probability);
                    opponent_I_list.push_back(depth_3_opponent_I);
                }
                else {
                    simulate_opponent_turn(depth_3_true_cards, depth_3_history, depth_3_reach_probability, depth_3_opponent_I, policy_obj, true_cards_list, history_list, reach_probability_list, opponent_I_list, Q_values, br_player, played_action);
                }
            }
            else {
                TerminalHistory H_T = TerminalHistory(depth_3_history.history);
                H_T.set_reward();

                if (br_player == 'x'){
                    Q_values[played_action] += H_T.reward[0] * depth_3_reach_probability;
                }
                else{
                    Q_values[played_action] += H_T.reward[1] * depth_3_reach_probability;
                }
            }
        }
    }
}


double get_max_Q_value_and_update_policy(std::vector<double>& Q_values, std::vector<int>& actions, PolicyVec& br, InformationSet& I) {
    double max_Q = -1.0;
    int best_action = -1;

    for (int a = 0; a < actions.size(); a++) {
        if (Q_values[actions[a]] >= max_Q) {
            max_Q = Q_values[actions[a]];
            best_action = actions[a];
        }
    }

    std::vector<double>& prob_dist = br.policy_dict[I.get_index()];
    for (int k = 0; k < prob_dist.size(); k++) {
        if (k == best_action) {
            prob_dist[k] = 1.0;
        } 
        else {
            prob_dist[k] = 0.0;
        }
    }

    return max_Q;
}


double compute_best_response(InformationSet& I, char br_player, std::vector<PokerTable>& true_cards_list, std::vector<History>& history_list, 
                 std::vector<double>& reach_probability_list, std::vector<InformationSet>& opponent_I_list, PolicyVec& br, PolicyVec& policy_obj, std::vector<std::vector<double>>& Infoset_Q_values) {    
    std::vector<int> actions;
    std::vector<double>& Q_values = Infoset_Q_values[I.get_index()];
    I.get_actions(actions);

    if (I.move_flag) {
        for (int a = 0; a < actions.size(); a++) {
            std::vector<PokerTable> depth_3_true_cards_list;
            std::vector<History> depth_3_history_list;
            std::vector<double> depth_3_reach_probability_list;
            std::vector<InformationSet> depth_3_opponent_I_list;
            
            for (int h = 0; h < history_list.size(); h++) {
                PokerTable depth_1_true_cards = true_cards_list[h];
                History depth_1_history = history_list[h];
                double depth_1_reach_probability = reach_probability_list[h];
                InformationSet depth_1_opponent_I = opponent_I_list[h];

                bool success = depth_1_true_cards.update_move(actions[a]);
                depth_1_history.history.push_back(actions[a]);

                char winner;
                if (success && !depth_1_true_cards.is_win(winner) && !depth_1_true_cards.is_over()) {
                    InformationSet new_I = I;
                    new_I.update_move(actions[a]);                 
                    simulate_opponent_turn(depth_1_true_cards, depth_1_history, depth_1_reach_probability, depth_1_opponent_I, policy_obj, depth_3_true_cards_list, depth_3_history_list, depth_3_reach_probability_list, depth_3_opponent_I_list, Q_values, br_player, actions[a]);
                }
                else {
                    TerminalHistory H_T = TerminalHistory(depth_1_history.history);
                    H_T.set_reward();

                    if (br_player == 'x'){
                        Q_values[actions[a]] += H_T.reward[0] * depth_1_reach_probability;
                    }
                    else{
                        Q_values[actions[a]] += H_T.reward[1] * depth_1_reach_probability;
                    }
                }
            }

            if (depth_3_history_list.size() > 0) {
                InformationSet new_I = I;
                new_I.update_move(actions[a]);
                Q_values[actions[a]] += compute_best_response(new_I, br_player, depth_3_true_cards_list, depth_3_history_list, depth_3_reach_probability_list, depth_3_opponent_I_list, br, policy_obj, Infoset_Q_values);
            }
        }
    }
    else {
        for (int a = 0; a < actions.size(); a++) {
            std::unordered_map<std::string, std::vector<PokerTable>> infoset_to_true_cards;
            std::unordered_map<std::string, std::vector<History>> infoset_to_history;
            std::unordered_map<std::string, std::vector<double>> infoset_to_reach_probability;
            std::unordered_map<std::string, std::vector<InformationSet>> infoset_to_opponent_I;
            std::unordered_set<std::string> infoset_set;

            for (int h = 0; h < history_list.size(); h++) {
                PokerTable& true_cards = true_cards_list[h];
                History& history = history_list[h];
                double reach_probability = reach_probability_list[h];
 
                InformationSet new_I = I;
                new_I.simulate_sense(actions[a], true_cards);

                History new_history = history;
                new_history.history.push_back(actions[a]);

                infoset_to_true_cards[new_I.hash].push_back(true_cards);
                infoset_to_history[new_I.hash].push_back(new_history);
                infoset_to_reach_probability[new_I.hash].push_back(reach_probability);
                infoset_to_opponent_I[new_I.hash].push_back(opponent_I_list[h]);
                infoset_set.insert(new_I.hash);
            }
            
            for (int t = 0; t < infoset_set.size(); t++) {
                std::string new_I_hash = *std::next(infoset_set.begin(), t);
                bool move_flag = get_move_flag(new_I_hash, I.player);
                InformationSet new_I(I.player, move_flag, new_I_hash);
  
                if (infoset_to_history[new_I.hash].size() > 0) {
                    Q_values[actions[a]] += compute_best_response(new_I, br_player, infoset_to_true_cards[new_I.hash], infoset_to_history[new_I.hash], infoset_to_reach_probability[new_I.hash], infoset_to_opponent_I[new_I.hash], br, policy_obj, Infoset_Q_values);
                }
            }
        }
    }

    double max_Q = get_max_Q_value_and_update_policy(Q_values, actions, br, I);
    return max_Q;
}


double compute_best_response_parallel(InformationSet& I, char br_player, std::vector<PokerTable>& true_cards_list, std::vector<History>& history_list, 
                 std::vector<double>& reach_probability_list, std::vector<InformationSet>& opponent_I_list, PolicyVec& br, PolicyVec& policy_obj, std::vector<std::vector<double>>& Infoset_Q_values) {    
    std::vector<int> actions;
    std::vector<double>& Q_values = Infoset_Q_values[I.get_index()];
    I.get_actions(actions);

    if (I.move_flag) {
        std::unordered_map<int, std::vector<PokerTable>> action_to_true_cards_list;
        std::unordered_map<int, std::vector<History>> action_to_history_list;
        std::unordered_map<int, std::vector<double>> action_to_reach_probability_list;
        std::unordered_map<int, std::vector<InformationSet>> action_to_opponent_I_list; 

        for (int a = 0; a < actions.size(); a++) {
            std::vector<PokerTable> depth_3_true_cards_list;
            std::vector<History> depth_3_history_list;
            std::vector<double> depth_3_reach_probability_list;
            std::vector<InformationSet> depth_3_opponent_I_list;
            
            for (int h = 0; h < history_list.size(); h++) {
                PokerTable depth_1_true_cards = true_cards_list[h];
                History depth_1_history = history_list[h];
                double depth_1_reach_probability = reach_probability_list[h];
                InformationSet depth_1_opponent_I = opponent_I_list[h];

                bool success = depth_1_true_cards.update_move(actions[a]);
                depth_1_history.history.push_back(actions[a]);

                char winner;
                if (success && !depth_1_true_cards.is_win(winner) && !depth_1_true_cards.is_over()) {
                    InformationSet new_I = I;
                    new_I.update_move(actions[a]);

                    // simulate opponent's turn     
                    simulate_opponent_turn(depth_1_true_cards, depth_1_history, depth_1_reach_probability, depth_1_opponent_I, policy_obj, depth_3_true_cards_list, depth_3_history_list, depth_3_reach_probability_list, depth_3_opponent_I_list, Q_values, br_player, actions[a]);
                }
                else {
                    TerminalHistory H_T = TerminalHistory(depth_1_history.history);
                    H_T.set_reward();

                    if (br_player == 'x'){
                        Q_values[actions[a]] += H_T.reward[0] * depth_1_reach_probability;
                    }
                    else{
                        Q_values[actions[a]] += H_T.reward[1] * depth_1_reach_probability;
                    }
                }
            }

            action_to_true_cards_list[actions[a]] = depth_3_true_cards_list;
            action_to_history_list[actions[a]] = depth_3_history_list;
            action_to_reach_probability_list[actions[a]] = depth_3_reach_probability_list;
            action_to_opponent_I_list[actions[a]] = depth_3_opponent_I_list;
        }

        std::unordered_map<std::string, std::vector<PokerTable>> infoset_to_true_cards;
        std::unordered_map<std::string, std::vector<History>> infoset_to_history;
        std::unordered_map<std::string, std::vector<double>> infoset_to_reach_probability;
        std::unordered_map<std::string, std::vector<InformationSet>> infoset_to_opponent_I;
        std::unordered_map<std::string, int> infoset_to_first_action_taken;
        std::unordered_map<std::string, int> infoset_to_second_action_taken;
        std::unordered_set<std::string> infoset_set;

        std::vector<std::vector<double>> depth_4_Q_values;
        std::vector<std::vector<int>> depth_4_actions;

        for (int i = 0; i < 6; i++) {
            std::vector<double> Q_value_vector;
            std::vector<int> action_vector;

            for (int j = 0; j < 6; j++) {
                Q_value_vector.push_back(0.0);
            }

            depth_4_Q_values.push_back(Q_value_vector);
            depth_4_actions.push_back(action_vector);
        }

        for (int a = 0; a < actions.size(); a++) {
            if (action_to_history_list[actions[a]].size() > 0) {
                InformationSet new_I = I;
                new_I.update_move(actions[a]);
    
                new_I.get_actions(depth_4_actions[actions[a]]);

                for (int b = 0; b < depth_4_actions[actions[a]].size(); b++) {
                    for (int h = 0; h < action_to_history_list[actions[a]].size(); h++) {
                        PokerTable& true_cards = action_to_true_cards_list[actions[a]][h];
                        History& history = action_to_history_list[actions[a]][h];
                        double reach_probability = action_to_reach_probability_list[actions[a]][h];

                        InformationSet depth_4_I = new_I;
                        depth_4_I.simulate_sense(depth_4_actions[actions[a]][b], true_cards);

                        History depth_4_history = history;
                        depth_4_history.history.push_back(depth_4_actions[actions[a]][b]);

                        infoset_to_true_cards[depth_4_I.hash].push_back(true_cards);
                        infoset_to_history[depth_4_I.hash].push_back(depth_4_history);
                        infoset_to_reach_probability[depth_4_I.hash].push_back(reach_probability);
                        infoset_to_opponent_I[depth_4_I.hash].push_back(action_to_opponent_I_list[actions[a]][h]);
                        infoset_to_first_action_taken[depth_4_I.hash] = actions[a];
                        infoset_to_second_action_taken[depth_4_I.hash] = depth_4_actions[actions[a]][b];

                        infoset_set.insert(depth_4_I.hash);
                    }
                }
            }
        }

        # pragma omp parallel for num_threads(96)
        for (int t = 0; t < infoset_set.size(); t++) {
            std::string new_I_hash = *std::next(infoset_set.begin(), t);
            bool move_flag = get_move_flag(new_I_hash, I.player);
            InformationSet new_I(I.player, move_flag, new_I_hash);

            int a_val = infoset_to_first_action_taken[new_I.hash];
            int b_val = infoset_to_second_action_taken[new_I.hash];

            if (infoset_to_history[new_I.hash].size() > 0) {
                depth_4_Q_values[a_val][b_val] += compute_best_response(new_I, br_player, infoset_to_true_cards[new_I.hash], infoset_to_history[new_I.hash], infoset_to_reach_probability[new_I.hash], infoset_to_opponent_I[new_I.hash], br, policy_obj, Infoset_Q_values);
            }
        }

        # pragma omp parallel for num_threads(96)
        for (int a = 0; a < actions.size(); a++) {
            if (action_to_history_list[actions[a]].size() > 0) {
                InformationSet new_I = I;
                new_I.update_move(actions[a]);

                Q_values[actions[a]] = get_max_Q_value_and_update_policy(depth_4_Q_values[actions[a]], depth_4_actions[actions[a]], br, new_I);
            }
        }
    }
    else {
        std::unordered_map<std::string, std::vector<PokerTable>> infoset_to_true_cards;
        std::unordered_map<std::string, std::vector<History>> infoset_to_history;
        std::unordered_map<std::string, std::vector<double>> infoset_to_reach_probability;
        std::unordered_map<std::string, std::vector<InformationSet>> infoset_to_opponent_I;
        std::unordered_map<std::string, int> infoset_to_action_taken;
        std::unordered_set<std::string> infoset_set;

        for (int a = 0; a < actions.size(); a++) {
            for (int h = 0; h < history_list.size(); h++) {
                PokerTable& true_cards = true_cards_list[h];
                History& history = history_list[h];
                double reach_probability = reach_probability_list[h];

                InformationSet new_I = I;
                new_I.simulate_sense(actions[a], true_cards);

                History new_history = history;
                new_history.history.push_back(actions[a]);

                infoset_to_true_cards[new_I.hash].push_back(true_cards);
                infoset_to_history[new_I.hash].push_back(new_history);
                infoset_to_reach_probability[new_I.hash].push_back(reach_probability);
                infoset_to_opponent_I[new_I.hash].push_back(opponent_I_list[h]);
                infoset_to_action_taken[new_I.hash] = actions[a];
                infoset_set.insert(new_I.hash);
            }
        }
        
        std::unordered_map<std::string, std::vector<PokerTable>> depth_2_infoset_to_true_cards;
        std::unordered_map<std::string, std::vector<History>> depth_2_infoset_to_history;
        std::unordered_map<std::string, std::vector<double>> depth_2_infoset_to_reach_probability;
        std::unordered_map<std::string, std::vector<InformationSet>> depth_2_infoset_to_opponent_I;
        std::unordered_map<std::string, std::string> depth_2_infoset_to_parent;
        std::unordered_map<std::string, int> depth_2_infoset_to_second_action_taken;
        std::unordered_set<std::string> depth_2_infoset_set;

        std::unordered_map<std::string, std::vector<double>> depth_2_Q_values;
        std::unordered_map<std::string, std::vector<int>> depth_2_actions;

        for (int t = 0; t < infoset_set.size(); t++) {
            std::vector<double> Q_value_vector;
            std::vector<int> action_vector;

            for (int i = 0; i < 6; i++) {
                Q_value_vector.push_back(0.0);
            }

            std::string new_I_hash = *std::next(infoset_set.begin(), t);
            bool move_flag = get_move_flag(new_I_hash, I.player);
            InformationSet new_I(I.player, move_flag, new_I_hash);

            depth_2_Q_values[new_I.hash] = Q_value_vector;
            depth_2_actions[new_I.hash] = action_vector;
        }

        for (int t = 0; t < infoset_set.size(); t++) {
            std::string new_I_hash = *std::next(infoset_set.begin(), t);
            bool move_flag = get_move_flag(new_I_hash, I.player);
            InformationSet new_I(I.player, move_flag, new_I_hash);

            if (infoset_to_history[new_I.hash].size() > 0) {
                int a_val = infoset_to_action_taken[new_I.hash];
                new_I.get_actions(depth_2_actions[new_I.hash]);
               
                for (int b = 0; b < depth_2_actions[new_I.hash].size(); b++) {
                    std::vector<PokerTable> depth_4_true_cards_list;
                    std::vector<History> depth_4_history_list;
                    std::vector<double> depth_4_reach_probability_list;
                    std::vector<InformationSet> depth_4_opponent_I_list;

                    for (int h = 0; h < infoset_to_history[new_I.hash].size(); h++) {
                        PokerTable depth_2_true_cards = infoset_to_true_cards[new_I.hash][h];
                        History depth_2_history = infoset_to_history[new_I.hash][h];
                        double depth_2_reach_probability = infoset_to_reach_probability[new_I.hash][h];
                        InformationSet depth_2_opponent_I = infoset_to_opponent_I[new_I.hash][h];

                        bool success = depth_2_true_cards.update_move(depth_2_actions[new_I.hash][b]);
                        depth_2_history.history.push_back(depth_2_actions[new_I.hash][b]);

                        char winner;
                        if (success && !depth_2_true_cards.is_win(winner) && !depth_2_true_cards.is_over()) {
                            InformationSet depth_3_I = new_I;
                            depth_3_I.update_move(depth_2_actions[new_I.hash][b]);

                            simulate_opponent_turn(depth_2_true_cards, depth_2_history, depth_2_reach_probability, depth_2_opponent_I, policy_obj, depth_4_true_cards_list, depth_4_history_list, depth_4_reach_probability_list, depth_4_opponent_I_list, depth_2_Q_values[new_I.hash], br_player, depth_2_actions[new_I.hash][b]);
                        }
                        else {
                            TerminalHistory H_T = TerminalHistory(depth_2_history.history);
                            H_T.set_reward();

                            if (br_player == 'x'){
                                depth_2_Q_values[new_I.hash][depth_2_actions[new_I.hash][b]] += H_T.reward[0] * depth_2_reach_probability;
                            }
                            else{
                                depth_2_Q_values[new_I.hash][depth_2_actions[new_I.hash][b]] += H_T.reward[1] * depth_2_reach_probability;
                            }
                        }
                    }

                    InformationSet depth_3_I = new_I;
                    depth_3_I.update_move(depth_2_actions[new_I.hash][b]);

                    depth_2_infoset_to_true_cards[depth_3_I.hash] = depth_4_true_cards_list;
                    depth_2_infoset_to_history[depth_3_I.hash] = depth_4_history_list;
                    depth_2_infoset_to_reach_probability[depth_3_I.hash] = depth_4_reach_probability_list;
                    depth_2_infoset_to_opponent_I[depth_3_I.hash] = depth_4_opponent_I_list;
                    depth_2_infoset_to_parent[depth_3_I.hash] = new_I.hash;
                    depth_2_infoset_to_second_action_taken[depth_3_I.hash] = depth_2_actions[new_I.hash][b];

                    depth_2_infoset_set.insert(depth_3_I.hash);
                }
            }
        }
        
        # pragma omp parallel for num_threads(96)
        for (int t = 0; t < depth_2_infoset_set.size(); t++) {
            std::string new_I_hash = *std::next(depth_2_infoset_set.begin(), t);
            bool move_flag = get_move_flag(new_I_hash, I.player);
            InformationSet new_I(I.player, move_flag, new_I_hash);

            std::string parent_hash = depth_2_infoset_to_parent[new_I.hash];
            int b_val = depth_2_infoset_to_second_action_taken[new_I.hash];

            if (depth_2_infoset_to_history[new_I.hash].size() > 0) {
                depth_2_Q_values[parent_hash][b_val] += compute_best_response(new_I, br_player, depth_2_infoset_to_true_cards[new_I.hash], depth_2_infoset_to_history[new_I.hash], depth_2_infoset_to_reach_probability[new_I.hash], depth_2_infoset_to_opponent_I[new_I.hash], br, policy_obj, Infoset_Q_values);
            }
        }

        for (int t = 0; t < infoset_set.size(); t++) {
            std::string new_I_hash = *std::next(infoset_set.begin(), t);
            bool move_flag = get_move_flag(new_I_hash, I.player);
            InformationSet new_I(I.player, move_flag, new_I_hash);

            if (infoset_to_history[new_I.hash].size() > 0) {
                int a_val = infoset_to_action_taken[new_I.hash];
                Q_values[a_val] += get_max_Q_value_and_update_policy(depth_2_Q_values[new_I.hash], depth_2_actions[new_I.hash], br, new_I);
            }
        }

    }
  
    return get_max_Q_value_and_update_policy(Q_values, actions, br, I);
}


void compute_best_response_wrapper(PolicyVec& policy_obj, PolicyVec& br, char br_player) {
    std::vector<std::string> unique_draws = {"JJQ", "JQJ", "QJJ", "QQJ", "QJQ", "JQQ", 
                                             "KKJ", "KJK", "JKK", "KKQ", "KQK", "QKK", 
                                             "QQK", "QKQ", "KQQ", "JJK", "JKJ", "KJJ",
                                             "JQK", "JKQ", "QJK", "QKJ", "KJQ", "KQJ"};
    double p = 1.0/30.0;
    std::vector<double> draw_probabilities = {p, p, p, p, p, p,
                                              p, p, p, p, p, p,
                                              p, p, p, p, p, p,
                                              2*p, 2*p, 2*p, 2*p, 2*p, 2*p};

    int num_infosets = br.policy_dict.size();
    std::vector<std::vector<double>> Infoset_Q_values(num_infosets, std::vector<double>(6, 0.0));

    for (int d = 0; d < unique_draws.size(); d++){
        std::string cards = unique_draws[d];
        PokerTable true_cards = PokerTable(cards);
        std::string hash_1 = "a-" + std::string(1, cards[0]) + "--";
        std::string hash_2 = "o-" + std::string(1, cards[1]) + "--";
        InformationSet I_1 = InformationSet('x', true, hash_1);
        InformationSet I_2 = InformationSet('o', false, hash_2);
        std::vector<int> h = {};
        h.push_back(cards[0]);
        h.push_back(cards[1]);
        h.push_back(cards[2]);
        TerminalHistory start_history = TerminalHistory(h);

        std::vector<PokerTable> true_cards_list;
        std::vector<History> history_list;
        std::vector<double> reach_probability_list;
        std::vector<InformationSet> opponent_I_list;

        if (br_player == 'x') {
            true_cards_list.push_back(true_cards);
            history_list.push_back(start_history);
            reach_probability_list.push_back(draw_probabilities[d]);
            opponent_I_list.push_back(I_2);

            compute_best_response(I_1, br_player, true_cards_list, history_list, reach_probability_list, opponent_I_list, br, policy_obj, Infoset_Q_values);
        } 
        else {
            std::vector<int> actions;
            I_1.get_actions_given_policy(actions, policy_obj);

            for (int a = 0; a < actions.size(); a++){
                PokerTable new_true_cards = true_cards;
                bool success = new_true_cards.update_move(actions[a]);

                History new_history = start_history;
                new_history.history.push_back(actions[a]);

                InformationSet new_I = I_1;
                new_I.update_move(actions[a]);

                true_cards_list.push_back(new_true_cards);
                history_list.push_back(new_history);
                reach_probability_list.push_back(policy_obj.policy_dict[I_1.get_index()][actions[a]] * draw_probabilities[d]);
                opponent_I_list.push_back(new_I);
            }

            compute_best_response(I_2, br_player, true_cards_list, history_list, reach_probability_list, opponent_I_list, br, policy_obj, Infoset_Q_values);
        }
    }
}


void save_map_txt(std::string output_file, std::vector<std::vector<double>>& map, std::vector<std::string>& Information_sets){
    std::ofstream f_out;
    f_out.open(output_file, std::ios::trunc);
    for (int j = 0; j < map.size(); j++) {
        // if all actions have zero probability, do not save information set
        bool all_zero = true;
        for (int i = 0; i < 6; i++) {
            if (map[j][i] > 0.0){
                all_zero = false;
                break;
            }
        }

        if (all_zero) {
            continue;
        }
        else {
            if (Information_sets[j] == "") {
                f_out << "* ";
            }
            else {
                f_out << Information_sets[j] << " ";
            }

            for (int i = 0; i < 6; i++) {
                if (map[j][i] > 0.0){
                    f_out << i << " " << map[j][i] << " ";
                }
            }
            f_out << std::endl;
        }
    }
    f_out.close();
}

