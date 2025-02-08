#include "cpp_headers/rbt_classes.hpp"
#include "cpp_headers/rbt_utilities.hpp"
#include <random>
#include <cmath>
#include "Eigen/Dense"
#include <functional>

int NUMBER_THREADS = 96;

static std::random_device rd;
static std::mt19937 generator(rd());


double kullback_leibler(const Eigen::VectorXd &p, const Eigen::VectorXd &q) {
    double kl = 0.0;
    for (int i = 0; i < p.size(); i++) {
        if (p(i) > 0) {
            if (q(i) > 0)
                kl += p(i) * std::log(p(i) / q(i));
            else
                return INFINITY;
        }
    }
    return kl;
}


double bernoulli_kullback_leibler(double p, double q) {
    double kl1 = (p > 0 && q > 0) ? p * std::log(p / q) : 0;
    double kl2 = (p < 1 && q < 1) ? (1 - p) * std::log((1 - p) / (1 - q)) : 0;
    return kl1 + kl2;
}


double d_bernoulli_kullback_leibler_dq(double p, double q) {
    return (1 - p) / (1 - q) - p / q;
}


double newton_iteration(std::function<double(double)> f, std::function<double(double)> df, double eps, double x0, double a, double b, double weight = 0.9, int max_iter = 100) {
    double x = std::numeric_limits<double>::infinity();
    double x_next = x0;
    int iter = 0;
    while (std::fabs(x - x_next) > eps && iter < max_iter) {
        iter++;
        x = x_next;
        double f_x = f(x); 

        double df_x;
        if (x == 1.0 || x == 0.0) df_x = (f_x - f(x-eps))/eps;
        else df_x = df(x);
        
        if (df_x != 0) x_next = x - f_x / df_x;

        if (x_next < a) x_next = weight * a + (1.0 - weight) * x;
        if (x_next > b) x_next = weight * b + (1.0 - weight) * x;
    }

    if (x_next < a) x_next = a;
    if (x_next > b) x_next = b;
    return x_next;
}


double kl_upper_bound(double _sum, int count, double threshold = 1.0, double eps = 1e-2, bool lower = false) {
    //     Upper Confidence Bound of the empirical mean built on the Kullback-Leibler divergence.
    //     The computation involves solving a small convex optimization problem using Newton Iteration
    // :param _sum: Sum of sample values
    // :param count: Number of samples
    // :param threshold: the maximum kl-divergence * count
    // :param eps: Absolute accuracy of the Newton Iteration
    // :param lower: Whether to compute a lower-bound instead of upper-bound
   
    if (count == 0) return lower ? 0 : 1;

    double mu = _sum / count;
    double max_div = threshold / count;

    // Solve KL(mu, q) = max_div
    std::function<double(double)> kl = [&](double q) { return bernoulli_kullback_leibler(mu, q) - max_div; };
    std::function<double(double)> d_kl = [&](double q) { return d_bernoulli_kullback_leibler_dq(mu, q); };

    double a, b;
    a = lower ? 0 : mu;
    b = lower ? mu : 1;

    return newton_iteration(kl, d_kl, eps, (a + b) / 2.0, a, b);
}


// Function to mimic np.where in Python
Eigen::VectorXi where(const Eigen::VectorXd& arr, std::function<bool(double)> condition) {
    std::vector<int> indices;
    for (int i = 0; i < arr.size(); ++i) {
        if (condition(arr(i))) { // Apply custom condition function
            indices.push_back(i);
        }
    }
    return Eigen::Map<Eigen::VectorXi>(indices.data(), indices.size()); // Convert std::vector to Eigen::VectorXi
}


