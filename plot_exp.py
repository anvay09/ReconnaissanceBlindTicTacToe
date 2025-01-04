import matplotlib.pyplot as plt


def read_exploitability_log(file_name):
    with open(file_name) as f:
        lines = f.readlines()
    iterations = []
    exploitabilities = []
    for line in lines:
        iteration, exploitability = line.split()
        iterations.append(int(iteration))
        exploitabilities.append(float(exploitability))
    return iterations, exploitabilities


def clean_data(file_name, num_experiments, num_iterations, step_size=1000, omit_range=1, multiplier=1,
               interpolation=False, player='x'):
    x = [step_size * i for i in range(0, num_iterations + 1)]
    y = [0.0 for i in range(0, num_iterations + 1)]

    for i in range(1, num_experiments + 1):
        iterations, exploitabilities = read_exploitability_log(file_name + f"_{i}.txt")
        y_curr = [-1.0 for i in range(0, num_iterations + 1)]
        for j in range(len(iterations)):
            x_index = iterations[j] // step_size
            y_curr[x_index] = multiplier * exploitabilities[j]

        if interpolation:
            for j in range(1, num_iterations):
                if y_curr[j] == -1.0:
                    y_curr[j] = (y_curr[j - 1] + y_curr[j + 1]) / 2.0

            if y_curr[-1] == -1.0:
                y_curr[-1] = y_curr[-2]
            if y_curr[0] == -1.0:
                y_curr[0] = y_curr[1]

        for j in range(0, num_iterations + 1):
            y[j] += y_curr[j]

    x = x[omit_range:num_iterations + 1]
    y = y[omit_range:num_iterations + 1]
    if player == 'x':
        y = [y[i] / num_experiments for i in range(len(y))]
    else:
        y = [-y[i] / num_experiments for i in range(len(y))]
    return x, y




