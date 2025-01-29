#!/bin/bash

g++-13 onpath_flipping_br.cpp rbt_classes.cpp rbt_utilities.cpp -O3 -o rbt_onpath
g++-13 upfront_flipping_br.cpp rbt_classes.cpp rbt_utilities.cpp -O3 -o rbt_upfront

num_iterations=1000000
log_size=10000
num_experiments=20
p1_nash_policy="P1_nash_normalised.txt"
p2_nash_policy="P2_nash_normalised.txt"
p1_start_policy="data/P1_start_policy.txt"
p2_start_policy="data/P2_start_policy.txt"
p1_balanced_policy="P1_balanced_exploration_policy.txt"
p2_balanced_policy="P2_balanced_exploration_policy.txt"


####################### Run the on-path flipping
echo "Running on-path flipping"
for player in x o
do
    echo "Player: $player"
    for k in  0.01 0.1 1 5 10
    do 
        echo "k: $k"
        base_path="data/onpath/nash_policy/k_$k"
        echo "base_path: $base_path"
        # Create the base directory if it doesn't already exist
        mkdir -p "$base_path"
        echo "./rbt_onpath $p1_nash_policy $p2_nash_policy 96 $p1_balanced_policy $p2_balanced_policy 1 $num_iterations $k $player $num_experiments $log_size $base_path"
        ./rbt_onpath "$p1_nash_policy" "$p2_nash_policy" 96 "$p1_balanced_policy" "$p2_balanced_policy" 1 "$num_iterations" "$k" "$player" "$num_experiments" "$log_size" "$base_path"
        base_path="data/onpath/start_policy/k_$k"
        echo "base_path: $base_path"
        # Create the base directory if it doesn't already exist
        mkdir -p "$base_path"
        echo "./rbt_onpath $p1_start_policy $p2_start_policy 96 $p1_balanced_policy $p2_balanced_policy 1 $num_iterations $k $player $num_experiments $log_size $base_path"
        ./rbt_onpath "$p1_start_policy" "$p2_start_policy" 96 "$p1_balanced_policy" "$p2_balanced_policy" 1 "$num_iterations" "$k" "$player" "$num_experiments" "$log_size" "$base_path"
    done
done

####################### Run the upfront flipping
echo "Running upfront flipping"
for player in x o
do
    echo "Player: $player"
    for k in 0.01 0.1 1 5 10
    do 
        echo "k: $k"
        base_path="data/upfront/nash_policy/k_$k"
        echo "base_path: $base_path"
        # Create the base directory if it doesn't already exist
        mkdir -p "$base_path"
        echo "./rbt_upfront $p1_nash_policy $p2_nash_policy 96 $p1_balanced_policy $p2_balanced_policy 1 $num_iterations $player $num_experiments $log_size $k $base_path"
        ./rbt_upfront "$p1_nash_policy" "$p2_nash_policy" 96 "$p1_balanced_policy" "$p2_balanced_policy" 1 "$num_iterations" "$player" "$num_experiments" "$log_size" "$k" "$base_path"
        base_path="data/upfront/start_policy/k_$k"
        echo "base_path: $base_path"
        # Create the base directory if it doesn't already exist
        mkdir -p "$base_path"
        ./rbt_upfront "$p1_start_policy" "$p2_start_policy" 96 "$p1_balanced_policy" "$p2_balanced_policy" 1 "$num_iterations" "$player" "$num_experiments" "$log_size" "$k" "$base_path"
    done
done