Eigen::VectorXd max_expectation_under_constraint(const Eigen::VectorXd &f, const Eigen::VectorXd &q, double c, double eps = 1e-2) {
    Eigen::VectorXd p_star = Eigen::VectorXd::Zero(q.size());
    Eigen::VectorXi x_plus = where(q, [](double x) { return x > 0; });
    Eigen::VectorXi x_zero = where(q, [](double x) { return x == 0; });
    double lambda_ = 0, z = 0;

    Eigen::VectorXd q_p = q(x_plus);
    Eigen::VectorXd f_p = f(x_plus);
    double f_star = f.maxCoeff();

    auto theta = [&](double l) {
        Eigen::ArrayXd l_m_f_p = l - f_p.array();  // Convert f_p to an array for element-wise operations
        return (q_p.array() * l_m_f_p.log()).sum() + log((q_p.array() / l_m_f_p).sum()) - c;
    };

    auto d_theta_dl = [&](double l) {
        Eigen::VectorXd l_m_f_p_inv = 1 / (l - f_p.array());
        return (q_p.array() * l_m_f_p_inv.array()).sum() - (q_p.array() * l_m_f_p_inv.array().square()).sum() / (q_p.array() * l_m_f_p_inv.array()).sum();
    };

    if (f_star > f_p.maxCoeff()) {
        double theta_star = theta(f_star);
        if (theta_star < 0) {
            lambda_ = f_star;
            z = 1 - exp(theta_star);
            for (int i = 0; i < x_zero.size(); ++i) {
                if (f(x_zero(i)) == f.maxCoeff()) {
                    p_star(x_zero(i)) = z / x_zero.size();
                }
            }
        }
    }

    if (lambda_ == 0) {
        if ((f_p.array() == f_p(0)).all()) {
            return q;
        } else {
            lambda_ = newton_iteration(theta, d_theta_dl, eps, f_star + 1, f_star, std::numeric_limits<double>::infinity());
        }
    }

    double beta = (1 - z) / (q_p.array() / (lambda_ - f_p.array())).sum();
    if (beta == 0) {
        for (int i = 0; i < q.size(); ++i) {
            if (q(i) > 0 && f(i) == f_star) {
                p_star(i) = (1 - z) / q.size();
            }
        }
    } else {
        p_star(x_plus) = beta * q_p.array() / (lambda_ - f_p.array());
    }
    return p_star;
}


int sampleIndex(const std::vector<double>& probabilities) {
    std::discrete_distribution<int> distribution(probabilities.begin(), probabilities.end());
    return distribution(generator);
}


int get_number_of_unknown_opponent_moves(InformationSet& I) {
    std::string B = I.get_board_from_hash();
    int count_x = 0;
    int count_o = 0;
    for (int i = 0; i < 9; i++) {
        if (B[i] == 'x') {
            count_x++;
        }
        if (B[i] == 'o') {
            count_o++;
        }
    }
    if (I.player == 'x') {
        return count_x - count_o;
    } else {
        return count_o - count_x + 1;
    }
}


void get_uncertain_squares(InformationSet& I, std::vector<int> &squares) {
    std::string B = I.get_board_from_hash();
    for (int i = 0; i < 9; i++) {
        if (B[i] == '-') {
            squares.push_back(i);
        }
    }
}


void get_states_in_infoset(InformationSet &I, std::vector<TicTacToeBoard> &states) {
    int num_unknown_opponent_moves = get_number_of_unknown_opponent_moves(I);
    std::string board_copy = I.get_board_from_hash();

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
        get_uncertain_squares(I, uncertain_ind);

        std::vector<char> base_perm(num_unknown_opponent_moves, I.other_player());
        base_perm.insert(base_perm.end(), uncertain_ind.size() - num_unknown_opponent_moves, '0');
        // sort 
        std::sort(base_perm.begin(), base_perm.end());

        do {
            TicTacToeBoard new_state(board_copy);
            for (int j = 0; j < base_perm.size(); j++) {
                new_state[uncertain_ind[j]] = base_perm[j];
            }
            char winner;
            if (!new_state.is_win(winner) && !new_state.is_over()) {
                states.push_back(new_state);
            }
        } while (std::next_permutation(base_perm.begin(), base_perm.end()));

    }
}


void get_cohort(InformationSet I, int action, std::unordered_set<std::string> &cohort) {
    if (I.move_flag) {
        I.update_move(action, I.player);
        I.reset_zeros();
        if (I.get_index() != -1) {
            cohort.insert(I.get_hash());
        }
        return;
    }
    else {
        std::vector<TicTacToeBoard> states;
        get_states_in_infoset(I, states);
        for (TicTacToeBoard &state : states) {
            InformationSet new_I = I;
            new_I.simulate_sense(action, state);
            if (new_I.get_index() != -1) {
                if (cohort.find(new_I.get_hash()) == cohort.end()) {
                    cohort.insert(new_I.get_hash());
                }
            }
        }
    }
}


