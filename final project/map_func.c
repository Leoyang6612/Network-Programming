#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <sys/socket.h>
#include "map_func.h"
#include "user_mngmt.h"
#include "openssl/aes.h"

grid_t grids[3][3] = {};
static const char *conversion[] = {
    "Apple",
    "Banana",
    "Orange"};

static void insertToArray(int x, int y, char str[])
{
    if (strcmp(str, "Apple") == 0)
    {
        grids[x][y].obj[Apple].n += 1;
    }
    if (strcmp(str, "Banana") == 0)
    {
        grids[x][y].obj[Banana].n += 1;
    }
    if (strcmp(str, "Orange") == 0)
    {
        grids[x][y].obj[Orange].n += 1;
    }
}

void printMap(void)
{
    printf("==== CURRENT MAP ====\n");
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            for (int k = 0; k < 3; k++)
            {
                if (grids[i][j].obj[k].n != 0)
                {
                    printf("[%d %d] %s:%d\n", i, j, conversion[k], grids[i][j].obj[k].n);
                }
            }
        }
    }
    printf("==== CURRENT MAP ====\n");
}

void buildMap(char filename[])
{
    int x, y;
    char str[10] = "";
    FILE *fp = fopen(filename, "r");
    while (fscanf(fp, "%d %d %s", &x, &y, str) != EOF)
    {
        // printf("%d %d %s\n", x, y, str);
        insertToArray(x, y, str);
    }

    fclose(fp);
}

int convertToInt(char str[])
{
    int idx = -1;
    for (int i = 0; i < 3; i++)
    {
        if (strcmp(str, conversion[i]) == 0)
        {
            idx = i;
        }
    }
    return idx;
}

void getItemsAt(int x, int y, char buffer[])
{
    bool atLeastOne = false;
    char *target = buffer;
    for (int i = 0; i < 3; i++)
    {
        if (grids[x][y].obj[i].n > 0)
        {
            atLeastOne = true;
            target += sprintf(target, "%s:%d  ", conversion[i], grids[x][y].obj[i].n);
        }
    }
    if (!atLeastOne)
    {
        sprintf(buffer, "(empty)");
    }
}

int takeItemAt(int __x, int __y, int itemID)
{
    if (itemID == -1)
        return -1;

    if (grids[__x][__y].obj[itemID].n > 0)
    {
        grids[__x][__y].obj[itemID].n -= 1;
        return 0;
    }
    return 1;
}

void depositItemAt(int __x, int __y, int itemID)
{
    grids[__x][__y].obj[itemID].n += 1;
}

void getPlayersAt(int __x, int __y, char __buffer[], int __myFd)
{
    char *target = __buffer;
    bool atLeastOne = false;

    for (clientInfo *currentptr = headptr; currentptr != NULL; currentptr = currentptr->next)
    {
        if ((currentptr->cord.x == __x && currentptr->cord.y == __y))
        {
            atLeastOne = true;
            target += sprintf(target, "%s%s ", currentptr->uname, currentptr->fd == __myFd ? "(Me)" : "");
        }
    }
    if (!atLeastOne)
    {
        sprintf(__buffer, "(empty)");
    }
}

void getInventoryByFd(int __fd, char __buffer[])
{
    char *target = __buffer;
    bool atLeastOne = false;

    clientInfo *node = findNodeByFd(__fd);
    assert(node != NULL);

    for (int i = 0; i < 3; i++)
    {
        if (node->inventory[i].n > 0)
        {
            atLeastOne = true;
            target += sprintf(target, "%s: %d  ", conversion[i], node->inventory[i].n);
        }
    }
    if (!atLeastOne)
    {
        sprintf(__buffer, "(empty)");
    }
}

void getCoordinateByFd(int __fd, int *__x, int *__y)
{
    clientInfo *node = findNodeByFd(__fd);
    assert(node != NULL);
    *__x = node->cord.x;
    *__y = node->cord.y;
}

