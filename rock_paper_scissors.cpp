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


int sampleIndex(const std::vector<double> &probabilities)
{
    std::random_device rd;
    std::mt19937 generator(rd());
    std::discrete_distribution<int> distribution(probabilities.begin(), probabilities.end());
    return distribution(generator);
}


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


double sample_game(std::vector<double>& strategy_x, std::vector<double>& strategy_o, std::pair<int, int>& game){
    int a_x = sampleIndex(strategy_x);
    int a_o = sampleIndex(strategy_o);
    game = std::make_pair(a_x, a_o);
    return reward(a_x, a_o);
}


bool LUCB_stopping_condition(std::vector<double>& UCB, std::vector<double>& LCB, double eps){
    int max_UCB_policy_index = 0;
    int second_max_UCB_policy_index = 0;
    
    for (int i = 0; i < UCB.size(); i++){
        if (UCB[i] > UCB[max_UCB_policy_index]){
            second_max_UCB_policy_index = max_UCB_policy_index;
            max_UCB_policy_index = i;
        }
        else if (UCB[i] > UCB[second_max_UCB_policy_index]){
            second_max_UCB_policy_index = i;
        }
    }

    if (LCB[max_UCB_policy_index] > UCB[second_max_UCB_policy_index] - eps){
        std::cout << "--------------- UCB values: " << UCB[0] << " " << UCB[1] << " " << UCB[2] << " ---------------" << std::endl;
        std::cout << "--------------- LCB values: " << LCB[0] << " " << LCB[1] << " " << LCB[2] << " ---------------" << std::endl;
    
        return true;
    }
    return false;
}


int get_arm_with_highest_empirical_mean(std::vector<double>& total_empirical_reward, std::vector<int>& pull_count){
    int max_empirical_mean_policy_index = 0;
    double max_empirical_mean = total_empirical_reward[0] / pull_count[0];

    for (int i = 1; i < total_empirical_reward.size(); i++){
        double empirical_mean = total_empirical_reward[i] / pull_count[i];
        if (empirical_mean > max_empirical_mean){
            max_empirical_mean = empirical_mean;
            max_empirical_mean_policy_index = i;
        }
    }

    return max_empirical_mean_policy_index;
}


int get_arm_with_highest_UCB(std::vector<double>& UCB){
    int max_UCB_policy_index = 0;
    double max_UCB = UCB[0];

    for (int i = 1; i < UCB.size(); i++){
        if (UCB[i] > max_UCB){
            max_UCB = UCB[i];
            max_UCB_policy_index = i;
        }
    }

    return max_UCB_policy_index;
}


void logging(int num_samples, std::vector<std::vector<double>>& arms, std::vector<double>& total_empirical_reward, std::vector<int>& pull_count, 
             std::vector<double>& strategy_x, std::vector<double>& strategy_o, char player, std::string output_file, double best_true_expected_utility,
             std::vector<double>& UCB, std::vector<double>& LCB){
    std::cout << "--------------- Num Samples: " << num_samples << " ---------------" << std::endl;
    std::cout << "--------------- UCB values: " << UCB[0] << " " << UCB[1] << " " << UCB[2] << " ---------------" << std::endl;
    std::cout << "--------------- LCB values: " << LCB[0] << " " << LCB[1] << " " << LCB[2] << " ---------------" << std::endl;

    int max_empirical_mean_policy_index = get_arm_with_highest_empirical_mean(total_empirical_reward, pull_count);
    double exploitability = 0.0;
    std::vector<double>& arm = arms[max_empirical_mean_policy_index];
    if (player == 'x'){
        double expected_utility_arm = get_expected_utility(arm, strategy_o);
        std::cout << "Expected utility of highest empirical mean arm: " << expected_utility_arm << std::endl;
        exploitability = best_true_expected_utility - expected_utility_arm;
    } else {
        double expected_utility_arm = get_expected_utility(strategy_x, arm);
        std::cout << "Expected utility of highest empirical mean arm: " << expected_utility_arm << std::endl;
        exploitability = best_true_expected_utility - expected_utility_arm;
    }

    // append to outfile
    std::ofstream outfile;
    outfile.open(output_file, std::ios_base::app);
    outfile << num_samples << " " << exploitability << std::endl;
    outfile.close();
}


