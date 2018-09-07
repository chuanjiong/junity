/*
 * jstring.c
 *
 * @chuanjiong
 */

#include "jstring.h"

static const jstring empty = {0, NULL};

jstring jstring_copy(const char *s)
{
    if (s == NULL)
        return empty;

    int size = strlen(s);
    if (size <= 0)
        return empty;

    jstring jstr;

    jstr.size = size + 1; //+1 for terminate string
    jstr.data = (char *)jmalloc(jstr.size);
    if (jstr.data == NULL)
        return empty;
    memset(jstr.data, 0, jstr.size);

    memcpy(jstr.data, s, size);

    return jstr;
}

jstring jstring_extract(const char *s, jstring_anchor p, jstring_anchor q)
{
    if (s == NULL)
        return empty;

    int ssize = strlen(s);
    if (ssize <= 0)
        return empty;

    int left, right;
    const char *s1 = s;

    //left bound - include [
    if (p.type == STRING_ANCHOR_POS)
    {
        if (p.clude == STRING_ANCHOR_INCLUDE)
            left = p.pos;
        else
            left = p.pos + 1;

        if (left >= ssize)
            return empty;
    }
    else //STRING_ANCHOR_STR
    {
        if (p.str == NULL)
            return empty;

        const char *a = strstr(s, p.str);
        s1 = a + strlen(p.str);
        if (a == NULL)
            return empty;

        if (p.clude == STRING_ANCHOR_INCLUDE)
            left = a - s;
        else
            left = a - s + strlen(p.str);

        if (left >= ssize)
            return empty;
    }

    //right bound - exclude )
    if (q.type == STRING_ANCHOR_POS)
    {
        if (q.clude == STRING_ANCHOR_INCLUDE)
            right = q.pos + 1;
        else
            right = q.pos;

        if (right > ssize)
            return empty;
    }
    else //STRING_ANCHOR_STR
    {
        if (q.str == NULL)
            return empty;

        const char *a = strstr(s1, q.str);
        if (a == NULL)
            return empty;

        if (q.clude == STRING_ANCHOR_INCLUDE)
            right = a - s + strlen(q.str);
        else
            right = a - s;

        if (right > ssize)
            return empty;
    }

    int size = right - left;
    if (size <= 0)
        return empty;

    jstring jstr;

    jstr.size = size + 1; //+1 for terminate string
    jstr.data = (char *)jmalloc(jstr.size);
    if (jstr.data == NULL)
        return empty;
    memset(jstr.data, 0, jstr.size);

    memcpy(jstr.data, &(s[left]), size);

    return jstr;
}

jstring jstring_link(const char *s1, const char *s2)
{
    if ((s1==NULL) || (s2==NULL))
        return empty;

    int size1 = strlen(s1);
    if (size1 <= 0)
        return empty;

    int size2 = strlen(s2);
    if (size2 <= 0)
        return empty;

    jstring jstr;

    jstr.size = size1 + size2 + 1; //+1 for terminate string
    jstr.data = (char *)jmalloc(jstr.size);
    if (jstr.data == NULL)
        return empty;
    memset(jstr.data, 0, jstr.size);

    memcpy(jstr.data, s1, size1);
    memcpy(jstr.data+size1, s2, size2);

    return jstr;
}

int jstring_compare(jstring s1, const char *s2)
{
    if ((s1.size<=1) || (s2==NULL))
        return -1;

    if (strlen(s2) <= 0)
        return -1;

    int i;
    for (i=0; i<s1.size; i++)
        if (s1.data[i] != s2[i])
            break;

    if (i == s1.size)
        return 0;
    else
        return -1;
}

jstring jstring_file_name(const char *url)
{
    if (url == NULL)
        return empty;

    const char *a = strrchr(url, '/');
    if (a == NULL)
        a = url;
    else
        a++;

    return jstring_copy(a);
}

jstring jstring_file_ext(const char *file)
{
    if (file == NULL)
        return empty;

    const char *a = strrchr(file, '.');
    if (a == NULL)
        return empty;
    else
        a++;

    if (*a == '/')
        return empty;
    else
        return jstring_copy(a);
}

jstring jstring_full_path(const char *url)
{
    if (url == NULL)
        return empty;

    const char *a = strrchr(url, '/');
    if (a == NULL)
    {
        return jstring_copy("./");
    }
    else
    {
        jstring_anchor p, q;
        p.type = STRING_ANCHOR_POS;
        p.pos = 0;
        p.clude = STRING_ANCHOR_INCLUDE;
        q.type = STRING_ANCHOR_POS;
        q.pos = a-url;
        q.clude = STRING_ANCHOR_INCLUDE;
        return jstring_extract(url, p, q);
    }
}

