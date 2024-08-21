#include "cpp_headers/rbt_classes.hpp"
#include "cpp_headers/json.hpp"
using json = nlohmann::json;

void split(std::string str, std::string splitBy, std::vector<std::string>& tokens)
{
    /* Store the original string in the array, so we can loop the rest
     * of the algorithm. */
    tokens.push_back(str);

    // Store the split index in a 'size_t' (unsigned integer) type.
    size_t splitAt;
    // Store the size of what we're splicing out.
    size_t splitLen = splitBy.size();
    // Create a string for temporarily storing the fragment we're processing.
    std::string frag;
    // Loop infinitely - break is internal.
    while(true)
    {
        /* Store the last string in the vector, which is the only logical
         * candidate for processing. */
        frag = tokens.back();
        /* The index where the split is. */
        splitAt = frag.find(splitBy);
        // If we didn't find a new split point...
        if(splitAt == std::string::npos)
        {
            // Break the loop and (implicitly) return.
            break;
        }
        /* Put everything from the left side of the split where the string
         * being processed used to be. */
        tokens.back() = frag.substr(0, splitAt);
        /* Push everything from the right side of the split to the next empty
         * index in the vector. */
        tokens.push_back(frag.substr(splitAt+splitLen, frag.size()-(splitAt+splitLen)));
    }
}


std::vector<int> intersection(std::vector<int> const& left_vector, std::vector<int> const& right_vector) {
    auto left = left_vector.begin();
    auto left_end = left_vector.end();
    auto right = right_vector.begin();
    auto right_end = right_vector.end();

    assert(is_sorted(left, left_end));
    assert(is_sorted(right, right_end));

    std::vector<int> result;

    while (left != left_end && right != right_end) {
        if (*left == *right) {
            result.push_back(*left);
            ++left;
            ++right;
            continue;
        }

        if (*left < *right) {
            ++left;
            continue;
        }

        assert(*left > *right);
        ++right;
    }

    return result;
}


TicTacToeBoard::TicTacToeBoard(std::string& board) {
    if (board.empty()) {
        this->board = EMPTY_BOARD;
    } else {
        this->board = board;
    }
}

char TicTacToeBoard::operator[](int key) const {
    return this->board[key];
}

char & TicTacToeBoard::operator[](int key) {
    return this->board[key];
}

void TicTacToeBoard::operator=(const TicTacToeBoard &other) {
    this->board = other.board;
}

bool TicTacToeBoard::operator==(const TicTacToeBoard &other) {
    return this->board == other.board;
}

TicTacToeBoard TicTacToeBoard::copy() {
    return TicTacToeBoard(this->board);
}

bool TicTacToeBoard::is_win(char& winner) {
    for (int i = 0; i < 3; i++) {
        if ((this->board[3 * i] == this->board[3 * i + 1]) && (this->board[3 * i + 1] == this->board[3 * i + 2]) && (this->board[3 * i] != '0')) {
            winner = this->board[3 * i];
            return true;
        }

        if ((this->board[i] == this->board[i + 3]) && (this->board[i + 3] == this->board[i + 6]) && (this->board[i] != '0')) {
            winner = this->board[i];
            return true;
        }
    }

    if ((this->board[0] == this->board[4] && this->board[4] == this->board[8] && this->board[0] != '0')) {
        winner = this->board[0];
        return true;
    }

    if ((this->board[2] == this->board[4] && this->board[4] == this->board[6] && this->board[2] != '0')) {
        winner = this->board[2];
        return true;
    }
    
    winner = '0';
    return false;
}

bool TicTacToeBoard::is_over() {
    for (int i = 0; i < 9; i++) {
        if (this->board[i] == '0') {
            return false;
        }
    }
    return true;
}

bool TicTacToeBoard::is_draw() {
    char winner;
    if (!this->is_win(winner) && this->is_over()) {
        return true;
    }
    return false;
}

bool TicTacToeBoard::is_valid_move(int square) {
    if (square < 0 || square > 8) {
        return false;
    } else {
        return this->board[square] == '0';
    }
}

bool TicTacToeBoard::update_move(int square, char player) {
    if (this->is_valid_move(square)) {
        this->board[square] = player;
        return true;
    }
    return false;
}

