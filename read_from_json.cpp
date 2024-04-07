#include "rbt_classes.h"
#include <chrono>
#include <ctime>
#include "json.hpp"
#include <fstream>

using json = nlohmann::json;

int main() {
    ifstream i("data/P1_DG_policy.json");
    json policy_obj_x;
    i >> policy_obj_x;
    cout << policy_obj_x["xxoxxooo0m"]["8"] << endl;
    return 0;
}