#include "cpp_headers/poker_classes.hpp"
#include <random>


void deal_cards(PokerTable& true_cards, std::vector<char>& deck){
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(deck.begin(), deck.end(), g);
    true_cards.cards[0] = deck[0];
    true_cards.cards[1] = deck[1];
    true_cards.cards[2] = deck[2];
    deck.erase(deck.begin(), deck.begin() + 3);
}

void play(InformationSet& I_1, PokerTable& true_cards, InformationSet& I_2, History &history, std::set<std::string>& P1_information_sets, std::set<std::string>& P2_information_sets){
    InformationSet& I = true_cards.player_to_move == 'x' ? I_1 : I_2;

    if (I.player == 'x'){
        P1_information_sets.insert(I.hash);
    }
    else {
        P2_information_sets.insert(I.hash);
    }

    std::vector<int> actions;
    I.get_actions(actions);

    // std::random_device rd;
    // std::mt19937 g(rd());
    // std::uniform_int_distribution<int> dis(0, actions.size() - 1);
    // int action = actions[dis(g)];

    if (I.move_flag){
        for (int i = 0; i < actions.size(); i++){
            int action = actions[i];

            PokerTable new_true_cards = true_cards;
            History new_history = history;
            bool success = new_true_cards.update_move(action);
            new_history.history.push_back(action);

            char winner;
            if (success && !new_true_cards.is_win(winner) && !new_true_cards.is_over()) {
                InformationSet new_I = I;
                new_I.update_move(action);

                if (new_I.player == 'x'){
                    play(new_I, new_true_cards, I_2, new_history, P1_information_sets, P2_information_sets);    
                }
                else {
                    play(I_1, new_true_cards, new_I, new_history, P1_information_sets, P2_information_sets);
                }
            }
            else {
                TerminalHistory H_T = TerminalHistory(history.history);
                H_T.set_reward();
            }
        }
    }
    else {
        for (int i = 0; i < actions.size(); i++){
            int action = actions[i];
            InformationSet new_I = I;
            History new_history = history;
            new_I.simulate_sense(action, true_cards);
            new_history.history.push_back(action);
            
            if (new_I.player == 'x'){
                play(new_I, true_cards, I_2, new_history, P1_information_sets, P2_information_sets);    
            }
            else {
                play(I_1, true_cards, new_I, new_history, P1_information_sets, P2_information_sets);
            }
        }
    }
}

int main() {
    std::set<std::string> P1_information_sets;
    std::set<std::string> P2_information_sets;

    std::vector<char> deck = {'J', 'J', 'Q', 'Q', 'K', 'K'};
    std::vector<std::string> unique_draws = {"JJQ", "JQJ", "QJJ", "QQJ", "QJQ", "JQQ", "KKJ", "KJK", "JKK", 
                                             "KKQ", "KQK", "QKK", "QQK", "QKQ", "KQQ", "JJK", "JKJ", "KJJ",
                                             "JQK", "JKQ", "QJK", "QKJ", "KJQ", "KQJ"};

    for (int i = 0; i < unique_draws.size(); i++){
        PokerTable true_cards;
        true_cards.cards = unique_draws[i];

        std::string hash_1 = "a " + std::string(1, true_cards.cards[0]) + "- ";
        std::string hash_2 = "o " + std::string(1, true_cards.cards[1]) + "- ";
        InformationSet I_1('x', true, hash_1);
        InformationSet I_2('o', false, hash_2);

        std::vector<int> h = {true_cards.cards[0], true_cards.cards[1], true_cards.cards[2]};
        History start_history = History(h);

        play(I_1, true_cards, I_2, start_history, P1_information_sets, P2_information_sets);
    }
   
    // write P1_information_sets and P2_information_sets to file
    std::ofstream P1_file("P1_information_sets_Leduc_Poker.txt");
    for (auto it = P1_information_sets.begin(); it != P1_information_sets.end(); it++){
        P1_file << *it << std::endl;
    }
    P1_file.close();

    std::ofstream P2_file("P2_information_sets_Leduc_Poker.txt");
    for (auto it = P2_information_sets.begin(); it != P2_information_sets.end(); it++){
        P2_file << *it << std::endl;
    }
    P2_file.close();
    return 0;
}
