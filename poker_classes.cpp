#include "cpp_headers/poker_classes.hpp"
#include "cpp_headers/json.hpp"
using json = nlohmann::json;

size_t split(const std::string &txt, std::vector<std::string> &strs, char ch)
{
    size_t pos = txt.find( ch );
    size_t initialPos = 0;
    strs.clear();

    // Decompose statement
    while( pos != std::string::npos ) {
        strs.push_back( txt.substr( initialPos, pos - initialPos ) );
        initialPos = pos + 1;

        pos = txt.find( ch, initialPos );
    }

    // Add the last one
    strs.push_back( txt.substr( initialPos, std::min( pos, txt.size() ) - initialPos + 1 ) );

    return strs.size();
}

// LEDUC POKER NOTATION
// 1. Actions: 0: check (x), 1: bet (b), 2: call (c), 3: raise (r), 4: fold (f), 5: get observation (artificial sense action for compatibility with Reconnaisance Blind Tic Tac Toe)
// 2. Bid sequence: string with actions concatenated, e.g. "bcr" for bet, call, raise
// bid sequence can contain the alphabets 'x', 'b', 'c', 'r', 'f', 'd', 's'
// x: check
// b: bet
// c: call
// r: raise
// f: fold
// d: deal community card
// s: showdown!
// 3. Players: for compatibility with Reconnaisance Blind Tic Tac Toe, 'x' and 'o' are used 

// KUHN POKER NOTATION
// 1. Actions: 0: check (x), 1: bet (b), 2: call (c), 4: fold (f), 5: get observation (artificial sense action for compatibility with Reconnaisance Blind Tic Tac Toe)
// 2. Bid sequence: string with actions concatenated, e.g. "bcr" for bet, call, raise
// bid sequence can contain the alphabets 'x', 'b', 'c', 'f', 's'
// x: check
// b: bet
// c: call
// f: fold
// s: showdown!
// 3. Players: for compatibility with Reconnaisance Blind Tic Tac Toe, 'x' and 'o' are used 

PokerTable::PokerTable(std::string& cards, std::string& bid_sequence, char player, char game) {
    this->cards = cards;
    this->bid_sequence = bid_sequence;
    this->player_to_move = player;
    this->game = game; // 'K' for Kuhn Poker, 'L' for Leduc Poker
}

char PokerTable::operator[](int key) const {
    return this->cards[key];
}

char & PokerTable::operator[](int key) {
    return this->cards[key];
}

void PokerTable::operator=(const PokerTable &other) {
    this->cards = other.cards;
    this->bid_sequence = other.bid_sequence;
    this->player_to_move = other.player_to_move;
    this->game = other.game;
}

bool PokerTable::operator==(const PokerTable &other) {
    return this->cards == other.cards && this->bid_sequence == other.bid_sequence && this->player_to_move == other.player_to_move && this->game == other.game;
}

PokerTable PokerTable::copy() {
    return PokerTable(this->cards, this->bid_sequence, this->player_to_move, this->game);
}

bool PokerTable::is_win(char& winner) {
    if (this->game == 'L'){ // Leduc Poker
        if (this->bid_sequence.back() == 'f') {
            winner = this->player_to_move;
            return true;
        }
        else if (this->bid_sequence.back() == 's'){
            if (this->cards[0] == this->cards[2]){ // player 1 has a pair
                winner = 'x';
                return true;
            }
            else if (this->cards[1] == this->cards[2]){ // player 2 has a pair
                winner = 'o';
                return true;
            }
            else {
                if ((this->cards[0] == 'J' && (this->cards[1] == 'Q' || this->cards[1] == 'K')) || 
                    (this->cards[0] == 'Q' && this->cards[1] == 'K')){ // player 2 has high card
                    winner = 'o';
                    return true;
                }
                else if ((this->cards[0] == 'K' && (this->cards[1] == 'Q' || this->cards[1] == 'J')) ||
                        (this->cards[0] == 'Q' && this->cards[1] == 'J')){ // player 1 has high card
                    winner = 'x';
                    return true;
                }
                else { // no pairs or high cards -- draw
                    winner = '0';
                    return false;
                }
            }
        }
        else {
            winner = '0';
            return false;
        }
    }
    else if (this->game == 'K'){ // Kuhn Poker
        if (this->bid_sequence.back() == 'f') {
            winner = this->player_to_move;
            return true;
        }
        else if (this->bid_sequence.back() == 's'){
            if ((this->cards[0] == 'J' && (this->cards[1] == 'Q' || this->cards[1] == 'K')) || 
                (this->cards[0] == 'Q' && this->cards[1] == 'K')){ // player 2 has high card
                winner = 'o';
                return true;
            }
            else if ((this->cards[0] == 'K' && (this->cards[1] == 'Q' || this->cards[1] == 'J')) ||
                    (this->cards[0] == 'Q' && this->cards[1] == 'J')){ // player 1 has high card
                winner = 'x';
                return true;
            }
        }
        else {
            winner = '0';
            return false;
        }
    }
    winner = '0';
    return false;
}

