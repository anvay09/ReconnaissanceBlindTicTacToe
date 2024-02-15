from rbt_utilties import get_counter_factual_utility
from rbt_classes import Policy, InformationSet

if __name__ == "__main__":
    information_set = InformationSet(player='o', board=[*"00-00----"])
    policy_obj_x = Policy(player='x')
    policy_obj_o = Policy(player='o')
    utility = get_counter_factual_utility(information_set, policy_obj_x, policy_obj_o)
    print(utility)
