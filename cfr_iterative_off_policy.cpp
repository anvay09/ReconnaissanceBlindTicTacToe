// Compile: g++-13 -O3 cfr_iterative_new.cpp rbt_classes.cpp rbt_utilities.cpp -o cfr_i -fopenmp -I /Users/anvay/Downloads/boost_1_84_0
// Expected utility avg: 0.39416214823722839

#include "cpp_headers/rbt_classes.hpp"
#include "cpp_headers/rbt_utilities.hpp"

int NUMBER_THREADS = 4;
int AVERAGE_DELAY = 5;

bool get_move_flag(std::string I_hash, char player){
    bool move_flag;
    if (I_hash.size() != 0){
        move_flag = I_hash[I_hash.size()-1] == '|' ? true : false;
    }
    else {
        move_flag = player == 'x' ? true : false;
    }
    return move_flag;
}

void save_map_json(std::string output_file, std::vector<std::vector<double>>& map, std::vector<std::string>& information_sets){
    std::ofstream f_out;
    f_out.open(output_file, std::ios::trunc);
    json jx;
    for (long int j = 0; j < map.size(); j++) {
        for (int i = 0; i < 13; i++) {
            jx[information_sets[j]][std::to_string(i)] = map[j][i];
        }
    }
    f_out << jx.dump() << std::endl;
    f_out.close();
}

//cfr
void run_cfr(int T, std::vector<std::string>& information_sets, std::vector<std::vector<double>>& regret_list, PolicyVec& policy_obj_x, PolicyVec& policy_obj_o, char player, std::string base_path){
        std::cout << "Starting iteration " << T << " for player " << player << "..." << std::endl;
        auto start = std::chrono::system_clock::now();

        #pragma omp parallel for num_threads(NUMBER_THREADS) shared(regret_list, policy_obj_x, policy_obj_o)
        for (long int i = 0; i < information_sets.size(); i++) {
            std::string I_hash = information_sets[i];
            // std::cout << "Starting iteration " << T << " for infoset " << I_hash << std::endl;

            bool move_flag = get_move_flag(I_hash, player);
            InformationSet I(player, move_flag, I_hash);
            calc_cfr_policy_given_I(I, policy_obj_x, policy_obj_o, T, regret_list[i]);
        }

        auto end = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = end - start;
        std::time_t end_time = std::chrono::system_clock::to_time_t(end);
        std::cout << "finished computation at " << std::ctime(&end_time)
                << "elapsed time: " << elapsed_seconds.count() << "s"
                << std::endl;


        std::cout << "Updating policy for player " << player << "..." << std::endl;
        start = std::chrono::system_clock::now();
        #pragma omp parallel for num_threads(NUMBER_THREADS) shared(regret_list, policy_obj_x, policy_obj_o)
        for (long int i = 0; i < information_sets.size(); i++) {
            std::string I_hash = information_sets[i];
            bool move_flag = get_move_flag(I_hash, player);
            InformationSet I(player, move_flag, I_hash);
            std::vector<double>& regret_vector = regret_list[i];
            double total_regret = 0.0;
            std::vector<int> actions;
            I.get_actions(actions);

            for (int action : actions) {
                total_regret += regret_vector[action];
            }

            PolicyVec& policy_obj = player == 'x' ? policy_obj_x : policy_obj_o;
            std::vector<double>& prob_dist = policy_obj.policy_dict[I.get_index()];
            if (total_regret > 0) {
                for (int action : actions) {
                    prob_dist[action] = regret_vector[action] / total_regret;
                }
            }
            else {
                for (int action : actions) {
                    prob_dist[action] = 1.0 / double(actions.size());
                }
            }
        }

        // if (player == 'x') {
        //     std::string output_policy_file_cfr = base_path + + "/cfr" + "/P1_iteration_" + std::to_string(T) + "_cfr_policy_cpp.json";
        //     save_map_json(output_policy_file_cfr, policy_obj_x.policy_dict, information_sets);
        // }
        // else if (player == 'o') {
        //     std::string output_policy_file_cfr = base_path + + "/cfr" + "/P2_iteration_" + std::to_string(T) + "_cfr_policy_cpp.json";
        //     save_map_json(output_policy_file_cfr, policy_obj_o.policy_dict, information_sets);
        // }

        end = std::chrono::system_clock::now();
        elapsed_seconds = end - start;
        end_time = std::chrono::system_clock::to_time_t(end);
        std::cout << "finished computation at " << std::ctime(&end_time)
                << "elapsed time: " << elapsed_seconds.count() << "s"
                << std::endl;


}