jstring jstring_protocol(const char *url)
{
    if (url == NULL)
        return empty;

    const char *a = strstr(url, "://");
    if (a == NULL)
    {
        return jstring_copy("file://");
    }
    else
    {
        jstring_anchor p, q;
        p.type = STRING_ANCHOR_POS;
        p.pos = 0;
        p.clude = STRING_ANCHOR_INCLUDE;
        q.type = STRING_ANCHOR_STR;
        q.str = "://";
        q.clude = STRING_ANCHOR_INCLUDE;
        return jstring_extract(url, p, q);
    }
}

jstring jstring_pick(const char *s, const char *s1, const char *s2)
{
    if ((s==NULL) || (s1==NULL) || (s2==NULL))
        return empty;

    jstring_anchor p, q;
    p.type = STRING_ANCHOR_STR;
    p.str = s1;
    p.clude = STRING_ANCHOR_EXCLUDE;
    q.type = STRING_ANCHOR_STR;
    q.str = s2;
    q.clude = STRING_ANCHOR_EXCLUDE;

    return jstring_extract(s, p, q);
}

int jstring_pick_value(const char *s, const char *s1, const char *s2)
{
    jstring str = jstring_pick(s, s1, s2);
    int v;
    if (stringsize(str) > 0)
        v = atoi(tostring(str));
    else
        v = -1;
    jstring_free(str);
    return v;
}

jstring jstring_pick_1st_word(const char *s, const char *s1, const char *s2)
{
    jstring temp1 = jstring_pick(s, s1, s2);
    jstring temp2 = jstring_discard_prev_space(temp1);
    jstring_free(temp1);

    int i;
    for (i=0; i<temp2.size; i++)
        if (!isalnum(temp2.data[i]))
            break;

    jstring_anchor p, q;
    p.type = STRING_ANCHOR_POS;
    p.pos = 0;
    p.clude = STRING_ANCHOR_INCLUDE;
    q.type = STRING_ANCHOR_POS;
    q.pos = i;
    q.clude = STRING_ANCHOR_EXCLUDE;
    jstring word = jstring_extract(tostring(temp2), p, q);

    jstring_free(temp2);
    return word;
}

jstring jstring_cut(const char *s, const char *s1)
{
    if ((s==NULL) || (s1==NULL))
        return empty;

    jstring_anchor p, q;
    p.type = STRING_ANCHOR_POS;
    p.pos = 0;
    p.clude = STRING_ANCHOR_INCLUDE;
    q.type = STRING_ANCHOR_STR;
    q.str = s1;
    q.clude = STRING_ANCHOR_EXCLUDE;

    return jstring_extract(s, p, q);
}

jstring jstring_discard_prev_space(jstring s)
{
    if (s.size <= 1)
        return empty;

    int i;
    int count = 0;

    for (i=0; i<s.size; i++)
        if ((s.data[i]==' ') || (s.data[i]=='\t'))
            count++;
        else
            break;

    jstring jstr;

    jstr.size = s.size - count;
    jstr.data = (char *)jmalloc(jstr.size);
    if (jstr.data == NULL)
        return empty;
    memset(jstr.data, 0, jstr.size);

    memcpy(jstr.data, &(s.data[count]), jstr.size);

    return jstr;
}

int jstring_char_count(const char *s, char c)
{
    if ((s==NULL) || (c<=0))
        return 0;

    int count = 0;

    while (*s)
    {
        if (*s++ == c)
            count++;
    }

    return count;
}

static const char *hex = "0123456789abcdef";

jstring jstring_from_hex(const unsigned char *buf, int size)
{
    if ((buf==NULL) || (size<=0))
        return empty;

    jstring jstr;

    jstr.size = size*2 + 1; //+1 for terminate string
    jstr.data = (char *)jmalloc(jstr.size);
    if (jstr.data == NULL)
        return empty;
    memset(jstr.data, 0, jstr.size);

    int i;
    for (i=0; i<size; i++)
    {
        jstr.data[i*2+0] = hex[(buf[i]>>4)&0xf];
        jstr.data[i*2+1] = hex[buf[i]&0xf];
    }

    return jstr;
}

int jstring_to_hex(uint8_t *buf, jstring s)
{
    if ((buf==NULL) || (s.size<=1) || ((stringsize(s)%2)!=0))
        return ERROR_FAIL;

    int i, v;
    for (i=0; i<(stringsize(s)/2); i++)
    {
        if (((s.data[i*2+0])>='0') || ((s.data[i*2+0])<='9'))
            v = s.data[i*2+0] - '0';
        else
            v = s.data[i*2+0] - 'a' + 10;
        buf[i] = (v&0xf) << 4;

        if (((s.data[i*2+1])>='0') || ((s.data[i*2+1])<='9'))
            v = s.data[i*2+1] - '0';
        else
            v = s.data[i*2+1] - 'a' + 10;
        buf[i] |= (v & 0xf);
    }

    return SUCCESS;
}

jstring jstring_free(jstring s)
{
    if (s.size > 0)
        jfree(s.data);

    return empty;
}


