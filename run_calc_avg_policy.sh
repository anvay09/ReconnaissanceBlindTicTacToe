for i in {1..30}
do
    python3 ./automated_calc_avg_policy.py --CurrentPlayer o --PolicyFileBase /data/Iterative_1/average/P2_avg_policy_after_round_{}.json --NumRounds $i  --BasePath data/Iterative_1
    python3 ./automated_calc_avg_policy.py --CurrentPlayer x --PolicyFileBase data/Iterative_1/average/P1_avg_policy_after_round_{}.json --NumRounds $i --BasePath data/Iterative_1
done