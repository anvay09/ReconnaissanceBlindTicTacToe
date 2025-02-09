import gymnasium as gym
from gymnasium import spaces
import numpy as np

class LeducPokerEnv(gym.Env):
    def __init__(self, S, A, T, R, gamma=0.99):
        super(LeducPokerEnv, self).__init__()
        
        self.states = S  # List of states
        self.actions = A  # Dictionary mapping state to available actions
        self.transition_function = T  # T[s][a] -> dict {s': prob}
        self.reward_function = R  # R[s][a] -> dict {reward: prob}
        self.gamma = gamma
        
        self.state = None
        self.last_reward = None
        
        self.observation_space = spaces.Discrete(len(S))
        self.action_space = spaces.Discrete(max(len(a) for a in A.values()))
    
    def reset(self, seed=None, options=None):
        super().reset(seed=seed)
        self.state = np.random.choice(self.states)
        self.last_reward = None
        return self.state, {}
    
    def step(self, action):
        if action not in self.actions[self.state]:
            raise ValueError(f"Invalid action {action} for state {self.state}")
        
        # Sample next state
        next_state_probs = self.transition_function[self.state][action]
        next_states, probs = zip(*next_state_probs.items())
        next_state = np.random.choice(next_states, p=probs)
        
        # Sample reward
        reward_probs = self.reward_function[self.state][action]
        rewards, probs = zip(*reward_probs.items())
        reward = np.random.choice(rewards, p=probs)
        
        # Update state and last reward
        self.state = next_state
        self.last_reward = reward
        
        # Assuming episodic tasks where we check termination externally
        done = False
        
        return next_state, reward, done, False, {}
    
    def render(self):
        print(f"Current State: {self.state}, Last Reward: {self.last_reward}")
    
    def close(self):
        pass
