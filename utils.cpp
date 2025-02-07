#include "cpp_headers/poker_classes.hpp"
#include "cpp_headers/poker_utilities.hpp"

int main(int argc, char* argv[]) {
    std::cout.precision(6);
    // test kl upper bound
    double kl_upper = kl_upper_bound(0.4615, 1, 27.34072137465, 1e-2, false);
    double kl_lower = kl_upper_bound(0.4615, 1, 27.34072137465, 1e-2, true);
    std::cout << "KL Upper Bound: " << kl_upper << " KL Lower Bound: " << kl_lower << std::endl;
}