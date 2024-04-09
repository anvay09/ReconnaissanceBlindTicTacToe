#include "rbt_classes.hpp"
#include <chrono>
#include <ctime>


void calc_average_policy(std::vector<Policy>& policy_obj_list, InformationSet& I, char initial_player, Policy& average_policy) {
    std::string I_hash = I.get_hash();
    std::cout << "Calculating average policy for " << I_hash << "..." << std::endl;
    average_policy.player = I.player;

    std::vector<double> prob_dist;
    for (int i = 0; i < 13; i++) {
        prob_dist.push_back(0);
    }
    
    std::vector<int> actions;
    I.get_actions(actions);

    for (int action: actions) {
        double numerator = 0;
        double denominator = 0;

        for (int p = 0; p < policy_obj_list.size(); p++) {
            std::vector<double>& policy = policy_obj_list[p].policy_dict[I_hash];
            numerator += policy[action];
            for (int act: actions) {
                denominator += policy[act];
            }
        }

        if (denominator == 0) {
            prob_dist[action] = 0;
        } else {
            prob_dist[action] = numerator / denominator;
        }
    }

    average_policy.policy_dict[I_hash] = prob_dist;
}



int main() {
    std::vector<Policy> policy_obj_list;
    char initial_player = 'x';
    std::string board_1 = "x0-0o----";
    InformationSet I = InformationSet('x', true, board_1);
    Policy average_policy;
    calc_average_policy(policy_obj_list, I, initial_player, average_policy);
    return 0;
}