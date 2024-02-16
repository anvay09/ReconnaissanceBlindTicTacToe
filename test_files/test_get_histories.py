from rbt_classes import InformationSet
from rbt_utilties import get_histories_given_I


if __name__ == '__main__':
    I = InformationSet(player='x', board=['-', '0', '0', 'x', 'x', 'o', 'o', '-', 'x'])
    move_flag = True
    H = get_histories_given_I(I)
    print(len(H))
