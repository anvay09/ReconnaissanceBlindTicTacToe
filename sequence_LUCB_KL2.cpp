#include "cpp_headers/rbt_classes.hpp"
#include "cpp_headers/rbt_utilities.hpp"
#include <random>
#include <cmath>
#include "Eigen/Dense"
int NUMBER_THREADS = 96;
int NUM_ACTIONS = 13;

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


void precompute_cohorts(std::vector<std::vector<std::unordered_set<std::string>>>& cohorts, std::vector<std::string>& player_information_sets, char br_player) {
    std::cerr << "Building cohorts" << std::endl;
    // precompute cohorts
    
    #pragma omp parallel for num_threads(NUMBER_THREADS)
    for (int i = 0; i < player_information_sets.size(); i++) {
        InformationSet I = InformationSet(br_player, get_move_flag(player_information_sets[i], br_player), player_information_sets[i]);
        std::vector<int> legal_actions;
        I.get_actions(legal_actions);
        for (int a : legal_actions) {
            std::unordered_set<std::string> cohort;
            get_cohort(I, a, cohort);
            cohorts[I.get_index()][a] = cohort;
        }
    }
    std::cerr << "Cohorts built" << std::endl;
}


double compute_similarity(PolicyVec& P1, PolicyVec& P2, std::vector<std::string>& player_information_sets, char player, std::vector<int>& infoset_reach_counts){
    int matches = 0;
    int total = 0;
    for (int i = 0; i < player_information_sets.size(); i++){
        std::string I_hash = player_information_sets[i];
        InformationSet I(player, get_move_flag(I_hash, player), I_hash);
        if (infoset_reach_counts[I.get_index()] > 0) {
            std::vector<double>& dict1 = P1.policy_dict[I.get_index()];
            std::vector<double>& dict2 = P2.policy_dict[I.get_index()];
    
            for (int a = 0; a < dict1.size(); a++){
                if (dict1[a] == 1.0 && dict2[a] == 1.0){
                    matches++;
                    break;
                }
            }
    
            total++;
        }
    }

    return (double) matches / (double) total;
}


double sample_game(InformationSet& I_1, InformationSet& I_2, TicTacToeBoard& true_board, History& current_history, char br_player, 
                   PolicyVec& policy_obj, PolicyVec& opponent_policy, Sequence& trajectory, char curr_player, std::vector<int>& infoset_reach_counts, std::vector<std::vector<int>>& action_reach_counts) {
    InformationSet I = curr_player == 'x' ? I_1 : I_2;
    int action = 0;
    
    if (br_player == I.player){
        infoset_reach_counts[I.get_index()] += 1;
        std::vector<double>& prob_dist = policy_obj.policy_dict[I.get_index()];
        action = sampleIndex(prob_dist);
        trajectory.extend(I, action);
        action_reach_counts[I.get_index()][action] += 1;
    }
    else {
        std::vector<double>& prob_dist = opponent_policy.policy_dict[I.get_index()];
        action = sampleIndex(prob_dist);
    }

    if (I.move_flag) {
        TicTacToeBoard new_board = true_board;
        bool success = new_board.update_move(action, curr_player);
        current_history.history.push_back(action);
        
        char winner;
        if (success && !new_board.is_win(winner) && !new_board.is_over()) {
            InformationSet new_I = I;
            new_I.update_move(action, curr_player);

            if (curr_player == 'x') {
                return sample_game(new_I, I_2, new_board, current_history, br_player, policy_obj, opponent_policy, trajectory, 'o', infoset_reach_counts, action_reach_counts);
            }
            else {
                return sample_game(I_1, new_I, new_board, current_history, br_player, policy_obj, opponent_policy, trajectory, 'x', infoset_reach_counts, action_reach_counts);
            }
        } else {
            TerminalHistory H_T = TerminalHistory(current_history.history);
            H_T.set_reward();
            double r = br_player == 'x' ? (double) H_T.reward[0] : (double) H_T.reward[1];
            return r; 
        }
    }
    else {
        InformationSet new_I = I;
        TicTacToeBoard new_board = true_board;
        new_I.simulate_sense(action, new_board);
        current_history.history.push_back(action);

        if (curr_player == 'x') {
            return sample_game(new_I, I_2, new_board, current_history, br_player, policy_obj, opponent_policy, trajectory, 'x', infoset_reach_counts, action_reach_counts);
        } else {
            return sample_game(I_1, new_I, new_board, current_history, br_player, policy_obj, opponent_policy, trajectory, 'o', infoset_reach_counts, action_reach_counts);
        }
    }
}