bool PokerTable::is_over() {
// game is over when the bid sequence ends in 's' or 'f'
    if (this->bid_sequence.back() == 's' || this->bid_sequence.back() == 'f') {
        return true;
    }
    return false;
}

bool PokerTable::is_draw() {
    if (this->game == 'L') {
        if (this->bid_sequence.back() == 's' && this->cards[0] == this->cards[1]) {
            return true;
        }
    }
    else if (this->game == 'K') {
        return false; // Kuhn Poker has no draws
    }
    return false;
}

bool PokerTable::is_valid_move(int action) {
    if (this->bid_sequence.back() == 's' || this->bid_sequence.back() == 'f') {
        return false;
    }
    else {
        if (this->bid_sequence.empty()){
            return action == 0 || action == 1; // x or b
        }
        else if (this->bid_sequence.back() == 'x'){
            if (this->bid_sequence.size() > 1) {
                if (this->bid_sequence[this->bid_sequence.size() - 2] == 'x'){ // xx sequence, can only lead to d or s
                    return false;
                }
                else { // x or b
                    return action == 0 || action == 1;
                }
            }
            else { // x or b
                return action == 0 || action == 1;
            }
        }
        else if (this->bid_sequence.back() == 'b'){ // c, r, or f
            if (this->game == 'L') { // raise is not a valid move in Kuhn Poker
                return action == 2 || action == 3 || action == 4;
            }
            else if (this->game == 'K') {
                return action == 2 || action == 4;
            }
            else {
                return false;
            }
        }
        else if (this->bid_sequence.back() == 'c' || this->bid_sequence.back() == 'f' || this->bid_sequence.back() == 's'){
            return false;
        }
        else if (this->bid_sequence.back() == 'r'){ // f or c
            return action == 2 || action == 4;
        }
        else if (this->bid_sequence.back() == 'd'){ // x or b
            return action == 0 || action == 1;
        }
        else {
            return false;
        }
    }   
}

bool PokerTable::update_move(int action) {
    if (this->is_valid_move(action)) {
        std::vector<char> action_to_char = {'x', 'b', 'c', 'r', 'f'};
        this->bid_sequence += action_to_char[action];
        this->player_to_move = (this->player_to_move == 'x') ? 'o' : 'x';

        bool preflop = true;
        int check_count = 0;
        int i = 0;

        while (i < this->bid_sequence.size()) {
            if (this->bid_sequence[i] == 'd'){
                preflop = false;
                check_count = 0;
            }
            else if (this->bid_sequence[i] == 'x'){
                check_count++;
            }
            else if (this->bid_sequence[i] == 'c'){
                check_count = 2;
            }

            i++;
        }

        if (check_count == 2 && preflop){
            if (this->game == 'L') {
                this->bid_sequence += "d";
                this->player_to_move = 'x';
            }
            else if (this->game == 'K') {
                this->bid_sequence += "s";
            }
        }
        else if (check_count == 2 && !preflop){
            this->bid_sequence += "s";
        }

        return true;
    }
    return false;
}

std::unordered_map<std::string, int > InformationSet::P1_hash_to_int_map = {};
std::unordered_map<std::string, int > InformationSet::P2_hash_to_int_map = {};

