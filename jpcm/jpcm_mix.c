/*
 * jpcm_mix.c
 *
 * @chuanjiong
 */

#include "jpcm_mix.h"

#define PCM_MAX         (32767)
#define PCM_MIN         (-32768)

int jpcm_mix(const short *src, short *dst, int sample)
{
    if ((src==NULL) || (dst==NULL) || (sample<=0))
        return -1;

    double f = 1;

    int i;
    for (i=0; i<sample; i++)
    {
        int tmp = (int)(src[i]) + dst[i];

        int out = tmp * f;
        if (out > PCM_MAX)
        {
            f = (double)PCM_MAX / (double)out;
            out = PCM_MAX;
        }
        if (out < PCM_MIN)
        {
            f = (double)PCM_MIN / (double)out;
            out = PCM_MIN;
        }

        if (f < 1)
            f += ((double)1 - f) / (double)32;

        dst[i] = (short)out;
    }

    return 0;
}