double build_max_policy(PolicyVec& policy_obj, InformationSet& I, Sequence& trajectory, std::vector<Sequence>& terminal_sequences,
                        std::unordered_map<std::string, int>& sequence_hash_to_index_map, bool UCB_flag, 
                        std::vector<double>& global_infoset_values, std::vector<std::vector<double>>& global_action_values, std::vector<std::vector<std::unordered_set<std::string>>>& cohorts) {
    std::vector<int> legal_actions;
    I.get_actions(legal_actions);
    std::vector<double> legal_action_values(NUM_ACTIONS, 0.0);
    double infoset_value = -1.0;

    for (int a : legal_actions){
        std::unordered_set<std::string>& cohort = cohorts[I.get_index()][a];

        Sequence new_trajectory = trajectory;
        new_trajectory.extend(I, a);

        for (std::string I_prime_hash : cohort){
            InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash);
            legal_action_values[a] += build_max_policy(policy_obj, I_prime, new_trajectory, terminal_sequences, sequence_hash_to_index_map, UCB_flag, global_infoset_values, global_action_values, cohorts);
        }

        int s_index = sequence_hash_to_index_map[new_trajectory.hash];
        double p_hat, r_hat = 0.0;

        if (UCB_flag){
            p_hat = terminal_sequences[s_index].ucb_p;
            r_hat = terminal_sequences[s_index].ucb_r;
        }
        else{
            p_hat = terminal_sequences[s_index].p;
            if (terminal_sequences[s_index].n != 0){
                r_hat = terminal_sequences[s_index].r / (double) terminal_sequences[s_index].n;
            }
        }

        legal_action_values[a] += p_hat * r_hat;

        if (!UCB_flag){
            if (cohort.size() == 0 && terminal_sequences[s_index].n == 0){
                legal_action_values[a] = 0.0;
            }
        }

        // make sure action values lie between [-1, 1]
        // legal_action_values[a] = std::min(1.0, legal_action_values[a]);
        legal_action_values[a] = std::max(-1.0, legal_action_values[a]);
    }

    for (int a : legal_actions){
        if (legal_action_values[a] > infoset_value){
            infoset_value = legal_action_values[a];
        }
    }

    std::vector<int> candidate_actions;
    for (int a : legal_actions){
        if (fabs(legal_action_values[a] - infoset_value) < 1e-6){
            candidate_actions.push_back(a);
        }
    }

    // sample from candidate actions
    int action = candidate_actions[sampleIndex(std::vector<double>(candidate_actions.size(), 1.0 / candidate_actions.size()))];
    std::vector<double>& prob_dist = policy_obj.policy_dict[I.get_index()];

    for (int a : legal_actions){
        if (a == action){
            prob_dist[a] = 1.0;
        }
        else{
            prob_dist[a] = 0.0;
        }
    }

    for (int a : legal_actions){
        global_action_values[I.get_index()][a] = legal_action_values[a];
    }
    global_infoset_values[I.get_index()] = infoset_value;

    return infoset_value;
}