if __name__ == "__main__":
    player = 'x'
    mccfr_num_experiments = 5
    onpath_num_experiments = 5
    upfront_num_experiments = 5
    greedy_mccfr_num_experiments = 5

    mccfr_x, mccfr_y = clean_data("data/eps_constant=0.100000_xmccfr_exploitability_log", mccfr_num_experiments, 99, 100000, 1,
                                  1, interpolation=False)
    onpath_x, onpath_y = clean_data("data/eps_constant=0.100000_xonpath_flipping_exploitability_log", onpath_num_experiments, 99, 100000, 1,
                                    1, interpolation=False)
    upfront_x, upfront_y = clean_data("data/eps_constant=0.100000_xupfront_flipping_exploitability_log", upfront_num_experiments, 99, 100000,
                                      1, 1, interpolation=False)
    greedy_mccfr_x, greedy_mccfr_y = clean_data("data/eps_constant=0.100000_xmccfr_greedy_exploitability_log", greedy_mccfr_num_experiments, 99, 100000, 1,
                                  1, interpolation=False)

    plt.plot(mccfr_x[0:99], mccfr_y[0:99], marker='', linewidth=1, color='green',
             label='mccfr:eps constant 0.1:player ' + player + ', averaged over ' + str(mccfr_num_experiments) + ' experiments')
    plt.plot(onpath_x[0:99], onpath_y[0:99], marker='', linewidth=1, color='blue',
             label='onpath:eps constant 0.1:player ' + player + ', averaged over ' + str(onpath_num_experiments) + ' experiments')
    plt.plot(upfront_x[0:99], upfront_y[0:99], marker='', linewidth=1, color='red',
             label='upfront:eps constant 0.1:player ' + player + ', averaged over ' + str(upfront_num_experiments) + ' experiments')
    plt.plot(greedy_mccfr_x[0:99], greedy_mccfr_y[0:99], marker='', linewidth=1, color='black',
             label='greedy-mccfr:eps constant 0.1:player ' + player + ', averaged over ' + str(greedy_mccfr_num_experiments) + ' experiments')

    # horizontal line
    plt.axhline(y=0, color='black', linestyle='--', linewidth=0.4)
    plt.yticks([0.01, 0.05, 0.1, 0.2, 0.3, 0.4, 0.5])
    plt.xlabel('Number of samples')
    plt.ylabel('Exploitability')
    plt.legend()
    #plt.show()
    plt.savefig("player_x_eps=0.1.png")
    plt.close()

    mccfr_x, mccfr_y = clean_data("data/eps_decay_step_size=500000_xmccfr_exploitability_log", mccfr_num_experiments, 99, 100000, 1,
                                  1, interpolation=False)
    onpath_x, onpath_y = clean_data("data/eps_decay_step_size=500000_xonpath_flipping_exploitability_log", onpath_num_experiments, 99, 100000, 1,
                                    1, interpolation=False)
    upfront_x, upfront_y = clean_data("data/eps_decay_step_size=500000_xupfront_flipping_exploitability_log", upfront_num_experiments, 99, 100000,
                                      1, 1, interpolation=False)
    greedy_mccfr_x, greedy_mccfr_y = clean_data("data/eps_decay_step_size=500000_xmccfr_greedy_exploitability_log", greedy_mccfr_num_experiments, 99, 100000, 1,
                                  1, interpolation=False)


    plt.plot(mccfr_x[0:99], mccfr_y[0:99], marker='', linewidth=1, color='green', linestyle='solid',
             label='mccfr:eps decay:player ' + player + ', averaged over ' + str(mccfr_num_experiments) + ' experiments')
    plt.plot(onpath_x[0:99], onpath_y[0:99], marker='', linewidth=1, color='blue', linestyle='solid',
             label='onpath:eps decay:player ' + player + ', averaged over ' + str(onpath_num_experiments) + ' experiments')
    plt.plot(upfront_x[0:99], upfront_y[0:99], marker='', linewidth=1, color='red', linestyle='solid',
             label='upfront:eps decay:player ' + player + ', averaged over ' + str(upfront_num_experiments) + ' experiments')
    plt.plot(greedy_mccfr_x[0:99], greedy_mccfr_y[0:99], marker='', linewidth=1, color='black', linestyle='solid',
             label='greedy-mccfr:eps decay:player ' + player + ', averaged over ' + str(greedy_mccfr_num_experiments) + ' experiments')

    # horizontal line
    plt.axhline(y=0, color='black', linestyle='--', linewidth=0.4)
    plt.yticks([0.01, 0.05, 0.1, 0.2, 0.3, 0.4, 0.5])
    plt.xlabel('Number of samples')
    plt.ylabel('Exploitability')
    plt.legend()
    #plt.show()
    plt.savefig("player_x_eps_decay_step=500000.png")
    plt.close()

    player = 'o'
    mccfr_x, mccfr_y = clean_data("data/eps_constant=0.100000_omccfr_exploitability_log", mccfr_num_experiments, 99, 100000, 1,
                                  1, interpolation=False, player='o')
    onpath_x, onpath_y = clean_data("data/eps_constant=0.100000_oonpath_flipping_exploitability_log", onpath_num_experiments, 99, 100000, 1,
                                    1, interpolation=False, player='o')
    upfront_x, upfront_y = clean_data("data/eps_constant=0.100000_oupfront_flipping_exploitability_log", upfront_num_experiments, 99, 100000,
                                      1, 1, interpolation=False, player='o')
    greedy_mccfr_x, greedy_mccfr_y = clean_data("data/eps_constant=0.100000_omccfr_greedy_exploitability_log", greedy_mccfr_num_experiments, 99, 100000, 1,
                                  1, interpolation=False, player='o')

    plt.plot(mccfr_x[0:99], mccfr_y[0:99], marker='', linewidth=1, color='green',
             label='mccfr:eps constant 0.1:player ' + player + ', averaged over ' + str(mccfr_num_experiments) + ' experiments')
    plt.plot(onpath_x[0:99], onpath_y[0:99], marker='', linewidth=1, color='blue',
             label='onpath:eps constant 0.1:player ' + player + ', averaged over ' + str(onpath_num_experiments) + ' experiments')
    plt.plot(upfront_x[0:99], upfront_y[0:99], marker='', linewidth=1, color='red',
             label='upfront:eps constant 0.1:player ' + player + ', averaged over ' + str(upfront_num_experiments) + ' experiments')
    plt.plot(greedy_mccfr_x[0:99], greedy_mccfr_y[0:99], marker='', linewidth=1, color='black',
             label='greedy-mccfr:eps constant 0.1:player ' + player + ', averaged over ' + str(
                 greedy_mccfr_num_experiments) + ' experiments')

    # horizontal line
    plt.axhline(y=0, color='black', linestyle='--', linewidth=0.4)
    plt.yticks([0.01, 0.05, 0.1, 0.2, 0.3, 0.4, 0.5])
    plt.xlabel('Number of samples')
    plt.ylabel('Exploitability')
    plt.legend()
    plt.title("RBT")
    #plt.show()
    plt.savefig("player_o_eps=0.1.png")
    plt.close()

    mccfr_x, mccfr_y = clean_data("data/eps_decay_step_size=500000_omccfr_exploitability_log", mccfr_num_experiments, 99, 100000, 1,
                                  1, interpolation=False, player='o')
    onpath_x, onpath_y = clean_data("data/eps_decay_step_size=500000_oonpath_flipping_exploitability_log", onpath_num_experiments, 99, 100000, 1,
                                    1, interpolation=False, player='o')
    upfront_x, upfront_y = clean_data("data/eps_decay_step_size=500000_oupfront_flipping_exploitability_log", upfront_num_experiments, 99, 100000,
                                      1, 1, interpolation=False, player='o')
    greedy_mccfr_x, greedy_mccfr_y = clean_data("data/eps_constant=0.100000_omccfr_greedy_exploitability_log", greedy_mccfr_num_experiments, 99, 100000, 1,
                                  1, interpolation=False, player='o')

    plt.plot(mccfr_x[0:99], mccfr_y[0:99], marker='', linewidth=1, color='green', linestyle='solid',
             label='mccfr:decay eps:player ' + player + ', averaged over ' + str(onpath_num_experiments) + ' experiments')
    plt.plot(onpath_x[0:99], onpath_y[0:99], marker='', linewidth=1, color='blue', linestyle='solid',
             label='onpath:decay eps:player ' + player + ', averaged over ' + str(onpath_num_experiments) + ' experiments')
    plt.plot(upfront_x[0:99], upfront_y[0:99], marker='', linewidth=1, color='red', linestyle='solid',
             label='upfront:decay eps:player ' + player + ', averaged over ' + str(upfront_num_experiments) + ' experiments')
    plt.plot(greedy_mccfr_x[0:99], greedy_mccfr_y[0:99], marker='', linewidth=1, color='black',
             label='greedy-mccfr:eps constant 0.1:player ' + player + ', averaged over ' + str(
                 greedy_mccfr_num_experiments) + ' experiments')

    # horizontal line
    plt.axhline(y=0, color='black', linestyle='--', linewidth=0.4)
    plt.yticks([0.01, 0.05, 0.1, 0.2, 0.3, 0.4, 0.5])
    plt.xlabel('Number of samples')
    plt.ylabel('Exploitability')
    plt.legend()
    plt.title("RBT")
    #plt.show()
    plt.savefig("player_o_eps_decay_step=500000.png")
    plt.close()

    ################################################ Average ###########################################################
    mccfr_x, mccfr_y = clean_data("data/eps_constant=0.100000_xaverage_mccfr_exploitability_log", mccfr_num_experiments, 99,
                                  100000, 1,
                                  1, interpolation=False)
    onpath_x, onpath_y = clean_data("data/eps_constant=0.100000_xaverage_onpath_flipping_exploitability_log",
                                    onpath_num_experiments, 99, 100000, 1,
                                    1, interpolation=False)
    upfront_x, upfront_y = clean_data("data/eps_constant=0.100000_xaverage_upfront_flipping_exploitability_log",
                                      upfront_num_experiments, 99, 100000,
                                      1, 1, interpolation=False)
    greedy_mccfr_x, greedy_mccfr_y = clean_data("data/eps_constant=0.100000_xaverage_mccfr_greedy_exploitability_log",
                                                greedy_mccfr_num_experiments, 99, 100000, 1,
                                                1, interpolation=False)

    plt.plot(mccfr_x[0:99], mccfr_y[0:99], marker='', linewidth=1, color='green',
             label='mccfr:eps constant 0.1:player ' + player + ', averaged over ' + str(
                 mccfr_num_experiments) + ' experiments')
    plt.plot(onpath_x[0:99], onpath_y[0:99], marker='', linewidth=1, color='blue',
             label='onpath:eps constant 0.1:player ' + player + ', averaged over ' + str(
                 onpath_num_experiments) + ' experiments')
    plt.plot(upfront_x[0:99], upfront_y[0:99], marker='', linewidth=1, color='red',
             label='upfront:eps constant 0.1:player ' + player + ', averaged over ' + str(
                 upfront_num_experiments) + ' experiments')
    plt.plot(greedy_mccfr_x[0:99], greedy_mccfr_y[0:99], marker='', linewidth=1, color='black',
             label='greedy-mccfr:eps constant 0.1:player ' + player + ', averaged over ' + str(
                 greedy_mccfr_num_experiments) + ' experiments')

    # horizontal line
    plt.axhline(y=0, color='black', linestyle='--', linewidth=0.4)
    plt.yticks([0.01, 0.05, 0.1, 0.2, 0.3, 0.4, 0.5])
    plt.xlabel('Number of samples')
    plt.ylabel('Exploitability')
    plt.legend()
    # plt.show()
    plt.savefig("player_x_average_eps=0.1.png")
    plt.close()

    mccfr_x, mccfr_y = clean_data("data/eps_decay_step_size=500000_xaverage_mccfr_exploitability_log", mccfr_num_experiments,
                                  99, 100000, 1,
                                  1, interpolation=False)
    onpath_x, onpath_y = clean_data("data/eps_decay_step_size=500000_xaverage_onpath_flipping_exploitability_log",
                                    onpath_num_experiments, 99, 100000, 1,
                                    1, interpolation=False)
    upfront_x, upfront_y = clean_data("data/eps_decay_step_size=500000_xaverage_upfront_flipping_exploitability_log",
                                      upfront_num_experiments, 99, 100000,
                                      1, 1, interpolation=False)
    greedy_mccfr_x, greedy_mccfr_y = clean_data("data/eps_decay_step_size=500000_xaverage_mccfr_greedy_exploitability_log",
                                                greedy_mccfr_num_experiments, 99, 100000, 1,
                                                1, interpolation=False)

    plt.plot(mccfr_x[0:99], mccfr_y[0:99], marker='', linewidth=1, color='green', linestyle='solid',
             label='mccfr:eps decay:player ' + player + ', averaged over ' + str(
                 mccfr_num_experiments) + ' experiments')
    plt.plot(onpath_x[0:99], onpath_y[0:99], marker='', linewidth=1, color='blue', linestyle='solid',
             label='onpath:eps decay:player ' + player + ', averaged over ' + str(
                 onpath_num_experiments) + ' experiments')
    plt.plot(upfront_x[0:99], upfront_y[0:99], marker='', linewidth=1, color='red', linestyle='solid',
             label='upfront:eps decay:player ' + player + ', averaged over ' + str(
                 upfront_num_experiments) + ' experiments')
    plt.plot(greedy_mccfr_x[0:99], greedy_mccfr_y[0:99], marker='', linewidth=1, color='black', linestyle='solid',
             label='greedy-mccfr:eps decay:player ' + player + ', averaged over ' + str(
                 greedy_mccfr_num_experiments) + ' experiments')

    # horizontal line
    plt.axhline(y=0, color='black', linestyle='--', linewidth=0.4)
    plt.yticks([0.01, 0.05, 0.1, 0.2, 0.3, 0.4, 0.5])
    plt.xlabel('Number of samples')
    plt.ylabel('Exploitability')
    plt.legend()
    # plt.show()
    plt.savefig("player_x_average_eps_decay_step=500000.png")
    plt.close()

    player = 'o'
    mccfr_x, mccfr_y = clean_data("data/eps_constant=0.100000_oaverage_mccfr_exploitability_log", mccfr_num_experiments, 99,
                                  100000, 1,
                                  1, interpolation=False, player='o')
    onpath_x, onpath_y = clean_data("data/eps_constant=0.100000_oaverage_onpath_flipping_exploitability_log",
                                    onpath_num_experiments, 99, 100000, 1,
                                    1, interpolation=False, player='o')
    upfront_x, upfront_y = clean_data("data/eps_constant=0.100000_oaverage_upfront_flipping_exploitability_log",
                                      upfront_num_experiments, 99, 100000,
                                      1, 1, interpolation=False, player='o')
    greedy_mccfr_x, greedy_mccfr_y = clean_data("data/eps_constant=0.100000_oaverage_mccfr_greedy_exploitability_log",
                                                greedy_mccfr_num_experiments, 99, 100000, 1,
                                                1, interpolation=False, player='o')

    plt.plot(mccfr_x[0:99], mccfr_y[0:99], marker='', linewidth=1, color='green',
             label='mccfr:eps constant 0.1:player ' + player + ', averaged over ' + str(
                 mccfr_num_experiments) + ' experiments')
    plt.plot(onpath_x[0:99], onpath_y[0:99], marker='', linewidth=1, color='blue',
             label='onpath:eps constant 0.1:player ' + player + ', averaged over ' + str(
                 onpath_num_experiments) + ' experiments')
    plt.plot(upfront_x[0:99], upfront_y[0:99], marker='', linewidth=1, color='red',
             label='upfront:eps constant 0.1:player ' + player + ', averaged over ' + str(
                 upfront_num_experiments) + ' experiments')
    plt.plot(greedy_mccfr_x[0:99], greedy_mccfr_y[0:99], marker='', linewidth=1, color='black',
             label='greedy-mccfr:eps constant 0.1:player ' + player + ', averaged over ' + str(
                 greedy_mccfr_num_experiments) + ' experiments')

    # horizontal line
    plt.axhline(y=0, color='black', linestyle='--', linewidth=0.4)
    plt.yticks([0.01, 0.05, 0.1, 0.2, 0.3, 0.4, 0.5])
    plt.xlabel('Number of samples')
    plt.ylabel('Exploitability')
    plt.legend()
    plt.title("RBT")
    # plt.show()
    plt.savefig("player_o_average_eps=0.1.png")
    plt.close()

    mccfr_x, mccfr_y = clean_data("data/eps_decay_step_size=500000_oaverage_mccfr_exploitability_log", mccfr_num_experiments,
                                  99, 100000, 1,
                                  1, interpolation=False, player='o')
    onpath_x, onpath_y = clean_data("data/eps_decay_step_size=500000_oaverage_onpath_flipping_exploitability_log",
                                    onpath_num_experiments, 99, 100000, 1,
                                    1, interpolation=False, player='o')
    upfront_x, upfront_y = clean_data("data/eps_decay_step_size=500000_oaverage_upfront_flipping_exploitability_log",
                                      upfront_num_experiments, 99, 100000,
                                      1, 1, interpolation=False, player='o')
    greedy_mccfr_x, greedy_mccfr_y = clean_data("data/eps_constant=0.100000_oaverage_mccfr_greedy_exploitability_log",
                                                greedy_mccfr_num_experiments, 99, 100000, 1,
                                                1, interpolation=False, player='o')

    plt.plot(mccfr_x[0:99], mccfr_y[0:99], marker='', linewidth=1, color='green', linestyle='solid',
             label='mccfr:decay eps:player ' + player + ', averaged over ' + str(
                 onpath_num_experiments) + ' experiments')
    plt.plot(onpath_x[0:99], onpath_y[0:99], marker='', linewidth=1, color='blue', linestyle='solid',
             label='onpath:decay eps:player ' + player + ', averaged over ' + str(
                 onpath_num_experiments) + ' experiments')
    plt.plot(upfront_x[0:99], upfront_y[0:99], marker='', linewidth=1, color='red', linestyle='solid',
             label='upfront:decay eps:player ' + player + ', averaged over ' + str(
                 upfront_num_experiments) + ' experiments')
    plt.plot(greedy_mccfr_x[0:99], greedy_mccfr_y[0:99], marker='', linewidth=1, color='black',
             label='greedy-mccfr:eps constant 0.1:player ' + player + ', averaged over ' + str(
                 greedy_mccfr_num_experiments) + ' experiments')

    # horizontal line
    plt.axhline(y=0, color='black', linestyle='--', linewidth=0.4)
    plt.yticks([0.01, 0.05, 0.1, 0.2, 0.3, 0.4, 0.5])
    plt.xlabel('Number of samples')
    plt.ylabel('Exploitability')
    plt.legend()
    plt.title("RBT")
    # plt.show()
    plt.savefig("player_o_average_eps_decay_step=500000.png")
    plt.close()
