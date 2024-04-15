// Compile: g++-13 -O3 openmptest.cpp -o testMP -fopenmp -I /Users/anvay/Downloads/boost_1_84_0

#include <vector>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <chrono>
#include <ctime>

int main () 
{
    int n=400000,  m=1000;  
    double x=0,y=0;
    double s=0;
    std::vector<double> shifts(n,0);

    auto start = std::chrono::system_clock::now();   

    #pragma omp parallel for 
    for (int j=0; j<n; j++) {

        double r=0.0;
        for (int i=0; i < m; i++){

            double rand_g1 = cos(i/double(m));
            double rand_g2 = sin(i/double(m));     

            x += rand_g1;
            y += rand_g2;
            r += sqrt(rand_g1*rand_g1 + rand_g2*rand_g2);
        }
        shifts[j] = r / m;
    }

    std::cout << *std::max_element( shifts.begin(), shifts.end() ) << std::endl;

    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end-start;
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);
 
    std::cout << "finished computation at " << std::ctime(&end_time)
              << "elapsed time: " << elapsed_seconds.count() << "s"
              << std::endl;
    return 0;
}