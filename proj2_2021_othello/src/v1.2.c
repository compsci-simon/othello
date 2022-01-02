/* vim: ai:sw=4:ts=4:sts:et */

/*H**********************************************************************
 *
 *	This is a skeleton to guide development of Othello engines to be used
 *	with the Ingenious Framework and a Tournament Engine. 
 *	The socket communication required is handled in the main method,
 *	which is provided with the skeleton. All socket communication is performed
 *	at rank 0.
 *
 *	Board co-ordinates for moves start at the top left corner of the board i.e.
 *	if your engine wishes to place a piece at the top left corner, the "gen_move"
 *	function must return "00".
 *
 *	The match is played by making alternating calls to each engine's "gen_move"
 *	and "play_move" functions. The progression of a match is as follows:
 *		1. Call gen_move for black player
 *		2. Call play_move for white player, providing the black player's move
 *		3. Call gen move for white player
 *		4. Call play_move for black player, providing the white player's move
 *		.
 *		.
 *		.
 *		N. A player makes the final move and "game_over" is called for both players
 *	
 *	IMPORTANT NOTE:
 *		Any output that you would like to see (for debugging purposes) needs
 *		to be written to file. This can be done using file FP, and fprintf(),
 *		don't forget to flush the stream. 
 *		I would suggest writing a method to make this
 *		easier, I'll leave that to you.
 *		The file name is passed as argv[4], feel free to change to whatever suits you.
 *H***********************************************************************/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<mpi.h>
#include<time.h>
#include<assert.h>

#define DEPTH 4
#define BIG 100
#define SMALL -100

const int EMPTY = 0;
const int BLACK = 1;
const int WHITE = 2;
const int OUTER = 3;
const int ALLDIRECTIONS[8]={-11, -10, -9, -1, 1, 9, 10, 11};
const int BOARDSIZE=100;
const int IPBUFSIZE=16;
const int LENBUFSIZE=3;
const int MOVEBUFSIZE=6;
const int MSGBUFSIZE=100;
const int LEGALMOVSBUFSIZE=65;
const char piecenames[4] ={'.','b','w','?'};

struct Node {
	struct Node *parent;
	struct Node **children;
	int move;
	int score;
	int alpha;
	int beta;
	int node_board[100];
};

struct Node root = {NULL, NULL, -1, 0};

void gen_move(char *move);
void play_move(char *move);
void game_over();
void run_worker();
void initialise_board();
void free_board();

void legalmoves (int player, int *moves);
int legalp (int move, int player);
int validp (int move);
int wouldflip (int move, int dir, int player);
int opponent (int player);
int findbracketingpiece(int square, int dir, int player);
int my_strategy(struct Node *parent, int depth, int level_colour);
void makemove (int move, int player);
void makeflips (int move, int dir, int player);
int get_loc(char* movestring);
void get_move_string(int loc, char *ms);
void printboard();
char nameof(int piece);
int count (int player, int * board);
int potential_move_score(int move, int player); 
void copy_array(int *source, int *dest, int length); 
void print_tree ( struct Node *node ); 

int my_colour;
int time_limit;
int running;
int rank;
int size;
int *board;
int firstrun = 1;
FILE *fp;
int graph_size = 0;