InformationSet::InformationSet(char player, bool move_flag, std::string& hash, char game) : PokerTable() {
    this->player = player;
    this->move_flag = move_flag;
    this->hash = hash;
    this->cards = this->get_cards_from_hash();
    this->game = game;
    
    if (player == 'x') {
        if (P1_hash_to_int_map.find(hash) == InformationSet::P1_hash_to_int_map.end()) {
            this->index = -1;
        }
        else {
            this->index = InformationSet::P1_hash_to_int_map[hash];
        }
    } else {
        if (P2_hash_to_int_map.find(hash) == InformationSet::P2_hash_to_int_map.end()) {
            this->index = -1;
        }
        else {
            this->index = InformationSet::P2_hash_to_int_map[hash];
        }
    }
}

InformationSet::InformationSet(char player, bool move_flag, std::string& hash, std::string& cards, char game) : PokerTable() {
    this->player = player;
    this->move_flag = move_flag;
    this->hash = hash;
    this->cards = cards;
    this->game = game;
    
    if (player == 'x') {
        if (P1_hash_to_int_map.find(hash) == InformationSet::P1_hash_to_int_map.end()) {
            this->index = -1;
        }
        else {
            this->index = InformationSet::P1_hash_to_int_map[hash];
        }
    } else {
        if (P2_hash_to_int_map.find(hash) == InformationSet::P2_hash_to_int_map.end()) {
            this->index = -1;
        }
        else {
            this->index = InformationSet::P2_hash_to_int_map[hash];
        }
    }
}

InformationSet::InformationSet(char player, bool move_flag, std::string& hash, std::string& cards, int index, char game) : PokerTable() {
    this->player = player;
    this->move_flag = move_flag;
    this->hash = hash;
    this->cards = cards;
    this->index = index;
    this->game = game;
}

std::string InformationSet::get_cards_from_hash() {
    std::string cards = "--";
    if (this->hash.empty()){
        return cards;
    }
    else {
        cards[0] = this->hash[2];
        cards[1] = this->hash[3];
        return cards;
    }
}

bool InformationSet::operator==(const InformationSet &other) {
    return this->hash == other.hash && this->player == other.player && this->move_flag == other.move_flag && this->cards == other.cards && this->index == other.index && this->game == other.game;
}

char InformationSet::other_player() {
    return (this->player == 'x') ? 'o' : 'x';
}

InformationSet InformationSet::copy() {
    return InformationSet(this->player, this->move_flag, this->hash, this->cards, this->index, this->game);
}

std::string InformationSet::get_hash() {
    return this->hash;
}

int InformationSet::get_index() {
    return this->index;
}

void InformationSet::get_actions(std::vector<int> &actions) {
    if (this->move_flag) {
        this->get_valid_moves(actions);
    } else {
        this->get_useful_senses(actions);
    }
}

void InformationSet::get_actions_given_policy(std::vector<int>& actions, PolicyVec &policy_obj) {
    if (this->index == -1) {
        return;
    }
    else {
        if (this->move_flag) {
            std::vector<double>& prob_dist = policy_obj.policy_dict[this->get_index()];
            for (int move = 0; move < 5; move++) {
                if (prob_dist[move] > 0) {
                    actions.push_back(move);
                }
            }
        } else {
            actions.push_back(5);
        }
    }
}

void InformationSet::get_valid_moves(std::vector<int> &actions) {
    if (this->hash.back() == 's' || this->hash.back() == 'f') {
        return;
    }
    else {
        if (this->hash.size() == 5){
            actions.push_back(0); // x
            actions.push_back(1); // b
            return; 
        }
        else if (this->hash.back() == 'x'){ 
            if (this->hash[this->hash.size() - 2] == 'x'){ // xx sequence, can only lead to d or s
                return;
            }
            else { // x or b
                actions.push_back(0); // x
                actions.push_back(1); // b
                return;
            }
        }
        else if (this->hash.back() == 'b'){ // c, r, or f
            actions.push_back(2);

            if (this->game == 'L') { // raise is not a valid move in Kuhn Poker
                actions.push_back(3);
            }

            actions.push_back(4);
            return;
        }
        else if (this->hash.back() == 'c' || this->hash.back() == 'f' || this->hash.back() == 's'){
            return;
        }
        else if (this->hash.back() == 'r'){ // f or c
            actions.push_back(2);
            actions.push_back(4);
            return;
        }
        else if (this->hash.back() == 'd'){ // x or b
            actions.push_back(0);
            actions.push_back(1);
            return;
        }
        else {
            return;
        }
    }   
}

void InformationSet::get_useful_senses(std::vector<int> &actions) {
    actions.push_back(5);
    return;
}

