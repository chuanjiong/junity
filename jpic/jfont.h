//
// jfont.h
//

#ifndef _J_FONT_H_
#define _J_FONT_H_

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct jfont_bitmap {
    int x, y;
    int w, h;
    unsigned char *data;
}jfont_bitmap;

int jfont_open(const char *font);

void jfont_close(void);

int jfont_get_bitmap(int code, jfont_bitmap *bp, int h);

void jfont_free_bitmap(jfont_bitmap *bp);

#ifdef __cplusplus
}
#endif

#endif //_J_FONT_H_