int main(int argc , char *argv[]) {
    int socket_desc, port, msg_len;
    char *cmd, *opponent_move;

    char ip[IPBUFSIZE];
    char msg_buf[MSGBUFSIZE]; 
    char len_buf[LENBUFSIZE];
    char my_move[MOVEBUFSIZE];

    struct sockaddr_in server;

    /* starts MPI */
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);	/* get current process id */
    MPI_Comm_size(MPI_COMM_WORLD, &size);	/* get number of processes */

    my_colour = EMPTY;

    initialise_board();

    // Rank 0 is responsible for handling communication with the server
    if (rank == 0 && argc == 5){
    	strncpy(ip, argv[1], IPBUFSIZE);
        port = atoi(argv[2]);
        time_limit = atoi(argv[3]);

        // fp = fopen(argv[4], "w");
        fp = fopen("output.txt", "w");
        fflush(fp);

        socket_desc = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_desc == -1) {
            fprintf(fp, "Could not create socket\n");
            fflush(fp);
            return -1;
        }
        server.sin_addr.s_addr = inet_addr(ip);
        server.sin_family = AF_INET;
        server.sin_port = htons(port);

        //Connect to remote server
        if (connect(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0){

            fprintf(fp, "Connect error\n");
            fflush(fp);
            return -1;
        }
        fprintf(fp, "Connected\n");
        fflush(fp);
        if (socket_desc == -1){
            return 1;
        }

        while (running == 1){
            memset(len_buf, 0, LENBUFSIZE);
            memset(msg_buf, 0, MSGBUFSIZE);
            memset(my_move, 0, MOVEBUFSIZE);

            if (firstrun == 1) {
                char tempColour[2]; tempColour[1] = 0;
                if(recv(socket_desc, tempColour , 1, 0) < 0){
                    fprintf(fp,"Receive failed\n");
                    fflush(fp);
                    running = 0;
                    break;
                }
                my_colour = atoi(tempColour);
                fprintf(fp,"Player colour is: %d\n", my_colour);
                fflush(fp);
                firstrun = 2;
            }


            if(recv(socket_desc, len_buf , 2, 0) < 0){


                fprintf(fp,"Receive failed\n");
                fflush(fp);
                running = 0;
                break;
            }

            msg_len = atoi(len_buf);


            if(recv(socket_desc, msg_buf, msg_len, 0) < 0){
                fprintf(fp,"Receive failed\n");
                fflush(fp);
                running = 0;
                break;
            }


            msg_buf[msg_len] = '\0';
            cmd = strtok(msg_buf, " ");

            if (strcmp(cmd, "game_over") == 0){
                running = 0;
                fprintf(fp, "Game over\n");
                fflush(fp);
                break;

            } else if (strcmp(cmd, "gen_move") == 0){
    		// Rank 0 calls gen_move to make its move 
                gen_move(my_move);
                if (send(socket_desc, my_move, strlen(my_move) , 0) < 0){
                    running = 0;
                    fprintf(fp,"Move send failed\n");
                    fflush(fp);
                    break;
                }
                printboard();
            } else if (strcmp(cmd, "play_move") == 0){
    		// Add the opponent's move to my board 
                opponent_move = strtok(NULL, " ");
                play_move(opponent_move);
                printboard();

            }
        }
    } else {
    	// Rank i (i != 0) calls run_worker to make its move 
        run_worker(rank);
    }
	fclose(fp);
    game_over();
}

/*
	Called at the start of execution on all ranks
 */
void initialise_board(){
    int i;
    running = 1;
    board = (int *)malloc(BOARDSIZE * sizeof(int));
    for (i = 0; i<=9; i++) board[i]=OUTER;
    for (i = 10; i<=89; i++) {
        if (i%10 >= 1 && i%10 <= 8) board[i]=EMPTY; else board[i]=OUTER;
    }
    for (i = 90; i<=99; i++) board[i]=OUTER;
    board[44]=WHITE; board[45]=BLACK; board[54]=BLACK; board[55]=WHITE;
}
void free_board(){
   free(board);
}

/*
	Rank i (i != 0) executes this code 
        ----------------------------------
	Called at the start of execution on all ranks except for rank 0.
	This is where messages are passed between workers to guide the search.
 */
void run_worker(){

}

/*
	Rank 0 executes this code: 
        --------------------------
	Called when your engine is required to make a move. It must return
	a string of the form "xy", where x and y represent the row and
	column where your piece is placed, respectively.

	play_move will not be called for your own engine's moves, so you
	must apply the move generated here to any relevant data structures
	before returning.
 */