double build_max_policy_parallel(PolicyVec& policy_obj, InformationSet& I, Sequence& trajectory, std::vector<Sequence>& terminal_sequences,
                        std::unordered_map<std::string, int>& sequence_hash_to_index_map, bool UCB_flag, 
                        std::vector<double>& global_infoset_values, std::vector<std::vector<double>>& global_action_values, std::vector<std::vector<std::unordered_set<std::string>>>& cohorts) {
    std::vector<int> legal_actions;
    I.get_actions(legal_actions);
    std::vector<double> legal_action_values(NUM_ACTIONS, 0.0);
    double infoset_value = -1.0;

    #pragma omp parallel for num_threads(NUMBER_THREADS)
    for (int a : legal_actions){
        std::unordered_set<std::string>& cohort = cohorts[I.get_index()][a];

        Sequence new_trajectory = trajectory;
        new_trajectory.extend(I, a);

        for (std::string I_prime_hash : cohort){
            InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash);
            legal_action_values[a] += build_max_policy(policy_obj, I_prime, new_trajectory, terminal_sequences, sequence_hash_to_index_map, UCB_flag, global_infoset_values, global_action_values, cohorts);
        }

        int s_index = sequence_hash_to_index_map[new_trajectory.hash];
        double p_hat, r_hat = 0.0;

        if (UCB_flag){
            p_hat = terminal_sequences[s_index].ucb_p;
            r_hat = terminal_sequences[s_index].ucb_r;
        }
        else{
            p_hat = terminal_sequences[s_index].p;
            if (terminal_sequences[s_index].n != 0){
                r_hat = terminal_sequences[s_index].r / (double) terminal_sequences[s_index].n;
            }
        }

        legal_action_values[a] += p_hat * r_hat;

        if (!UCB_flag){
            if (cohort.size() == 0 && terminal_sequences[s_index].n == 0){
                legal_action_values[a] = 0.0;
            }
        }

        // make sure action values lie between [-1, 1]
        // legal_action_values[a] = std::min(1.0, legal_action_values[a]);
        legal_action_values[a] = std::max(-1.0, legal_action_values[a]);
    }

    for (int a : legal_actions){
        if (legal_action_values[a] > infoset_value){
            infoset_value = legal_action_values[a];
        }
    }

    std::vector<int> candidate_actions;
    for (int a : legal_actions){
        if (fabs(legal_action_values[a] - infoset_value) < 1e-6){
            candidate_actions.push_back(a);
        }
    }

    // sample from candidate actions
    int action = candidate_actions[sampleIndex(std::vector<double>(candidate_actions.size(), 1.0 / candidate_actions.size()))];
    std::vector<double>& prob_dist = policy_obj.policy_dict[I.get_index()];

    for (int a : legal_actions){
        if (a == action){
            prob_dist[a] = 1.0;
        }
        else{
            prob_dist[a] = 0.0;
        }
    }

    for (int a : legal_actions){
        global_action_values[I.get_index()][a] = legal_action_values[a];
    }
    global_infoset_values[I.get_index()] = infoset_value;

    return infoset_value;
}


void update_max_reward_policy_given_trajectory(PolicyVec& policy_obj, Sequence& trajectory, std::vector<Sequence>& terminal_sequences, std::unordered_map<std::string, int>& sequence_hash_to_index_map,
                                               std::vector<double>& global_infoset_values, std::vector<std::vector<double>>& global_action_values, char br_player, std::vector<std::vector<std::unordered_set<std::string>>>& cohorts) {
    for (int j = trajectory.seq.size() - 1; j >= 0; j--){
        std::string I_hash = trajectory.seq[j].first;
        int played_action = trajectory.seq[j].second;
        bool move_flag = get_move_flag(I_hash, br_player);
        InformationSet I(br_player, move_flag, I_hash);

        std::string seq_hash = I_hash + "-" + std::to_string(played_action);
        int s_index = sequence_hash_to_index_map[seq_hash];
        std::vector<double>& prob_dist = policy_obj.policy_dict[I.get_index()];

        // compute value of played action
        std::unordered_set<std::string>& cohort = cohorts[I.get_index()][played_action];
        // std::cout << "Played action: " << played_action << std::endl;

        double action_value = 0.0;
        for (std::string I_prime_hash : cohort){
            InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash);
            action_value += global_infoset_values[I_prime.get_index()];
            // std::cout << "Cohort: " << I_prime.get_hash() << " - Value: " << global_infoset_values[I_prime.get_index()] << std::endl;
        }

        double p_hat, r_hat = 0.0;
        p_hat = terminal_sequences[s_index].p;
        if (terminal_sequences[s_index].n != 0){
            r_hat = terminal_sequences[s_index].r / (double) terminal_sequences[s_index].n;
        }
        action_value += p_hat * r_hat;
        // std::cout << "Terminal value: " << p_hat * r_hat << std::endl;

        if (cohort.size() == 0 && terminal_sequences[s_index].n == 0){
            action_value = 0.0;
        }
        
        // make sure action value lies between [-1, 1]
        // action_value = std::min(1.0, action_value);
        action_value = std::max(-1.0, action_value);

        // update played action value
        global_action_values[I.get_index()][played_action] = action_value;

        // compare action values to find best action
        std::vector<int> legal_actions;
        I.get_actions(legal_actions);
        double infoset_value = -1.0;
        // std::cout << "Infoset: " << I.get_hash() << " - Action values: ";
        
        for (int a : legal_actions){            
            // std::cout << a << " " << global_action_values[I.get_index()][a] << " ";
            if (global_action_values[I.get_index()][a] > infoset_value){
                infoset_value = global_action_values[I.get_index()][a];
            }
        }
        // std::cout << std::endl;

        std::vector<int> candidate_actions;
        for (int a: legal_actions){
            if (fabs(global_action_values[I.get_index()][a] - infoset_value) < 1e-6){
                candidate_actions.push_back(a);
            }
        }

        // sample from candidate actions
        int best_action = candidate_actions[sampleIndex(std::vector<double>(candidate_actions.size(), 1.0 / candidate_actions.size()))];
        for (int a : legal_actions){
            if (a == best_action){
                prob_dist[a] = 1.0;
            }
            else{
                prob_dist[a] = 0.0;
            }
        }
        global_infoset_values[I.get_index()] = infoset_value;
    }
}


