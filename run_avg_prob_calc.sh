for i in {2..100}
do
    python3 ./automated_avg_prob.py --CurrentPlayer o --PolicyFileX ./data_files/P1_cfr_policy_round_$(($i-1)).json --PolicyFileO ./data_files/P2_cfr_policy_round_$(($i-1)).json --Round $i
    python3 ./automated_avg_prob.py --CurrentPlayer x --PolicyFileX ./data_files/P1_cfr_policy_round_$(($i-1)).json --PolicyFileO ./data_files/P2_cfr_policy_round_$i.json --Round $i
done
