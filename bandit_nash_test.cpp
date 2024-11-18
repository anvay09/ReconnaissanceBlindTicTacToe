#include "cpp_headers/rbt_classes.hpp"
#include "cpp_headers/rbt_utilities.hpp"
#include <random>
#include <cmath>
int NUMBER_THREADS = 4;
int AVERAGE_DELAY = 5;


//avg
void calc_average_terms(char player, std::vector<std::string>& information_sets, PolicyVec& policy_obj, std::vector<std::vector<double>>& avg_policy_numerator, std::vector<double>& avg_policy_denominator, int t){
    //int weight = T > AVERAGE_DELAY ? T - AVERAGE_DELAY : 0;
    int weight = 1;

    #pragma omp parallel for num_threads(NUMBER_THREADS) shared(avg_policy_numerator, avg_policy_denominator, policy_obj)
    for (long int i = 0; i < information_sets.size(); i++) {
        std::string I_hash = information_sets[i];
        bool move_flag = get_move_flag(I_hash, player);
        InformationSet I(player, move_flag, I_hash);

        std::vector<int> actions;
        I.get_actions(actions);
        for (int action: actions) {
            std::vector<double>& policy = policy_obj.policy_dict[I.get_index()];
            avg_policy_numerator[I.get_index()][action] += weight * policy[action];
            avg_policy_denominator[I.get_index()] += weight * policy[action];
        }

    }
}

void calc_average_policy(std::vector<std::string>& information_sets, PolicyVec& avg_policy_obj, std::vector<std::vector<double>> avg_policy_numerator, std::vector<double> avg_policy_denominator, char player){
    #pragma omp parallel for num_threads(NUMBER_THREADS) shared(avg_policy_obj, avg_policy_numerator, avg_policy_denominator)
    for (long int i = 0; i < information_sets.size(); i++) {
        std::string I_hash = information_sets[i];
        bool move_flag = get_move_flag(I_hash, player);
        InformationSet I(player, move_flag, I_hash);

        std::vector<int> actions;
        I.get_actions(actions);
        for (int action: actions) {
            std::vector<double>& policy = avg_policy_obj.policy_dict[I.get_index()];
            policy[action] = avg_policy_denominator[I.get_index()] > 0 ? avg_policy_numerator[I.get_index()][action] / avg_policy_denominator[I.get_index()] : 0;
        }
    }
}
//avg


void pretty_print(std::chrono::time_point<std::chrono::system_clock> start, std::chrono::time_point<std::chrono::system_clock> end, std::string msg, int flag) {
    if (flag) {
    std::chrono::duration<double> elapsed_seconds = end-start;
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);
    std::cout << "finished " << msg << " in " << elapsed_seconds.count() << "s" << std::endl;
    }
}


int sampleIndex(const std::vector<double>& probabilities) {
    std::random_device rd;
    std::mt19937 generator(rd());
    std::discrete_distribution<int> distribution(probabilities.begin(), probabilities.end());
    return distribution(generator);
}


double sample_terminal_history(InformationSet& I_1, InformationSet& I_2, TicTacToeBoard& true_board, PolicyVec& policy_obj_x, 
                               PolicyVec& policy_obj_o, History& current_history, char player, double& reward) {
    InformationSet& I = player == 'x' ? I_1 : I_2;
    PolicyVec& policy_obj = player == 'x' ? policy_obj_x : policy_obj_o;
    std::vector<double> prob_dist = policy_obj.policy_dict[I.get_index()];

    int action = sampleIndex(prob_dist);

    if (I.move_flag) {
        bool success = true_board.update_move(action, player);
        current_history.history.push_back(action);

        char winner;
        if (success && !true_board.is_win(winner) && !true_board.is_over()) {
            InformationSet new_I = I;
            new_I.update_move(action, player);
            new_I.reset_zeros();

            if (player == 'x') {
                return sample_terminal_history(new_I, I_2, true_board, policy_obj_x, policy_obj_o, current_history, 'o', reward);
            } else {
                return sample_terminal_history(I_1, new_I, true_board, policy_obj_x, policy_obj_o, current_history, 'x', reward);
            }
        } else {
            TerminalHistory H_T = TerminalHistory(current_history.history);
            H_T.set_reward();
            reward = (double) H_T.reward[0];
            return reward;
        }
    }
    else {
        InformationSet new_I = I;
        new_I.simulate_sense(action, true_board);
        current_history.history.push_back(action);

        if (player == 'x') {
            return sample_terminal_history(new_I, I_2, true_board, policy_obj_x, policy_obj_o, current_history, 'x', reward);
        } else {
            return sample_terminal_history(I_1, new_I, true_board, policy_obj_x, policy_obj_o, current_history, 'o', reward);
        }
    }
}


