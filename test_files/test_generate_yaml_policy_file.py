import yaml
from rbt_classes import Policy, InformationSet
import logging

logging.basicConfig(format='%(levelname)s - %(asctime)s - %(message)s', datefmt='%d-%b-%y %H:%M:%S',
                    level=logging.INFO)

# def generate_yaml_from_txt(file_name):
#     with open('./data_files/{}.txt'.format(file_name), 'r') as f:
#         arr = f.read().splitlines()

#     dict_is = dict()
#     for item in arr:
#         dict_is["{}".format(item)] = 0

#     with open('./data_files/{}.yml'.format(file_name), 'w') as outfile:
#         yaml.safe_dump(dict_is, outfile, sort_keys=False, default_style='')


# generate_yaml_from_txt('P1_information_sets')
# generate_yaml_from_txt('P2_information_sets')

logging.info('Creating policy object for player x...')
policy_obj_x = Policy(player='x')
logging.info('Creating policy object for player o...')
policy_obj_o = Policy(player='o')
logging.info('Saving policy objects to file...')
policy_obj_x.save_policy_to_file('P1_uniform_policy.yml')
policy_obj_o.save_policy_to_file('P2_uniform_policy.yml')