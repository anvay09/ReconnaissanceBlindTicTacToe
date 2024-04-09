#include "rbt_classes.hpp"
#include <chrono>
#include <ctime>
#include "json.hpp"
#include <fstream>

using json = nlohmann::json;

void read_policy_from_json(string file_path, map<string, map<int, double> >& policy_map){
    ifstream i(file_path);
    json policy_obj;
    i >> policy_obj;
    
    for (json::iterator it = policy_obj.begin(); it != policy_obj.end(); ++it) {
        string key = it.key();
        map <int, double> probability_distribution;
        if (key.back() == 's') {
            vector<string> sense_keys = {"9", "10", "11", "12"};
            for (int i = 0; i < sense_keys.size(); i++) {
                probability_distribution[stoi(sense_keys[i])] = policy_obj[key][sense_keys[i]];
            }
        }
        else if (key.back() == 'm') {
            vector<string> move_keys = {"0", "1", "2", "3", "4", "5", "6", "7", "8"};
            for (int i = 0; i < move_keys.size(); i++) {
                probability_distribution[stoi(move_keys[i])] = policy_obj[key][move_keys[i]];
            }
        }

        policy_map[key] = probability_distribution;
    }
}

int main() {
    string file_path = "data/full_tree/cfr_policy/P1_iteration_265_cfr_policy.json";
    map<string, map<int, double> > policy_map;
    read_policy_from_json(file_path, policy_map);
}