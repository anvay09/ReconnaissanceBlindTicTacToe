for i in {1..5}
do
    echo "Starting Round $i..."
    ./automated_calc_avg o data/Iterative_1/average/P2_avg_policy_after_round_{}.json $i data/Iterative_1
    ./automated_calc_avg x data/Iterative_1/average/P1_avg_policy_after_round_{}.json $i data/Iterative_1
    echo "Finished round $i..."
done