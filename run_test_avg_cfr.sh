for i in {2..100}
do
    python3 ./temp_automated_cfr_ram_test.py --CurrentPlayer o --PolicyFileX ./data_files/P1_cfr_policy_round_$(($i-1)).json --PolicyFileO ./data_files/P2_cfr_policy_round_$(($i-1)).json --Round $i
    python3 ./temp_automated_cfr_ram_test.py --CurrentPlayer x --PolicyFileX ./data_files/P1_cfr_policy_round_$(($i-1)).json --PolicyFileO ./data_files/P2_cfr_policy_round_$i.json --Round $i
done