double sample_terminal_history(InformationSet& I_1, InformationSet& I_2, TicTacToeBoard& true_board, PolicyVec& player_policy, PolicyVec& opponent_policy, 
                               History& current_history, std::vector<std::pair<std::string, int>>& trajectory, char player, char br_player, 
                               std::vector<std::vector<double>>& action_UCB, std::vector<std::vector<double>>& action_LCB, int first_action, 
                               std::vector<std::vector<double>>& R, std::vector<int>& infoset_reach_count, std::vector<std::vector<int>>& terminal_reach_count) {
    InformationSet& I = player == 'x' ? I_1 : I_2;
    int action;

    if (I.player == br_player) { 
        std::cout << "InfoSet: " << I.get_hash() << std::endl;
        std::vector<int> legal_actions;
        I.get_actions(legal_actions);
        action = legal_actions[0];

        if (first_action == -1){
            // choose the action with the highest UCB
            double max_UCB = 0.0;
            for (int a : legal_actions) {
                std::cout << "Action: " << a << " UCB: " << action_UCB[I.get_index()][a] << std::endl;
                if (action_UCB[I.get_index()][a] >= max_UCB) {
                    max_UCB = action_UCB[I.get_index()][a];
                    action = a;
                }
            }
        }
        else {
            action = first_action;
            first_action = -1;
        }

        double max_LCB = 0.0;
        int max_LCB_action = legal_actions[0];
        for (int a : legal_actions) {
            std::cout << "Action: " << a << " LCB: " << action_LCB[I.get_index()][a] << std::endl;
            if (action_LCB[I.get_index()][a] >= max_LCB) {
                max_LCB = action_LCB[I.get_index()][a];
                max_LCB_action = a;
            }
        }
        // update player policy
        std::vector<double>& prob_dist = player_policy.policy_dict[I.get_index()];
        for (int i = 0; i < prob_dist.size(); i++) { prob_dist[i] = 0.0; }
        prob_dist[max_LCB_action] = 1.0;

        // update reach count
        infoset_reach_count[I.get_index()] += 1;

        // update trajectory
        trajectory.push_back(std::make_pair(I.get_hash(), action));
    }
    else {
        std::vector<double>& prob_dist = opponent_policy.policy_dict[I.get_index()];
        action = sampleIndex(prob_dist);
    }

    if (I.move_flag) {
        bool success = true_board.update_move(action, player);
        current_history.history.push_back(action);

        char winner;
        if (success && !true_board.is_win(winner) && !true_board.is_over()) {
            InformationSet new_I = I;
            new_I.update_move(action, player);
            new_I.reset_zeros();

            if (player == 'x') {
                return sample_terminal_history(new_I, I_2, true_board, player_policy, opponent_policy, current_history, trajectory, 'o', br_player, action_UCB, action_LCB, first_action, R, infoset_reach_count, terminal_reach_count);
            } else {
                return sample_terminal_history(I_1, new_I, true_board, player_policy, opponent_policy, current_history, trajectory, 'x', br_player, action_UCB, action_LCB, first_action, R, infoset_reach_count, terminal_reach_count);
            }
        } else {
            TerminalHistory H_T = TerminalHistory(current_history.history);
            H_T.set_reward();
            double reward = 0.0;
            // scale reward between 0 and 1
            if (br_player == 'x'){
                reward = ((double) H_T.reward[0] + 1.0)/ 2.0;
                R[I_1.get_index()][action] += reward;
                terminal_reach_count[I_1.get_index()][action] += 1;
            } else {
                reward = ((double) H_T.reward[1] + 1.0)/ 2.0;
                R[I_2.get_index()][action] += reward;
                terminal_reach_count[I_2.get_index()][action] += 1;
            }
            
            return reward;
        }
    }
    else {
        InformationSet new_I = I;
        new_I.simulate_sense(action, true_board);
        current_history.history.push_back(action);

        if (player == 'x') {
            return sample_terminal_history(new_I, I_2, true_board, player_policy, opponent_policy, current_history, trajectory, 'x', br_player, action_UCB, action_LCB, first_action, R, infoset_reach_count, terminal_reach_count);
        } else {
            return sample_terminal_history(I_1, new_I, true_board, player_policy, opponent_policy, current_history, trajectory, 'o', br_player, action_UCB, action_LCB, first_action, R, infoset_reach_count, terminal_reach_count);
        }
    }
}


