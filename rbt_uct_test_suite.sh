#!/bin/bash

g++-13 uct_br.cpp rbt_classes.cpp rbt_utilities.cpp -O3 -o rbt_uct

num_iterations=1000000
log_size=10000
num_experiments=20
p1_nash_policy="P1_nash_normalised.txt"
p2_nash_policy="P2_nash_normalised.txt"
p1_start_policy="data/P1_start_policy.txt"
p2_start_policy="data/P2_start_policy.txt"
p1_balanced_policy="P1_balanced_exploration_policy.txt"
p2_balanced_policy="P2_balanced_exploration_policy.txt"

####################### Run the UCT
echo "Running UCT"
for player in x o
do
    echo "Player: $player"
    for C in 1 5 10 50 100 200 500 1000 2000 5000
    do 
        echo "C: $C"
        base_path="data/uct/nash_policy/C_$C"
        echo "base_path: $base_path"
        # Create the base directory if it doesn't already exist
        mkdir -p "$base_path"
        echo "./rbt_uct $p1_nash_policy $p2_nash_policy 96 $num_iterations $player $num_experiments $log_size $C $base_path"
        ./rbt_uct "$p1_nash_policy" "$p2_nash_policy" 96 "$num_iterations" "$player" "$num_experiments" "$log_size" "$C" "$base_path"
        base_path="data/uct/start_policy/C_$C"
        echo "base_path: $base_path"
        # Create the base directory if it doesn't already exist
        mkdir -p "$base_path"
        echo "./rbt_uct $p1_start_policy $p2_start_policy 96 $num_iterations $player $num_experiments $log_size $C $base_path"
        ./rbt_uct "$p1_start_policy" "$p2_start_policy" 96" $num_iterations" "$player" "$num_experiments" "$log_size" "$C" "$base_path"
    done
done