void copy_max_reward_policy_given_trajectory(PolicyVec& policy_obj, PolicyVec& policy_obj_copy, Sequence& trajectory, char br_player) {
    for (int j = trajectory.seq.size() - 1; j >= 0; j--){
        std::string I_hash = trajectory.seq[j].first;
        bool move_flag = get_move_flag(I_hash, br_player);
        InformationSet I(br_player, move_flag, I_hash);
        std::vector<int> legal_actions;
        I.get_actions(legal_actions);

        for (int a : legal_actions){            
            policy_obj_copy.policy_dict[a] = policy_obj.policy_dict[a];
        }
    }
}


double update_max_ucb_policy_given_trajectory(PolicyVec& update_policy_obj, PolicyVec& played_policy_obj, PolicyVec& player_br, InformationSet& I, Sequence& trajectory, std::vector<Sequence>& terminal_sequences, std::vector<int>& infoset_reach_counts,
                                              std::unordered_map<std::string, int>& sequence_hash_to_index_map, std::vector<double>& global_infoset_values, std::vector<std::vector<double>>& global_action_values, std::vector<std::vector<std::unordered_set<std::string>>>& cohorts, int B, int H) {
    std::vector<int> legal_actions;
    I.get_actions(legal_actions);
    
    std::vector<int> played_actions;
    I.get_actions_given_policy(played_actions, played_policy_obj);

    std::vector<int> br_actions;
    I.get_actions_given_policy(br_actions, player_br);

    int played_action = played_actions[0];
    int br_action = br_actions[0];

    double infoset_value = -1.0;
    double delta = 0.01;
    double played_action_value = 0.0;

    std::unordered_set<std::string>& cohort = cohorts[I.get_index()][played_action];
    std::unordered_map<std::string, double> cohort_values;

    Eigen::VectorXd p_hat(cohort.size() + 1);
    Eigen::VectorXd u_next(cohort.size() + 1);
    p_hat.setZero();
    u_next.setZero();

    Sequence new_trajectory = trajectory;
    new_trajectory.extend(I, played_action);
    int s_index = sequence_hash_to_index_map[new_trajectory.hash];
    int reach_sum = terminal_sequences[s_index].n;

    for (std::string I_prime_hash : cohort){
        InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash);
        reach_sum += infoset_reach_counts[I_prime.get_index()];
        cohort_values[I_prime_hash] = update_max_ucb_policy_given_trajectory(update_policy_obj, played_policy_obj, player_br, I_prime, new_trajectory, terminal_sequences, infoset_reach_counts, sequence_hash_to_index_map, global_infoset_values, global_action_values, cohorts, B, H);
    }

    if (reach_sum == 0){
        played_action_value = 1.0;
    }
    else {
        int i = 0;

        if (terminal_sequences[s_index].n > 0){
            p_hat[i] = (double) terminal_sequences[s_index].n / (double) reach_sum;
            u_next[i] = terminal_sequences[s_index].ucb_r;
        }
        else {
            p_hat[i] == 0.0;
            u_next[i] = 0.0;
        }
        
        for (std::string I_prime_hash : cohort){
            i++;
            InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash);
            p_hat[i] = (double) infoset_reach_counts[I_prime.get_index()] / (double) reach_sum;
            u_next[i] = cohort_values[I_prime_hash];
        }

        double beta_p = std::log(3) + std::log(B) + std::log(1.0 / delta) + 0.5 * B * std::log(3.0 * (double) reach_sum / (double) B);
        Eigen::VectorXd p_plus = max_expectation_under_constraint(u_next, p_hat, beta_p / (double) reach_sum, 1e-2);
        played_action_value = p_plus.dot(u_next);
    }

    // make sure action values lie between [-1, 1]
    played_action_value = std::max(-1.0, played_action_value);

    global_action_values[I.get_index()][played_action] = played_action_value;
    
    for (int a : legal_actions){
        if (br_action != a){
            if (global_action_values[I.get_index()][a] > infoset_value){
                infoset_value = global_action_values[I.get_index()][a];
            }
        }
    }

    std::vector<int> candidate_actions;
    for (int a : legal_actions){
        if (fabs(global_action_values[I.get_index()][a] - infoset_value) < 1e-6){
            candidate_actions.push_back(a);
        }
    }

    if (candidate_actions.size() == 0){
        candidate_actions.push_back(br_action);
        infoset_value = global_action_values[I.get_index()][br_action];
    }

    // sample from candidate actions
    int action = candidate_actions[sampleIndex(std::vector<double>(candidate_actions.size(), 1.0 / candidate_actions.size()))];
    std::vector<double>& prob_dist = update_policy_obj.policy_dict[I.get_index()];

    for (int a : legal_actions){
        if (a == action){
            prob_dist[a] = 1.0;
        }
        else{
            prob_dist[a] = 0.0;
        }
    }

    global_infoset_values[I.get_index()] = infoset_value;

    return infoset_value;
}


