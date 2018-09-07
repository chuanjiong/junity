/*
 * jatomic.h
 *
 * @chuanjiong
 */

#ifndef _J_ATOMIC_H_
#define _J_ATOMIC_H_

#ifdef __cplusplus
extern "C"
{
#endif

int jatomic_true(volatile int *var);

int jatomic_false(volatile int *var);

int jatomic_get(volatile int *var);

int jatomic_add(volatile int *var, int v);

int jatomic_sub(volatile int *var, int v);

#ifdef __cplusplus
}
#endif

#endif //_J_ATOMIC_H_


