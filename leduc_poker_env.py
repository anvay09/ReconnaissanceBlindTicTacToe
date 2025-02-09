import gymnasium as gym
from gymnasium import spaces
import numpy as np
from gymnasium.envs.registration import register

class LeducPokerEnv(gym.Env):
    MAX_UTIL = 13
    
    def __init__(self, **kwargs):
        super(LeducPokerEnv, self).__init__()

        # Extract keyword arguments
        self.states = kwargs["S"]
        self.actions = kwargs["A"]
        self.transition_function = kwargs["T"]
        self.reward_function = kwargs["R"]
        self.state_strings = kwargs["state_strings"]
        self.player = kwargs["player"]
        self.gamma = kwargs.get("gamma", 0.99)  # Default discount factor

        self.starting_states = [0, 48, 96] if self.player == 'x' else [144, 165, 186]
        
        self.state = None
        self.last_reward = None
        self.last_action = None

        self.observation_space = spaces.Discrete(len(self.states))
        self.action_space = spaces.Discrete(max(len(a) for a in self.actions.values()))

    def reset(self, seed=None, options=None):
        super().reset(seed=seed)
        self.state = np.random.choice(self.starting_states)
        self.last_reward = None
        self.last_action = None
        return self.state, {}
    
    def step(self, action):
        if action not in self.actions[self.state]:
            raise ValueError(f"Invalid action {action} for state {self.state}")
        
        # Sample next state
        next_state_probs = self.transition_function[self.state][action]
        next_states, probs = zip(*next_state_probs.items())
        next_state = np.random.choice(next_states, p=probs)
        
        # Default reward
        reward = 0
        
        # Only sample reward if next state is terminal
        if self.state_strings[next_state] == "-" and self.state in self.reward_function and action in self.reward_function[self.state]:
            reward_probs = self.reward_function[self.state][action]
            rewards, probs = zip(*reward_probs.items())
            reward = (np.random.choice(rewards, p=probs) + self.MAX_UTIL) / (2.0 * self.MAX_UTIL)  # Scale reward
        
        # Update state, last reward, and last action
        self.last_action = action
        self.state = next_state
        self.last_reward = reward
        
        # Check if episode should terminate
        done = self.state_strings[self.state] == "-"
        
        return next_state, reward, done, False, {}
    
    def render(self):
        print(f"Current State: {self.state_strings[self.state]}, Last Action: {self.last_action}, Last Reward: {self.last_reward}")
    
    def close(self):
        pass
    
# Parsing functions
def parse_states(filename):
    states = []
    state_strings = {}
    with open(filename, 'r') as f:
        for line in f:
            state_index, state_string = line.strip().split(maxsplit=1)
            state_index = int(state_index)
            states.append(state_index)
            state_strings[state_index] = state_string
    return states, state_strings

def parse_actions(filename):
    actions = {}
    with open(filename, 'r') as f:
        for line in f:
            parts = list(map(int, line.strip().split()))
            state_index = parts[0]
            actions[state_index] = parts[1:]
    return actions

def normalize_probabilities(prob_dict):
    total_prob = sum(prob_dict.values())
    if total_prob > 0:
        return {key: prob / total_prob for key, prob in prob_dict.items()}
    return prob_dict

def parse_transitions(filename):
    transitions = {}
    with open(filename, 'r') as f:
        for line in f:
            state, action, next_state, prob = line.strip().split()
            state, action, next_state = int(state), int(action), int(next_state)
            prob = float(prob)
            
            if state not in transitions:
                transitions[state] = {}
            if action not in transitions[state]:
                transitions[state][action] = {}
            transitions[state][action][next_state] = prob
    
    # Normalize probabilities
    for state in transitions:
        for action in transitions[state]:
            transitions[state][action] = normalize_probabilities(transitions[state][action])
    
    return transitions

def parse_rewards(filename):
    rewards = {}
    with open(filename, 'r') as f:
        for line in f:
            parts = line.strip().split()
            state, action = int(parts[0]), int(parts[1])
            reward_probs = {int(parts[i]): float(parts[i + 1]) for i in range(2, len(parts), 2)}  # Keep raw rewards
            
            if state not in rewards:
                rewards[state] = {}
            rewards[state][action] = normalize_probabilities(reward_probs)
    return rewards
