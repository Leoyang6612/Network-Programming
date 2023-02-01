#ifndef _USER_MNGMT_H
#define _USER_MNGMT_H

#include "general.h"
#include <stdbool.h>

extern clientInfo *headptr;

static void generateRandomString(char *, const int);
void printList(void);
void addToList(int);
void removeFromList(int);
clientInfo *findNodeByFd(int);
clientInfo *findNodeByName(char *);
void copyMemberInList(char *);
int updateMaxFd(void);
void printUserInfo(clientInfo *);
int userLogin(clientInfo *, char *, char *, char *);
void userRegister(clientInfo *, char *, char *);
int userSave(clientInfo *);

#endif