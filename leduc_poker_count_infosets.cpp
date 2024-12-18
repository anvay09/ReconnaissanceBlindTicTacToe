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

void play(InformationSet& I_1, PokerTable& true_cards, InformationSet& I_2, History &history){
    InformationSet& I = true_cards.player_to_move == 'x' ? I_1 : I_2;
    std::vector<int> actions;
    I.get_actions(actions);

    std::random_device rd;
    std::mt19937 g(rd());
    std::uniform_int_distribution<int> dis(0, actions.size() - 1);
    int action = actions[dis(g)];

    if (I.move_flag){
        bool success = true_cards.update_move(action);
        history.history.push_back(action);

        char winner;
        if (success && !true_cards.is_win(winner) && !true_cards.is_over()) {
            I.update_move(action);

            play(I_1, true_cards, I_2, history);
        }
        else {
            TerminalHistory H_T = TerminalHistory(history.history);
            H_T.set_reward();
        }
    }
    else {
        I.simulate_sense(action, true_cards);
        history.history.push_back(action);
        play(I_1, true_cards, I_2, history);
    }
}

int main() {
    std::vector<char> deck = {'J', 'J', 'Q', 'Q', 'K', 'K'};
    PokerTable true_cards;
    deal_cards(true_cards, deck);
    std::string hash_1 = std::string(1, true_cards.cards[0]) + "-";
    std::string hash_2 = std::string(1, true_cards.cards[1]) + "-";
    InformationSet I_1('x', true, hash_1);
    InformationSet I_2('o', false, hash_2);
    std::vector<int> h = {true_cards.cards[0], true_cards.cards[1], true_cards.cards[2]};
    History start_history = History(h);
    play(I_1, true_cards, I_2, start_history);
    std::cout << "History: ";
    start_history.print_history();
    std::cout << "True cards bidding sequence: " << true_cards.bid_sequence << std::endl;

    return 0;
}