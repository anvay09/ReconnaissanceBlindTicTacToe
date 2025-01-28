#include <iostream>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <vector>
#include <algorithm>
#include <string>
#include <set>
#include <iterator>
#include <utility>
#include <tuple>
#include <cassert>
#include <chrono>
#include <ctime>
#include <random>
#include <cmath>


static std::random_device rd;
static std::mt19937 generator(rd());

// g++ .\rock_paper_scissors_mccfr_variants.cpp -O -o rps
// .\rps 0.1 1000 1000 mccfr x

int reward(int a_x, int a_o){
    // 0: rock, 1: paper, 2: scissors, reward from perspective of x
    int diff = a_x - a_o;
    if(diff == 0) return 0;
    if(diff == 1 || diff == -2) return 1;
    if(diff == -1 || diff == 2) return -1;
    return 0;
}


double get_expected_utility(std::vector<double>& strategy_x, std::vector<double>& strategy_o){
    double expected_utility = 0.0;
    for (int i = 0; i < 3; i++){
        for (int j = 0; j < 3; j++){
            expected_utility += strategy_x[i] * strategy_o[j] * reward(i, j);
        }
    }
    return expected_utility;
}


int sampleIndex(const std::vector<double> &probabilities)
{
    std::discrete_distribution<int> distribution(probabilities.begin(), probabilities.end());
    return distribution(generator);
}


double sample_game(std::vector<double> &player_policy, std::vector<double> &opponent_policy, double &payoff, int& trajectory, char update_player, double eps, std::vector<double> &uniform_policy)
{   int a_player = -1;
    int a_opponent = -1;
    std::vector<double> eps_prob_dist = {eps, 1.0 - eps};
    if (sampleIndex(eps_prob_dist))
    {   a_player = sampleIndex(player_policy);}
    else
    {   a_player = sampleIndex(uniform_policy);}
    a_opponent = sampleIndex(opponent_policy);
    trajectory = a_player;
    if (update_player == 'x')
    {   payoff = reward(a_player, a_opponent);}
    else
    {   payoff = reward(a_opponent, a_player);}
    return ((1.0 - eps) * player_policy[a_player] + eps * uniform_policy[a_player]);
}


void compute_regret(std::vector<double> &player_br_policy, std::vector<double> &player_cumulative_strategy, char br_player,
                                           long int t, std::vector<double> &regret_list,
                                           long int &markers, int &trajectory, double q_z, double payoff)
{       double played_action_prob = player_br_policy[trajectory];
        double reach_prob = 0.0;
        double regret_sum = 0.0;
        for (int i = 0; i < 3; i++)
        {   if (i == trajectory)
            {   if (played_action_prob > 0)
                {   regret_list[i] += (payoff * (1 - played_action_prob)) / q_z;}
                else
                {   regret_list[i] += (payoff) / (q_z);}
            }
            else
            {   regret_list[i] += -payoff * played_action_prob / q_z;
            }
            player_cumulative_strategy[i] += (t - markers) * player_br_policy[i];
            regret_sum += regret_list[i] > 0 ? regret_list[i] : 0;
        }
        markers = t;
        // regret matching
        for (int i = 0; i < 3; i++)
        {   if (regret_sum > 0)
            {   player_br_policy[i] = regret_list[i] > 0 ? regret_list[i] / regret_sum : 0.0;}
            else
            {   player_br_policy[i] = 1.0 / 3.0;}
        }
}

