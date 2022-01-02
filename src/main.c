#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Declaring constansts and other global variables */

int DEPTH = 8;
int change_depth = 0;
int DEBUG = 1;
double TIME = 3.85;
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
int *board;


/*
 * Method declarations
 */
void initialize_board();
void print_board();


/*
 * Methods start here
 */

int main() {
    initialize_board();
    board[77] = BLACK;
    print_board();
    return 0;
}

int* valid_moves(int player) {
    for (int i = 0; i < BOARDSIZE; i++) {
        
    }
    return NULL;
}

/**
 * @brief Used to initialize the board object
 * 
 */
void initialize_board() {
    board = malloc(BOARDSIZE);
    for (int i = 0; i < 10; i++) {
        board[i] = OUTER;
        board[i + 90] = OUTER;
    }

    for (int i = 10; i < 90; i++) {
        if (i % 10 == 0 || i % 10 == 9) {
            board[i] = OUTER;
        } else {
            board[i] = EMPTY;
        }
    }
    board[44] = WHITE;
    board[45] = BLACK;
    board[54] = BLACK;
    board[55] = WHITE;
}

/**
 * @brief Used to print the board object
 * 
 */
void print_board() {
    char string[72];
    int string_place = 0;
    for (int i = 10; i < 90 && string_place < 72; i++) {
        switch (board[i]) {
            case EMPTY:
                string[string_place] = piecenames[0];
                break;
            case BLACK:
                string[string_place] = piecenames[1];
                break;
            case WHITE:
                string[string_place] = piecenames[2];
                break;
            default:
                string_place--;
                break;
        }
        if (i % 10 == 9 && string_place < 71) {
            string_place++;
            string[string_place] = '\n';
        }
        string_place++;
    }
    printf("%s", string);
}
