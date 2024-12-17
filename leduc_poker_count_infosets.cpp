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

void play(InformationSet& I_1, PokerTable& true_cards, InformationSet& I_2, History &history, char current_player){
    InformationSet& I = current_player == 'x' ? I_1 : I_2;
    std::vector<int> actions;
    I.get_actions(actions);
    int action = actions[std::rand() % actions.size()];

    if (I.move_flag){
        bool success = true_cards.update_move(action, current_player);
        history.history.push_back(action);

        char winner;
        if (success && !true_cards.is_win(winner) && !true_cards.is_over()) {
            I.update_move(action, current_player);

            play(I_1, true_cards, I_2, history, current_player == 'x' ? 'o' : 'x');
        }
        else {
            TerminalHistory H_T = TerminalHistory(history.history);
            H_T.set_reward();

            std::cout << "Reward: " << H_T.reward[0] << std::endl;
        }
    }
    else {
        I.simulate_sense(action, true_cards);
        history.history.push_back(action);
        play(I_1, true_cards, I_2, history, current_player);
    }
}

void main() {
    std::vector<char> deck = {'J', 'J', 'Q', 'Q', 'K', 'K'};
    PokerTable true_cards;
    deal_cards(true_cards, deck);
    std::cout << "True cards: " << true_cards.cards << std::endl;
    std::string hash_1 = std::string(1, true_cards.cards[0]) + "-";
    std::string hash_2 = std::string(1, true_cards.cards[1]) + "-";
    InformationSet I_1('x', true, hash_1);
    InformationSet I_2('o', false, hash_2);
    std::vector<int> h = {true_cards.cards[0], true_cards.cards[1], true_cards.cards[2]};
    History start_history = History(h);
    play(I_1, true_cards, I_2, start_history, 'x');
    std::cout << "I_1 hash: " << I_1.get_hash() << std::endl;
    std::cout << "I_2 hash: " << I_2.get_hash() << std::endl;
    std::cout << "I_1 cards: " << I_1.get_cards_from_hash() << std::endl;
    std::cout << "I_2 cards: " << I_2.get_cards_from_hash() << std::endl;
    std::cout << "History: ";
    start_history.print_history();
    std::cout << "True cards bidding sequence: " << true_cards.bid_sequence << std::endl;
}