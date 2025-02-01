#!/bin/bash

g++-13 uct_smooth_br.cpp rbt_classes.cpp rbt_utilities.cpp -O3 -o rbt_uct_smooth

num_iterations=1000000
log_size=10000
num_experiments=3
p1_nash_policy="P1_nash_normalised.txt"
p2_nash_policy="P2_nash_normalised.txt"
p1_start_policy="data/P1_start_policy.txt"
p2_start_policy="data/P2_start_policy.txt"
p1_balanced_policy="P1_balanced_exploration_policy.txt"
p2_balanced_policy="P2_balanced_exploration_policy.txt"

####################### Run the UCT smooth
echo "Running UCT smooth"
for player in x o
do
    echo "Player: $player"
    for C in 1 5 10 25
    do 
        for eps in 0.1
        do
            for n0 in 0.9
            do
                for d in 0.0001 0.001 0.01
                do
                    echo "C: $C"
                    base_path="data/uct_smooth/nash_policy/C_$C/eps_$eps/n0_$n0/d_$d"
                    echo "base_path: $base_path"
                    # Create the base directory if it doesn't already exist
                    mkdir -p "$base_path"
                    echo "./rbt_uct_smooth $p1_nash_policy $p2_nash_policy 96 $num_iterations $player $num_experiments $log_size $C $n0 $d $eps $base_path"
                    ./rbt_uct_smooth "$p1_nash_policy" "$p2_nash_policy" 96 "$num_iterations" "$player" "$num_experiments" "$log_size" "$C" "$n0" "$d" "$eps" "$base_path"
                    base_path="data/uct_smooth/start_policy/C_$C/eps_$eps/n0_$n0/d_$d"
                    echo "base_path: $base_path"
                    # Create the base directory if it doesn't already exist
                    mkdir -p "$base_path"
                    echo "./rbt_uct_smooth $p1_start_policy $p2_start_policy 96 $num_iterations $player $num_experiments $log_size $C $n0 $d $eps $base_path"
                    ./rbt_uct_smooth "$p1_start_policy" "$p2_start_policy" 96 "$num_iterations" "$player" "$num_experiments" "$log_size" "$C" "$n0" "$d" "$eps" "$base_path"
                done
            done
        done
    done
done