void LUCB(std::vector<double>& strategy_x, std::vector<double>& strategy_o, char br_player, double eps, double delta, std::string output_file, int log_freq){
    std::vector<std::vector<double>> arms = {{1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}};
    std::vector<double> true_expected_utilities(3, 0.0);
    std::vector<double> UCB(3, 0.0);
    std::vector<double> LCB(3, 0.0);
    std::vector<double> total_empirical_reward(3, 0.0);
    std::vector<int> pull_count(3, 0);
    int T = 0;
    double k = 4.0/5.0;
    int max_UCB_policy_index = 0;
    int max_empirical_mean_policy_index = 0;
    int num_samples = 0;
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

    for (int i = 0; i < 3; i++){
        std::vector<double>& arm = arms[i];
        if (br_player == 'x'){
            std::pair<int, int> game;
            double reward = sample_game(arm, strategy_o, game);
            total_empirical_reward[i] += reward;
        }
        else{
            std::pair<int, int> game;
            double reward = sample_game(strategy_x, arm, game);
            total_empirical_reward[i] += reward;
        }

        pull_count[i] += 1;
        num_samples += 1;
        T += 1;
    }

    for (int i = 0; i < 3; i++){
        UCB[i] = total_empirical_reward[i] / pull_count[i] + std::sqrt(std::log(k * 3.0 * std::pow(T, 4) / delta) / (2 * pull_count[i]));
        LCB[i] = total_empirical_reward[i] / pull_count[i] - std::sqrt(std::log(k * 3.0 * std::pow(T, 4) / delta) / (2 * pull_count[i]));
    }

    while (!LUCB_stopping_condition(UCB, LCB, eps)){
        // select arm with highest UCB
        max_UCB_policy_index = get_arm_with_highest_UCB(UCB);

        // sample terminal history
        std::vector<double>& arm_UCB = arms[max_UCB_policy_index];
        if (br_player == 'x'){
            std::pair<int, int> game;
            double reward_UCB = sample_game(arm_UCB, strategy_o, game);
            total_empirical_reward[max_UCB_policy_index] += reward_UCB;
        } else {
            std::pair<int, int> game;
            double reward_UCB = sample_game(strategy_x, arm_UCB, game);
            total_empirical_reward[max_UCB_policy_index] += reward_UCB;
        }
        
        // update empirical mean, UCB, LCB
        pull_count[max_UCB_policy_index] += 1;
        num_samples += 1;

        if (num_samples % log_freq == 0 && num_samples > 0){
            logging(num_samples, arms, total_empirical_reward, pull_count, strategy_x, strategy_o, br_player, output_file, best_true_expected_utility, UCB, LCB);
        }

        for (int i = 0; i < 3; i++) {
            UCB[i] = total_empirical_reward[i] / pull_count[i] + std::sqrt(std::log(k * 3.0 * std::pow(T, 4) / delta) / (2 * pull_count[i]));
            LCB[i] = total_empirical_reward[i] / pull_count[i] - std::sqrt(std::log(k * 3.0 * std::pow(T, 4) / delta) / (2 * pull_count[i]));
        }

        // select arm with highest empirical mean
        max_empirical_mean_policy_index = get_arm_with_highest_empirical_mean(total_empirical_reward, pull_count);

        // sample terminal history
        std::vector<double>& arm = arms[max_empirical_mean_policy_index];
        if (br_player == 'x'){
            std::pair<int, int> game;
            double reward = sample_game(arm, strategy_o, game);
            total_empirical_reward[max_empirical_mean_policy_index] += reward;
        } else {
            std::pair<int, int> game;
            double reward = sample_game(strategy_x, arm, game);
            total_empirical_reward[max_empirical_mean_policy_index] += reward;
        }

        // update empirical mean, UCB, LCB
        pull_count[max_empirical_mean_policy_index] += 1;
        num_samples += 1;
        if (num_samples % log_freq == 0 && num_samples > 0){
            logging(num_samples, arms, total_empirical_reward, pull_count, strategy_x, strategy_o, br_player, output_file, best_true_expected_utility, UCB, LCB);
        }

        for (int i = 0; i < 3; i++) {
            UCB[i] = total_empirical_reward[i] / pull_count[i] + std::sqrt(std::log(k * 3.0 * std::pow(T, 4) / delta) / (2 * pull_count[i]));
            LCB[i] = total_empirical_reward[i] / pull_count[i] - std::sqrt(std::log(k * 3.0 * std::pow(T, 4) / delta) / (2 * pull_count[i]));
        }

        // update T
        T += 1;
    }

    std::cout << "--------------- Stopping Condition Reached ---------------" << std::endl;
    std::cout << "--------------- Num Samples: " << num_samples << " ---------------" << std::endl;
    max_empirical_mean_policy_index = get_arm_with_highest_empirical_mean(total_empirical_reward, pull_count);
    std::vector<double>& arm = arms[max_empirical_mean_policy_index];
    if (br_player == 'x'){
        double expected_utility_arm = get_expected_utility(arm, strategy_o);
        std::cout << "Expected utility of highest empirical mean arm: " << expected_utility_arm << std::endl;
    } else {
        double expected_utility_arm = get_expected_utility(strategy_x, arm);
        std::cout << "Expected utility of highest empirical mean arm: " << expected_utility_arm << std::endl;
    }
}