void MCCFR(std::vector<double> &opponent_policy, std::vector<double> &player_br_policy, char br_player, long int T, double eps, long int log_size, double exact_br_value, std::vector<double> &player_uniform_policy, std::string algorithm, int experiment_number)
{
    std::vector<double> regret_list = {0.0, 0.0, 0.0};
    long int markers = 0;
    std::vector<double> cumulative_strategy = {0.0, 0.0, 0.0};
    std::vector<long int> pulls = {0, 0, 0};
    std::vector<std::pair<int, double>> exploitability_log;
    std::vector<std::pair<int, double>> exploitability_log_average;

    for (int t = 1; t <= T; t++)
    {
        int trajectory = 0;
        double q_z = 0.0;
        double payoff = 0.0;
        if (algorithm == "onpath") {
            double val = 10.0 / (std::sqrt(std::sqrt(t)));
            eps = val > 1.0 ? 1.0 : val;
        }
        q_z = sample_game(player_br_policy, opponent_policy, payoff, trajectory, br_player, eps, player_uniform_policy);
        compute_regret(player_br_policy, cumulative_strategy, br_player, t, regret_list, markers, trajectory, q_z, payoff);
        pulls[trajectory] += 1;

        if (t % log_size == 0 && t != 0)
        {   std::cout << "Experiment number" << experiment_number << std::endl;
            std::cout << "############################################################" << std::endl;
            std::vector<double> average_strategy = cumulative_strategy;
            // normalize the cumulative strategy
            double sum = 0.0;
            for (int j = 0; j < 3; j++)
            {   sum += average_strategy[j];}
            if (sum > 0){
                {   for (int j = 0; j < 3; j++)
                    {   average_strategy[j] /= sum;}
                }
            }
            std::cout << "-------------------------------- Iteration " << t << " --------------------------------" << std::endl;
            double expected_utility = 0.0;
            std::cout << "---------- Per Iteration Strategy: " << player_br_policy[0] << " " << player_br_policy[1] << " " << player_br_policy[2] << " ----------" << std::endl;
            std::cout << "---------- Average Strategy: " << average_strategy[0] << " " << average_strategy[1] << " " << average_strategy[2] << " ----------" << std::endl;

            if (br_player == 'x') {
                expected_utility = get_expected_utility(player_br_policy, opponent_policy);
                exploitability_log.push_back(std::make_pair(t, exact_br_value - expected_utility));
            } else {
                expected_utility = get_expected_utility(opponent_policy, player_br_policy);
                exploitability_log.push_back(std::make_pair(t, exact_br_value - expected_utility));
            }
            std::cout << "Expected utility of per iteration strategy: " << expected_utility << std::endl;
            

            if (br_player == 'x') {
                expected_utility = get_expected_utility(average_strategy, opponent_policy);
                exploitability_log_average.push_back(std::make_pair(t, exact_br_value - expected_utility));
            } else {
                expected_utility = get_expected_utility(opponent_policy, average_strategy);
                exploitability_log_average.push_back(std::make_pair(t, exact_br_value - expected_utility));
            }
            std::cout << "Expected utility of average strategy: " << expected_utility << std::endl;

            std::cout << "Arm pulls:" << std::endl;
            for (int k = 0; k < 3; k++){
                std::cout << "Arm " << k << " pulls: " << pulls[k] << std::endl;
            }
        }
    }

    std::cout << "Saving exploitability logs" << std::endl;
    std::string file_name = "data/RPS/" + std::string(1, br_player) + algorithm + "_exploitability_log_" + std::to_string(experiment_number) + ".txt";
    std::string file_name_average = "data/RPS/" + std::string(1, br_player) + algorithm + "_average_exploitability_log_" + std::to_string(experiment_number) + ".txt";

    std::ofstream f(file_name);
    for (int i = 0; i < exploitability_log.size(); i++)
    {
        f << exploitability_log[i].first << " " << exploitability_log[i].second << std::endl;
    }
    f.close();
    std::ofstream f_avg(file_name_average);
    for (int i = 0; i < exploitability_log_average.size(); i++)
    {
        f_avg << exploitability_log_average[i].first << " " << exploitability_log_average[i].second << std::endl;
    }
    f_avg.close();
}

int main(int argc, char* argv[]){
    std::cout.precision(17);
    double eps = std::stod(argv[1]); // also can be used as gamma
    int log_freq = std::stoi(argv[2]);
    int iterations = std::stoi(argv[3]);
    std::string algorithm = argv[4];
    char br_player = std::string(argv[5])[0];
    int num_experiments = std::stoi(argv[6]);
    int start_index = std::stoi(argv[7]);

    std::vector<double> strategy_x = {0.5, 0.3, 0.2};
    std::vector<double> strategy_o = {0.32, 0.33, 0.35};
    std::vector<double> uniform_x = {1.0/3.0, 1.0/3.0, 1.0/3.0};
    std::vector<double> uniform_o = {1.0/3.0, 1.0/3.0, 1.0/3.0};
    std::vector<double> br_x = {uniform_x};
    std::vector<double> br_o = uniform_o;

    // compute best expected utility
    std::vector<std::vector<double>> arms = {{1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}};
    std::vector<double> true_expected_utilities(3, 0.0);
    double best_true_expected_utility = br_player == 'x' ? -1.0 : 1.0;
    for (int i = 0; i < 3; i++){
        if (br_player == 'x'){
            true_expected_utilities[i] = get_expected_utility(arms[i], strategy_o);
            std::cout << "Arm " << i << " true expected utility: " << true_expected_utilities[i] << std::endl;
            std::cout << "Strategy: " << arms[i][0] << " " << arms[i][1] << " " << arms[i][2] << std::endl;
            if (true_expected_utilities[i] > best_true_expected_utility){
                best_true_expected_utility = true_expected_utilities[i];
            }
        }
        else{
            true_expected_utilities[i] = get_expected_utility(strategy_x, arms[i]);
            std::cout << "Arm " << i << " true expected utility: " << true_expected_utilities[i] << std::endl;
            std::cout << "Strategy: " << arms[i][0] << " " << arms[i][1] << " " << arms[i][2] << std::endl;
            if (true_expected_utilities[i] < best_true_expected_utility){
                best_true_expected_utility = true_expected_utilities[i];
            }
        }
    }
    std::cout << "Best true expected utility: " << best_true_expected_utility << std::endl;

    for (int j = start_index; j < num_experiments + start_index; j++){
        if (algorithm == "mccfr") {
            if (br_player == 'x') {
                MCCFR(strategy_o, br_x, br_player, iterations, eps, log_freq, best_true_expected_utility, uniform_x, algorithm, j);
            } else {
                MCCFR(strategy_x, br_o, br_player, iterations, eps, log_freq, best_true_expected_utility, uniform_o, algorithm, j);
            }
        }
        else if (algorithm == "onpath"){
            // on-path counterfactual regret minimization
            if (br_player == 'x') {
                MCCFR(strategy_o, br_x, br_player, iterations, eps, log_freq, best_true_expected_utility, uniform_x, algorithm, j);
            } else {
                MCCFR(strategy_x, br_o, br_player, iterations, eps, log_freq, best_true_expected_utility, uniform_o, algorithm, j);
            }
        }
        else{
            std::cout << "Invalid algorithm" << std::endl;
        }
    }

    return 0;
}