#include <string.h>

#include "ttDict.h"
#include "ttInternal.h"
#include "ttMem.h"

struct TTDictionary {
    int count;
    AVDictionaryEntry *elems;
};

size_t ttv_strlcpy(char *dst, const char *src, size_t size)
{
	size_t len = 0;
	while (++len < size && *src)
		*dst++ = *src++;
	if (len <= size)
		*dst = 0;
	return len + strlen(src) - 1;
}

size_t ttv_strlcat(char *dst, const char *src, size_t size)
{
	size_t len = strlen(dst);
	if (size <= len + 1)
		return len + strlen(src);
	return len + ttv_strlcpy(dst + len, src, size - len);
}

static inline ttv_const int ttv_toupper(int c)
{
	if (c >= 'a' && c <= 'z')
		c ^= 0x20;
	return c;
}

AVDictionaryEntry *ttv_dict_get(TT_CONST_AVUTIL53 TTDictionary *m, const char *key,
                               const AVDictionaryEntry *prev, int flags)
{
    unsigned int i, j;

    if (!m)
        return NULL;

    if (prev)
        i = prev - m->elems + 1;
    else
        i = 0;

    for (; i < m->count; i++) {
        const char *s = m->elems[i].key;
        if (flags & TTV_DICT_MATCH_CASE)
            for (j = 0; s[j] == key[j] && key[j]; j++)
                ;
        else
            for (j = 0; ttv_toupper(s[j]) == ttv_toupper(key[j]) && key[j]; j++)
                ;
        if (key[j])
            continue;
        if (s[j] && !(flags & TTV_DICT_IGNORE_SUFFIX))
            continue;
        return &m->elems[i];
    }
    return NULL;
}

int ttv_dict_set(TTDictionary **pm, const char *key, const char *value,
                int flags)
{
    TTDictionary *m = *pm;
    AVDictionaryEntry *tag = ttv_dict_get(m, key, NULL, flags);
    char *oldval = NULL;

    if (!m)
        m = *pm = ttv_mallocz(sizeof(*m));

    if (tag) {
        if (flags & TTV_DICT_DONT_OVERWRITE) {
            if (flags & TTV_DICT_DONT_STRDUP_KEY) ttv_free((void*)key);
            if (flags & TTV_DICT_DONT_STRDUP_VAL) ttv_free((void*)value);
            return 0;
        }
        if (flags & TTV_DICT_APPEND)
            oldval = tag->value;
        else
            ttv_free(tag->value);
        ttv_free(tag->key);
        *tag = m->elems[--m->count];
    } else {
        AVDictionaryEntry *tmp = ttv_realloc(m->elems,
                                            (m->count + 1) * sizeof(*m->elems));
        if (!tmp)
            goto err_out;
        m->elems = tmp;
    }
    if (value) {
        if (flags & TTV_DICT_DONT_STRDUP_KEY)
            m->elems[m->count].key = (char*)(intptr_t)key;
        else
            m->elems[m->count].key = ttv_strdup(key);
        if (flags & TTV_DICT_DONT_STRDUP_VAL) {
            m->elems[m->count].value = (char*)(intptr_t)value;
        } else if (oldval && flags & TTV_DICT_APPEND) {
            int len = strlen(oldval) + strlen(value) + 1;
            char *newval = ttv_mallocz(len);
            if (!newval)
                goto err_out;
            ttv_strlcat(newval, oldval, len);
            ttv_freep(&oldval);
            ttv_strlcat(newval, value, len);
            m->elems[m->count].value = newval;
        } else
            m->elems[m->count].value = ttv_strdup(value);
        m->count++;
    }
    if (!m->count) {
        ttv_free(m->elems);
        ttv_freep(pm);
    }

    return 0;

err_out:
    if (!m->count) {
        ttv_free(m->elems);
        ttv_freep(pm);
    }
    if (flags & TTV_DICT_DONT_STRDUP_KEY) ttv_free((void*)key);
    if (flags & TTV_DICT_DONT_STRDUP_VAL) ttv_free((void*)value);
    return AVERROR(ENOMEM);
}



void ttv_dict_free(TTDictionary **pm)
{
    TTDictionary *m = *pm;

    if (m) {
        while (m->count--) {
            ttv_free(m->elems[m->count].key);
            ttv_free(m->elems[m->count].value);
        }
        ttv_free(m->elems);
    }
    ttv_freep(pm);
}

void ttv_dict_copy(TTDictionary **dst, TT_CONST_AVUTIL53 TTDictionary *src, int flags)
{
    AVDictionaryEntry *t = NULL;

    while ((t = ttv_dict_get(src, "", t, TTV_DICT_IGNORE_SUFFIX)))
        ttv_dict_set(dst, t->key, t->value, flags);
}