void InformationSet::simulate_sense(int action, PokerTable& true_cards) {
    bool reveal_flop = false;
    if (true_cards.bid_sequence.size() > 2) {
        if (this->player == 'x') {
            if (true_cards.bid_sequence.back() == 'd') {
                reveal_flop = true;
            }
        }
        else {
            if (true_cards.bid_sequence[true_cards.bid_sequence.size() - 2] == 'd') {
                reveal_flop = true;
            }
        }
    }
    
    if (reveal_flop) {
        this->cards[1] = true_cards.cards[2];
        this->hash = "a-" + this->cards + "-" + true_cards.bid_sequence;
    }
    else {
        this->hash = "a-" + this->cards + "-" + true_cards.bid_sequence;
    }

    this->move_flag = true;
    if (this->player == 'x'){
        if (InformationSet::P1_hash_to_int_map.find(this->hash) == InformationSet::P1_hash_to_int_map.end()) {
            this->index = -1;
        }
        else {
            this->index = InformationSet::P1_hash_to_int_map[this->hash];
        }
    }
    else {
        if (InformationSet::P2_hash_to_int_map.find(this->hash) == InformationSet::P2_hash_to_int_map.end()) {
            this->index = -1;
        }
        else {
            this->index = InformationSet::P2_hash_to_int_map[this->hash];
        }
    }
}

bool InformationSet::is_valid_move(int action) { 
    if (this->hash.back() == 's' || this->hash.back() == 'f') {
        return false;
    }
    else {
        if (this->hash.size() == 5){
            return action == 0 || action == 1; // x or b
        }
        else if (this->hash.back() == 'x'){ 
            if (this->hash[this->hash.size() - 2] == 'x'){ // xx sequence, can only lead to d or s
                return false;
            }
            else { // x or b
                return action == 0 || action == 1;
            }
        }
        else if (this->hash.back() == 'b'){ // c, r, or f
            if (this->game == 'L') {
                return action == 2 || action == 3 || action == 4;
            }
            else if (this->game == 'K') {
                return action == 2 || action == 4;
            }
            else {
                return false;
            }
        }
        else if (this->hash.back() == 'c' || this->hash.back() == 'f' || this->hash.back() == 's'){
            return false;
        }
        else if (this->hash.back() == 'r'){ // f or c
            return action == 2 || action == 4;
        }
        else if (this->hash.back() == 'd'){ // x or b
            return action == 0 || action == 1;
        }
        else {
            return false;
        }
    }   
}

bool InformationSet::update_move(int action) { 
    if (this->is_valid_move(action)) {
        std::vector<char> action_to_char = {'x', 'b', 'c', 'r', 'f'};
        this->hash += std::string(1, action_to_char[action]);
        this->hash[0] = 'o';
        this->move_flag = false;
        if (this->player == 'x'){
            if (InformationSet::P1_hash_to_int_map.find(this->hash) == InformationSet::P1_hash_to_int_map.end()) {
                this->index = -1;
            }
            else {
                this->index = InformationSet::P1_hash_to_int_map[this->hash];
            }
        }
        else {
            if (InformationSet::P2_hash_to_int_map.find(this->hash) == InformationSet::P2_hash_to_int_map.end()) {
                this->index = -1;
            }
            else {
                this->index = InformationSet::P2_hash_to_int_map[this->hash];
            }
        }

        return true;
    }
    return false;
}

bool InformationSet::is_over() { 
    if (this->hash.back() == 's' || this->hash.back() == 'f') {
        return true;
    }
    return false;
}

// History: first node in the game is a chance node, so the history should start with 3 cards followed by the action sequence
// We use 74 for J, 81 for Q, 75 for K (as per ASCII values), history format will be < player 1 card, player 2 card, board card, action sequence >

History::History(std::vector<int>& history) {
    if (history.empty()) {
        this->history = {};
    } else {
        this->history = history;
    }
    this->track_traversal_index = 0;
}

char History::other_player(char player) {
    return (player == 'x') ? 'o' : 'x';
}

