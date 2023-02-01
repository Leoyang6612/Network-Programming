#ifndef _MAP_STRUCT_H
#define _MAP_STRUCT_H

enum items
{
    Apple,
    Banana,
    Orange
};

typedef struct
{
    int n;
    // enum items type;
} obj_t;

typedef struct
{
    obj_t obj[3];
} grid_t;

#endif
