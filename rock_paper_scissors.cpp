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


double expected_utility(std::vector<double>& strategy_x, std::vector<double>& strategy_o){
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


void LUCB(std::vector<double>& strategy_x, std::vector<double>& strategy_o, char br_player){
    std::vector<std::vector<double>> arms = {{1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}};
    std::vector<double> UCB(3, 0.0);
    std::vector<double> LCB(3, 0.0);
    std::vector<int> empirical_reward(3, 0);
    std::vector<int> pull_count(3, 0);
}

int main(int argc, char* argv[]){

    return 0;
}