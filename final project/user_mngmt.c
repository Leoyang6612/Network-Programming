#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "user_mngmt.h"

#define USER_DIR "user_info"
clientInfo *headptr = NULL;
static const char *conversion[] = {
    "Apple",
    "Banana",
    "Orange"};

// random a string for the user, only used in addToList
static void generateRandomString(char *s, const int len)
{
    static const char alphanum[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    for (int i = 0; i < len; ++i)
    {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }
    s[len] = '\0';
}

// traversal through the nodes in the user list!
void printList(void)
{
    clientInfo *currentptr = headptr;
    if (currentptr == NULL)
    {
        printf("No node in list!\n");
        return;
    }
    while (currentptr != NULL)
    {
        printf("--> fd%d uname: %s\n", currentptr->fd, currentptr->uname);
        currentptr = currentptr->next;
    }
}

void addToList(int __fd)
{
    clientInfo *newptr = (clientInfo *)calloc(1, sizeof(clientInfo));
    newptr->fd = __fd;
    generateRandomString(newptr->uname, 5);

    if (headptr == NULL)
    {
        headptr = newptr;
        return;
    }

    clientInfo *currentptr = headptr;
    while (currentptr->next != NULL)
    {
        currentptr = currentptr->next;
    }
    currentptr->next = newptr;
}

void removeFromList(int __fd)
{
    clientInfo *currentptr = headptr;
    clientInfo *previousptr = NULL;
    clientInfo *nextptr = NULL;
    if (currentptr == NULL)
    {
        printf("No node to be deleted!\n");
        return;
    }

    // the first node is the one to be deleted
    if (currentptr->fd == __fd)
    {
        headptr = currentptr->next;
        free(currentptr);
        return;
    }

    do
    {
        previousptr = currentptr;
        currentptr = currentptr->next;
    } while (currentptr != NULL && currentptr->fd != __fd);

    // __fd not found in linked list
    if (currentptr == NULL)
    {
        return;
    }

    previousptr->next = currentptr->next;
    printf("user: %s(fd %d) deleted from list!\n", currentptr->uname, currentptr->fd);
    free(currentptr);
}

clientInfo *findNodeByFd(int __fd)
{
    clientInfo *currentptr = headptr;
    if (currentptr == NULL)
    {
        printf("No node in list!\n");
        return NULL;
    }
    while (currentptr != NULL && currentptr->fd != __fd)
    {
        currentptr = currentptr->next;
    }
    return currentptr;
}

clientInfo *findNodeByName(char *name)
{
    clientInfo *currentptr = headptr;
    if (currentptr == NULL)
    {
        printf("No node in list!\n");
        return NULL;
    }
    while (currentptr != NULL && strcmp(currentptr->uname, name))
    {
        currentptr = currentptr->next;
    }
    return currentptr;
}

void copyMemberInList(char *buff)
{
    for (clientInfo *currentptr = headptr; currentptr != NULL; currentptr = currentptr->next)
    {
        strcat(buff, currentptr->uname);
        strcat(buff, " ");
    }
}

int updateMaxFd(void)
{
    // if there is no node in user list, update maxfd to 3(acceptSock)
    int maxfd = 3;

    for (clientInfo *currentptr = headptr; currentptr != NULL; currentptr = currentptr->next)
    {
        if (currentptr->fd > maxfd)
        {
            maxfd = currentptr->fd;
        }
    }
    return maxfd;
}

void printUserInfo(clientInfo *node)
{
    printf("name: %s\npasswd%s\n", node->uname, node->passwd);
    printf("cord: %d, %d\n", node->cord.x, node->cord.y);
    for (int i = 0; i < 3; i++)
    {

        printf("item%d: %d\n", i, node->inventory[i].n);
    }
    printf("stage: %d\n", node->stage);
}

int userLogin(clientInfo *node, char *uname, char *passwd, char *buff)
{
    strcpy(node->uname, uname);
    strcpy(node->passwd, passwd);

    char filename[20] = "";
    sprintf(filename, "%s/%s.txt", USER_DIR, uname);

    FILE *fp = fopen(filename, "r");

    if (fp == NULL)
    {
        sprintf(buff, "Find no user!\n");
        return -1;
    }

    char tmp[100] = "";
    fscanf(fp, "passwd: %s", tmp);
    if (strcmp(tmp, passwd) != 0)
    {
        sprintf(buff, "passwd INcorrect!");
        fclose(fp);
        return -1;
    }

    // deal with position
    int x, y;
    fscanf(fp, " position: %d %d", &x, &y);
    node->cord.x = x;
    node->cord.y = y;

    char *target = buff;
    target += sprintf(target, "====== Recovery ======\nposition: (%d, %d) ", x, y);

    // deal with item: count
    int num = 0;
    bzero(tmp, sizeof(tmp));
    fscanf(fp, " %s", tmp);

    while (fscanf(fp, " %s %d", tmp, &num) != EOF)
    {
        int idx = -1;
        for (int i = 0; i < 3; i++)
        {
            if (strcmp(tmp, conversion[i]) == 0)
            {
                idx = i;
            }
        }
        node->inventory[idx].n = num;
        target += sprintf(target, "%s: %d ", tmp, num);
    }
    fclose(fp);

    node->stage = LOGIN;
    return 0;
}

void userRegister(clientInfo *node, char *uname, char *passwd)
{
    strcpy(node->uname, uname);
    strcpy(node->passwd, passwd);

    char filename[20] = "";
    sprintf(filename, "%s/%s.txt", USER_DIR, uname);

    FILE *fp = fopen(filename, "w");
    char content[100] = "";
    sprintf(content, "passwd: %s\nposition: 0 0\ninventory:", passwd);
    fprintf(fp, "%s", content);
    fclose(fp);

    node->stage = LOGIN;
}

int userSave(clientInfo *node)
{

    char filename[20] = "";
    sprintf(filename, "%s/%s.txt", USER_DIR, node->uname);

    FILE *fp = fopen(filename, "w");
    if (fp == NULL)
        return -1;

    char content[100] = "";
    char *target = content;
    target += sprintf(target, "passwd: %s\nposition: %d %d\ninventory:\n", node->passwd, node->cord.x, node->cord.y);
    for (int i = 0; i < 3; i++)
    {
        if (node->inventory[i].n > 0)
        {
            target += sprintf(target, "%s %d", conversion[i], node->inventory[i].n);
        }
    }

    fprintf(fp, "%s", content);
    fclose(fp);
    return 0;
}
