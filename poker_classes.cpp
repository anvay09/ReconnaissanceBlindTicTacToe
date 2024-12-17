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

PokerTable::PokerTable(std::string& cards, std::string& bid_sequence) {
    if (cards.empty()) {
        this->cards = EMPTY_TABLE;
    } else {
        this->cards = cards;
    }

    if (bid_sequence.empty()) {
        this->bid_sequence = "";
    } else {
        this->bid_sequence = bid_sequence;
    }
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
}

bool PokerTable::operator==(const PokerTable &other) {
    return this->cards == other.cards && this->bid_sequence == other.bid_sequence;
}

PokerTable PokerTable::copy() {
    return PokerTable(this->cards, this->bid_sequence);
}

bool PokerTable::is_win(char& winner) {
// if bid sequence ends in f, calculate which player folded; if bid sequence ends in s, check for pairs and high cards
    if (this->bid_sequence.back() == 'f') {
        bool turn = true; 
        int i = 0;
        while (i < this->bid_sequence.size()) {
            if (this->bid_sequence[i] != 'd'){
                turn = !turn;
            }
            else{
                turn = true;
            }
            i++;
        }

        winner = turn ? 'x' : 'o';
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

bool PokerTable::is_over() {
// game is over when the bid sequence ends in 's' or 'f'
    if (this->bid_sequence.back() == 's' || this->bid_sequence.back() == 'f') {
        return true;
    }
    return false;
}

bool PokerTable::is_draw() {
// game is a draw when the bid sequence ends in 's' and both players have the same hand
    if (this->bid_sequence.back() == 's' && this->cards[0] == this->cards[1]) {
        return true;
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
            return action == 2 || action == 3 || action == 4;
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

bool PokerTable::update_move(int action, char player) {
    if (this->is_valid_move(action)) {
        std::vector<char> action_to_char = {'x', 'b', 'c', 'r', 'f'};
        this->bid_sequence += action_to_char[action];
        return true;
    }
    return false;
}

std::unordered_map<std::string, int > InformationSet::P1_hash_to_int_map = {};
std::unordered_map<std::string, int > InformationSet::P2_hash_to_int_map = {};

InformationSet::InformationSet(char player, bool move_flag, std::string& hash) : PokerTable() {
    this->player = player;
    this->move_flag = move_flag;
    this->hash = hash;
    this->cards = this->get_cards_from_hash();
    
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

InformationSet::InformationSet(char player, bool move_flag, std::string& hash, std::string& cards) : PokerTable() {
    this->player = player;
    this->move_flag = move_flag;
    this->hash = hash;
    this->cards = cards;
    
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

InformationSet::InformationSet(char player, bool move_flag, std::string& hash, std::string& cards, int index) : PokerTable() {
    this->player = player;
    this->move_flag = move_flag;
    this->hash = hash;
    this->cards = cards;
    this->index = index;
}

std::string InformationSet::get_cards_from_hash() {
    std::string cards = "--";
    if (this->hash.empty()){
        return cards;
    }
    else {
        cards[0] = this->hash[0];
        if (this->hash[1] == 'J' || this->hash[1] == 'Q' || this->hash[1] == 'K'){
            cards[1] = this->hash[1];
        }
        return cards;
    }
}

bool InformationSet::operator==(const InformationSet &other) {
    return this->hash == other.hash && this->player == other.player && this->move_flag == other.move_flag;
}

char InformationSet::other_player() {
    return (this->player == 'x') ? 'o' : 'x';
}

InformationSet InformationSet::copy() {
    return InformationSet(this->player, this->move_flag, this->hash, this->cards, this->index);
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
            for (int move = 0; move < 9; move++) {
                if (prob_dist[move] > 0) {
                    actions.push_back(move);
                }
            }
        } else {
            std::vector<double>& prob_dist = policy_obj.policy_dict[this->get_index()];
            for (int sense = 9; sense < 13; sense++) {
                if (prob_dist[sense] > 0) {
                    actions.push_back(sense);
                }
            }
        }
    }
}


void InformationSet::get_valid_moves(std::vector<int> &actions) {
    int w = this->win_exists();
    if (w != -1) {
        actions.push_back(w);
    }
    else {
        for (int i = 0; i < 9; i++) {
            if (this->cards[i] == '0' || this->cards[i] == '-') {
                actions.push_back(i);
            }
        }
    }
}

void InformationSet::get_played_actions(std::vector<int> &actions) {
    bool move_action = this->player == 'x' ? true : false;
    bool sense_action = this->player == 'x' ? false : true;
    bool observation = false;
    int i = 0;

    while (i < this->hash.size()) {
        switch (this->hash[i]) { 
            case '|': 
                if (observation) { 
                    observation = false;
                    move_action = true;
                }
                else{
                    observation = true;
                    sense_action = false;
                }

                i++;
                break;

            case '_': 
                move_action = false;
                sense_action = true;

                i++;
                break;

            default: 
                if (move_action) {
                    actions.push_back(this->hash[i] - '0');
                    i++;
                }
                else if (sense_action) { 
                    actions.push_back(this->hash[i] - '0' + 9);
                    i++;
                }
                else if (observation) { 
                    i += 4;
                }
        }
    }
}

void InformationSet::get_useful_senses(std::vector<int> &actions) {
    std::unordered_map<int, std::vector<int> > sense_action_dict = {{9, {0, 1, 3, 4}}, {10, {1, 2, 4, 5}}, {11, {3, 4, 6, 7}}, {12, {4, 5, 7, 8}}};
    for (auto &sense : sense_action_dict) {
        for (int i = 0; i < 4; i++) {
            if (this->cards[sense.second[i]] == '-') {
                actions.push_back(sense.first);
                break;
            }
        }
    }
}

void InformationSet::simulate_sense(int action, PokerTable& true_cards) {
    this->reset_zeros();
    std::string observation = "----";
    int count = 0;
    std::unordered_map<int, std::string> sense_action_mapping = {{9, "0"}, {10, "1"}, {11, "2"}, {12, "3"}};
    std::unordered_map<int, std::vector<int> > sense_action_dict = {{9, {0, 1, 3, 4}}, {10, {1, 2, 4, 5}}, {11, {3, 4, 6, 7}}, {12, {4, 5, 7, 8}}};
    for (int action : sense_action_dict[action]) {
        this->cards[action] = true_cards[action];
        observation[count] = true_cards[action];
        count++;
    }
    this->hash = this->hash + sense_action_mapping[action] + "|" + observation + "|";
    this->move_flag = true;
    if (this->player == 'x'){
        if (InformationSet::P1_hash_to_int_map.find(this->hash) == InformationSet::P1_hash_to_int_map.end()) {
            this->index = -1;
            // std::cout << "InformationSet::simulate_sense: KeyError: " << this->hash << " not found in P1_hash_to_int_map" << std::endl;
        }
        else {
            this->index = InformationSet::P1_hash_to_int_map[this->hash];
        }
    }
    else {
        if (InformationSet::P2_hash_to_int_map.find(this->hash) == InformationSet::P2_hash_to_int_map.end()) {
            this->index = -1;
            // std::cout << "InformationSet::simulate_sense: KeyError: " << this->hash << " not found in P2_hash_to_int_map" << std::endl;
        }
        else {
            this->index = InformationSet::P2_hash_to_int_map[this->hash];
        }
    }
}

void InformationSet::reset_zeros(std::string& cards) {
    for (int i = 0; i < 9; i++) {
        if (cards[i] == '0') {
            cards[i] = '-';
        }
    }
}

void InformationSet::reset_zeros() {
    for (int i = 0; i < 9; i++) {
        if (this->cards[i] == '0') {
            this->cards[i] = '-';
        }
    }
}

bool InformationSet::is_valid_move(int action) {
    if (action < 0 || action > 8) {
        return false;
    } else {
        return this->cards[action] == '0' || this->cards[action] == '-';
    }
}

bool InformationSet::update_move(int action, char player) {
    if (this->is_valid_move(action)) {
        this->cards[action] = player;
        this->hash = this->hash + std::to_string(action) + "_";
        this->move_flag = false;
        if (this->player == 'x'){
            if (InformationSet::P1_hash_to_int_map.find(this->hash) == InformationSet::P1_hash_to_int_map.end()) {
                this->index = -1;
                // std::cout << "InformationSet::update_move: KeyError: " << this->hash << " not found in P1_hash_to_int_map" << std::endl;
            }
            else {
                this->index = InformationSet::P1_hash_to_int_map[this->hash];
            }
        }
        else {
            if (InformationSet::P2_hash_to_int_map.find(this->hash) == InformationSet::P2_hash_to_int_map.end()) {
                this->index = -1;
                // std::cout << "InformationSet::update_move: KeyError: " << this->hash << " not found in P2_hash_to_int_map" << std::endl;
            }
            else {
                this->index = InformationSet::P2_hash_to_int_map[this->hash];
            }
        }

        return true;
    }
    return false;
}

bool InformationSet::is_win_for_player() {
    for (int i = 0; i < 3; i++) {
        if ((this->cards[3 * i] == this->cards[3 * i + 1] && this->cards[3 * i + 1] == this->cards[3 * i + 2] && this->cards[3 * i] == this->player)) {
            return true;
        }

        if ((this->cards[i] == this->cards[i + 3] && this->cards[i + 3] == this->cards[i + 6] && this->cards[i] == this->player)) {
            return true;
        }
    }

    if ((this->cards[0] == this->cards[4] && this->cards[4] == this->cards[8] && this->cards[0] == this->player)) {
        return true;
    }

    if ((this->cards[2] == this->cards[4] && this->cards[4] == this->cards[6] && this->cards[2] == this->player)) {
        return true;
    }

    return false;
}

int InformationSet::win_exists() {
    for (int i = 0; i < 9; i++) {
        if (this->cards[i] == '0') {
            this->cards[i] = this->player;
            if (this->is_win_for_player()) {
                this->cards[i] = '0';
                return i;
            }
            this->cards[i] = '0';
        }
    }

    return -1;
}

int InformationSet::draw_exists() {
    std::vector<int> zeroes;
    for (int i = 0; i < 9; i++) {
        if (this->cards[i] == '0') {
            zeroes.push_back(i);
        }
    }

    for (int zero : zeroes) {
        std::string new_I_cards = this->cards;
        new_I_cards[zero] = this->player;
        if (InformationSet(this->player, this->move_flag, this->hash, this->cards).is_over()) {
            return zero;
        }
    }

    return -1;
}

bool InformationSet::is_over() {
    for (int i = 0; i < 9; i++) {
        if (this->cards[i] == '0' || this->cards[i] == '-') {
            return false;
        }
    }
    return true;
}

double InformationSet::get_number_of_actions() {
    int i = 0;
    double count_underscore = 0;
    double count_pipe = 0;
    while (i < this->hash.size())
    {
        if (this->hash[i] == '_') {
            count_underscore += 1.0;
        }
        if (this->hash[i] == '|') {
            count_pipe += 1.0;
        }
        i++;
    }

    return count_underscore + (count_pipe/2);

}


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

bool History::get_cards(PokerTable &true_cards, char& curr_player) {
    curr_player = 'x';
    for (int action : this->history) {
        if (action < 9) {
            if (!true_cards.update_move(action, curr_player)) {
                return true;
            }
            curr_player = this->other_player(curr_player);
        }
    }
    curr_player = '0';
    return false;
}

void History::get_information_sets(InformationSet &I_1, InformationSet &I_2) {
    PokerTable true_cards;
    char curr_player = 'x';
    for (int action : this->history) {
        if (action < 9) {
            if (curr_player == 'x') {
                I_1.update_move(action, curr_player);
                I_1.reset_zeros();
            } else {
                I_2.update_move(action, curr_player);
                I_2.reset_zeros();
            }
            true_cards.update_move(action, curr_player);
            curr_player = this->other_player(curr_player);
        } else {
            if (curr_player == 'x') {
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
    bool overlapping_move_flag;
    char overlapping_move_player;

    overlapping_move_flag = this->get_cards(true_cards, overlapping_move_player);

    if (overlapping_move_flag) {
        if (overlapping_move_player == 'x'){
            this->reward[0] = -1.0;
            this->reward[1] = 1.0;
        }
        else {
            this->reward[0] = 1.0;
            this->reward[1] = -1.0;
        }

    } else {
        char winner;
        if (true_cards.is_win(winner)) {
            if (winner == 'x') {
                this->reward[0] = 1.0;
                this->reward[1] = -1.0;
            } else {
                this->reward[0] = -1.0;
                this->reward[1] = 1.0;
            }
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

PolicyVec::PolicyVec(char player, std::vector<std::string> & information_sets) {
    this->player = player;
    std::vector< std::vector<double> > policy_list(information_sets.size());

    for (long int i = 0; i < information_sets.size(); i++) {
        std::string I_hash = information_sets[i];
        bool move_flag;

        if (I_hash.size() != 0){
            move_flag = I_hash[I_hash.size()-1] == '|' ? true : false;
        }
        else {
            move_flag = player == 'x' ? true : false;
        }

        InformationSet I(player, move_flag, I_hash);
        std::vector<int> actions;
        I.get_actions(actions);

        std::vector<double> probability_distribution(13, 0.0);

        if (actions.size() > 0) {
            for (int action : actions) {
                probability_distribution[action] = 1.0 / ((double) actions.size());
            }
        }
      
        policy_list[I.get_index()] = probability_distribution;
    }

    this->policy_dict = policy_list;
}

PolicyVec::PolicyVec(char player, std::string& file_path) {
    this->player = player;
    this->policy_dict = this->read_policy_from_json(file_path, player);
}

PolicyVec::PolicyVec(char player, std::string& file_path, bool from_txt) {
    this->player = player;
    if (from_txt) {
        this->policy_dict = this->read_policy_from_txt(file_path, player);
    }
    else {
        this->policy_dict = this->read_policy_from_json(file_path, player);
    }
}

PolicyVec::PolicyVec(char player, std::vector< std::vector<double> >& policy_dict) {
    this->player = player;
    this->policy_dict = policy_dict;
}

PolicyVec PolicyVec::copy() {
    return PolicyVec(this->player, this->policy_dict);
}

std::vector< std::vector<double> > PolicyVec::read_policy_from_json(std::string& file_path, char player){
    long int policy_size = player == 'x' ? InformationSet::P1_hash_to_int_map.size() : InformationSet::P2_hash_to_int_map.size();
    std::vector< std::vector<double> > policy_list(policy_size);
    
    std::ifstream i(file_path);
    json policy_obj;
    i >> policy_obj;
    
    for (json::iterator it = policy_obj.begin(); it != policy_obj.end(); ++it) {
        std::string I_hash = it.key();
        bool move_flag;
        if (I_hash.size() != 0){
            move_flag = I_hash[I_hash.size()-1] == '|' ? true : false;
        }
        else {
            move_flag = player == 'x' ? true : false;
        }

        InformationSet I(player, move_flag, I_hash);

        std::vector <double> probability_distribution(13);
        // initialise all values to zero
        for (int i = 0; i < 13; i++) {
            probability_distribution[i] = 0.0;
        }

        if (I_hash.back() == '_') {
            std::vector<std::string> sense_keys = {"9", "10", "11", "12"};
            for (int i = 0; i < sense_keys.size(); i++) {
                probability_distribution[stoi(sense_keys[i])] = policy_obj[I_hash][sense_keys[i]];
            }
        }
        else if (I_hash.back() == '|') {
            std::vector<std::string> move_keys = {"0", "1", "2", "3", "4", "5", "6", "7", "8"};
            for (int i = 0; i < move_keys.size(); i++) {
                probability_distribution[stoi(move_keys[i])] = policy_obj[I_hash][move_keys[i]];
            }
        }
        else {
            if (player == 'x'){
                std::vector<std::string> move_keys = {"0", "1", "2", "3", "4", "5", "6", "7", "8"};
                for (int i = 0; i < move_keys.size(); i++) {
                    probability_distribution[stoi(move_keys[i])] = policy_obj[I_hash][move_keys[i]];
                }
            }
            else{
                std::vector<std::string> sense_keys = {"9", "10", "11", "12"};
                for (int i = 0; i < sense_keys.size(); i++) {
                    probability_distribution[stoi(sense_keys[i])] = policy_obj[I_hash][sense_keys[i]];
                }
            }
        }

        policy_list[I.get_index()] = probability_distribution;
    }

    return policy_list;
}

std::vector< std::vector<double> > PolicyVec::read_policy_from_txt(std::string& file_path, char player){
    long int policy_size = player == 'x' ? InformationSet::P1_hash_to_int_map.size() : InformationSet::P2_hash_to_int_map.size();
    std::vector< std::vector<double> > policy_list(policy_size);

    for (long int i = 0; i < policy_size; i++) {
        std::vector<double> probability_distribution(13);
        // initialise all values to zero
        for (int i = 0; i < 13; i++) {
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
        if (I_hash == "*") {
            I_hash = "";
        }
        
        bool move_flag;
        if (I_hash.size() != 0){
            move_flag = I_hash[I_hash.size()-1] == '|' ? true : false;
        }
        else {
            move_flag = player == 'x' ? true : false;
        }

        InformationSet I(player, move_flag, I_hash);

        std::vector <double> probability_distribution(13);
        // initialise all values to zero
        for (int i = 0; i < 13; i++) {
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
