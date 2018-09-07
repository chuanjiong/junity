/*
 * jpic.h
 *
 * @chuanjiong
 */

#ifndef _J_PIC_H_
#define _J_PIC_H_

#include "jlib.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct jpic_frame {
    int width;
    int height;
    // 1: grey
    // 2: grey, alpha
    // 3: red, green, blue
    // 4: red, green, blue, alpha
    int comp;
    uint8_t *data;
}jpic_frame;

int jpic_read(const char *url, jpic_frame **frame);

int jpic_write(const char *url, jpic_frame *frame);

int jpic_resize(const jpic_frame *src, jpic_frame **dst, int dst_w, int dst_h);

void jpic_frame_free(jpic_frame *frame);

#ifdef __cplusplus
}
#endif

#endif //_J_PIC_H_


