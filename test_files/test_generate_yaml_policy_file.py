import yaml


def generate_yaml_from_txt(file_name):
    with open('./../data_files/{}.txt'.format(file_name), 'r') as f:
        arr = f.read().splitlines()

    dict_is = dict()
    for item in arr:
        dict_is["{}".format(item)] = 0

    with open('./../data_files/{}.yml'.format(file_name), 'w') as outfile:
        yaml.safe_dump(dict_is, outfile, sort_keys=False, default_style='')


generate_yaml_from_txt('P1_information_sets')
generate_yaml_from_txt('P2_information_sets')

# with open('./../data_files/P1_information_sets.yml', 'r') as file:
#     a = yaml.safe_load(file)
pass

