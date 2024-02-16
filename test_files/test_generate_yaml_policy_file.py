import yaml
from rbt_classes import Policy, InformationSet


# def generate_yaml_from_txt(file_name):
#     with open('./../data_files/{}.txt'.format(file_name), 'r') as f:
#         arr = f.read().splitlines()
#
#     dict_is = dict()
#     for item in arr:
#         dict_is["{}".format(item)] = 0
#
#     with open('./../data_files/{}.yml'.format(file_name), 'w') as outfile:
#         yaml.safe_dump(dict_is, outfile, sort_keys=False, default_style='')
#
#
# generate_yaml_from_txt('P1_information_sets')
# generate_yaml_from_txt('P2_information_sets')

policy_obj = Policy(player='x', policy_file='./../data_files/P1_information_sets.yml')
for key, val in policy_obj.policy_dict.items():
    information_set_obj = InformationSet(player=policy_obj.player, board=[*key])
    if not information_set_obj.is_curr_action_move():
        policy_obj.policy_dict[key] = 9
pass