double sample_terminal_history_wrapper(PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, History& current_history, double& reward) {
    std::string board = "000000000";
    TicTacToeBoard true_board = TicTacToeBoard(board);
    std::string hash_1 = "";
    std::string hash_2 = "";
    InformationSet I_1 = InformationSet('x', true, hash_1);
    InformationSet I_2 = InformationSet('o', false, hash_2);
    return sample_terminal_history(I_1, I_2, true_board, policy_obj_x, policy_obj_o, current_history, 'x', reward);
}


void update_ucb(std::vector<std::vector<double>>& infoset_ucb_values, std::vector<std::vector<double>>& infoset_empirical_reward, std::vector<std::vector<long int>>& infoset_pull_count, std::vector<long int>& infoset_time_steps, PolicyVec& policy_obj, double reward, TerminalHistory& history, char player, int C) {
    // TODO
    std::string board = "000000000";
    TicTacToeBoard true_board = TicTacToeBoard(board);
    std::string hash_1 = "";
    std::string hash_2 = "";
    InformationSet I_1 = InformationSet('x', true, hash_1);
    InformationSet I_2 = InformationSet('o', false, hash_2);
    char curr_player = 'x';
    double total_reward = 0.0;
    long int total_pull = 0;

    for (int action : history.history) {

        if (curr_player == player) {
            InformationSet I = curr_player == 'x' ? I_1 : I_2;
            total_pull = infoset_pull_count[I.get_index()][action];
            total_reward = infoset_empirical_reward[I.get_index()][action] * total_pull;
            infoset_pull_count[I.get_index()][action] += 1;
            infoset_empirical_reward[I.get_index()][action] =  (total_reward + reward) / (total_pull + 1);
            infoset_time_steps[I.get_index()] += 1;
            std::vector<int> legal_actions;
            I.get_actions(legal_actions);
            double sum_ucb = 0.0;
            for (int a : legal_actions){
                double ucb_value = 0.0;
                if (infoset_pull_count[I.get_index()][a] > 0){
                    ucb_value = infoset_empirical_reward[I.get_index()][a] + sqrt(C*log(infoset_time_steps[I.get_index()]) / infoset_pull_count[I.get_index()][a]);
                    infoset_ucb_values[I.get_index()][a] = ucb_value;
                }
                else {
                    ucb_value = infoset_empirical_reward[I.get_index()][a] + sqrt(C*log(infoset_time_steps[I.get_index()]) / 1);
                    infoset_ucb_values[I.get_index()][a] = ucb_value;
                }
                sum_ucb += ucb_value;
            }
            std::vector<double>& policy_infoset = policy_obj.policy_dict[I.get_index()];
            for (int a : legal_actions){
                if (sum_ucb > 0){
                    policy_infoset[a] = infoset_ucb_values[I.get_index()][a]/sum_ucb;
                }
                else {
                    policy_infoset[a] = 1.0/legal_actions.size();
                }
            }
        }

        if (action < 9) {
            if (curr_player == 'x') {
                I_1.update_move(action, curr_player);
                I_1.reset_zeros();
            } else {
                I_2.update_move(action, curr_player);
                I_2.reset_zeros();
            }
            true_board.update_move(action, curr_player);
            curr_player = (curr_player == 'x') ? 'o' : 'x';
        } 
        else {
            if (curr_player == 'x') {
                I_1.simulate_sense(action, true_board);
            } else {
                I_2.simulate_sense(action, true_board);
            }
        }
    }

}