void updateBounds(std::vector<std::vector<double>>& R, std::vector<int>& infoset_reach_count, std::vector<std::vector<int>>& terminal_reach_count, 
                  std::vector<std::vector<double>>& reward_UCB, std::vector<std::vector<double>>& reward_LCB, std::vector<std::vector<double>>& action_UCB, 
                  std::vector<std::vector<double>>& action_LCB, std::vector<std::pair<std::string, int>>& trajectory, char br_player, double eps, double delta, int H, int B){
    int _H = trajectory.size();

    for (int h = _H-1; h >=0; h--){
        std::string I_hash = trajectory[h].first;
        InformationSet I = InformationSet(br_player, get_move_flag(I_hash, br_player), I_hash);
        int a = trajectory[h].second;
        double n_t = terminal_reach_count[I.get_index()][a];

        // std::cout << "Updating bounds for: " << I.get_hash() << " " << a << " Index: " << I.get_index() << std::endl;
        
        std::unordered_set<std::string> cohort;
        get_cohort(I, a, cohort);
        // std::cout << "Cohort size: " << cohort.size() << std::endl;
        // initialise p_hat as an Eigen vector
        Eigen::VectorXd p_hat(cohort.size() + 1);
        Eigen::VectorXd u_next(cohort.size() + 1);
        Eigen::VectorXd l_next(cohort.size() + 1);
        p_hat.setZero();
        u_next.setZero();
        l_next.setZero();
        
        for (std::string I_prime_hash : cohort){
            InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash);
            n_t += infoset_reach_count[I_prime.get_index()];
        }

        double beta_cnt = std::log(3.0 * std::pow(13 * B, H) / delta);
        double beta_r = beta_cnt + std::log(1.0 + n_t) + 1.0;
        double beta_p = beta_cnt + (B - 1.0) * (1.0 + std::log(1.0 + (n_t) / (B - 1.0)));

        // std::cout << "Beta_r: " << beta_r << " Beta_p: " << beta_p << std::endl;

        // update reward bounds
        reward_UCB[I.get_index()][a] = kl_upper_bound(R[I.get_index()][a], n_t, beta_r, 1e-2, false);
        reward_LCB[I.get_index()][a] = kl_upper_bound(R[I.get_index()][a], n_t, beta_r, 1e-2, true);

        int i = 0;
        if (n_t == 0.0){
            p_hat[i] = 1.0 / (cohort.size() + 1);
        }
        else{
            p_hat[i] = (double) terminal_reach_count[I.get_index()][a] / n_t;
        }

        i += 1;
        for (std::string I_prime_hash : cohort){
            InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash);
            // std::cout << "I_prime: " << I_prime.get_hash() << " Index: " << I_prime.get_index() << std::endl;

            if (n_t == 0.0){
                p_hat[i] = 1.0 / (cohort.size() + 1);
            }
            else{
                p_hat[i] = (double) infoset_reach_count[I_prime.get_index()] / n_t;
            }

            Eigen::VectorXd c_value_upper(13);
            Eigen::VectorXd c_value_lower(13);
            for (int j = 0; j < 13; j++){
                c_value_upper[j] = action_UCB[I_prime.get_index()][j];
                c_value_lower[j] = action_LCB[I_prime.get_index()][j];
            }

            u_next[i] = c_value_upper.maxCoeff();
            l_next[i] = c_value_lower.maxCoeff();

            // std::cout << "P_hat: " << p_hat[i] << " U_next: " << u_next[i] << " L_next: " << l_next[i] << std::endl;

            i += 1;
        }

        // update action bounds
        // solve KL optimization problem
        // https://github.com/eleurent/rl-agents/blob/master/rl_agents/utils.py#L123
        Eigen::VectorXd p_plus = max_expectation_under_constraint(u_next, p_hat, beta_p / n_t, eps);

        // std::cout << "P_plus: " << p_plus << std::endl;
    
        Eigen::VectorXd p_minus = max_expectation_under_constraint( - l_next, p_hat, beta_p / n_t, eps);

        // std::cout << "P_minus: " << p_minus << std::endl;
        
        action_UCB[I.get_index()][a] = reward_UCB[I.get_index()][a] + p_plus.dot(u_next);
        action_LCB[I.get_index()][a] = reward_LCB[I.get_index()][a] + p_minus.dot(l_next);

        // std::cout << "Reward UCB: " << reward_UCB[I.get_index()][a] << " Reward LCB: " << reward_LCB[I.get_index()][a] << std::endl;
        // std::cout << "Action UCB: " << action_UCB[I.get_index()][a] << " Action LCB: " << action_LCB[I.get_index()][a] << std::endl;
    }
}


