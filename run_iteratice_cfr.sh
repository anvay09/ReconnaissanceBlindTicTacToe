num_iterations = 2
base_file = "output_files_iterative_2"

python3 ./automated_cfr.py --CurrentPlayer o --PolicyFileX ./$base_file/P1_deterministic_policy.json --PolicyFileO ./$(base_file)/P2_deterministic_policy.json --Round 1 --ReachableISFlag 1 --FilterValidHistoriesFlag 1 --BasePath $(base_file)
for i in {2.. $(num_iterations)}
do
  python3 ./automated_cfr.py --CurrentPlayer o --PolicyFileX ./$base_file/P1_deterministic_policy.json --PolicyFileO ./$base_file/P2_cfr_policy_round_$($i-1).json --Round $i --ReachableISFlag 1 --FilterValidHistoriesFlag 1 --BasePath $base_file
done

python3 ./automated_cfr.py --CurrentPlayer x --PolicyFileX ./$base_file/P1_deterministic_policy.json --PolicyFileO ./$base_file/P2_cfr_policy_round_$num_iterations.json --Round 1 --ReachableISFlag 1 --FilterValidHistoriesFlag 1 --BasePath $base_file
for i in {2..$num_iterations}
do
  python3 ./automated_cfr.py --CurrentPlayer o --PolicyFileX ./$base_file/P1_cfr_policy_policy_round_$($i-1).json --PolicyFileO ./$base_file/P2_cfr_policy_round_$num_iterations.json --Round $i --ReachableISFlag 1 --FilterValidHistoriesFlag 1 --BasePath $base_file
done


for i in {2..100}
do
  for j in {1..$num_iterations}
  do
    python3 ./automated_cfr.py --CurrentPlayer o --PolicyFileX ./$base_file/P1_cfr_policy_round_ $(($num_iterations*i-$num_iterations)).json --PolicyFileO ./$base_file/P2_cfr_policy_round_ $(($num_iterations*i-$num_iterations+$j-1)).json --Round $(($num_iterations*i-$num_iterations+$j)) --ReachableISFlag 1 --FilterValidHistoriesFlag 1 --BasePath $base_file
  done

  for j in {1..$num_iterations}
  do
    python3 ./automated_cfr.py --CurrentPlayer x --PolicyFileX ./$base_file/P1_cfr_policy_round_ $(($num_iterations*i-$num_iterations+$j-1)).json --PolicyFileO ./$base_file/P2_cfr_policy_round_ $(($num_iterations*i)).json --Round $(($num_iterations*i-$num_iterations+$j)) --ReachableISFlag 1 --FilterValidHistoriesFlag 1 --BasePath $base_file
  done
done