void calc_nash_ucb(long int num_iterations, std::vector<std::string>& x_information_sets, std::vector<std::string>& o_information_sets,  int log_flag, long int log_frequency, int C, PolicyVec& x_ucb_policy, PolicyVec& o_ucb_policy) {
    std::vector<long int> x_infoset_time_steps(x_information_sets.size(), 0);
    std::vector<std::vector<double>> x_infoset_ucb_values(x_information_sets.size(), std::vector<double>(13, 0.0));
    std::vector<std::vector<double>> x_infoset_empirical_reward(x_information_sets.size(), std::vector<double>(13, 0.0));
    std::vector<std::vector<long int>> x_infoset_pull_count(x_information_sets.size(), std::vector<long int>(13, 0));
    std::vector<long int> o_infoset_time_steps(o_information_sets.size(), 0);
    std::vector<std::vector<double>> o_infoset_ucb_values(o_information_sets.size(), std::vector<double>(13, 0.0));
    std::vector<std::vector<double>> o_infoset_empirical_reward(o_information_sets.size(), std::vector<double>(13, 0.0));
    std::vector<std::vector<long int>> o_infoset_pull_count(o_information_sets.size(), std::vector<long int>(13, 0));
    std::vector<std::vector<double>> avg_x_policy_numerator(x_information_sets.size(), std::vector<double>(13, 0.0));
    std::vector<double> avg_x_policy_denominator;
    for (long int i = 0; i < x_information_sets.size(); i++) {
        avg_x_policy_denominator.push_back(0.0);
    }
    PolicyVec avg_x_policy('x', x_information_sets);
    avg_x_policy = x_ucb_policy;
    std::vector<std::vector<double>> avg_o_policy_numerator(o_information_sets.size(), std::vector<double>(13, 0.0));
    std::vector<double> avg_o_policy_denominator;
    for (long int i = 0; i < o_information_sets.size(); i++) {
        avg_o_policy_denominator.push_back(0.0);
    }
    PolicyVec avg_o_policy('o', o_information_sets);
    avg_o_policy = o_ucb_policy;

    for (long int t = 0; t < num_iterations; t++) {
        std::vector<int> h = {};
        TerminalHistory start_history = TerminalHistory(h);
        double reward = 0.0;
        sample_terminal_history_wrapper(avg_x_policy, avg_o_policy, start_history, reward);
        // update ucb values
        update_ucb(x_infoset_ucb_values, x_infoset_empirical_reward, x_infoset_pull_count, x_infoset_time_steps, x_ucb_policy, reward, start_history, 'x', C);
        double reward_new = 0.0 - reward;
        update_ucb(o_infoset_ucb_values, o_infoset_empirical_reward, o_infoset_pull_count, o_infoset_time_steps, o_ucb_policy, reward_new, start_history, 'o', C);
        if (t % log_frequency == 0 && t != 0){
            calc_average_terms('x', x_information_sets, x_ucb_policy, avg_x_policy_numerator, avg_x_policy_denominator, t);
            calc_average_policy(x_information_sets, avg_x_policy, avg_x_policy_numerator, avg_x_policy_denominator, 'x');
            calc_average_terms('o', o_information_sets, o_ucb_policy, avg_o_policy_numerator, avg_o_policy_denominator, t);
            calc_average_policy(o_information_sets, avg_o_policy, avg_o_policy_numerator, avg_o_policy_denominator, 'o');     
            double expected_utility = 0.0;
            expected_utility = get_expected_utility_wrapper(avg_o_policy, avg_o_policy);
            std::cout << "Expected utility after iteration " << t << ": " << expected_utility << std::endl;
        }
    } 
}

int main(int argc, char* argv[]) {
    std::cout.precision(17);
    std::string start_file_path_1 = argv[1];
    std::string start_file_path_2 = argv[2];
    int log_flag = std::stoi(argv[3]);
    NUMBER_THREADS = std::stoi(argv[4]); //96;

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
    for (long int i = 0; i < P1_information_sets.size(); i++) {
        InformationSet::P1_hash_to_int_map[P1_information_sets[i]] = i;
    }
    for (long int i = 0; i < P2_information_sets.size(); i++) {
        InformationSet::P2_hash_to_int_map[P2_information_sets[i]] = i;
    }

    // load policies
    auto start = std::chrono::system_clock::now(); 
    PolicyVec start_policy_obj_x('x', start_file_path_1);
    PolicyVec start_policy_obj_o('o', start_file_path_2);
    auto end = std::chrono::system_clock::now();
    pretty_print(start, end, "loading policies", log_flag);

    // compute epsilon best response
    char continue_exp = 'y';
    while (continue_exp == 'y') {
        long int num_iterations = 10000;
        long int log_frequency = 10000;
        int C = 1;
        std::cout << "Enter number of iterations: ";
        std::cin >> num_iterations;
        std::cout << "Enter log frequnecy: ";
        std::cin >> log_frequency;
        std::cout << "Enter hyperparameter c for UCB:";
        std::cin >> C;

        calc_nash_ucb(num_iterations, P1_information_sets, P2_information_sets, log_flag, log_frequency, C, start_policy_obj_x, start_policy_obj_o);
        std::cout << "Continue experiments? (y/n): ";
        std::cin >> continue_exp;
    }
    
}
