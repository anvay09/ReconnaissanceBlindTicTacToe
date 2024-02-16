from rbt_classes import InformationSet
from rbt_utilties import get_histories_given_I
import logging

if __name__ == '__main__':
    # I = InformationSet(player='o', board=['o', '-', '-', '-', 'x', 'o', 'o', '-', 'x'])
    I = InformationSet(player='x', board=['-', '0', '0', 'x', 'x', 'o', 'o', '-', 'x'])
    H = get_histories_given_I(I)
    logging.info('Number of histories: {}'.format(len(H)))
    logging.info('First 5 histories: ')
    print(H[:5])
