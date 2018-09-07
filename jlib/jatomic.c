/*
 * jatomic.c
 *
 * @chuanjiong
 */

#include "jatomic.h"

int jatomic_true(volatile int *var)
{
    if (jatomic_get(var) == 0)
        return __sync_add_and_fetch(var, 1);
    else
        return __sync_add_and_fetch(var, 0);
}

int jatomic_false(volatile int *var)
{
    if (jatomic_get(var) == 1)
        return __sync_sub_and_fetch(var, 1);
    else
        return __sync_add_and_fetch(var, 0);
}

int jatomic_get(volatile int *var)
{
    return __sync_add_and_fetch(var, 0);
}

int jatomic_add(volatile int *var, int v)
{
    return __sync_add_and_fetch(var, v);
}

int jatomic_sub(volatile int *var, int v)
{
    return __sync_sub_and_fetch(var, v);
}


