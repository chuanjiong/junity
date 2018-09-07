/*
 * jstring.h
 *
 * @chuanjiong
 */

#ifndef _J_STRING_H_
#define _J_STRING_H_

#include "jcommon.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define tostring(str)       ((const char *)((str).data))
#define stringsize(str)     ((str).size-1)

typedef struct jstring {
    int size;
    char *data;
}jstring;

typedef enum jstring_anchor_type {
    STRING_ANCHOR_POS,
    STRING_ANCHOR_STR,
}jstring_anchor_type;

typedef enum jstring_anchor_clude {
    STRING_ANCHOR_INCLUDE,
    STRING_ANCHOR_EXCLUDE,
}jstring_anchor_clude;

typedef struct jstring_anchor {
    jstring_anchor_type type;
    union {
        int pos;
        const char *str;
    };
    jstring_anchor_clude clude;
}jstring_anchor;

jstring jstring_copy(const char *s);

jstring jstring_extract(const char *s, jstring_anchor p, jstring_anchor q);

jstring jstring_link(const char *s1, const char *s2);

int jstring_compare(jstring s1, const char *s2);

jstring jstring_file_name(const char *url);

jstring jstring_file_ext(const char *file);

jstring jstring_full_path(const char *url);

jstring jstring_protocol(const char *url);

jstring jstring_pick(const char *s, const char *s1, const char *s2);

int jstring_pick_value(const char *s, const char *s1, const char *s2);

jstring jstring_pick_1st_word(const char *s, const char *s1, const char *s2);

jstring jstring_cut(const char *s, const char *s1);

jstring jstring_discard_prev_space(jstring s);

int jstring_char_count(const char *s, char c);

jstring jstring_from_hex(const unsigned char *buf, int size);

int jstring_to_hex(unsigned char *buf, jstring s);

jstring jstring_free(jstring s);

#ifdef __cplusplus
}
#endif

#endif //_J_STRING_H_


