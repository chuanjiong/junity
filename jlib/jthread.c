/*
 * jthread.c
 *
 * @chuanjiong
 */

#include "jthread.h"
#include "jatomic.h"

typedef struct jthread {
    pthread_t pid;

    jthread_routine r;
    void *arg;

    volatile int running;
}jthread;

static void *helper(void *arg)
{
    if (arg == NULL)
        return NULL;

    jthread *t = (jthread *)arg;

    t->r(t, t->arg);

    return NULL;
}

jhandle jthread_setup(jthread_routine r, void *arg)
{
    if (r == NULL)
        return NULL;

    jthread *t = (jthread *)jmalloc(sizeof(jthread));
    if (t == NULL)
        return NULL;
    memset(t, 0, sizeof(jthread));

    t->r = r;
    t->arg = arg;

    jatomic_true(&(t->running));

    if (pthread_create(&(t->pid), NULL, helper, t) != 0)
    {
        jfree(t);
        return NULL;
    }

    return t;
}

int jthread_is_running(jhandle h)
{
    if (h == NULL)
        return 0;

    jthread *t = (jthread *)h;

    return jatomic_get(&(t->running));
}

void jthread_shutdown(jhandle h)
{
    if (h == NULL)
        return;

    jthread *t = (jthread *)h;

    jatomic_false(&(t->running));

    pthread_join(t->pid, NULL);

    jfree(t);
}


