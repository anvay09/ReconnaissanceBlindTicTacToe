#include "rbt_classes.h"

TicTacToeBoard::TicTacToeBoard(vector<char> board) {
    if (board.empty()) {
        this->board = {'0', '0', '0', '0', '0', '0', '0', '0', '0'};
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

pair<bool, char> TicTacToeBoard::is_win() {
    for (int i = 0; i < 3; i++) {
        if ((this->board[3 * i] == this->board[3 * i + 1]) && (this->board[3 * i + 1] == this->board[3 * i + 2]) && (this->board[3 * i] != '0')) {
            return {true, this->board[3 * i]};
        }

        if ((this->board[i] == this->board[i + 3]) && (this->board[i + 3] == this->board[i + 6]) && (this->board[i] != '0')) {
            return {true, this->board[i]};
        }
    }

    if ((this->board[0] == this->board[4] && this->board[4] == this->board[8] && this->board[0] != '0')) {
        return {true, this->board[0]};
    }

    if ((this->board[2] == this->board[4] && this->board[4] == this->board[6] && this->board[2] != '0')) {
        return {true, this->board[2]};
    }
    
    return {false, '0'};
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
    if (!this->is_win().first && this->is_over()) {
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
    cout << "+---+---+---+" << endl;
    for (int i = 0; i < 3; i++) {
        cout << "|";
        for (int j = 0; j < 3; j++) {
            if (this->board[3 * i + j] == 'x') {
                cout << " x |";
            } else if (this->board[3 * i + j] == 'o') {
                cout << " o |";
            } else {
                cout << " 0 |";
            }
        }
        cout << endl << "+---+---+---+" << endl;
    }
}


InformationSet::InformationSet() : TicTacToeBoard() {
    this->sense_square_dict = {{9, {0, 1, 3, 4}}, {10, {1, 2, 4, 5}}, {11, {3, 4, 6, 7}}, {12, {4, 5, 7, 8}}};
    this->player = 'x';
    this->move_flag = true;
}

InformationSet::InformationSet(char player, bool move_flag, vector<char> board) : TicTacToeBoard(board) {
    this->sense_square_dict = {{9, {0, 1, 3, 4}}, {10, {1, 2, 4, 5}}, {11, {3, 4, 6, 7}}, {12, {4, 5, 7, 8}}};
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

string InformationSet::get_hash() {
    if (this->move_flag) {
        return string(this->board.begin(), this->board.end()) + "m";
    } else {
        return string(this->board.begin(), this->board.end()) + "s";
    }
}

vector<TicTacToeBoard> InformationSet::get_states() {
    int num_unknown_opponent_moves = this->get_number_of_unknown_opponent_moves();
    vector<char> board_copy = this->board;
    for (int i = 0; i < board_copy.size(); i++) {
        if (board_copy[i] == '-') {
            board_copy[i] = '0';
        }
    }

    if (num_unknown_opponent_moves == 0) {
        return {TicTacToeBoard(board_copy)};
    } 
    else {
        vector<TicTacToeBoard> output_states;
        vector<int> uncertain_ind = this->get_uncertain_squares();
        vector<char> base_perm(num_unknown_opponent_moves, this->other_player());
        base_perm.insert(base_perm.end(), uncertain_ind.size() - num_unknown_opponent_moves, '0');

        do {
            TicTacToeBoard new_state(board_copy);
            for (int j = 0; j < base_perm.size(); j++) {
                new_state[uncertain_ind[j]] = base_perm[j];
            }
            if (!new_state.is_win().first && !new_state.is_over()) {
                output_states.push_back(new_state);
            }
        } while (next_permutation(base_perm.begin(), base_perm.end()));

        return output_states;
    }
}

vector<int> InformationSet::get_actions() {
    if (this->move_flag) {
        return this->get_valid_moves();
    } else {
        return this->get_useful_senses();
    }
}

vector<int> InformationSet::get_actions_given_policy(Policy policy_obj) {
    vector<int> action_list;
    if (this->move_flag) {
        for (int move = 0; move < 9; move++) {
            try {
                if (policy_obj.policy_dict[this->get_hash()][move] > 0) {
                    action_list.push_back(move);
                }
            } catch (const out_of_range &e) {
                cout << this->get_hash() << " " << move << " " << this->player << endl;
                throw e;
            }
        }
    } else {
        for (auto &sense : this->sense_square_dict) {
            try {
                if (policy_obj.policy_dict[this->get_hash()][sense.first] > 0) {
                    action_list.push_back(sense.first);
                }
            } catch (const out_of_range &e) {
                cout << this->get_hash() << " " << sense.first << " " << this->player << endl;
                throw e;
            }
        }
    }

    return action_list;
}

vector<int> InformationSet::get_valid_moves() {
    int w = this->win_exists();
    if (w != -1) {
        return {w};
    }

    vector<int> valid_moves;
    for (int i = 0; i < this->board.size(); i++) {
        if (this->board[i] == '0' || this->board[i] == '-') {
            valid_moves.push_back(i);
        }
    }
    return valid_moves;
}

vector<int> InformationSet::get_played_actions() {
    vector<int> played_moves;
    for (int i = 0; i < this->board.size(); i++) {
        if (this->board[i] == this->player) {
            played_moves.push_back(i);
        }
    }
    return played_moves;
}

vector<int> InformationSet::get_useful_senses() {
    vector<int> valid_sense;
    for (auto &sense : this->sense_square_dict) {
        for (int i = 0; i < sense.second.size(); i++) {
            if (this->board[sense.second[i]] == '-') {
                valid_sense.push_back(sense.first);
                break;
            }
        }
    }
    return valid_sense;
}

int InformationSet::get_number_of_unknown_opponent_moves() {
    int count_x = 0;
    int count_o = 0;
    for (int i = 0; i < this->board.size(); i++) {
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

vector<int> InformationSet::get_uncertain_squares() {
    vector<int> uncertain_squares;
    for (int i = 0; i < this->board.size(); i++) {
        if (this->board[i] == '-') {
            uncertain_squares.push_back(i);
        }
    }
    return uncertain_squares;
}

void InformationSet::simulate_sense(int action, TicTacToeBoard true_board) {
    this->reset_zeros();
    for (int square : this->sense_square_dict[action]) {
        this->board[square] = true_board[square];
    }

    this->move_flag = true;
}

void InformationSet::reset_zeros() {
    for (int i = 0; i < this->board.size(); i++) {
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
    vector<int> zeroes;
    for (int i = 0; i < this->board.size(); i++) {
        if (this->board[i] == '0') {
            zeroes.push_back(i);
        }
    }

    for (int zero : zeroes) {
        vector<char> new_I_board = this->board;
        new_I_board[zero] = this->player;
        if (InformationSet(this->player, this->move_flag, new_I_board).is_win_for_player()) {
            return zero;
        }
    }

    return -1;
}

int InformationSet::draw_exists() {
    vector<int> zeroes;
    for (int i = 0; i < this->board.size(); i++) {
        if (this->board[i] == '0') {
            zeroes.push_back(i);
        }
    }

    for (int zero : zeroes) {
        vector<char> new_I_board = this->board;
        new_I_board[zero] = this->player;
        if (InformationSet(this->player, this->move_flag, new_I_board).is_over()) {
            return zero;
        }
    }

    return -1;
}

bool InformationSet::is_over() {
    for (int i = 0; i < this->board.size(); i++) {
        if (this->board[i] == '0' || this->board[i] == '-') {
            return false;
        }
    }
    return true;
}

int InformationSet::num_self_moves() {
    int count = 0;
    for (int i = 0; i < this->board.size(); i++) {
        if (this->board[i] == this->player) {
            count++;
        }
    }
    return count;
}


History::History(vector<int> history) {
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

tuple<TicTacToeBoard, bool, char> History::get_board() {
    TicTacToeBoard true_board;
    char curr_player = 'x';
    for (int action : this->history) {
        if (action < 9) {
            if (!true_board.update_move(action, curr_player)) {
                return {true_board, true, curr_player};
            }
            curr_player = this->other_player(curr_player);
        }
    }
    return {true_board, false, '0'};
}

pair<InformationSet, InformationSet> History::get_information_sets() {
    InformationSet I_1('x', true, {'0', '0', '0', '0', '0', '0', '0', '0', '0'});
    InformationSet I_2('o', false, {'-', '-', '-', '-', '-', '-', '-', '-', '-'});
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
    return {I_1, I_2};
}

TerminalHistory::TerminalHistory(vector<int> history, map<char, int> reward) : History(history) {
    if (reward.empty()) {
        this->reward = {{'x', 0}, {'o', 0}};
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
    tie(true_board, overlapping_move_flag, overlapping_move_player) = this->get_board();

    if (overlapping_move_flag) {
        this->reward[overlapping_move_player] = -1;
        this->reward[this->other_player(overlapping_move_player)] = 1;
    } else {
        pair<bool, char> win = true_board.is_win();
        if (win.first) {
            this->reward[win.second] = 1;
            this->reward[this->other_player(win.second)] = -1;
        }
    }
}

NonTerminalHistory::NonTerminalHistory(vector<int> history) : History(history) {}

NonTerminalHistory NonTerminalHistory::copy() {
    return NonTerminalHistory(this->history);
}


Policy::Policy(char player, map<string, map<int, double> > policy_dict) {
    this->player = player;
    this->policy_dict = policy_dict;
    vector<string> keys;
    for (auto &policy : this->policy_dict) {
        keys.push_back(policy.first);
    }
    for (string I_hash : keys) {
        map<int, double> prob_dist = this->policy_dict[I_hash];
        vector<int> actions;
        for (auto &action : prob_dist) {
            actions.push_back(action.first);
        }
        for (int action : actions) {
            if (typeid(action) == typeid(string)) {
                prob_dist[(int)action] = prob_dist[action];
                prob_dist.erase(action);
            }
        }
    }
}

Policy Policy::copy() {
    return Policy(this->player, this->policy_dict);
}

void Policy::update_policy_for_given_information_set(InformationSet information_set, vector<double> prob_distribution) {
    string I_hash = information_set.get_hash();
    map<int, double> prob_dist;
    for (int i = 0; i < prob_distribution.size(); i++) {
        prob_dist[i] = prob_distribution[i];
    }
    this->policy_dict[I_hash] = prob_dist;
}
