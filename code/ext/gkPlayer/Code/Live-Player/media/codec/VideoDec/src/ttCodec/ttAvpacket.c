#include <string.h>

#include "ttAvassert.h"
#include "ttInternal.h"
#include "ttMem.h"
#include "ttIntreadwrite.h"
#include "ttBSwap.h"

void tt_init_packet(TTPacket *pkt)
{
    pkt->pts                  = TTV_NOPTS_VALUE;
    pkt->dts                  = TTV_NOPTS_VALUE;
    pkt->pos                  = -1;
    pkt->duration             = 0;
    pkt->convergence_duration = 0;
    pkt->flags                = 0;
    pkt->stream_index         = 0;
#if TT_API_DESTRUCT_PACKET
TT_DISABLE_DEPRECATION_WARNINGS
    pkt->destruct             = NULL;
TT_ENABLE_DEPRECATION_WARNINGS
#endif
    pkt->buf                  = NULL;
    pkt->side_data            = NULL;
    pkt->side_data_elems      = 0;
}

static int packet_alloc(TTBufferRef **buf, int size)
{
    int ret;
    if ((unsigned)size >= (unsigned)size + TT_INPUT_BUFFER_PADDING_SIZE)
        return AVERROR(EINVAL);

    ret = ttv_buffer_realloc(buf, size + TT_INPUT_BUFFER_PADDING_SIZE);
    if (ret < 0)
        return ret;

    memset((*buf)->data + size, 0, TT_INPUT_BUFFER_PADDING_SIZE);

    return 0;
}

#define ALLOC_MALLOC(data, size) data = ttv_malloc(size)


#define DUP_DATA(dst, src, size, padding, ALLOC)                        \
    do {                                                                \
        void *data;                                                     \
        if (padding) {                                                  \
            if ((unsigned)(size) >                                      \
                (unsigned)(size) + TT_INPUT_BUFFER_PADDING_SIZE)        \
                goto failed_alloc;                                      \
            ALLOC(data, size + TT_INPUT_BUFFER_PADDING_SIZE);           \
        } else {                                                        \
            ALLOC(data, size);                                          \
        }                                                               \
        if (!data)                                                      \
            goto failed_alloc;                                          \
        memcpy(data, src, size);                                        \
        if (padding)                                                    \
            memset((uint8_t *)data + size, 0,                           \
                   TT_INPUT_BUFFER_PADDING_SIZE);                       \
        dst = data;                                                     \
    } while (0)





void ttv_packet_free_side_data(TTPacket *pkt)
{
    int i;
    for (i = 0; i < pkt->side_data_elems; i++)
        ttv_freep(&pkt->side_data[i].data);
    ttv_freep(&pkt->side_data);
    pkt->side_data_elems = 0;
}

void tt_free_packet(TTPacket *pkt)
{
    if (pkt) {
TT_DISABLE_DEPRECATION_WARNINGS
        if (pkt->buf)
            ttv_buffer_unref(&pkt->buf);
#if TT_API_DESTRUCT_PACKET
        else if (pkt->destruct)
            pkt->destruct(pkt);
        pkt->destruct = NULL;
#endif
TT_ENABLE_DEPRECATION_WARNINGS
        pkt->data            = NULL;
        pkt->size            = 0;

        ttv_packet_free_side_data(pkt);
    }
}

uint8_t *ttv_packet_new_side_data(TTPacket *pkt, enum TTPacketSideDataType type,
                                 int size)
{
    int elems = pkt->side_data_elems;

    if ((unsigned)elems + 1 > INT_MAX / sizeof(*pkt->side_data))
        return NULL;
    if ((unsigned)size > INT_MAX - TT_INPUT_BUFFER_PADDING_SIZE)
        return NULL;

    pkt->side_data = ttv_realloc(pkt->side_data,
                                (elems + 1) * sizeof(*pkt->side_data));
    if (!pkt->side_data)
        return NULL;

    pkt->side_data[elems].data = ttv_mallocz(size + TT_INPUT_BUFFER_PADDING_SIZE);
    if (!pkt->side_data[elems].data)
        return NULL;
    pkt->side_data[elems].size = size;
    pkt->side_data[elems].type = type;
    pkt->side_data_elems++;

    return pkt->side_data[elems].data;
}

