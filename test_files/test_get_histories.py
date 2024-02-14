from rbt_classes import InformationSet
from rbt_utilties import get_histories


if __name__ == '__main__':
    I = InformationSet(player='x', board=['-', '0', '0', 'x', 'x', 'o', 'o', '-', 'x'])
    move_flag = True
    H = get_histories(I)
    print(len(H))
