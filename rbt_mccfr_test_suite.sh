#!/bin/bash

g++-13 mccfr_outcome_sampling_br.cpp rbt_classes.cpp rbt_utilities.cpp -O3 -o rbt_mccfr
g++-13 mccfr_outcome_sampling_eps_greedy_br.cpp rbt_classes.cpp rbt_utilities.cpp -O3 -o rbt_mccfr_greedy

num_iterations=1000000
log_size=10000
num_experiments=20
p1_nash_policy="P1_nash_normalised.txt"
p2_nash_policy="P2_nash_normalised.txt"
p1_start_policy="data/P1_start_policy.txt"
p2_start_policy="data/P2_start_policy.txt"
p1_balanced_policy="P1_balanced_exploration_policy.txt"
p2_balanced_policy="P2_balanced_exploration_policy.txt"

####################### Run the MCCFR
echo "Running MCCFR"
for player in x o
do
    echo "Player: $player"
    for eps in $(seq 0.05 0.05 0.2)
    do 
        echo "eps: $eps"
        base_path="data/mccfr/nash_policy/eps_$eps"
        echo "base_path: $base_path"
        # Create the base directory if it doesn't already exist
        mkdir -p "$base_path"
        echo "./rbt_mccfr $p1_nash_policy $p2_nash_policy 96 $num_iterations $player $num_experiments $log_size $eps $base_path"
        ./rbt_mccfr "$p1_nash_policy" "$p2_nash_policy" 96 "$num_iterations" "$player" "$num_experiments" "$log_size" "$eps" "$base_path"
        base_path="data/mccfr/start_policy/eps_$eps"
        echo "base_path: $base_path"
        # Create the base directory if it doesn't already exist
        mkdir -p "$base_path"
        echo "./rbt_mccfr $p1_start_policy $p2_start_policy 96 $num_iterations $player $num_experiments $log_size $eps $base_path"
        ./rbt_mccfr "$p1_start_policy" "$p2_start_policy" 96 "$num_iterations" "$player" "$num_experiments" "$log_size" "$eps" "$base_path"
    done
done

####################### Run the MCCFR with epsilon-greedy exploration
echo "Running MCCFR with epsilon-greedy exploration"
for player in x o 
do
    echo "Player: $player"
    for eps in $(seq 0.05 0.05 0.2)
    do 
        echo "eps: $eps"
        base_path="data/mccfr_greedy/nash_policy/eps_$eps"
        echo "base_path: $base_path"
        # Create the base directory if it doesn't already exist
        mkdir -p "$base_path"
        echo "./rbt_mccfr_greedy $p1_nash_policy $p2_nash_policy 96 $num_iterations $player $num_experiments $log_size $eps $base_path"
        ./rbt_mccfr_greedy "$p1_nash_policy" "$p2_nash_policy" 96 "$num_iterations" "$player" "$num_experiments" "$log_size" "$eps" "$base_path"
        base_path="data/mccfr_greedy/start_policy/eps_$eps"
        echo "base_path: $base_path"
        # Create the base directory if it doesn't already exist
        mkdir -p "$base_path"
        echo "./rbt_mccfr_greedy $p1_start_policy $p2_start_policy 96 $num_iterations $player $num_experiments $log_size $eps $base_path"
        ./rbt_mccfr_greedy "$p1_start_policy" "$p2_start_policy" 96 "$num_iterations" "$player" "$num_experiments" "$log_size" "$eps" "$base_path"
    done
done