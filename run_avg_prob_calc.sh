python3 ./automated_avg_prob.py --CurrentPlayer o --PolicyFileX ./data/P1_DG_policy.json --PolicyFileO ./data/P2_uniform_policy.json --Round 1 --BasePath data/Iterative_1
python3 ./automated_avg_prob.py --CurrentPlayer x --PolicyFileX ./data/P1_DG_policy.json --PolicyFileO ./data_files/P2_cfr_policy_round_1.json --Round 1 --BasePath data/Iterative_1
for i in {2..100}
do
    python3 ./automated_avg_prob.py --CurrentPlayer o --PolicyFileX ./data/Iterative_1/cfr_policy/P1_cfr_policy_round_$(($i-1)).json --PolicyFileO ./data/Iterative_1/cfr_policy/P2_cfr_policy_round_$(($i-1)).json --Round $i --BasePath data/Iterative_1
    python3 ./automated_avg_prob.py --CurrentPlayer x --PolicyFileX ./data_files/P1_cfr_policy_round_$(($i-1)).json --PolicyFileO ./data_files/P2_cfr_policy_round_$i.json --Round $i --BasePath data/Iterative_1
done