double construct_loss_estimator(std::vector<double>& mu_t, std::vector<double>& mu_star, double gamma, double reward, int trajectory) {
    double mu_star_reach = 1.0;
    double mu_t_reach = 1.0;

    mu_star_reach *= mu_star[trajectory];
    mu_t_reach *= mu_t[trajectory];
    double loss_obj = ((1.0 - reward) / (mu_t_reach + gamma * mu_star_reach));
    return loss_obj;
}


void update_policy(std::vector<double>& mu_t, std::vector<double>& mu_star, double loss_obj, double learning_rate, int trajectory) {
    double Z_t = 1.0;
    double term = 0.0;
        
    term = -learning_rate * mu_star[trajectory] * loss_obj;
    Z_t = 1 - mu_t[trajectory] + mu_t[trajectory] * std::exp(term);

    for (int i = 0; i < 3; i++){
        if (i == trajectory){
            mu_t[i] *= std::exp(term - std::log(Z_t));
        }
        else {
            mu_t[i] *= std::exp( - std::log(Z_t));
        }
    }
}


void balanced_OMD(std::vector<double>& mu_t, std::vector<double>& mu_star, char br_player, double gamma, double learning_rate, int iterations, int log_frequency, std::vector<double>& opp_policy){
    // compute best expected utility
    std::vector<std::vector<double>> arms = {{1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}};
    std::vector<double> true_expected_utilities(3, 0.0);
    double best_true_expected_utility = br_player == 'x' ? -1.0 : 1.0;

    for (int i = 0; i < 3; i++){
        if (br_player == 'x'){
            true_expected_utilities[i] = get_expected_utility(arms[i], opp_policy);
            std::cout << "Arm " << i << " true expected utility: " << true_expected_utilities[i] << std::endl;
            std::cout << "Strategy: " << arms[i][0] << " " << arms[i][1] << " " << arms[i][2] << std::endl;
            if (true_expected_utilities[i] > best_true_expected_utility){
                best_true_expected_utility = true_expected_utilities[i];
            }
        }
        else{
            true_expected_utilities[i] = get_expected_utility(opp_policy, arms[i]);
            std::cout << "Arm " << i << " true expected utility: " << true_expected_utilities[i] << std::endl;
            std::cout << "Strategy: " << arms[i][0] << " " << arms[i][1] << " " << arms[i][2] << std::endl;
            if (true_expected_utilities[i] < best_true_expected_utility){
                best_true_expected_utility = true_expected_utilities[i];
            }
        }
    }

    std::cout << "Best true expected utility: " << best_true_expected_utility << std::endl;
    std::cout << "Starting balanced OMD..." << std::endl;
    
    for (int t = 0; t <= iterations; t++){
        std::pair<int, int> game;
        int trajectory = 0;
        double reward = 0.0;
        if (br_player == 'x'){
            reward = sample_game(mu_t, opp_policy, game);
            trajectory = game.first;
        }
        else {
            reward = sample_game(opp_policy, mu_t, game);
            trajectory = game.second;
        }

        // fit reward between 0 and 1
        reward = (reward + 1.0) / (2.0);

        double loss_obj = construct_loss_estimator(mu_t, mu_star, gamma, reward, trajectory);
        update_policy(mu_t, mu_star, loss_obj, learning_rate, trajectory);

        if (t % log_frequency == 0){
            std::cout << "-------------------------------- Iteration " << t << " --------------------------------" << std::endl;
            double expected_utility = 0.0;

            if (br_player == 'x') {
                expected_utility = get_expected_utility(mu_t, opp_policy);
            } else {
                expected_utility = get_expected_utility(opp_policy, mu_t);
            }

            std::cout << "Expected utility: " << expected_utility << std::endl;
        }
    }
}


int main(int argc, char* argv[]){
    double eps = std::stod(argv[1]); // also can be used as gamma
    double delta = std::stod(argv[2]); // also can be used as learning rate
    int log_freq = std::stoi(argv[3]);
    int iterations = std::stoi(argv[4]);
    std::string algorithm = argv[5];

    std::vector<double> strategy_x = {0.5, 0.3, 0.2};
    std::vector<double> strategy_o = {0.33, 0.32, 0.35};

    if (algorithm == "LUCB") {
        LUCB(strategy_x, strategy_o, 'x', eps, delta, "data/RPS/trial.txt", log_freq);
    }
    else if (algorithm == "OMD") {
        std::vector<double> mu_star = {0.333333, 0.333333, 0.333334};
        balanced_OMD(strategy_x, mu_star, 'x', eps, delta, iterations, log_freq, strategy_o);
    }

    return 0;
}