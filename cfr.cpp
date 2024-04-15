// Compile
// mpic++ -std=c++17 -o mpi-test cfr.cpp rbt_utilities.cpp rbt_classes.cpp -O3 -I /Users/anvay/Downloads/boost_1_84_0 -L /Users/anvay/Downloads/boost_1_84_0/stage/lib -lboost_mpi -lboost_serialization
// Run
// mpirun -np 8 ./mpi-test

#include <boost/mpi/environment.hpp>
#include <boost/mpi/communicator.hpp>
#include <iostream>
#include "rbt_classes.hpp"
#include "rbt_utilities.hpp"
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
}