void TicTacToeBoard::print_board() {
    std::cout << "+---+---+---+" << std::endl;
    for (int i = 0; i < 3; i++) {
        std::cout << "|";
        for (int j = 0; j < 3; j++) {
            if (this->board[3 * i + j] == 'x') {
                std::cout << " x |";
            } else if (this->board[3 * i + j] == 'o') {
                std::cout << " o |";
            } else {
                std::cout << " 0 |";
            }
        }
        std::cout << std::endl << "+---+---+---+" << std::endl;
    }
}

// std::unordered_map<int, std::vector<int> > InformationSet::sense_square_dict = {{9, {0, 1, 3, 4}}, {10, {1, 2, 4, 5}}, {11, {3, 4, 6, 7}}, {12, {4, 5, 7, 8}}};
std::unordered_map<std::string, long int > InformationSet::P1_hash_to_int_map = {};
std::unordered_map<std::string, long int > InformationSet::P2_hash_to_int_map = {};

InformationSet::InformationSet(char player, bool move_flag, std::string& hash) : TicTacToeBoard() {
    this->player = player;
    this->move_flag = move_flag;
    this->hash = hash;
    this->board = this->get_board_from_hash();
    
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

InformationSet::InformationSet(char player, bool move_flag, std::string& hash, std::string& board) : TicTacToeBoard() {
    this->player = player;
    this->move_flag = move_flag;
    this->hash = hash;
    this->board = board;
    
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

InformationSet::InformationSet(char player, bool move_flag, std::string& hash, std::string& board, long int index) : TicTacToeBoard() {
    this->player = player;
    this->move_flag = move_flag;
    this->hash = hash;
    this->board = board;
    this->index = index;
}

std::string InformationSet::get_board_from_hash() {
    // example hash: (P1): 6_1|00o0|0_2|0ox0|
    std::string new_board = this->player == 'x' ? "000000000" : "---------";
    bool move_action = this->player == 'x' ? true : false;
    bool sense_action = this->player == 'x' ? false : true;
    bool observation = false;
    int curr_sense_move = -1;
    int i = 0;
    std::unordered_map<int, std::vector<int> > sense_square_dict = {{9, {0, 1, 3, 4}}, {10, {1, 2, 4, 5}}, {11, {3, 4, 6, 7}}, {12, {4, 5, 7, 8}}};

    while (i < this->hash.size()) {
        switch (this->hash[i]) { // change flags at delimiters
            case '|': // indicates start or end of observation sequence
                if (observation) { // end of observation sequence
                    observation = false;
                    move_action = true;
                }
                else{ // beginning of observation sequence
                    observation = true;
                    sense_action = false;
                }

                i++;
                break;

            case '_': // an underscore only occurs immediately after a move action
                move_action = false;
                sense_action = true;

                i++;
                break;

            default: // if not a delimiter, update board
                if (move_action) {
                    this->reset_zeros(new_board); // only need to reset zeros before a move action is made
                    new_board[this->hash[i] - '0'] = this->player;
                    
                    i++;
                }
                else if (sense_action) { // simply setting the current sense move variable so the board can be updated in the observation condition below
                    curr_sense_move = this->hash[i] - '0' + 9; // sense actions are between 0 - 3 but need to be converted to 9 - 12, hence 9 is added
                    
                    i++;
                }
                else if (observation) { // update board for 4 squares based on observation
                    for (int square: sense_square_dict[curr_sense_move]) {
                        new_board[square] = this->hash[i++];
                    }
                }
        }
    }

    return new_board;
}

bool InformationSet::operator==(const InformationSet &other) {
    return this->hash == other.hash && this->player == other.player && this->move_flag == other.move_flag;
}

char InformationSet::other_player() {
    return (this->player == 'x') ? 'o' : 'x';
}

InformationSet InformationSet::copy() {
    return InformationSet(this->player, this->move_flag, this->hash, this->board, this->index);
}

std::string InformationSet::get_hash() {
    return this->hash;
}

std::string InformationSet::get_v1_hash(){
    std::string v1_hash = this->board;
    v1_hash += this->move_flag ? "m" : "s";
    return v1_hash;
}

long int InformationSet::get_index() {
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
    // if (policy_obj.policy_dict.find(this->get_hash()) == policy_obj.policy_dict.end()) {
    //     // std::cout << "KeyError: " << this->get_hash() << " not found in policy dictionary" << std::endl;
    //     return;
    // }

    if (this->index == -1) {
        // std::cout << "KeyError: " << this->get_hash() << " not found in policy dictionary" << std::endl;
        return;
    }
    else {
        // std::cout << "Index: " << this->get_index() << " for hash: " << this->get_hash() << std::endl;
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

void InformationSet::get_actions_given_policy(std::vector<int>& actions, Policy& policy_obj){
    // this->board = this->get_board_from_hash();
    std::string v1_hash = this->get_v1_hash();
    std::vector<double> prob_dist = policy_obj.policy_dict[v1_hash];
    if (this->move_flag) {
        for (int move = 0; move < 9; move++) {
            if (prob_dist[move] > 0) {
                actions.push_back(move);
            }
        }
    } else {
        for (int sense = 9; sense < 13; sense++) {
            if (prob_dist[sense] > 0) {
                actions.push_back(sense);
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
            if (this->board[i] == '0' || this->board[i] == '-') {
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
    std::unordered_map<int, std::vector<int> > sense_square_dict = {{9, {0, 1, 3, 4}}, {10, {1, 2, 4, 5}}, {11, {3, 4, 6, 7}}, {12, {4, 5, 7, 8}}};
    for (auto &sense : sense_square_dict) {
        for (int i = 0; i < 4; i++) {
            if (this->board[sense.second[i]] == '-') {
                actions.push_back(sense.first);
                break;
            }
        }
    }
}

void InformationSet::simulate_sense(int action, TicTacToeBoard& true_board) {
    this->reset_zeros();
    std::string observation = "----";
    int count = 0;
    std::unordered_map<int, std::string> sense_square_mapping = {{9, "0"}, {10, "1"}, {11, "2"}, {12, "3"}};
    std::unordered_map<int, std::vector<int> > sense_square_dict = {{9, {0, 1, 3, 4}}, {10, {1, 2, 4, 5}}, {11, {3, 4, 6, 7}}, {12, {4, 5, 7, 8}}};
    for (int square : sense_square_dict[action]) {
        this->board[square] = true_board[square];
        observation[count] = true_board[square];
        count++;
    }
    this->hash = this->hash + sense_square_mapping[action] + "|" + observation + "|";
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

void InformationSet::reset_zeros(std::string& board) {
    for (int i = 0; i < 9; i++) {
        if (board[i] == '0') {
            board[i] = '-';
        }
    }
}

void InformationSet::reset_zeros() {
    for (int i = 0; i < 9; i++) {
        if (this->board[i] == '0') {
            this->board[i] = '-';
        }
    }
}

bool InformationSet::is_valid_move(int square) {
    if (square < 0 || square > 8) {
        return false;
    } else {
        return this->board[square] == '0' || this->board[square] == '-';
    }
}

bool InformationSet::update_move(int square, char player) {
    if (this->is_valid_move(square)) {
        this->board[square] = player;
        this->hash = this->hash + std::to_string(square) + "_";
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
        if ((this->board[3 * i] == this->board[3 * i + 1] && this->board[3 * i + 1] == this->board[3 * i + 2] && this->board[3 * i] == this->player)) {
            return true;
        }

        if ((this->board[i] == this->board[i + 3] && this->board[i + 3] == this->board[i + 6] && this->board[i] == this->player)) {
            return true;
        }
    }

    if ((this->board[0] == this->board[4] && this->board[4] == this->board[8] && this->board[0] == this->player)) {
        return true;
    }

    if ((this->board[2] == this->board[4] && this->board[4] == this->board[6] && this->board[2] == this->player)) {
        return true;
    }

    return false;
}

int InformationSet::win_exists() {
    for (int i = 0; i < 9; i++) {
        if (this->board[i] == '0') {
            this->board[i] = this->player;
            if (this->is_win_for_player()) {
                this->board[i] = '0';
                return i;
            }
            this->board[i] = '0';
        }
    }

    return -1;
}

int InformationSet::draw_exists() {
    std::vector<int> zeroes;
    for (int i = 0; i < 9; i++) {
        if (this->board[i] == '0') {
            zeroes.push_back(i);
        }
    }

    for (int zero : zeroes) {
        std::string new_I_board = this->board;
        new_I_board[zero] = this->player;
        if (InformationSet(this->player, this->move_flag, this->hash, this->board).is_over()) {
            return zero;
        }
    }

    return -1;
}

bool InformationSet::is_over() {
    for (int i = 0; i < 9; i++) {
        if (this->board[i] == '0' || this->board[i] == '-') {
            return false;
        }
    }
    return true;
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

bool History::get_board(TicTacToeBoard &true_board, char& curr_player) {
    curr_player = 'x';
    for (int action : this->history) {
        if (action < 9) {
            if (!true_board.update_move(action, curr_player)) {
                return true;
            }
            curr_player = this->other_player(curr_player);
        }
    }
    curr_player = '0';
    return false;
}

void History::get_information_sets(InformationSet &I_1, InformationSet &I_2) {
    TicTacToeBoard true_board;
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
            true_board.update_move(action, curr_player);
            curr_player = this->other_player(curr_player);
        } else {
            if (curr_player == 'x') {
                I_1.simulate_sense(action, true_board);
            } else {
                I_2.simulate_sense(action, true_board);
            }
        }
    }
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
    TicTacToeBoard true_board;
    bool overlapping_move_flag;
    char overlapping_move_player;

    overlapping_move_flag = this->get_board(true_board, overlapping_move_player);

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
        if (true_board.is_win(winner)) {
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


Policy::Policy() {
    this->player = '0';
    this->policy_dict = std::unordered_map<std::string, std::vector<double> >();
}

Policy::Policy(char player, std::string& file_path) {
    this->player = player;
    this->policy_dict = this->read_policy_from_json(file_path);
}

Policy::Policy(char player, std::unordered_map<std::string, std::vector<double> >& policy_dict) {
    this->player = player;
    this->policy_dict = policy_dict;
}

Policy Policy::copy() {
    return Policy(this->player, this->policy_dict);
}

std::unordered_map<std::string, std::vector<double> > Policy::read_policy_from_json(std::string& file_path){
    std::ifstream i(file_path);
    json policy_obj;
    i >> policy_obj;
    
    for (json::iterator it = policy_obj.begin(); it != policy_obj.end(); ++it) {
        std::string key = it.key();
        std::vector <double> probability_distribution(13);
        // initialise all values to zero
        for (int i = 0; i < 13; i++) {
            probability_distribution[i] = 0;
        }

        if (key.back() == 's') {
            std::vector<std::string> sense_keys = {"9", "10", "11", "12"};
            for (int i = 0; i < sense_keys.size(); i++) {
                probability_distribution[stoi(sense_keys[i])] = policy_obj[key][sense_keys[i]];
            }
        }
        else if (key.back() == 'm') {
            std::vector<std::string> move_keys = {"0", "1", "2", "3", "4", "5", "6", "7", "8"};
            for (int i = 0; i < move_keys.size(); i++) {
                probability_distribution[stoi(move_keys[i])] = policy_obj[key][move_keys[i]];
            }
        }
        policy_dict[key] = probability_distribution;
    }

    return policy_dict;
}


PolicyVec::PolicyVec() {
    this->player = '0';
    this->policy_dict = std::vector< std::vector<double> >();
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
    
    std::ifstream i(file_path);
    std::string line;
    
    while (std::getline(i, line)) {
        int token_idx = 0;
        std::vector<std::string> tokens;
        split(line, " ", tokens);
        std::cout << "Number of tokens: " << tokens.size() << std::endl;
        for (int j = 0; j < tokens.size(); j++) {
            std::cout << tokens[j] << " ";
        }
        std::cout << std::endl;
        std::string I_hash = tokens[token_idx++];
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

        while (token_idx < tokens.size() - 1) {
            std::cout << "Token: " << tokens[token_idx] << std::endl;
            int key = std::stoi(tokens[token_idx++]);
            std::cout << "Token: " << tokens[token_idx] << std::endl;
            double value = std::stod(tokens[token_idx++]);
            probability_distribution[key] = value;
        }

        policy_list[I.get_index()] = probability_distribution;
    }

    return policy_list;
}
