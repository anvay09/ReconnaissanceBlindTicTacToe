#!/bin/bash

# Compile the poker test suite
g++-13 bandit_poker.cpp poker_classes.cpp poker_utilities.cpp -O3 -o bandit_p
g++-13 mccfr_outcome_sampling_br_poker.cpp poker_classes.cpp poker_utilities.cpp -O3 -o mccfr_p
g++-13 onpath_flipping_br_poker.cpp poker_classes.cpp poker_utilities.cpp -O3 -o onpath_p
g++-13 uct_smooth_br_poker.cpp poker_classes.cpp poker_utilities.cpp -o uct_smooth_poker -O3
g++-13 uct_br_poker.cpp poker_classes.cpp poker_utilities.cpp -o uct_poker -O3
g++-13 poker_exact_LUCB_update_all.cpp poker_classes.cpp poker_utilities.cpp -O3 -o poker_exact_LUCB_a
g++-13 poker_exact_LUCB.cpp poker_classes.cpp poker_utilities.cpp -O3 -o poker_exact_LUCB

# Run the poker test suite for Kuhn Poker

./bandit_p data/P1_nash_Kuhn_Poker.txt data/P2_nash_Kuhn_Poker.txt K x 1000 10 100 16 final_exp LUCB 1
./bandit_p data/P1_nash_Kuhn_Poker.txt data/P2_nash_Kuhn_Poker.txt K o 1000 10 100 16 final_exp LUCB 1
./mccfr_p data/P1_nash_Kuhn_Poker.txt data/P2_nash_Kuhn_Poker.txt K x 1000 10 100 0.1 1  
./mccfr_p data/P1_nash_Kuhn_Poker.txt data/P2_nash_Kuhn_Poker.txt K o 1000 10 100 0.1 1  
./onpath_p data/P1_nash_Kuhn_Poker.txt data/P2_nash_Kuhn_Poker.txt 1 1 K P1_balanced_exploration_policy_K.txt P2_balanced_exploration_policy_K.txt 0 final_exp
./uct_smooth_poker data/P1_nash_Kuhn_Poker.txt data/P2_nash_Kuhn_Poker.txt K 1000 10 100 x 0.5 0.9 0 0.1 1
./uct_smooth_poker data/P1_nash_Kuhn_Poker.txt data/P2_nash_Kuhn_Poker.txt K 1000 10 100 o 0.5 0.9 0 0.1 1
./uct_poker data/P1_nash_Kuhn_Poker.txt data/P2_nash_Kuhn_Poker.txt K
./poker_exact_LUCB data/P1_nash_Kuhn_Poker.txt data/P2_nash_Kuhn_Poker.txt 0.01 0.01 x 100 10 final_exp
./poker_exact_LUCB data/P1_nash_Kuhn_Poker.txt data/P2_nash_Kuhn_Poker.txt 0.01 0.01 o 100 10 final_exp
./poker_exact_LUCB_a data/P1_nash_Kuhn_Poker.txt data/P2_nash_Kuhn_Poker.txt 0.01 0.01 x 100 10 final_exp
./poker_exact_LUCB_a data/P1_nash_Kuhn_Poker.txt data/P2_nash_Kuhn_Poker.txt 0.01 0.01 o 100 10 final_exp
