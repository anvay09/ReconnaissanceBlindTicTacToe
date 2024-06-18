for i in {1..90}
do
    python3 ./automated_calc_avg_policy.py --CurrentPlayer o --PolicyFileBase ./data/full_tree/average/P2_avg_policy_after_round_{}.json --NumRounds $i  --BasePath data/full_tree
    python3 ./automated_calc_avg_policy.py --CurrentPlayer x --PolicyFileBase ./data/full_tree/average/P1_avg_policy_after_round_{}.json --NumRounds $i --BasePath data/full_tree
done