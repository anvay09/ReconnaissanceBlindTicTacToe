# ReconnaissanceBlindTicTacToe

## How to run
Install pygame and then run `python3 play_rbt.py`.

## How to play

1. Player 1 plays 'x' and player 2 plays 'o'. 
2. You play as player 2 against a stochastic policy for player 1.
3. Players cannot receive no knowledge of each other's actions. 
4. Players turn consists of two parts - a sense action followed by a move action. The only information about the board is from what is known as a 'sense' action, where you choose a 2x2 region of the board and see all past opponent move actions made in that region.  
6. If a player makes a move action on the same square where the opponent has previously made a move action, the player loses instantly. 

