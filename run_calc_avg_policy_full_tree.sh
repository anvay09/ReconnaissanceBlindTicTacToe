for i in {155..265}
do
    echo "Starting Round $i..."
    ./automated_calc_avg o data/full_tree/average/P2_avg_policy_after_round_{}.json $i data/full_tree
    ./automated_calc_avg x data/full_tree/average/P1_avg_policy_after_round_{}.json $i data/full_tree
    echo "Finished round $i..."
done