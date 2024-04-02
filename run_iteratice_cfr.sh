numiterations=2
basefile="output_files_iterative_2"
echo "python3 ./automated_cfr.py --CurrentPlayer o --PolicyFileX ./$basefile/P1_deterministic_policy.json --PolicyFileO ./$basefile/P2_deterministic_policy.json --Round 1 --ReachableISFlag 1 --FilterValidHistoriesFlag 1 --BasePath $basefile"
python3 ./automated_cfr.py --CurrentPlayer o --PolicyFileX ./$basefile/P1_deterministic_policy.json --PolicyFileO ./$basefile/P2_deterministic_policy.json --Round 1 --ReachableISFlag 1 --FilterValidHistoriesFlag 1 --BasePath $basefile
for i in $(eval echo {2..$numiterations})
do
  echo "python3 ./automated_cfr.py --CurrentPlayer o --PolicyFileX ./$basefile/P1_deterministic_policy.json --PolicyFileO ./$basefile/P2_cfr_policy_round_$(($i-1)).json --Round $i --ReachableISFlag 1 --FilterValidHistoriesFlag 1 --BasePath $basefile"
  python3 ./automated_cfr.py --CurrentPlayer o --PolicyFileX ./$basefile/P1_deterministic_policy.json --PolicyFileO ./$basefile/P2_cfr_policy_round_$(($i-1)).json --Round $i --ReachableISFlag 1 --FilterValidHistoriesFlag 1 --BasePath $basefile
done

echo "python3 ./automated_cfr.py --CurrentPlayer x --PolicyFileX ./$basefile/P1_deterministic_policy.json --PolicyFileO ./$basefile/P2_cfr_policy_round_$numiterations.json --Round 1 --ReachableISFlag 1 --FilterValidHistoriesFlag 1 --BasePath $basefile"
python3 ./automated_cfr.py --CurrentPlayer x --PolicyFileX ./$basefile/P1_deterministic_policy.json --PolicyFileO ./$basefile/P2_cfr_policy_round_$numiterations.json --Round 1 --ReachableISFlag 1 --FilterValidHistoriesFlag 1 --BasePath $basefile
for i in $(eval echo {2..$numiterations})
do
  echo "python3 ./automated_cfr.py --CurrentPlayer x --PolicyFileX ./$basefile/P1_cfr_policy_round_$(($i-1)).json --PolicyFileO ./$basefile/P2_cfr_policy_round_$numiterations.json --Round $i --ReachableISFlag 1 --FilterValidHistoriesFlag 1 --BasePath $basefile"
  python3 ./automated_cfr.py --CurrentPlayer x --PolicyFileX ./$basefile/P1_cfr_policy_round_$(($i-1)).json --PolicyFileO ./$basefile/P2_cfr_policy_round_$numiterations.json --Round $i --ReachableISFlag 1 --FilterValidHistoriesFlag 1 --BasePath $basefile
done


for i in {2..100}
do
  for j in $(eval echo {1..$numiterations})
  do
    echo "python3 ./automated_cfr.py --CurrentPlayer o --PolicyFileX ./$basefile/P1_cfr_policy_round_$(($numiterations*i-$numiterations)).json --PolicyFileO ./$basefile/P2_cfr_policy_round_$(($numiterations*i-$numiterations+$j-1)).json --Round $(($numiterations*i-$numiterations+$j)) --ReachableISFlag 1 --FilterValidHistoriesFlag 1 --BasePath $basefile"
    python3 ./automated_cfr.py --CurrentPlayer o --PolicyFileX ./$basefile/P1_cfr_policy_round_$(($numiterations*i-$numiterations)).json --PolicyFileO ./$basefile/P2_cfr_policy_round_$(($numiterations*i-$numiterations+$j-1)).json --Round $(($numiterations*i-$numiterations+$j)) --ReachableISFlag 1 --FilterValidHistoriesFlag 1 --BasePath $basefile
  done

  for j in $(eval echo {1..$numiterations})
  do
    echo "python3 ./automated_cfr.py --CurrentPlayer x --PolicyFileX ./$basefile/P1_cfr_policy_round_$(($numiterations*i-$numiterations+$j-1)).json --PolicyFileO ./$basefile/P2_cfr_policy_round_$(($numiterations*i)).json --Round $(($numiterations*i-$numiterations+$j)) --ReachableISFlag 1 --FilterValidHistoriesFlag 1 --BasePath $basefile"
    python3 ./automated_cfr.py --CurrentPlayer x --PolicyFileX ./$basefile/P1_cfr_policy_round_$(($numiterations*i-$numiterations+$j-1)).json --PolicyFileO ./$basefile/P2_cfr_policy_round_$(($numiterations*i)).json --Round $(($numiterations*i-$numiterations+$j)) --ReachableISFlag 1 --FilterValidHistoriesFlag 1 --BasePath $basefile
  done
done
