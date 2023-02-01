#ifndef _MAP_FUNC_H
#define _MAP_FUNC_H
#include "general.h"
#include <stdbool.h>

extern grid_t grids[3][3];
// MAP RELATED
static void insertToArray(int, int, char[]);
void printMap(void);
void buildMap(char[]);

// USER ACTIONS
int convertToInt(char str[]);
void getItemsAt(int, int, char[]);
int takeItemAt(int, int, int);
void depositItemAt(int, int, int);
void getPlayersAt(int, int, char[], int);
void getInventoryByFd(int, char[]);
void getCoordinateByFd(int, int *, int *);
int movePositionByFd(int, char[]);
bool inTheSameSpace(clientInfo *, clientInfo *);
void broadcastExcept(char *, int);
void broadcast(char *);

// ENCRYPT/DECRYPT FUNCTIONS
int encrypt(char *, char **);
void decrypt(char *, char **, int);
int encryptSend(int, char *);

#endif