void gen_move(char *move){
    int loc;
    if (my_colour == EMPTY){
        my_colour = BLACK;
    }
	
graph_size = 0;
    loc = my_strategy(&root, DEPTH, my_colour);
fprintf(fp, "graph size = %d\n", graph_size);
	fclose(fp);
	fp = fopen("output.txt", "a");
    if (loc == -1 || root.move == -1){
        strncpy(move, "pass\n", MOVEBUFSIZE);
    } else {
		loc = root.move;
		fprintf(fp, "loc = %d\n", loc);
		fclose(fp);
		fp = fopen("output.txt", "a");
        get_move_string(loc, move);
        makemove(loc, my_colour);
    }
}

/*
	Called when the other engine has made a move. The move is given in a
	string parameter of the form "xy", where x and y represent the row
	and column where the opponent's piece is placed, respectively.
 */
void play_move(char *move){
    int loc;
    if (my_colour == EMPTY){
        my_colour = WHITE;
    }
    if (strcmp(move, "pass\n") == 0){
        return;
    }

    loc = get_loc(move);

    makemove(loc, opponent(my_colour));
}

void game_over(){
    free_board();
    MPI_Finalize();
}

void get_move_string(int loc, char *ms){
    int row, col, new_loc;
    new_loc = loc - (9 + 2 * (loc / 10));
    row = new_loc / 8;
    col = new_loc % 8;
    ms[0] = row + '0';
    ms[1] = col + '0';
    ms[2] = '\n';
    ms[3] = 0;
}

int get_loc(char* movestring){
    int row, col;
    row = movestring[0] - '0';
    col = movestring[1] - '0';
    return (10 * (row + 1)) + col + 1;
}

void legalmoves (int player, int *moves) {
    int move, i;
    moves[0] = 0;
    i = 0;
    for (move=11; move<=88; move++)
        if (legalp(move, player)) {
            i++;
            moves[i]=move;
        }
    moves[0]=i;
}

int legalp (int move, int player) {
    int i;
    if (!validp(move)) return 0;
    if (board[move]==EMPTY) {
        i=0;
        while (i<=7 && !wouldflip(move, ALLDIRECTIONS[i], player)) i++;
        if (i==8) return 0; else return 1;
    }
    else return 0;
}

int validp (int move) {
    if ((move >= 11) && (move <= 88) && (move%10 >= 1) && (move%10 <= 8))
        return 1;
    else return 0;
}

int wouldflip (int move, int dir, int player) {
    int c;
    c = move + dir;
    if (board[c] == opponent(player))
        return findbracketingpiece(c+dir, dir, player);
    else return 0;
}

int findbracketingpiece(int square, int dir, int player) {
    while (board[square] == opponent(player)) square = square + dir;
    if (board[square] == player) return square;
    else return 0;
}

int opponent (int player) {
    switch (player) {
        case 1: return 2;
        case 2: return 1;
        default: printf("illegal player\n"); return 0;
    }
}

void copy_array(int *source, int *dest, int length) {
	for (int i = 0; i < length; i++) {
		dest[i] = source[i];
	}
}

