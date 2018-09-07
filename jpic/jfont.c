//
// jfont.c
//

#include "jfont.h"
#include "jlib.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

static stbtt_fontinfo g_font;
static unsigned char *g_font_buf;
int jfont_open(const char *font)
{
    if (font == NULL)
        return ERROR_FAIL;

    FILE *fp = fopen(font, "rb");
    if (fp == NULL)
    {
        jerr("[jfont] open %s fail\n", font);
        return ERROR_FAIL;
    }
    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    g_font_buf = jmalloc(size);
    if (g_font_buf == NULL)
    {
        jerr("[jfont] malloc %d fail\n", size);
        fclose(fp);
        return ERROR_FAIL;
    }
    fseek(fp, 0, SEEK_SET);
    if (fread(g_font_buf, 1, size, fp) != size)
    {
        jerr("[jfont] read font fail\n");
        jfree(g_font_buf);
        fclose(fp);
        return ERROR_FAIL;
    }
    fclose(fp);

    stbtt_InitFont(&g_font, g_font_buf,
        stbtt_GetFontOffsetForIndex(g_font_buf,0));
    return SUCCESS;
}

void jfont_close(void)
{
    jfree(g_font_buf);
}

int jfont_get_bitmap(int code, jfont_bitmap *bp, int h)
{
    if ((bp==NULL) || (h<=0))
        return ERROR_FAIL;

    bp->data = stbtt_GetCodepointBitmap(&g_font, 0,
        stbtt_ScaleForPixelHeight(&g_font, h), code, &(bp->w), &(bp->h), &(bp->x), &(bp->y));
    bp->y += bp->h;
    return SUCCESS;
}

void jfont_free_bitmap(jfont_bitmap *bp)
{
    if (bp == NULL)
        return;

    stbtt_FreeBitmap(bp->data, NULL);
}
