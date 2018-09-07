/*
 * jqueue.h
 *
 * @chuanjiong
 */

#ifndef _J_QUEUE_H_
#define _J_QUEUE_H_

#include "jcommon.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*
    queue:

    +---+---+-----+---+
 <- | 1 | 2 | ... | n | <-
    +---+---+-----+---+
*/

jhandle jqueue_alloc(void);

void jqueue_free(jhandle h);

int jqueue_push(jhandle h, uint8_t *buf, int size);

int jqueue_pop(jhandle h, uint8_t **buf, int *size);

int jqueue_size(jhandle h);

#ifdef __cplusplus
}
#endif

#endif //_J_QUEUE_H_


