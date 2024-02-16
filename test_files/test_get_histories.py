from rbt_classes import InformationSet
from rbt_utilties import get_histories_given_I


if __name__ == '__main__':
    I = InformationSet(player='o', board=['o', '-', '-', '-', 'x', 'o', 'o', '-', 'x'])
    move_flag = False
    H = get_histories_given_I(I)
    print(len(H))