void algorithm(double eps, double delta, int H, int B, char br_player, PolicyVec& player_policy, PolicyVec& opponent_policy, std::vector<std::string>& player_information_sets, int T, int log_freq){
    std::vector<std::vector<double>> R(player_information_sets.size(), std::vector<double>(13, 0.0));
    std::vector<std::vector<double>> reward_UCB(player_information_sets.size(), std::vector<double>(13, 1.0));
    std::vector<std::vector<double>> reward_LCB(player_information_sets.size(), std::vector<double>(13, 0.0));
    std::vector<int> infoset_reach_count(player_information_sets.size(), 0);
    std::vector<std::vector<int>> terminal_reach_count(player_information_sets.size(), std::vector<int>(13, 0));

    std::vector<std::vector<double>> action_UCB(player_information_sets.size(), std::vector<double>(13, 1.0));
    std::vector<std::vector<double>> action_LCB(player_information_sets.size(), std::vector<double>(13, 0.0));

    for (int t = 1; t <= T; t++){
        if (t % log_freq == 0){
            std::cout << "-------------- Iteration: " << t << " --------------" << std::endl;
            double expected_utility = 0.0;
            if (br_player == 'x'){
                expected_utility = get_expected_utility_wrapper(player_policy, opponent_policy);
            }
            else if (br_player == 'o'){
                expected_utility = get_expected_utility_wrapper(opponent_policy, player_policy);
            }
            std::cout << "Expected Utility: " << expected_utility << std::endl;
        }

        int first_action = 0;
        int b_t = 0;
        int c_t = 0;

        std::string board = "000000000";
        TicTacToeBoard true_board = TicTacToeBoard(board);
        std::string hash_1 = "";
        std::string hash_2 = "";
        InformationSet I_1 = InformationSet('x', true, hash_1);
        InformationSet I_2 = InformationSet('o', false, hash_2);
        InformationSet& I = br_player == 'x' ? I_1 : I_2;
        std::vector<int> h = {};
        History current_history = History(h);
        std::vector<std::pair<std::string, int>> trajectory = {};

        std::vector<int> legal_actions;
        I.get_actions(legal_actions);

        if (legal_actions.size() == 1){
            first_action = legal_actions[0];
        }
        else {
            // best
            double min_width = 1.0;
            for (int a : legal_actions){
                double max_U_1 = 0.0;
                for (int b : legal_actions){
                    if (b == a){ continue; }
                    else { if (action_UCB[I.get_index()][b] >= max_U_1){ max_U_1 = action_UCB[I.get_index()][b]; }}
                }

                if (max_U_1 - action_LCB[I.get_index()][a] <= min_width){
                    min_width = max_U_1 - action_LCB[I.get_index()][a];
                    b_t = a;
                }
            }
            // std::cout << "Best action: " << b_t << std::endl;
            // challenger
            double max_U_1 = 0.0;
            for (int a : legal_actions){
                if (a == b_t){ continue; }
                if (action_UCB[I.get_index()][a] >= max_U_1){
                    max_U_1 = action_UCB[I.get_index()][a];
                    c_t = a;
                }
            }
            // std::cout << "Challenger action: " << c_t << std::endl;
            // exploration
            double width_b = action_UCB[I.get_index()][b_t] - action_LCB[I.get_index()][b_t];
            double width_c = action_UCB[I.get_index()][c_t] - action_LCB[I.get_index()][c_t];
            first_action = width_b > width_c ? b_t : c_t;
            // std::cout << "First action: " << first_action << std::endl;
        }
        
        // sample game
        double reward = sample_terminal_history(I_1, I_2, true_board, player_policy, opponent_policy, current_history, trajectory, 'x', br_player, action_UCB, action_LCB, first_action, R, infoset_reach_count, terminal_reach_count);
        // std::cout << "Reward: " << reward << std::endl;
        // std::cout << "------------- History ------------" << std::endl;
        // current_history.print_history();

        // update bounds
        updateBounds(R, infoset_reach_count, terminal_reach_count, reward_UCB, reward_LCB, action_UCB, action_LCB, trajectory, br_player, eps, delta, H, B);
    }
}


