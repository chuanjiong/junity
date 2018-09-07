/*
 * jdict.h
 *
 * @chuanjiong
 */

#ifndef _J_DICT_H_
#define _J_DICT_H_

#include "jcommon.h"
#include "jstring.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum jdict_value_type {
    DICT_VALUE_TYPE_NONE,
    DICT_VALUE_TYPE_STRING,
    DICT_VALUE_TYPE_UINT,
}jdict_value_type;

typedef struct jdict_value {
    jdict_value_type type;
    union {
        jstring s;
        unsigned int v;
    };
}jdict_value;

jhandle jdict_alloc(void);

void jdict_free(jhandle h);

int jdict_set_value(jhandle h, const char *key, jdict_value value);

jdict_value jdict_get_value(jhandle h, const char *key);

int jdict_get_capacity(jhandle h);

int jdict_get_value_by_index(jhandle h, int index, jstring *key, jdict_value *value);

#ifdef __cplusplus
}
#endif

#endif //_J_DICT_H_


