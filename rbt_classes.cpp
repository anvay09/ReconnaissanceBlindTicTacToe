#include "rbt_classes.hpp"
#include "json.hpp"
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


InformationSet::InformationSet() : TicTacToeBoard() {
    this->player = 'x';
    this->move_flag = true;
}

InformationSet::InformationSet(char player, bool move_flag, std::string& board) : TicTacToeBoard(board) {
    this->player = player;
    this->move_flag = move_flag;
}

bool InformationSet::operator==(const InformationSet &other) {
    return this->board == other.board && this->player == other.player && this->move_flag == other.move_flag;
}

char InformationSet::other_player() {
    return (this->player == 'x') ? 'o' : 'x';
}

InformationSet InformationSet::copy() {
    return InformationSet(this->player, this->move_flag, this->board);
}

std::string InformationSet::get_hash() {
    if (this->move_flag) {
        return this->board + "m";
    } else {
        return this->board + "s";
    }
}

void InformationSet::get_states(std::vector<TicTacToeBoard> &states) {
    int num_unknown_opponent_moves = this->get_number_of_unknown_opponent_moves();
    std::string board_copy = this->board;
    for (int i = 0; i < 9; i++) {
        if (board_copy[i] == '-') {
            board_copy[i] = '0';
        }
    }

    if (num_unknown_opponent_moves == 0) {
        states.push_back(TicTacToeBoard(board_copy));
    } 
    else {
        std::vector<int> uncertain_ind;
        this->get_uncertain_squares(uncertain_ind);
        std::vector<char> base_perm(num_unknown_opponent_moves, this->other_player());
        base_perm.insert(base_perm.end(), uncertain_ind.size() - num_unknown_opponent_moves, '0');

        do {
            TicTacToeBoard new_state(board_copy);
            for (int j = 0; j < base_perm.size(); j++) {
                new_state[uncertain_ind[j]] = base_perm[j];
            }
            char winner;
            if (!new_state.is_win(winner) && !new_state.is_over()) {
                states.push_back(new_state);
            }
        } while (next_permutation(base_perm.begin(), base_perm.end()));

    }
}

void InformationSet::get_actions(std::vector<int> &actions) {
    if (this->move_flag) {
        this->get_valid_moves(actions);
    } else {
        this->get_useful_senses(actions);
    }
}

void InformationSet::get_actions_given_policy(std::vector<int>& actions, Policy &policy_obj) {
    if (this->move_flag) {
        std::vector<double>& prob_dist = policy_obj.policy_dict[this->get_hash()];
        for (int move = 0; move < 9; move++) {
            if (prob_dist[move] > 0) {
                actions.push_back(move);
            }
        }
    } else {
        std::vector<double>& prob_dist = policy_obj.policy_dict[this->get_hash()];
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
    for (int i = 0; i < 9; i++) {
        if (this->board[i] == this->player) {
            actions.push_back(i);
        }
    }
}

void InformationSet::get_useful_senses(std::vector<int> &actions) {
    for (auto &sense : sense_square_dict) {
        for (int i = 0; i < 4; i++) {
            if (this->board[sense.second[i]] == '-') {
                actions.push_back(sense.first);
                break;
            }
        }
    }
}

int InformationSet::get_number_of_unknown_opponent_moves() {
    int count_x = 0;
    int count_o = 0;
    for (int i = 0; i < 9; i++) {
        if (this->board[i] == 'x') {
            count_x++;
        }
        if (this->board[i] == 'o') {
            count_o++;
        }
    }
    if (this->player == 'x') {
        return count_x - count_o;
    } else {
        return count_o - count_x + 1;
    }
}

void InformationSet::get_uncertain_squares(std::vector<int> &squares) {
    for (int i = 0; i < 9; i++) {
        if (this->board[i] == '-') {
            squares.push_back(i);
        }
    }
}

void InformationSet::simulate_sense(int action, TicTacToeBoard& true_board) {
    this->reset_zeros();
    for (int square : sense_square_dict[action]) {
        this->board[square] = true_board[square];
    }

    this->move_flag = true;
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
        this->move_flag = false;
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
            if (InformationSet(this->player, this->move_flag, this->board).is_win_for_player()) {
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
        if (InformationSet(this->player, this->move_flag, new_I_board).is_over()) {
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

int InformationSet::num_self_moves() {
    int count = 0;
    for (int i = 0; i < 9; i++) {
        if (this->board[i] == this->player) {
            count++;
        }
    }
    return count;
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

TerminalHistory::TerminalHistory(std::vector<int>& history, std::vector<int> reward) : History(history) {
    if (reward.empty()) {
        this->reward = {0, 0};
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
            this->reward[0] = -1;
            this->reward[1] = 1;
        }
        else {
            this->reward[0] = 1;
            this->reward[1] = -1;
        }

    } else {
        char winner;
        if (true_board.is_win(winner)) {
            if (winner == 'x') {
                this->reward[0] = 1;
                this->reward[1] = -1;
            } else {
                this->reward[0] = -1;
                this->reward[1] = 1;
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

void Policy::update_policy_for_given_information_set(InformationSet& information_set, std::vector<double>& prob_distribution) {
    std::vector<double> prob_dist;
    for (int i = 0; i < prob_distribution.size(); i++) {
        prob_dist[i] = prob_distribution[i];
    }
    this->policy_dict[information_set.get_hash()] = prob_dist;
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
