/*
 * jpic.c
 *
 * @chuanjiong
 */

#include "jpic.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

static jpic_frame *jpic_frame_alloc(int w, int h, int comp)
{
    jpic_frame *frame = (jpic_frame *)jmalloc(sizeof(jpic_frame));
    if (frame == NULL)
        return NULL;
    memset(frame, 0, sizeof(jpic_frame));

    int size = w * h * comp;
    if (size > 0)
    {
        frame->data = (uint8_t *)stbi__malloc(size);
        if (frame->data == NULL)
        {
            jfree(frame);
            return NULL;
        }
        memset(frame->data, 0, size);
        frame->width = w;
        frame->height = h;
        frame->comp = comp;
    }

    return frame;
}

int jpic_read(const char *url, jpic_frame **frame)
{
    if ((url==NULL) || (frame==NULL))
        return ERROR_FAIL;

    *frame = NULL;

    int w, h, n;
    unsigned char *data = stbi_load(url, &w, &h, &n, 0);
    if (data == NULL)
        return ERROR_FAIL;

    *frame = jpic_frame_alloc(0, 0, 0);
    if (*frame == NULL)
        return ERROR_FAIL;

    (*frame)->width = w;
    (*frame)->height = h;
    (*frame)->comp = n;
    (*frame)->data = data;

    return SUCCESS;
}

int jpic_write(const char *url, jpic_frame *frame)
{
    if ((url==NULL) || (frame==NULL))
        return ERROR_FAIL;

    int w, h, n;
    unsigned char *data;
    w = frame->width;
    h = frame->height;
    n = frame->comp;
    data = frame->data;

    jstring ext = jstring_file_ext(url);
    if (!jstring_compare(ext, "png"))
    {
        if (stbi_write_png(url, w, h, n, data, 0) != 1)
        {
            jstring_free(ext);
            return ERROR_FAIL;
        }
    }
    else if (!jstring_compare(ext, "bmp"))
    {
        if (stbi_write_bmp(url, w, h, n, data) != 1)
        {
            jstring_free(ext);
            return ERROR_FAIL;
        }
    }
    else if (!jstring_compare(ext, "jpg"))
    {
        if (stbi_write_jpg(url, w, h, n, data, 0) != 1)
        {
            jstring_free(ext);
            return ERROR_FAIL;
        }
    }
    else
    {
        jerr("[jpic] unknown file ext\n");
        jstring_free(ext);
        return ERROR_FAIL;
    }
    jstring_free(ext);
    return SUCCESS;
}

int jpic_resize(const jpic_frame *src, jpic_frame **dst, int dst_w, int dst_h)
{
    if ((src==NULL) || (dst==NULL) || (dst_w<=0) || (dst_h<=0))
        return ERROR_FAIL;

    *dst = jpic_frame_alloc(dst_w, dst_h, src->comp);
    if (*dst == NULL)
        return ERROR_FAIL;

    if (stbir_resize_uint8(src->data, src->width, src->height, 0, (*dst)->data, dst_w, dst_h, 0, src->comp) != 1)
    {
        jpic_frame_free(*dst);
        return ERROR_FAIL;
    }

    return SUCCESS;
}

void jpic_frame_free(jpic_frame *frame)
{
    if (frame == NULL)
        return;

    stbi_image_free(frame->data);

    jfree(frame);
}


