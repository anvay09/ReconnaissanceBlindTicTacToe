#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <numeric>

// Function to sample from Dirichlet distribution
std::vector<double> sample_dirichlet(const std::vector<double>& alpha, std::mt19937& gen) {
    std::vector<double> gamma_samples(alpha.size());
    
    // Sample from Gamma distribution
    for (size_t i = 0; i < alpha.size(); ++i) {
        std::gamma_distribution<double> gamma_dist(alpha[i], 1.0);
        gamma_samples[i] = gamma_dist(gen);
    }

    // Normalize
    double sum = std::accumulate(gamma_samples.begin(), gamma_samples.end(), 0.0);
    for (double& value : gamma_samples) {
        value /= sum;
    }

    return gamma_samples;
}

int main(int argc, char* argv[]) {
    double a1 = std::stod(argv[1]);
    double a2 = std::stod(argv[2]);
    double a3 = std::stod(argv[3]);
    std::vector<double> alpha = {a1, a2, a3};  // Dirichlet parameters
    int num_samples = 5000;
    
    std::random_device rd;
    std::mt19937 gen(rd());  // Mersenne Twister RNG
    
    std::ofstream outfile("samples.txt");
    if (!outfile) {
        std::cerr << "Error opening file for writing!" << std::endl;
        return 1;
    }

    for (int i = 0; i < num_samples; ++i) {
        std::vector<double> sample = sample_dirichlet(alpha, gen);
        outfile << sample[0] << " " << sample[1] << " " << sample[2] << "\n";
    }

    outfile.close();
    std::cout << "Generated " << num_samples << " samples and saved to samples.txt" << std::endl;

    return 0;
}
