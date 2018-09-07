/*
 * jmalloc.c
 *
 * @chuanjiong
 */

#include "jmalloc.h"
#include "jcommon.h"

#if (MEM_DEBUG == 1)

static int alloc_count = 0;
static int alloc_total_size = 0;
static pthread_mutex_t alloc_mutex = PTHREAD_MUTEX_INITIALIZER;

#if (MEM_DEBUG_TRACE == 1)
static uint8_t **alloc_addr_array = NULL;
static int alloc_addr_array_size = 0;
#endif

void jmalloc_setup(void)
{
    alloc_count = 0;
    alloc_total_size = 0;

#if (MEM_DEBUG_TRACE == 1)
    alloc_addr_array_size = MEM_DEBUG_TRACE_SIZE;
    alloc_addr_array = (uint8_t **)malloc(alloc_addr_array_size * sizeof(uint8_t *));
    if (alloc_addr_array)
        memset(alloc_addr_array, 0, alloc_addr_array_size * sizeof(uint8_t *));
#endif
}

void jmalloc_shutdown(void)
{
    if (alloc_count)
    {
        jerr("[jmalloc] malloc dismatch free - (%d:%d), fix it!\n", alloc_count, alloc_total_size);
#if (MEM_DEBUG_TRACE == 1)
        if (alloc_addr_array)
        {
            int i;
            for (i=0; i<alloc_addr_array_size; i++)
                if (alloc_addr_array[i])
                    jerr("file: %46.46s, line:%6.6s, size: %d\n",
                        alloc_addr_array[i]+4, alloc_addr_array[i]+50, *((int32_t*)(&alloc_addr_array[i][56])));
        }
#endif
    }
    else
    {
        jlog("[jmalloc] malloc match free\n");
    }

#if (MEM_DEBUG_TRACE == 1)
    if (alloc_addr_array)
        free(alloc_addr_array);
#endif
}

/*  0    4   50   56   60
    +----+----+----+----+   +----+
    |jMAL|file|line|size|...|LAMj|
    +----+----+----+----+   +----+
*/
void *jmalloc_fence(int size, const char *file, int line)
{
    if (size <= 0)
        return NULL;

    uint8_t *p = (uint8_t *)malloc(size + 64);
    if (p == NULL)
    {
        jerr("**** [jmalloc] please check system and program or everything ****\n");
        return NULL;
    }

    pthread_mutex_lock(&alloc_mutex);
    alloc_count++;
    alloc_total_size += size;
#if (MEM_DEBUG_TRACE == 1)
    if (alloc_addr_array)
    {
        int i;
        for (i=0; i<alloc_addr_array_size; i++)
            if (alloc_addr_array[i] == NULL)
            {
                alloc_addr_array[i] = p;
                break;
            }
    }
#endif
    pthread_mutex_unlock(&alloc_mutex);

    //add fence
    p[0] = 'j';
    p[1] = 'M';
    p[2] = 'A';
    p[3] = 'L';
    //file
    jsnprintf((char*)&p[4], 46, "%s", file);
    //line
    jsnprintf((char*)&p[50], 6, "%d", line);
    *((int32_t*)(&p[56])) = size;

    p[size+60] = 'L';
    p[size+61] = 'A';
    p[size+62] = 'M';
    p[size+63] = 'j';

    return (void *)(p+60);
}

void jfree_fence(void *p, const char *file, int line)
{
    if (p == NULL)
        return;

    uint8_t *pf = ((uint8_t *)p) - 60;
    if ((pf[0]!='j') || (pf[1]!='M')
        || (pf[2]!='A') || (pf[3]!='L'))
    {
        jerr("============= [jmalloc] =============\n");
        jerr(" free check fence failed - 1, fix it!\n");
        jerr(" mem: %46.46s:%6.6s\n", &pf[4], &pf[50]);
        jerr(" caller: %s:%d, addr:%p\n", file, line, __builtin_return_address(0));
        while (1) jsleep(1000);
    }

    int size = *((int32_t*)(&pf[56]));
    if ((pf[size+60]!='L') || (pf[size+61]!='A')
        || (pf[size+62]!='M') || (pf[size+63]!='j'))
    {
        jerr("============= [jmalloc] =============\n");
        jerr(" free check fence failed - 2, fix it!\n");
        jerr(" mem: %46.46s:%6.6s\n", &pf[4], &pf[50]);
        jerr(" caller: %s:%d, addr:%p\n", file, line, __builtin_return_address(0));
        while (1) jsleep(1000);
    }

    pthread_mutex_lock(&alloc_mutex);
    alloc_count--;
    alloc_total_size -= size;
#if (MEM_DEBUG_TRACE == 1)
    if (alloc_addr_array)
    {
        int i;
        for (i=0; i<alloc_addr_array_size; i++)
            if (alloc_addr_array[i] == pf)
            {
                alloc_addr_array[i] = NULL;
                break;
            }
    }
#endif
    pthread_mutex_unlock(&alloc_mutex);

    free(pf);
}

int jmalloc_trace(char *buf, int size)
{
    if ((buf==NULL) || (size<=0))
        return 0;

    int total = 0;

    pthread_mutex_lock(&alloc_mutex);
    total += jsnprintf(buf+total, size-total, "memory alloc count: %d, total size: %d<br>", alloc_count, alloc_total_size);
#if (MEM_DEBUG_TRACE == 1)
    if (alloc_addr_array)
    {
        int i;
        for (i=0; i<alloc_addr_array_size; i++)
            if (alloc_addr_array[i])
                total += jsnprintf(buf+total, size-total, "%d file: %46.46s, line:%6.6s, size: %d<br>",
                    i, alloc_addr_array[i]+4, alloc_addr_array[i]+50, *((int32_t*)(&alloc_addr_array[i][56])));
    }
#endif
    pthread_mutex_unlock(&alloc_mutex);

    return total;
}

#endif


