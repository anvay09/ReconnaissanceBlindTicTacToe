import gymnasium as gym
from leduc_poker_env import parse_states, parse_actions, parse_transitions, parse_rewards, LeducPokerEnv
from gymnasium.envs.registration import register

# register the environment
register(
    id="LeducPoker-v0",
    entry_point="leduc_poker_env:LeducPokerEnv",
)

def load_env():
    player = 'x'
    state_file = f"Leduc_Poker_S_{player}.txt"
    action_file = f"Leduc_Poker_A_{player}.txt"
    transition_file = f"Leduc_Poker_T_{player}.txt"
    reward_file = f"Leduc_Poker_R_{player}.txt"

    S, state_strings = parse_states(state_file)
    A = parse_actions(action_file)
    T = parse_transitions(transition_file)
    R = parse_rewards(reward_file)

    env = gym.make("LeducPoker-v0", env_config={"S": S, "A": A, "T": T, "R": R, "state_strings": state_strings, "player": player})
    print("Environment initialized.")
    return env


def confugure_env():
    player = 'x'
    state_file = f"Leduc_Poker_S_{player}.txt"
    action_file = f"Leduc_Poker_A_{player}.txt"
    transition_file = f"Leduc_Poker_T_{player}.txt"
    reward_file = f"Leduc_Poker_R_{player}.txt"

    S, state_strings = parse_states(state_file)
    A = parse_actions(action_file)
    T = parse_transitions(transition_file)
    R = parse_rewards(reward_file)

    env_config = {"S": S, "A": A, "T": T, "R": R, "state_strings": state_strings, "player": player}
    return env_config






