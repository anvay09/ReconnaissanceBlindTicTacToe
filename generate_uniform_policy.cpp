#include "cpp_headers/rbt_classes.hpp"
#include "cpp_headers/rbt_utilities.hpp"

void save_map_json(std::string output_file, std::unordered_map<std::string, std::vector<double>>& map){
    std::ofstream f_out;
    f_out.open(output_file, std::ios::trunc);
    json jx;
    for (auto& it: map) {
        for (int i = 0; i < 13; i++) {
            jx[it.first][std::to_string(i)] = it.second[i];
        }
    }
    f_out << jx.dump() << std::endl;
    f_out.close();
}

void save_policy(std::string information_set_file, char player) {
    std::ifstream f_is(information_set_file);
    std::string line_is;

    std::unordered_map<std::string, std::vector<double> > policy_map;
    bool move_flag;
    
    while (std::getline(f_is, line_is)) {
        for (int i = 0; i < 13; i++) {
            policy_map[line_is][i] = 0;
        }
        
        std::string I_hash = line_is;
        if (I_hash.size() != 0){
            move_flag = I_hash[I_hash.size()-1] == '|' ? true : false;
        }
        else {
            if (player == 'x'){
                move_flag = true;
            }
            else {
                move_flag = false;
            }
        }

        InformationSet I(player, move_flag, I_hash);

        std::vector<int> actions;
        I.get_actions(actions);

        for (int action : actions){
            policy_map[line_is][action] = 1.0 / actions.size();
        }
    }
    f_is.close();
    if (player == 'x'){
        save_map_json("P1_unfiorm_policy_v2.json", policy_map);
    }
    else {
        save_map_json("P2_uniform_policy_v2.json", policy_map);
    }
}

int main(int argc, char* argv[]) {
    std::string information_set_file = argv[1];
    char player = argv[2][0];
    save_policy(information_set_file, player);
    return 0;
}