void get_sequences(InformationSet& I, Sequence& trajectory, std::vector<Sequence>& terminal_sequences, std::unordered_map<std::string, int>& sequence_hash_to_index_map, std::vector<std::vector<std::unordered_set<std::string>>>& cohorts) {
    std::vector<int> legal_actions;
    I.get_actions(legal_actions);
    
    for (int a : legal_actions){
        Sequence new_trajectory = trajectory;
        new_trajectory.extend(I, a);
        terminal_sequences.push_back(new_trajectory);
        sequence_hash_to_index_map[new_trajectory.hash] = terminal_sequences.size() - 1;

        std::unordered_set<std::string>& cohort = cohorts[I.get_index()][a];

        for (std::string I_prime_hash : cohort){
            InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash);
            get_sequences(I_prime, new_trajectory, terminal_sequences, sequence_hash_to_index_map, cohorts);
        }
    }
}


void get_sequences_wrapper(std::vector<Sequence>& terminal_sequences, char br_player, std::unordered_map<std::string, int>& sequence_hash_to_index_map, 
                           std::vector<std::vector<std::unordered_set<std::string>>>& cohorts) {
    std::string board = "000000000";
    TicTacToeBoard true_board = TicTacToeBoard(board);
    std::string hash_1 = "";
    std::string hash_2 = "";
    InformationSet I_1 = InformationSet('x', true, hash_1);
    InformationSet I_2 = InformationSet('o', false, hash_2);
    Sequence empty_sequence = Sequence();
    InformationSet root = br_player == 'x' ? I_1 : I_2;
    get_sequences(root, empty_sequence, terminal_sequences, sequence_hash_to_index_map, cohorts);
}


void update_reach_and_sequence_data(PolicyVec& policy_obj, PolicyVec& update_policy, InformationSet& I, Sequence& trajectory, std::unordered_map<std::string, int>& sequence_hash_to_index_map, 
                                                    std::vector<std::vector<std::unordered_set<std::string>>>& cohorts, std::vector<int>& infoset_reach_counts,
                                                    double reach, std::vector<Sequence>& terminal_sequences, double reward_game, 
                                                    std::string terminal_hash_game, double C_r, double C_p, std::vector<std::vector<int>>& action_reach_counts) {
    std::vector<int> policy_actions;
    I.get_actions_given_policy(policy_actions, policy_obj);
    
    for (int a : policy_actions){
        Sequence new_trajectory = trajectory;
        new_trajectory.extend(I, a);
        int index = sequence_hash_to_index_map[new_trajectory.hash];

        // update data for the sequence
        if (terminal_sequences[index].hash == terminal_hash_game){
            terminal_sequences[index].r += reward_game;
            terminal_sequences[index].n += 1;
            terminal_sequences[index].n_pi += 1;
            terminal_sequences[index].p = reach * (double) terminal_sequences[index].n / (double) action_reach_counts[I.get_index()][a];
            terminal_sequences[index].ucb_r = ((double) terminal_sequences[index].r / (double) terminal_sequences[index].n) + C_r * std::sqrt(1.0 / (double) terminal_sequences[index].n);
        }
        else{
            terminal_sequences[index].n_pi += 1;
            terminal_sequences[index].p = reach * (double) terminal_sequences[index].n / (double) action_reach_counts[I.get_index()][a];
        }
        
        std::unordered_set<std::string>& cohort = cohorts[I.get_index()][a];
        int reach_sum = action_reach_counts[I.get_index()][a];

        for (std::string I_prime_hash : cohort){
            InformationSet I_prime(I.player, get_move_flag(I_prime_hash, I.player), I_prime_hash);
            double new_reach = 0.0;
            if (reach_sum != 0) {
                new_reach = reach * ((double) infoset_reach_counts[I_prime.get_index()] / (double) reach_sum);
            }
            update_reach_and_sequence_data(policy_obj, update_policy, I_prime, new_trajectory, sequence_hash_to_index_map, cohorts, infoset_reach_counts, new_reach, terminal_sequences, reward_game, terminal_hash_game, C_r, C_p, action_reach_counts);
        }
    }
}