std::vector<double> History::update_true_cards_given_history(PokerTable &true_cards) {
    double investment_x = 1.0;
    double investment_o = 1.0;
    bool preflop = true;
    std::vector<char> action_to_char = {'x', 'b', 'c', 'r', 'f'};
    true_cards.cards[0] = this->history[0];
    true_cards.cards[1] = this->history[1];
    true_cards.cards[2] = this->history[2];
 
    for (int action : this->history) {
        if (action < 5) {
            if (action == 1) { // bet
                if (preflop) {
                    if (true_cards.player_to_move == 'x'){
                        if (true_cards.game == 'L'){
                            investment_x += 2.0;
                        }
                        else if (true_cards.game == 'K'){
                            investment_x += 1.0;
                        }
                    }
                    else {
                        if (true_cards.game == 'L'){
                            investment_o += 2.0;
                        }
                        else if (true_cards.game == 'K'){
                            investment_o += 1.0;
                        }
                    }
                }
                else {
                    if (true_cards.player_to_move == 'x'){
                        if (true_cards.game == 'L'){
                            investment_x += 4.0;
                        }
                    }
                    else {
                        if (true_cards.game == 'L'){
                            investment_o += 4.0;
                        }
                    }
                }
            }
            else if (action == 2) { // call
                if (preflop) {
                    if (true_cards.player_to_move == 'x'){
                        investment_x = investment_o;
                    }
                    else {
                        investment_o = investment_x;
                    }
                }
                else {
                    if (true_cards.player_to_move == 'x'){
                        investment_x = investment_o;
                    }
                    else {
                        investment_o = investment_x;
                    }
                }
            }
            else if (action == 3) { // raise
                if (preflop) {
                    if (true_cards.player_to_move == 'x'){
                        if (true_cards.game == 'L'){
                            investment_x += 4.0;
                        }
                    }
                    else {
                        if (true_cards.game == 'L'){
                            investment_o += 4.0;
                        }
                    }
                }
                else {
                    if (true_cards.player_to_move == 'x'){
                        if (true_cards.game == 'L'){
                            investment_x += 8.0;
                        }
                    }
                    else {
                        if (true_cards.game == 'L'){
                            investment_o += 8.0;
                        }
                    }
                }
            }
            
            true_cards.update_move(action);
            if (true_cards.bid_sequence.back() == 'd') {
                preflop = false;
            }
        }
    }

    std::vector<double> investments = {investment_x, investment_o};
    return investments;
}

void History::get_information_sets(InformationSet &I_1, InformationSet &I_2) {
    PokerTable true_cards;
    true_cards.game = I_1.game;
    true_cards.cards[0] = this->history[0];
    true_cards.cards[1] = this->history[1];
    true_cards.cards[2] = this->history[2];
    
    for (int action : this->history) {
        if (action < 5) {
            if (true_cards.player_to_move == 'x') {
                true_cards.update_move(action);
                I_1.update_move(action);
            } else {
                true_cards.update_move(action);
                I_2.update_move(action);
            }
            
        } else if (action == 5) {
            if (true_cards.player_to_move == 'x') {
                I_1.simulate_sense(action, true_cards);
            } else {
                I_2.simulate_sense(action, true_cards);
            }
        }
    }
}

void History::print_history() {
    for (int action : this->history) {
        std::cout << action << " ";
    }
    std::cout << std::endl;
}

TerminalHistory::TerminalHistory(std::vector<int>& history, std::vector<double> reward) : History(history) {
    if (reward.empty()) {
        this->reward = {0.0, 0.0};
    } else {
        this->reward = reward;
    }
}

TerminalHistory TerminalHistory::copy() {
    return TerminalHistory(this->history, this->reward);
}

void TerminalHistory::set_reward() { 
    PokerTable true_cards;
    std::vector<double> investments = this->update_true_cards_given_history(true_cards);
    char winner;
    
    if (true_cards.is_win(winner)) {
        if (winner == 'x') {
            this->reward[0] = investments[1];
            this->reward[1] = - investments[1];
        } else {
            this->reward[0] = - investments[0];
            this->reward[1] = investments[0];
        }
    }
}

NonTerminalHistory::NonTerminalHistory(std::vector<int>& history) : History(history) {}

NonTerminalHistory NonTerminalHistory::copy() {
    return NonTerminalHistory(this->history);
}


PolicyVec::PolicyVec() {
    this->player = '0';
    this->policy_dict = std::vector< std::vector<double> >();
}

