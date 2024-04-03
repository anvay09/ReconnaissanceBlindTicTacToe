for i in {1..30}
do
    python3 ./calc_avg_policy.py --CurrentPlayer o --PolicyFileBase data_files_avg/P2_avg_policy_after_round_ --NumRounds $i --ISetFile data_files/P2_information_sets.txt
    python3 ./calc_avg_policy.py --CurrentPlayer x --PolicyFileBase data_files_avg/P1_avg_policy_after_round_ --NumRounds $i --ISetFile data_files/P1_information_sets.txt
done