int main(int argc, char* argv[]) {
    std::cout.precision(17);
    std::string file_path_1 = argv[1];
    std::string file_path_2 = argv[2];
    long int num_iterations = std::stol(argv[3]);
    char player = argv[4][0];
    int num_experiments = std::stoi(argv[5]);
    long int log_freq = std::stol(argv[6]);
    double eps = std::stod(argv[7]);
    double delta = std::stod(argv[8]);
    std::string base_path = argv[9];

    int experiment_number = 1;
    // instance specific constants
    int B = 7; // max number of infosets in cohort
    int H = 9; // max horizon

    std::vector<std::string> P1_information_sets;
    std::vector<std::string> P2_information_sets;
    std::string P1_information_sets_file = "data/P1_information_sets_V2.txt";
    std::string P2_information_sets_file = "data/P2_information_sets_V2.txt";

    // read the P1 information sets
    std::ifstream P1_f_is(P1_information_sets_file);
    std::string P1_line_is;
    while (std::getline(P1_f_is, P1_line_is)) {
        P1_information_sets.push_back(P1_line_is);
    }
    P1_f_is.close();
    // read the P2 information sets
    std::ifstream P2_f_is(P2_information_sets_file);
    std::string P2_line_is;
    while (std::getline(P2_f_is, P2_line_is)) {
        P2_information_sets.push_back(P2_line_is);
    }
    P2_f_is.close();
    // create hash to int maps (for performance reasons)
    for (long int i = 0; i < P1_information_sets.size(); i++) {
        InformationSet::P1_hash_to_int_map[P1_information_sets[i]] = i;
    }
    for (long int i = 0; i < P2_information_sets.size(); i++) {
        InformationSet::P2_hash_to_int_map[P2_information_sets[i]] = i;
    }

    std::cout << "Loading start policies..." << std::endl;
    PolicyVec policy_obj_x('x', file_path_1, true);
    PolicyVec policy_obj_o('o', file_path_2, true);
    std::cout << "Start policies loaded." << std::endl;
    PolicyVec br_x('x', P1_information_sets);
    PolicyVec br_o('o', P2_information_sets);

    double expected_utility = 0.0;
    if (player == 'x'){
        expected_utility = compute_best_response_wrapper(policy_obj_o, br_x, 'x');
    }
    else if (player == 'o'){
        expected_utility = compute_best_response_wrapper(policy_obj_x, br_o, 'o');
    }

    std::cout << "----------- Expected Utility of Best Response: " << expected_utility << " -----------" << std::endl;

    while (experiment_number <= num_experiments){
        if (player == 'x'){
            PolicyVec uniform_policy_obj_x('x', P1_information_sets);
            PolicyVec player_br_policy = uniform_policy_obj_x;
            algorithm(eps, delta, H, B, player, player_br_policy, policy_obj_o, P1_information_sets, num_iterations, log_freq);
        }
        else if (player == 'o'){
            PolicyVec uniform_policy_obj_o('o', P2_information_sets);
            PolicyVec player_br_policy = uniform_policy_obj_o;
            algorithm(eps, delta, H, B, player, player_br_policy, policy_obj_x, P2_information_sets, num_iterations, log_freq);
        }
        experiment_number += 1;
    }
}