void update_reach_and_sequence_data_wrapper(PolicyVec& policy_obj, PolicyVec& update_policy, std::unordered_map<std::string, int>& sequence_hash_to_index_map, char br_player, 
                                                            std::vector<std::vector<std::unordered_set<std::string>>>& cohorts, std::vector<int>& infoset_reach_counts, 
                                                            std::vector<Sequence>& terminal_sequences, double reward_game, std::string terminal_hash_game, double C_r, double C_p, std::vector<std::vector<int>>& action_reach_counts) {
    std::string board = "000000000";
    TicTacToeBoard true_board = TicTacToeBoard(board);
    std::string hash_1 = "";
    std::string hash_2 = "";
    InformationSet I_1 = InformationSet('x', true, hash_1);
    InformationSet I_2 = InformationSet('o', false, hash_2);
    Sequence empty_sequence = Sequence();
    InformationSet root = br_player == 'x' ? I_1 : I_2;
    update_reach_and_sequence_data(policy_obj, update_policy, root, empty_sequence, sequence_hash_to_index_map, cohorts, infoset_reach_counts, 1.0, terminal_sequences, reward_game, terminal_hash_game, C_r, C_p, action_reach_counts);
}


void calc_br_sequence_LUCB(PolicyVec& opponent_policy, char br_player, std::vector<std::string>& player_information_sets, int log_frequency, PolicyVec& player_br, 
                           int experiment_number, int iterations, double C_r, double C_p, std::string& exp_name, std::unordered_map<std::string, int>& sequence_hash_to_index_map, 
                           std::vector<Sequence>& terminal_sequences, std::vector<std::vector<std::unordered_set<std::string>>>& cohorts, int B, int H) {  
    // store infoset and action values to avoid recomputation for parts of the tree that don't change
    std::vector<double> infoset_empirical_values(player_information_sets.size(), 0.0);
    std::vector<std::vector<double>> action_empirical_values(player_information_sets.size(), std::vector<double>(NUM_ACTIONS, 0.0));

    std::vector<double> infoset_upper_bound_values(player_information_sets.size(), 0.0);
    std::vector<std::vector<double>> action_upper_bound_values(player_information_sets.size(), std::vector<double>(NUM_ACTIONS, 0.0));

    std::vector<std::vector<int>> action_reach_counts(player_information_sets.size(), std::vector<int>(NUM_ACTIONS, 0));
    std::vector<int> infoset_reach_counts(player_information_sets.size(), 0);

    PolicyVec player_max_ucb_policy(br_player, player_information_sets, false);
    std::vector<std::pair<int, double>> exploitability_log; 

    // compute exact best response for comparison
    PolicyVec exact_br(br_player, player_information_sets, false);
    compute_best_response_wrapper(opponent_policy, exact_br, br_player);
    double exact_br_value = 0.0;

    if (br_player == 'x') {
        exact_br_value = get_expected_utility_wrapper(exact_br, opponent_policy);
    } 
    else {
        exact_br_value = get_expected_utility_wrapper(opponent_policy, exact_br);
    }
    std::cerr << "Exact best response value: " << exact_br_value << std::endl;

    std::string board = "000000000";
    TicTacToeBoard true_board = TicTacToeBoard(board);
    std::string hash_1 = "";
    std::string hash_2 = "";
    InformationSet I_1 = InformationSet('x', true, hash_1);
    InformationSet I_2 = InformationSet('o', false, hash_2);
    Sequence empty_sequence = Sequence();
    InformationSet root = br_player == 'x' ? I_1 : I_2;
    double root_val = build_max_policy_parallel(player_br, root, empty_sequence, terminal_sequences, sequence_hash_to_index_map, false, infoset_empirical_values, action_empirical_values, cohorts);  
    PolicyVec player_br_copy = player_br;
    std::cerr << "Max Reward policy computed" << std::endl;

    board = "000000000";
    true_board = TicTacToeBoard(board);
    hash_1 = "";
    hash_2 = "";
    I_1 = InformationSet('x', true, hash_1);
    I_2 = InformationSet('o', false, hash_2);
    empty_sequence = Sequence();
    root = br_player == 'x' ? I_1 : I_2;
    double max_UCB = build_max_policy_parallel(player_max_ucb_policy, root, empty_sequence, terminal_sequences, sequence_hash_to_index_map, true, infoset_upper_bound_values, action_upper_bound_values, cohorts);
    
    std::cerr << "Max UCB policy computed" << std::endl;
    bool max_UCB_flag = true;

    for (int t = 1; t <= iterations; t += 1) {
        if (max_UCB_flag) {
            board = "000000000";
            true_board = TicTacToeBoard(board);
            std::string hash_1 = "";
            std::string hash_2 = "";
            I_1 = InformationSet('x', true, hash_1);
            I_2 = InformationSet('o', false, hash_2);

            std::vector<int> h = {};
            TerminalHistory start_history = TerminalHistory(h);
            Sequence trajectory = Sequence();
            double reward = sample_game(I_1, I_2, true_board, start_history, br_player, player_max_ucb_policy, opponent_policy, trajectory, 'x', infoset_reach_counts, action_reach_counts);

            update_reach_and_sequence_data_wrapper(player_max_ucb_policy, player_max_ucb_policy, sequence_hash_to_index_map, br_player, cohorts, infoset_reach_counts, terminal_sequences, reward, trajectory.hash, C_r, C_p, action_reach_counts);

            // update policies 
            update_max_reward_policy_given_trajectory(player_br, trajectory, terminal_sequences, sequence_hash_to_index_map, infoset_empirical_values, action_empirical_values, br_player, cohorts);

            hash_1 = "";
            hash_2 = "";
            I_1 = InformationSet('x', true, hash_1);
            I_2 = InformationSet('o', false, hash_2);
            empty_sequence = Sequence();
            root = br_player == 'x' ? I_1 : I_2;
            double max_UCB = update_max_ucb_policy_given_trajectory(player_max_ucb_policy, player_max_ucb_policy, player_br, root, empty_sequence, terminal_sequences, infoset_reach_counts, sequence_hash_to_index_map, infoset_upper_bound_values, action_upper_bound_values, cohorts, B, H);
            
            copy_max_reward_policy_given_trajectory(player_br, player_br_copy, trajectory, br_player);
            max_UCB_flag = false;
        }
        else {
            board = "000000000";
            true_board = TicTacToeBoard(board);
            std::string hash_1 = "";
            std::string hash_2 = "";
            I_1 = InformationSet('x', true, hash_1);
            I_2 = InformationSet('o', false, hash_2);
            
            std::vector<int> h = {};
            TerminalHistory start_history = TerminalHistory(h);
            Sequence trajectory = Sequence();
            double reward = sample_game(I_1, I_2, true_board, start_history, br_player, player_br, opponent_policy, trajectory, 'x', infoset_reach_counts, action_reach_counts);

            update_reach_and_sequence_data_wrapper(player_br, player_max_ucb_policy, sequence_hash_to_index_map, br_player, cohorts, infoset_reach_counts, terminal_sequences, reward, trajectory.hash, C_r, C_p, action_reach_counts);

            // update policies
            update_max_reward_policy_given_trajectory(player_br, trajectory, terminal_sequences, sequence_hash_to_index_map, infoset_empirical_values, action_empirical_values, br_player, cohorts);

            hash_1 = "";
            hash_2 = "";
            I_1 = InformationSet('x', true, hash_1);
            I_2 = InformationSet('o', false, hash_2);
            empty_sequence = Sequence();
            root = br_player == 'x' ? I_1 : I_2;
            double max_UCB = update_max_ucb_policy_given_trajectory(player_max_ucb_policy, player_br_copy, player_br, root, empty_sequence, terminal_sequences, infoset_reach_counts, sequence_hash_to_index_map, infoset_upper_bound_values, action_upper_bound_values, cohorts, B, H);
            
            copy_max_reward_policy_given_trajectory(player_br, player_br_copy, trajectory, br_player);
            max_UCB_flag = true;
        }

        if (t % log_frequency == 0) { 
            double expected_utility = 0.0;
            double exploitability = 0.0;
        
            if (br_player == 'x') {
                expected_utility = get_expected_utility_wrapper(player_br, opponent_policy);
                exploitability_log.push_back(std::make_pair(t, exact_br_value - expected_utility));
            } 
            else {
                expected_utility = get_expected_utility_wrapper(opponent_policy, player_br);
                exploitability_log.push_back(std::make_pair(t, exact_br_value - expected_utility));
            }
            std::cerr << "Expected utility of best response policy: " << expected_utility << std::endl;
            std::cerr << "Number of games sampled so far: " << t << std::endl;
            // std::cerr << "Similarity between BR and UCB policies: " << compute_similarity(player_br, player_max_ucb_policy, player_information_sets, br_player, infoset_reach_counts) << std::endl;
        }
    }

    std::cerr << "Saving exploitability log" << std::endl;
    std::string file_name = "data/sequence/" + exp_name + "_rbt_" + std::string(1, br_player) + "_C_r=" + std::to_string(C_r) + "_C_p=" + std::to_string(C_p) + "_seq-LUCB_exploitability_log_" + std::to_string(experiment_number) + ".txt";

    std::ofstream f(file_name);
    for (int i = 0; i < exploitability_log.size(); i++) {
        f << exploitability_log[i].first << " " << exploitability_log[i].second << std::endl;
    }
    f.close();
}


