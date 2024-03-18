for i in {61..100}
do
    python3 ./automated_cfr.py --CurrentPlayer o --PolicyFileX ./data_files/P1_cfr_policy_round_$(($i-1)).json --PolicyFileO ./data_files/P2_cfr_policy_round_$(($i-1)).json --Round $i --ReachableISFlag 1 --FilterValidHistoriesFlag 0
    python3 ./automated_cfr.py --CurrentPlayer x --PolicyFileX ./data_files/P1_cfr_policy_round_$(($i-1)).json --PolicyFileO ./data_files/P2_cfr_policy_round_$i.json --Round $i --ReachableISFlag 1 --FilterValidHistoriesFlag 0
done