void initialize_start(std::string policy_file, std::string information_set_file, std::vector<std::string>& information_sets, std::vector<std::vector<double>>& regret_list,  std::vector<double>& prob_reaching_list, PolicyVec& policy_obj, PolicyVec& avg_policy_obj, std::vector<double>& avg_policy_denominator, char player) {
    std::cout << "initialize_start for player " << player << std::endl;
    auto start = std::chrono::system_clock::now();
    policy_obj = PolicyVec(player, policy_file);
    avg_policy_obj = policy_obj;
    
    std::ifstream f_is(information_set_file);
    std::string line_is;
        
    while (std::getline(f_is, line_is)) {
        avg_policy_denominator.push_back(0.0);
        // information_sets.push_back(line_is);
        std::vector<double> regret_vector;
        for (int i = 0; i < 13; i++) {
            regret_vector.push_back(0.0);
        }
        regret_list.push_back(regret_vector);
        prob_reaching_list.push_back(0.0);
    }
    f_is.close();
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);
    std::cout << "finished computation at " << std::ctime(&end_time)
            << "elapsed time: " << elapsed_seconds.count() << "s"
            << std::endl;
}

void initialize_continue(std::string policy_file, std::string information_set_file, std::vector<std::string>& information_sets, std::vector<std::vector<double>>& regret_list, std::vector<std::vector<double>>& regret_map, std::vector<double>& prob_reaching_list, PolicyVec& policy_obj, PolicyVec& avg_policy_obj, std::vector<double>& avg_policy_denominator, char player) {
    policy_obj = PolicyVec(player, policy_file);
    avg_policy_obj = policy_obj;
    
    std::ifstream f_is(information_set_file);
    std::string line_is;
    int line_number = 0;
        
    while (std::getline(f_is, line_is)) {
        avg_policy_denominator.push_back(0.0);
        // information_sets.push_back(line_is);
        std::vector<double> regret_vector;
        for (int i = 0; i < 13; i++) {
            regret_vector.push_back(regret_map[line_number][i]); // maybe can be merged with initialize_start
        }
        regret_list.push_back(regret_vector);
        prob_reaching_list.push_back(0.0);
        line_number += 1;
    }
    f_is.close();
}


void save_output(std::string output_policy_file, std::string output_regret_file, char player, std::vector<std::string>& information_sets, std::vector<std::vector<double>>& regret_list, PolicyVec& policy_obj) {
    std::vector<std::vector<double> > regret_map;
    std::cout << "Saving regrets for player " << player << "..." << std::endl;
    for (long int i = 0; i < information_sets.size(); i++) {
        std::string I_hash = information_sets[i];
        std::vector<double>& regret_vector = regret_list[i];
        regret_map.push_back(regret_vector);
    }
    save_map_json(output_regret_file, regret_map, information_sets);

    std::cout << "Saving policy for player " << player << "..." << std::endl;
    save_map_json(output_policy_file, policy_obj.policy_dict, information_sets);
}

//cfr