int main(int argc, char* argv[]) {
    std::cerr.precision(17);
    std::string file_path_1 = argv[1]; // start policy P1
    std::string file_path_2 = argv[2]; // start policy P2
    char player = argv[3][0];
    int iterations = std::stoi(argv[4]);
    int log_frequency = std::stoi(argv[5]);
    int experiments = std::stoi(argv[6]);
    double C_r = std::stod(argv[7]);
    double C_p = std::stod(argv[8]);
    std::string exp_name = argv[9];
    int start_index = std::stoi(argv[10]);
    
    // load information sets
    std::vector<std::string> P1_information_sets;
    std::vector<std::string> P2_information_sets;
    std::string P1_information_sets_file = "data/P1_information_sets_V2.txt";
    std::string P2_information_sets_file = "data/P2_information_sets_V2.txt";
    std::ifstream P1_f_is(P1_information_sets_file);
    std::string P1_line_is;
    while (std::getline(P1_f_is, P1_line_is)) {
        P1_information_sets.push_back(P1_line_is);
    }
    P1_f_is.close();
    std::ifstream P2_f_is(P2_information_sets_file);
    std::string P2_line_is;
    while (std::getline(P2_f_is, P2_line_is)) {
        P2_information_sets.push_back(P2_line_is);
    }
    P2_f_is.close();

    // create hash to int maps
    for (int i = 0; i < P1_information_sets.size(); i++) {
        InformationSet::P1_hash_to_int_map[P1_information_sets[i]] = i;
    }
    for (int i = 0; i < P2_information_sets.size(); i++) {
        InformationSet::P2_hash_to_int_map[P2_information_sets[i]] = i;
    }

    // load policies
    std::cerr << "Loading policies" << std::endl;
    PolicyVec policy_obj_x('x', file_path_1, true);
    PolicyVec policy_obj_o('o', file_path_2, true);
    double expected_utility = get_expected_utility_wrapper(policy_obj_x, policy_obj_o);
    std::cerr << "Expected utility of initial policies: " << expected_utility << std::endl;

    // compute epsilon best response
    int experiment_num = start_index;
    int B = 5;
    int H = 9;

    std::vector<std::string>& player_information_sets = player == 'x' ? P1_information_sets : P2_information_sets;
    std::vector<std::vector<std::unordered_set<std::string>>> cohorts(player_information_sets.size(), std::vector<std::unordered_set<std::string>>(NUM_ACTIONS));
    precompute_cohorts(cohorts, player_information_sets, player);

    while (experiment_num < experiments + start_index) {
        // initialize sequences for player
        std::vector<Sequence> terminal_sequences;
        std::unordered_map<std::string, int> sequence_hash_to_index_map;
        get_sequences_wrapper(terminal_sequences, player, sequence_hash_to_index_map, cohorts);
        std::cerr << "Number of sequences: " << terminal_sequences.size() << std::endl;

        if (player == 'x') {
            PolicyVec random_x('x', P1_information_sets, false);
            calc_br_sequence_LUCB(policy_obj_o, 'x', P1_information_sets, log_frequency, random_x, experiment_num, iterations, C_r, C_p, exp_name, sequence_hash_to_index_map, terminal_sequences, cohorts, B, H);
        }
        else {
            PolicyVec random_o('o', P2_information_sets, false);
            calc_br_sequence_LUCB(policy_obj_x, 'o', P2_information_sets, log_frequency, random_o, experiment_num, iterations, C_r, C_p, exp_name, sequence_hash_to_index_map, terminal_sequences, cohorts, B, H);
        }

        std::cerr << "(" << experiment_num << " experiments done)" << std::endl;
        experiment_num += 1;
    }
}