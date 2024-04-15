numiterations=2
basepath="./data/Iterative_2"

echo "python3 ./automated_cfr.py --CurrentPlayer o --PolicyFileX ./data/P1_deterministic_policy.json --PolicyFileO ./data/P2_deterministic_policy.json --Round 1 --ReachableISFlag 1 --FilterValidHistoriesFlag 0 --BasePath $basepath"
python3 ./automated_cfr.py --CurrentPlayer o --PolicyFileX ./data/P1_deterministic_policy.json --PolicyFileO ./data/P2_deterministic_policy.json --Round 1 --ReachableISFlag 1 --FilterValidHistoriesFlag 0 --BasePath $basepath
for i in $(eval echo {2..$numiterations})
do
  echo "python3 ./automated_cfr.py --CurrentPlayer o --PolicyFileX ./data/P1_deterministic_policy.json --PolicyFileO ./$basepath/cfr_policy/P2_cfr_policy_round_$(($i-1)).json --Round $i --ReachableISFlag 1 --FilterValidHistoriesFlag 0 --BasePath $basepath"
  python3 ./automated_cfr.py --CurrentPlayer o --PolicyFileX ./data/P1_deterministic_policy.json --PolicyFileO ./$basepath/cfr_policy/P2_cfr_policy_round_$(($i-1)).json --Round $i --ReachableISFlag 1 --FilterValidHistoriesFlag 0 --BasePath $basepath
done

echo "python3 ./automated_cfr.py --CurrentPlayer x --PolicyFileX ./data/P1_deterministic_policy.json --PolicyFileO ./$basepath/cfr_policy/P2_cfr_policy_round_$numiterations.json --Round 1 --ReachableISFlag 1 --FilterValidHistoriesFlag 0 --BasePath $basepath"
python3 ./automated_cfr.py --CurrentPlayer x --PolicyFileX ./data/P1_deterministic_policy.json --PolicyFileO ./$basepath/cfr_policy/P2_cfr_policy_round_$numiterations.json --Round 1 --ReachableISFlag 1 --FilterValidHistoriesFlag 0 --BasePath $basepath
for i in $(eval echo {2..$numiterations})
do
  echo "python3 ./automated_cfr.py --CurrentPlayer x --PolicyFileX ./$basepath/cfr_policy/P1_cfr_policy_round_$(($i-1)).json --PolicyFileO ./$basepath/cfr_policy/P2_cfr_policy_round_$numiterations.json --Round $i --ReachableISFlag 1 --FilterValidHistoriesFlag 0 --BasePath $basepath"
  python3 ./automated_cfr.py --CurrentPlayer x --PolicyFileX ./$basepath/cfr_policy/P1_cfr_policy_round_$(($i-1)).json --PolicyFileO ./$basepath/cfr_policy/P2_cfr_policy_round_$numiterations.json --Round $i --ReachableISFlag 1 --FilterValidHistoriesFlag 0 --BasePath $basepath
done


for i in {2..100}
do
  for j in $(eval echo {1..$numiterations})
  do
    echo "python3 ./automated_cfr.py --CurrentPlayer o --PolicyFileX ./$basepath/cfr_policy/P1_cfr_policy_round_$(($numiterations*i-$numiterations)).json --PolicyFileO ./$basepath/cfr_policy/P2_cfr_policy_round_$(($numiterations*i-$numiterations+$j-1)).json --Round $(($numiterations*i-$numiterations+$j)) --ReachableISFlag 1 --FilterValidHistoriesFlag 0 --BasePath $basepath"
    python3 ./automated_cfr.py --CurrentPlayer o --PolicyFileX ./$basepath/cfr_policy/P1_cfr_policy_round_$(($numiterations*i-$numiterations)).json --PolicyFileO ./$basepath/cfr_policy/P2_cfr_policy_round_$(($numiterations*i-$numiterations+$j-1)).json --Round $(($numiterations*i-$numiterations+$j)) --ReachableISFlag 1 --FilterValidHistoriesFlag 0 --BasePath $basepath
  done


  for j in $(eval echo {1..$numiterations})
  do
    echo "python3 ./automated_cfr.py --CurrentPlayer x --PolicyFileX ./$basepath/cfr_policy/P1_cfr_policy_round_$(($numiterations*i-$numiterations+$j-1)).json --PolicyFileO ./$basepath/cfr_policy/P2_cfr_policy_round_$(($numiterations*i)).json --Round $(($numiterations*i-$numiterations+$j)) --ReachableISFlag 1 --FilterValidHistoriesFlag 0 --BasePath $basepath"
    python3 ./automated_cfr.py --CurrentPlayer x --PolicyFileX ./$basepath/cfr_policy/P1_cfr_policy_round_$(($numiterations*i-$numiterations+$j-1)).json --PolicyFileO ./$basepath/cfr_policy/P2_cfr_policy_round_$(($numiterations*i)).json --Round $(($numiterations*i-$numiterations+$j)) --ReachableISFlag 1 --FilterValidHistoriesFlag 0 --BasePath $basepath
  done
done
