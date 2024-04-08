python3 ./automated_avg_prob.py --CurrentPlayer o --PolicyFileX ./data/P1_deterministic_policy.json --PolicyFileO ./data/P2_deterministic_policy.json --Round 1 --BasePath data/full_tree
python3 ./automated_avg_prob.py --CurrentPlayer x --PolicyFileX ./data/P1_deterministic_policy.json --PolicyFileO ./data/P2_deterministic_policy.json --Round 1 --BasePath data/full_tree
for i in {2..250}
do
    python3 ./automated_avg_prob.py --CurrentPlayer o --PolicyFileX ./data/full_tree/cfr_policy/P1_iteration_$(($i-1))_cfr_policy.json --PolicyFileO ./data/full_tree/cfr_policy/P2_iteration_$(($i-1))_cfr_policy.json --Round $i --BasePath data/full_tree
    python3 ./automated_avg_prob.py --CurrentPlayer x --PolicyFileX ./data/full_tree/cfr_policy/P1_iteration_$(($i-1))_cfr_policy.json --PolicyFileO ./data/full_tree/cfr_policy/P2_iteration_$(($i-1))_cfr_policy.json --Round $i --BasePath data/full_tree
done
~   
