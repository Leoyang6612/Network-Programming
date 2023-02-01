#ifndef _CLIENT_STRUCT_H
#define _CLIENT_STRUCT_H
typedef enum
{
    SUSPEND,
    WAITING_TO_LOGIN,
    WAITING_TO_REGISTER,
    LOGIN
} stage_t;

typedef struct
{
    int x;
    int y;
} cord_t;

typedef struct clientInfo_t
{
    int fd;
    char uname[10];
    char passwd[20];
    obj_t inventory[3];
    cord_t cord;
    stage_t stage;
    struct clientInfo_t *next;
} clientInfo;

#endif