// This is my strategy that I am implementing -------------------------------
int my_strategy(struct Node *current, int depth, int level_colour) {

	if (depth > 0) {

		int *moves = (int *)malloc(LEGALMOVSBUFSIZE * sizeof(int));
		int temp_board[BOARDSIZE]; 

		copy_array(board, temp_board, BOARDSIZE);

		memset(moves, 0, LEGALMOVSBUFSIZE);
		legalmoves(my_colour, moves);
		if (moves[0] == 0){
			current->move = -1;
			if (my_colour == level_colour) {
				current->score = SMALL;
			} else {
				current->score = BIG;
			}
			return 0;
		}

		int max = SMALL;
		int min = BIG;
		current->children = malloc(moves[0]*sizeof(struct Node));
		graph_size += moves[0];

		for (int i = 1; i < moves[0] + 1; i++) {
			copy_array(temp_board, board, BOARDSIZE);

			struct Node *child_node = malloc(sizeof(struct Node));
			child_node->parent = current;

			child_node->alpha = current->alpha;
			child_node->beta = current->beta;

			child_node->move = moves[i];
			makemove(moves[i], level_colour);
			copy_array(board, child_node->node_board, BOARDSIZE);
			my_strategy(child_node, depth-1, opponent(level_colour));

			if (level_colour != my_colour) {
				if (child_node->score > max) {
					max = child_node->score;
					current->score = max;
					current->move = moves[i];
					if (max > current->alpha) {
						current->alpha = max;
					}
					if (current->beta <= current->alpha) {
						copy_array(temp_board, board, BOARDSIZE);
						free(child_node);
						free(current->children);
						free(moves);
						return 1;
					}
				}
				free(child_node);
			} else {
				if (child_node->score < min) {
					min = child_node->score;
					current->score = min;
					current->move = moves[i];
					if (min < current->beta) {
						current->beta = min;
					}
					if (current->beta <= current->alpha) {
						copy_array(temp_board, board, BOARDSIZE);
						free(child_node);
						free(current->children);
						free(moves);
						return 1;
					}
				}
				free(child_node);
			}

		}
		copy_array(temp_board, board, BOARDSIZE);
		free(current->children);
		free(moves);
		return 1;

	} else {

		int me, opp;
		me = count(opponent(my_colour), board);
		opp = count(opponent(my_colour), board);
		current->score = me - opp;
		return 1;

	}
}

void print_tree ( struct Node *node ) {

	if (node == NULL) {
		fprintf(fp, "Null node passed\n");
		return;
	}

	while (1) {
		fprintf(fp, "%d ", node->move);
		if (node->children == NULL) {
			break;
		} else {
			for (int i = 0; i <
			sizeof(node->children)/sizeof(node->children[0]); i++) {
				print_tree(node->children[i]);
			}
		}
	}

	return;

}

int potential_move_score(int move, int player) {

    int *board_copy = (int *)malloc(BOARDSIZE * sizeof(int));
	int black = 0;
	int white = 0;
	memset(board_copy, 0, BOARDSIZE);

	for (int i = 0; i < BOARDSIZE; i++) {
		board_copy[i] = board[i];
	}

	makemove(move, player);

	black = count(BLACK, board);
	white = count(WHITE, board);

	for (int i = 0; i < BOARDSIZE; i++) {
		board[i] = board_copy[i];
	}

	free(board_copy);
	if (player == BLACK) {
		return black - white;
	} else {
		return white - black;
	}


}

void makemove (int move, int player) {
    int i;
    board[move] = player;
    for (i=0; i<=7; i++) makeflips(move, ALLDIRECTIONS[i], player);
}

void makeflips (int move, int dir, int player) {
    int bracketer, c;
    bracketer = wouldflip(move, dir, player);
    if (bracketer) {
        c = move + dir;
        do {
            board[c] = player;
            c = c + dir;
        } while (c != bracketer);
    }
}

void printboard(){
    int row, col;
    fprintf(fp,"   1 2 3 4 5 6 7 8 [%c=%d %c=%d]\n",
            nameof(BLACK), count(BLACK, board), nameof(WHITE), count(WHITE, board));
    for (row=1; row<=8; row++) {
        fprintf(fp,"%d  ", row);
        for (col=1; col<=8; col++)
            fprintf(fp,"%c ", nameof(board[col + (10 * row)]));
        fprintf(fp,"\n");
		fflush(fp);
    }
    fflush(fp);
	fclose(fp);
	fp = fopen("output.txt", "a");
}



char nameof (int piece) {
    assert(0 <= piece && piece < 5);
    return(piecenames[piece]);
}

int count (int player, int * board) {
    int i, cnt;
    cnt=0;
    for (i=1; i<=88; i++)
        if (board[i] == player) cnt++;
    return cnt;
}


