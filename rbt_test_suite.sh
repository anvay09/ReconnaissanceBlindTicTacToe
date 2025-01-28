#!/bin/bash

# Compile the rbt test suite
g++-13 mccfr_outcome_sampling_br.cpp rbt_classes.cpp rbt_utilities.cpp -O3 -o rbt_mccfr
g++-13 mccfr_outcome_sampling_eps_greedy_br.cpp rbt_classes.cpp rbt_utilities.cpp -O3 -o rbt_mccfr_greedy
g++-13 onpath_flipping_br.cpp rbt_classes.cpp rbt_utilities.cpp -O3 -o rbt_onpath
g++-13 upfront_flipping_br.cpp rbt_classes.cpp rbt_utilities.cpp -O3 -o rbt_upfront
g++-13 uct_smooth_br.cpp rbt_classes.cpp rbt_utilities.cpp -O3 -o rbt_uct_smooth
g++-13 uct_br.cpp rbt_classes.cpp rbt_utilities.cpp -O3 -o rbt_uct

num_iterations=10
log_size=3
num_experiments=1
p1_nash_policy = "P1_nash_normalised.txt"
p2_nash_policy = "P2_nash_normalised.txt"
p1_start_policy = "P1_start_policy.txt"
p2_start_policy = "P2_start_policy.txt"
p1_balanced_policy =  "P1_balanced_exploration_policy.txt"
p2_balanced_policy =  "P2_balanced_exploration_policy.txt"

####################### (1) Run the MCCFR
echo "Running MCCFR"
for player in x o; 
do
    echo "Player: $player"
    for eps in $(seq 0.02 0.02 0.2);
    do 
        echo "eps: $eps"
        base_path="data/mccfr/nash_policy/eps_$eps"
        echo "base_path: $base_path"
        # Create the base directory if it doesn't already exist
        mkdir -p "$base_path"
        ./rbt_mccfr $p1_nash_policy $p2_nash_policy 96 $num_iterations $player $num_experiments $log_size $eps $base_path
        base_path="data/mccfr/start_policy/eps_$eps"
        echo "base_path: $base_path"
        # Create the base directory if it doesn't already exist
        mkdir -p "$base_path"
        ./rbt_mccfr $p1_start_policy $p2_start_policy 96 $num_iterations $player $num_experiments $log_size $eps $base_path
    
    done
done

####################### (2) Run the MCCFR with epsilon-greedy exploration
echo "Running MCCFR with epsilon-greedy exploration"
for player in x o; 
do
    echo "Player: $player"
    for eps in $(seq 0.02 0.02 0.2);
    do 
        echo "eps: $eps"
        base_path="data/mccfr_greedy/nash_policy/eps_$eps"
        echo "base_path: $base_path"
        # Create the base directory if it doesn't already exist
        mkdir -p "$base_path"
        ./rbt_mccfr $p1_nash_policy $p2_nash_policy 96 $num_iterations $player $num_experiments $log_size $eps $base_path
        base_path="data/mccfr_greedy/start_policy/eps_$eps"
        echo "base_path: $base_path"
        # Create the base directory if it doesn't already exist
        mkdir -p "$base_path"
        ./rbt_mccfr $p1_start_policy $p2_start_policy 96 $num_iterations $player $num_experiments $log_size $eps $base_path
    
    done
done

####################### (3) Run the on-path flipping
echo "Running on-path flipping"
for player in x o;
do
    echo "Player: $player"
    for k in 0.1 1 5 10 50 100 200 500 1000 5000;
    do 
        echo "k: $k"
        base_path="data/onpath/nash_policy/k_$k"
        echo "base_path: $base_path"
        # Create the base directory if it doesn't already exist
        mkdir -p "$base_path"
        ./rbt_onpath $p1_nash_policy $p2_nash_policy 96 $p1_balanced_policy $p2_balanced_policy 1 $num_iterations $k $player $num_experiments $log_size $base_path
        base_path="data/onpath/start_policy/k_$k"
        echo "base_path: $base_path"
        # Create the base directory if it doesn't already exist
        mkdir -p "$base_path"
        ./rbt_onpath $p1_start_policy $p2_start_policy 96 $p1_balanced_policy $p2_balanced_policy 1 $num_iterations $k $player $num_experiments $log_size $base_path
    done
done

####################### (4) Run the upfront flipping
echo "Running upfront flipping"
for player in x o;
do
    echo "Player: $player"
    for k in 0.1 1 5 10 50 100 200 500 1000 5000;
    do 
        echo "k: $k"
        base_path="data/upfront/nash_policy/k_$k"
        echo "base_path: $base_path"
        # Create the base directory if it doesn't already exist
        mkdir -p "$base_path"
        ./rbt_onpath $p1_nash_policy $p2_nash_policy 96 $p1_balanced_policy $p2_balanced_policy 1 $num_iterations $player $num_experiments $log_size $k $base_path
        base_path="data/upfront/start_policy/k_$k"
        echo "base_path: $base_path"
        # Create the base directory if it doesn't already exist
        mkdir -p "$base_path"
        ./rbt_onpath $p1_start_policy $p2_start_policy 96 $p1_balanced_policy $p2_balanced_policy 1 $num_iterations $player $num_experiments $log_size $k $base_path
    done
done


####################### (5) Run the UCT
echo "Running UCT"
for player in x o;
do
    echo "Player: $player"
    for C in 1 2 5 10 50 100 200 500 1000 5000;
    do 
        echo "C: $C"
        base_path="data/uct/nash_policy/C_$C"
        echo "base_path: $base_path"
        # Create the base directory if it doesn't already exist
        mkdir -p "$base_path"
        ./rbt_uct $p1_nash_policy $p2_nash_policy 96 $num_iterations $player $num_experiments $log_size $C $base_path
        base_path="data/uct/start_policy/C_$C"
        echo "base_path: $base_path"
        # Create the base directory if it doesn't already exist
        mkdir -p "$base_path"
        ./rbt_uct $p1_start_policy $p2_start_policy 96 $num_iterations $player $num_experiments $log_size $C $base_path
    done
done


####################### (6) Run the UCT smooth
echo "Running UCT smooth"
for player in x o;
do
    echo "Player: $player"
    for C in 1 2 5 10 50 100 200 500 1000 5000;
    do 
        for eps in $(seq 0.02 0.02 0.2);
        do
            for n0 in $(seq 0.8 0.02 1);
            do
                for d in 0.0001 0.0005 0.001 0.005 0.01 0.05;
                do
                    echo "C: $C"
                    base_path="data/uct_smooth/nash_policy/C_$C/eps_$eps/n0_$n0/d_$d"
                    echo "base_path: $base_path"
                    # Create the base directory if it doesn't already exist
                    mkdir -p "$base_path"
                    ./rbt_uct $p1_nash_policy $p2_nash_policy 96 $num_iterations $player $num_experiments $log_size $C $n0 $d $eps $base_path
                    base_path="data/uct_smooth/start_policy/C_$C/eps_$eps/n0_$n0/d_$d"
                    echo "base_path: $base_path"
                    # Create the base directory if it doesn't already exist
                    mkdir -p "$base_path"
                    ./rbt_uct $p1_start_policy $p2_start_policy 96 $num_iterations $player $num_experiments $log_size $C $C $n0 $d $eps $base_path
                done
            done
        done
    done
done