uint8_t *ttv_packet_get_side_data(TTPacket *pkt, enum TTPacketSideDataType type,
                                 int *size)
{
    int i;

    for (i = 0; i < pkt->side_data_elems; i++) {
        if (pkt->side_data[i].type == type) {
            if (size)
                *size = pkt->side_data[i].size;
            return pkt->side_data[i].data;
        }
    }
    return NULL;
}

#define TT_MERGE_MARKER 0x8c4d9d108e25e9feULL



int ttv_packet_split_side_data(TTPacket *pkt){
    if (!pkt->side_data_elems && pkt->size >12 && TTV_RB64(pkt->data + pkt->size - 8) == TT_MERGE_MARKER){
        int i;
        unsigned int size;
        uint8_t *p;

        p = pkt->data + pkt->size - 8 - 5;
        for (i=1; ; i++){
            size = TTV_RB32(p);
            if (size>INT_MAX || p - pkt->data < size)
                return 0;
            if (p[4]&128)
                break;
            p-= size+5;
        }

        pkt->side_data = ttv_malloc_array(i, sizeof(*pkt->side_data));
        if (!pkt->side_data)
            return AVERROR(ENOMEM);

        p= pkt->data + pkt->size - 8 - 5;
        for (i=0; ; i++){
            size= TTV_RB32(p);
            ttv_assert0(size<=INT_MAX && p - pkt->data >= size);
            pkt->side_data[i].data = ttv_mallocz(size + TT_INPUT_BUFFER_PADDING_SIZE);
            pkt->side_data[i].size = size;
            pkt->side_data[i].type = p[4]&127;
            if (!pkt->side_data[i].data)
                return AVERROR(ENOMEM);
            memcpy(pkt->side_data[i].data, p-size, size);
            pkt->size -= size + 5;
            if(p[4]&128)
                break;
            p-= size+5;
        }
        pkt->size -= 8;
        pkt->side_data_elems = i+1;
        return 1;
    }
    return 0;
}



int ttv_packet_unpack_dictionary(const uint8_t *data, int size, TTDictionary **dict)
{
    const uint8_t *end = data + size;
    int ret = 0;

    if (!dict || !data || !size)
        return ret;
    if (size && end[-1])
        return TTERROR_INVALIDDATA;
    while (data < end) {
        const uint8_t *key = data;
        const uint8_t *val = data + strlen(key) + 1;

        if (val >= end)
            return TTERROR_INVALIDDATA;

        ret = ttv_dict_set(dict, key, val, 0);
        if (ret < 0)
            break;
        data = val + strlen(val) + 1;
    }

    return ret;
}



int ttv_packet_copy_props(TTPacket *dst, const TTPacket *src)
{
    int i;

    dst->pts                  = src->pts;
    dst->dts                  = src->dts;
    dst->pos                  = src->pos;
    dst->duration             = src->duration;
    dst->convergence_duration = src->convergence_duration;
    dst->flags                = src->flags;
    dst->stream_index         = src->stream_index;

    for (i = 0; i < src->side_data_elems; i++) {
         enum TTPacketSideDataType type = src->side_data[i].type;
         int size          = src->side_data[i].size;
         uint8_t *src_data = src->side_data[i].data;
         uint8_t *dst_data = ttv_packet_new_side_data(dst, type, size);

        if (!dst_data) {
            ttv_packet_free_side_data(dst);
            return AVERROR(ENOMEM);
        }
        memcpy(dst_data, src_data, size);
    }

    return 0;
}

void ttv_packet_unref(TTPacket *pkt)
{
    ttv_packet_free_side_data(pkt);
    ttv_buffer_unref(&pkt->buf);
    tt_init_packet(pkt);
    pkt->data = NULL;
    pkt->size = 0;
}

int ttv_packet_ref(TTPacket *dst, const TTPacket *src)
{
    int ret;

    ret = ttv_packet_copy_props(dst, src);
    if (ret < 0)
        return ret;

    if (!src->buf) {
        ret = packet_alloc(&dst->buf, src->size);
        if (ret < 0)
            goto fail;
        memcpy(dst->buf->data, src->data, src->size);
    } else
        dst->buf = ttv_buffer_ref(src->buf);

    dst->size = src->size;
    dst->data = dst->buf->data;
    return 0;
fail:
    ttv_packet_free_side_data(dst);
    return ret;
}