PolicyVec::PolicyVec(char player, std::vector<std::string> &information_sets, char game) { 
    this->player = player;
    std::vector< std::vector<double> > policy_list(information_sets.size());

    for (int i = 0; i < information_sets.size(); i++) {
        std::string I_hash = information_sets[i];
        bool move_flag;

        if (I_hash.size() != 0){
            move_flag = I_hash[0] == 'a' ? true : false;
        }
        else {
            move_flag = player == 'x' ? true : false;
        }

        InformationSet I(player, move_flag, I_hash, game);
        std::vector<int> actions;
        I.get_actions(actions);

        std::vector<double> probability_distribution(6, 0.0);

        if (actions.size() > 0) {
            for (int action : actions) {
                probability_distribution[action] = 1.0 / ((double) actions.size());
            }
        }
      
        policy_list[I.get_index()] = probability_distribution;
    }

    this->policy_dict = policy_list;
}

PolicyVec::PolicyVec(char player, std::string& file_path, char game) {
    this->player = player;
    this->policy_dict = this->read_policy_from_json(file_path, player, game);
}

PolicyVec::PolicyVec(char player, std::string& file_path, char game, bool from_txt) {
    this->player = player;
    if (from_txt) {
        this->policy_dict = this->read_policy_from_txt(file_path, player, game);
    }
    else {
        this->policy_dict = this->read_policy_from_json(file_path, player, game);
    }
}

PolicyVec::PolicyVec(char player, std::vector< std::vector<double> >& policy_dict) {
    this->player = player;
    this->policy_dict = policy_dict;
}

PolicyVec PolicyVec::copy() {
    return PolicyVec(this->player, this->policy_dict);
}

std::vector< std::vector<double>> PolicyVec::read_policy_from_json(std::string& file_path, char player, char game){ 
    int policy_size = player == 'x' ? InformationSet::P1_hash_to_int_map.size() : InformationSet::P2_hash_to_int_map.size();
    std::vector< std::vector<double> > policy_list(policy_size);
    
    std::ifstream i(file_path);
    json policy_obj;
    i >> policy_obj;
    
    for (json::iterator it = policy_obj.begin(); it != policy_obj.end(); ++it) {
        std::string I_hash = it.key();
        bool move_flag;
        if (I_hash.size() != 0){
            move_flag = I_hash[0] == 'a' ? true : false;
        }
        else {
            move_flag = player == 'x' ? true : false;
        }

        InformationSet I(player, move_flag, I_hash, game);

        std::vector <double> probability_distribution(6);
        // initialise all values to zero
        for (int i = 0; i < 6; i++) {
            probability_distribution[i] = 0.0;
        }

        if (!move_flag) {
            std::vector<std::string> sense_keys = {"5"};
            for (int i = 0; i < sense_keys.size(); i++) {
                probability_distribution[stoi(sense_keys[i])] = policy_obj[I_hash][sense_keys[i]];
            }
        }
        else if (move_flag) {
            std::vector<std::string> move_keys = {"0", "1", "2", "3", "4"};
            for (int i = 0; i < move_keys.size(); i++) {
                probability_distribution[stoi(move_keys[i])] = policy_obj[I_hash][move_keys[i]];
            }
        }

        policy_list[I.get_index()] = probability_distribution;
    }

    return policy_list;
}

std::vector< std::vector<double> > PolicyVec::read_policy_from_txt(std::string& file_path, char player, char game){
    int policy_size = player == 'x' ? InformationSet::P1_hash_to_int_map.size() : InformationSet::P2_hash_to_int_map.size();
    std::vector< std::vector<double> > policy_list(policy_size);

    for (int i = 0; i < policy_size; i++) {
        std::vector<double> probability_distribution(6);
        // initialise all values to zero
        for (int i = 0; i < 6; i++) {
            probability_distribution[i] = 0.0;
        }
        policy_list[i] = probability_distribution;
    }
    
    std::ifstream i_file(file_path);
    std::string line;
    
    while (std::getline(i_file, line)) {
        int token_idx = 0;
        std::vector<std::string> tokens;
        split(line, tokens, ' ');

        std::string I_hash = tokens[token_idx++];
        
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

        while (token_idx < tokens.size()-1) {
            int key = std::stoi(tokens[token_idx++]);
            double value = std::stod(tokens[token_idx++]);
            probability_distribution[key] = value;
        }

        policy_list[I.get_index()] = probability_distribution;
    }

    return policy_list;
}