//avg
void calc_average_terms(char player, std::vector<std::string>& information_sets, PolicyVec& policy_obj, std::vector<std::vector<double>>& avg_policy_numerator, std::vector<double>& avg_policy_denominator, int T){
    int weight = T > AVERAGE_DELAY ? T - AVERAGE_DELAY : 0;

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

// std::unordered_map<int, std::vector<int> > InformationSet::sense_square_dict = {{9, {0, 1, 3, 4}}, {10, {1, 2, 4, 5}}, {11, {3, 4, 6, 7}}, {12, {4, 5, 7, 8}}};
// std::unordered_map<std::string, long int > InformationSet::P1_hash_to_int_map = {};
// std::unordered_map<std::string, long int > InformationSet::P2_hash_to_int_map = {};

int main(int argc, char* argv[])  {
    std::cout.precision(17);
    NUMBER_THREADS = std::stoi(argv[1]); //96;
    std::string base_path = argv[2]; //"data/Iterative_1";
    int start_iter = std::stoi(argv[3]); //1;
    int end_iter = std::stoi(argv[4]); //1000;
    std::string policy_file_x = argv[5]; //"data/P1_uniform_policy.json";
    std::string policy_file_o = argv[6]; //"data/P2_uniform_policy.json";

    // read information set file

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


    for (long int i = 0; i < P1_information_sets.size(); i++) {
        InformationSet::P1_hash_to_int_map[P1_information_sets[i]] = i;
    }
    for (long int i = 0; i < P2_information_sets.size(); i++) {
        InformationSet::P2_hash_to_int_map[P2_information_sets[i]] = i;
    }
    // std::string P1_information_sets_mapping_file = "data/P1_information_sets_mapping.txt";
    // std::string P2_information_sets_mapping_file = "data/P2_information_sets_mapping.txt";

    PolicyVec policy_obj_x;
    PolicyVec policy_obj_o;
    PolicyVec avg_policy_obj_x;
    PolicyVec avg_policy_obj_o;
    std::vector<std::vector<double>> avg_policy_numerator_x(P1_information_sets.size(), std::vector<double>(13, 0));
    std::vector<std::vector<double>> avg_policy_numerator_o(P2_information_sets.size(), std::vector<double>(13, 0));
    std::vector<double> avg_policy_denominator_x;
    std::vector<double> avg_policy_denominator_o;
    std::vector<std::vector<double>> regret_list_x;
    std::vector<std::vector<double>> regret_list_o;
    std::vector<double> prob_reaching_list_x;
    std::vector<double> prob_reaching_list_o;

    if (start_iter == 1) {
        initialize_start(policy_file_x, P1_information_sets_file, P1_information_sets, regret_list_x, prob_reaching_list_x, policy_obj_x, avg_policy_obj_x, avg_policy_denominator_x, 'x');
        initialize_start(policy_file_o, P2_information_sets_file, P2_information_sets, regret_list_o, prob_reaching_list_o, policy_obj_o, avg_policy_obj_o, avg_policy_denominator_o, 'o');
    }

    else {
        std::string prev_regret_file_x = base_path + "/regret/P1_iteration_" + std::to_string(start_iter-1) + "_regret_cpp.json";
        std::string prev_regret_file_o = base_path + "/regret/P2_iteration_" + std::to_string(start_iter-1) + "_regret_cpp.json";
        std::vector<std::vector<double> > regret_map_x;
        std::vector<std::vector<double> > regret_map_o;
        regret_map_x = get_prev_regrets(prev_regret_file_x, 'x');
        regret_map_o = get_prev_regrets(prev_regret_file_o, 'o');
        initialize_continue(policy_file_x, P1_information_sets_file, P1_information_sets, regret_list_x, regret_map_x, prob_reaching_list_x, policy_obj_x, avg_policy_obj_x, avg_policy_denominator_x, 'x');
        initialize_continue(policy_file_o, P2_information_sets_file, P2_information_sets, regret_list_o, regret_map_o, prob_reaching_list_o, policy_obj_o, avg_policy_obj_o, avg_policy_denominator_o, 'o');
    }

    std::cout << "Information set size for player 1: " << P1_information_sets.size() << std::endl;
    std::cout << "Information set size for player 2: " << P2_information_sets.size() << std::endl;
    std::cout << "Prob reaching size for player 1: " << prob_reaching_list_x.size() << std::endl;
    std::cout << "Prob reaching size for player 2: " << prob_reaching_list_o.size() << std::endl;
    std::cout << "Regret list size for player 1: " << regret_list_x.size() << std::endl;
    std::cout << "Regret list size for player 2: " << regret_list_o.size() << std::endl;


    for (int T = start_iter; T <= end_iter; T++) {
        double expected_utility = get_expected_utility_wrapper(policy_obj_x, policy_obj_o);
        std::cout << "Expected utility: " << expected_utility << std::endl; 
        run_cfr(T, P1_information_sets, regret_list_x, policy_obj_x, policy_obj_o, 'x', base_path);
        run_cfr(T, P2_information_sets, regret_list_o, policy_obj_x, policy_obj_o, 'o', base_path);
        calc_average_terms('x', P1_information_sets, policy_obj_x, avg_policy_numerator_x, avg_policy_denominator_x, T);
        calc_average_terms('o', P2_information_sets, policy_obj_o, avg_policy_numerator_o, avg_policy_denominator_o, T);
    }

    calc_average_policy(P1_information_sets, avg_policy_obj_x, avg_policy_numerator_x, avg_policy_denominator_x, 'x');
    calc_average_policy(P2_information_sets, avg_policy_obj_o, avg_policy_numerator_o, avg_policy_denominator_o, 'o');

    std::string output_policy_file_x = base_path + "/average" + "/P1_iteration_" + std::to_string(end_iter) + "_average_cfr_policy_cpp_off_policy_average.json";
    std::string output_policy_file_o = base_path + "/average" + "/P2_iteration_" + std::to_string(end_iter) + "_average_cfr_policy_cpp_off_policy_average.json";
    std::string output_regret_file_x = base_path + "/regret/P1_iteration_" + std::to_string(end_iter) + "_regret_cpp.json";
    std::string output_regret_file_o = base_path + "/regret/P2_iteration_" + std::to_string(end_iter) + "_regret_cpp.json";
    save_output(output_policy_file_x, output_regret_file_x, 'x', P1_information_sets, regret_list_x, avg_policy_obj_x);
    save_output(output_policy_file_o, output_regret_file_o, 'o', P2_information_sets, regret_list_o, avg_policy_obj_o);
    double expected_utility = get_expected_utility_wrapper(avg_policy_obj_x, avg_policy_obj_o);
    std::cout << "Expected utility avg: " << expected_utility << std::endl; 
}
