// Compile
// mpic++ -std=c++17 -o mpi-test cfr.cpp rbt_utilities.cpp rbt_classes.cpp -O3 -I /Users/anvay/Downloads/boost_1_84_0 -L /Users/anvay/Downloads/boost_1_84_0/stage/lib -lboost_mpi -lboost_serialization
// Run
// mpirun -np 8 ./mpi-test

#include <boost/mpi/environment.hpp>
#include <boost/mpi/communicator.hpp>
#include <iostream>
#include "cpp_headers/rbt_classes.hpp"
#include "cpp_headers/rbt_utilities.hpp"
namespace mpi = boost::mpi;

int main(int argc, char* argv[])
{
    mpi::environment env(argc, argv);
    mpi::communicator world;
    int procid, numprocs;

    MPI_Comm_rank(world, &procid);
    MPI_Comm_size(world, &numprocs);

    int N = 24;
    
    unsigned int partition = N / numprocs;
    
    for ( int i = 0; i <= N; i++ ) {
        if (i% numprocs != procid) continue;

        std::cout << "Rank " << procid << " is processing " << i << std::endl;
    }

    std::string policy_file_x = "data/P1_deterministic_policy.json";
    std::string policy_file_o = "data/P2_deterministic_policy.json";
    std::string P1_information_sets_file = "data/P1_information_sets.txt";
    std::string P2_information_sets_file = "data/P2_information_sets.txt";
    std::vector<std::string> P1_information_sets;
    std::vector<std::string> P2_information_sets;

    std::ifstream f1(P1_information_sets_file);
    std::string line;
    while (std::getline(f1, line)) {
        P1_information_sets.push_back(line);
    }
    f1.close();

    std::ifstream f2(P2_information_sets_file);
    while (std::getline(f2, line)) {
        P2_information_sets.push_back(line);
    }
    f2.close();

    for (int T = 1; T <= 1000; T++) {
        continue;
    }

    return 0;
}

