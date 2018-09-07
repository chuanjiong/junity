/*
 * jdynarray.h
 *
 * @chuanjiong
 */

#ifndef _J_DYNARRAY_H_
#define _J_DYNARRAY_H_

#include "jcommon.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*
    usage:

    jdynarray(int, array);

    jdynarray_alloc(int, array, 8);

    jdynarray_index(array, 32);
    array[32] = 0;

    jdynarray_free(array);
*/

#define jdynarray(type,name)            type *name; \
                                        int name##_dynarray_capacity; \
                                        int name##_dynarray_type_size

#define jdynarray_alloc(type,name,size) { \
                                            name = (type *)jmalloc(size * sizeof(type)); \
                                            memset(name, 0, size * sizeof(type)); \
                                            name##_dynarray_capacity = size; \
                                            name##_dynarray_type_size = sizeof(type); \
                                        }

#define jdynarray_index(name,idx)       { \
                                            if (idx >= name##_dynarray_capacity) \
                                            { \
                                                uint8_t *temp = (uint8_t *)jmalloc(name##_dynarray_capacity * name##_dynarray_type_size * 2); \
                                                memset(temp, 0, name##_dynarray_capacity * name##_dynarray_type_size * 2); \
                                                memcpy(temp, name, name##_dynarray_capacity * name##_dynarray_type_size); \
                                                jfree(name); \
                                                name = (void *)temp; \
                                                name##_dynarray_capacity *= 2; \
                                            } \
                                        }

#define jdynarray_free(name)            { \
                                            jfree(name); \
                                        }

#ifdef __cplusplus
}
#endif

#endif //_J_DYNARRAY_H_


