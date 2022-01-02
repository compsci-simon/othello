Execute 
1. in a terminal window
Make 
. runall.sh

OR 
Execute
1. in a terminal window, 
Make 
. runserver.sh 

2. in a second terminal window,
. runlobby.sh

3. also in the second terminal window (or in a separate window),
. runplayer1.sh 

4. and in a third terminal window
. runplayer2.sh

# Compilation
I have modified the makefile to include a make me which will compile the
latest version of my player. This will compile a player named latest in the
src directory.

# Running
Make sure to run the player called latest after compilation by having the
game.json file use the correct player from the src directory.

# Structure of my player
My player executes a move from a given board state as follows:

1: All moves from the root state are evaluated by all processes and the best one
is chosen.

2: From the best root move a full minimax search is run to determine the alpha
value for that root.

3: All processes move up one level to the root state and then run minimax with
the shared alpha value determined form step 2. Processes are assigned a move on
which to perform minimax.

# Timing
There is a predetermined depth to run to but if the process runs out of time for
that minimax then it recursively returns so that a move is return within 4
seconds.

# Evaluation
My evluation function weights position very heavily giving corner positions the
highest value, followed by side pieces followed by diagonal pieces from corner
to corner and then giving the four center pieces a little extra weight. This is
beacause in othello until the lategame positioning is more important than the
number of pieces you have on the board.