int movePositionByFd(int __fd, char direction[])
{
    clientInfo *node = findNodeByFd(__fd);
    assert(node != NULL);

    int x = node->cord.x, y = node->cord.y;
    if (strcasecmp(direction, "east") == 0)
    {
        if (y + 1 >= 3)
        {
            return -1;
        }
        node->cord.y += 1;
    }
    else if (strcasecmp(direction, "west") == 0)
    {
        if (y - 1 < 0)
        {
            return -1;
        }
        node->cord.y -= 1;
    }
    else if (strcasecmp(direction, "north") == 0)
    {
        if (x - 1 < 0)
        {
            return -1;
        }
        node->cord.x -= 1;
    }
    else if (strcasecmp(direction, "south") == 0)
    {
        if (x + 1 >= 3)
        {
            return -1;
        }
        node->cord.x += 1;
    }
    else
    {
        return -1;
    }
    return 0;
}

bool inTheSameSpace(clientInfo *node1, clientInfo *node2)
{
    return (node1->cord.x == node2->cord.x && node1->cord.y == node2->cord.y);
}

void broadcastExcept(char *msg, int __exceptfd)
{
    for (clientInfo *currentptr = headptr; currentptr != NULL; currentptr = currentptr->next)
    {
        if (currentptr->fd != __exceptfd)
        {
            encryptSend(currentptr->fd, msg);
        }
    }
}

void broadcast(char *msg)
{
    for (clientInfo *currentptr = headptr; currentptr != NULL; currentptr = currentptr->next)
    {
        encryptSend(currentptr->fd, msg);
    }
}

int encrypt(char *input_string, char **encrypt_string)
{
    AES_KEY aes;
    unsigned char key[AES_BLOCK_SIZE]; // AES_BLOCK_SIZE = 16
    unsigned char iv[AES_BLOCK_SIZE];  // init vector
    unsigned int len;                  // encrypt length (in multiple of AES_BLOCK_SIZE)
    unsigned int i;

    // set the encryption length
    len = 0;
    if ((strlen(input_string) + 1) % AES_BLOCK_SIZE == 0)
    {
        len = strlen(input_string) + 1;
    }
    else
    {
        len = ((strlen(input_string) + 1) / AES_BLOCK_SIZE + 1) * AES_BLOCK_SIZE;
    }

    // Generate AES 128-bit key
    for (i = 0; i < 16; ++i)
    {
        key[i] = 32 + i;
    }

    // Set encryption key
    for (i = 0; i < AES_BLOCK_SIZE; ++i)
    {
        iv[i] = 0;
    }
    if (AES_set_encrypt_key(key, 128, &aes) < 0)
    {
        fprintf(stderr, "Unable to set encryption key in AES\n");
        exit(0);
    }

    // alloc encrypt_string
    *encrypt_string = (unsigned char *)calloc(len, sizeof(unsigned char));
    if (*encrypt_string == NULL)
    {
        fprintf(stderr, "Unable to allocate memory for encrypt_string\n");
        exit(-1);
    }

    // encrypt (iv will change)
    AES_cbc_encrypt(input_string, *encrypt_string, len, &aes, iv, AES_ENCRYPT);
    return len;
}

void decrypt(char *encrypt_string, char **decrypt_string, int len)
{
    unsigned char key[AES_BLOCK_SIZE]; // AES_BLOCK_SIZE = 16
    unsigned char iv[AES_BLOCK_SIZE];  // init vector
    AES_KEY aes;
    int i;
    // Generate AES 128-bit key

    for (i = 0; i < 16; ++i)
    {
        key[i] = 32 + i;
    }

    // alloc decrypt_string
    *decrypt_string = (unsigned char *)calloc(len, sizeof(unsigned char));
    if (*decrypt_string == NULL)
    {
        fprintf(stderr, "Unable to allocate memory for decrypt_string\n");
        exit(-1);
    }

    // Set decryption key
    for (i = 0; i < AES_BLOCK_SIZE; ++i)
    {
        iv[i] = 0;
    }
    if (AES_set_decrypt_key(key, 128, &aes) < 0)
    {
        fprintf(stderr, "Unable to set decryption key in AES\n");
        exit(-1);
    }

    // decrypt
    AES_cbc_encrypt(encrypt_string, *decrypt_string, len, &aes, iv,
                    AES_DECRYPT);
}

int encryptSend(int __fd, char *in)
{
    int n;
    // printf("bef: %s\n", in);
    char *enc_out = NULL;
    encrypt(in, &enc_out);
    // printf("aft: %s\n", enc_out);

    n = send(__fd, enc_out, BUFFER_MAX, 0);
    free(enc_out);
    return n;
}