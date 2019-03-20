#ifndef __TTPOD_TT_DICT_H_
#define __TTPOD_TT_DICT_H_

#include <stdint.h>

#include "ttVersion.h"

/**
 * @addtogroup lavu_dict TTDictionary
 * @ingroup lavu_data
 *
 * @brief Simple key:value store
 *
 * @{
 * Dictionaries are used for storing key:value pairs. To create
 * an TTDictionary, simply pass an address of a NULL pointer to
 * ttv_dict_set(). NULL can be used as an empty dictionary wherever
 * a pointer to an TTDictionary is required.
 * Use ttv_dict_get() to retrieve an entry or iterate over all
 * entries and finally ttv_dict_free() to free the dictionary
 * and all its contents.
 *
 @code
   TTDictionary *d = NULL;           // "create" an empty dictionary
   AVDictionaryEntry *t = NULL;

   ttv_dict_set(&d, "foo", "bar", 0); // add an entry

   char *k = ttv_strdup("key");       // if your strings are already allocated,
   char *v = ttv_strdup("value");     // you can avoid copying them like this
   ttv_dict_set(&d, k, v, TTV_DICT_DONT_STRDUP_KEY | TTV_DICT_DONT_STRDUP_VAL);

   while (t = ttv_dict_get(d, "", t, TTV_DICT_IGNORE_SUFFIX)) {
       <....>                             // iterate over all entries in d
   }
   ttv_dict_free(&d);
 @endcode
 *
 */

#define TTV_DICT_MATCH_CASE      1   /**< Only get an entry with exact-case key match. Only relevant in ttv_dict_get(). */
#define TTV_DICT_IGNORE_SUFFIX   2   /**< Return first entry in a dictionary whose first part corresponds to the search key,
                                         ignoring the suffix of the found key string. Only relevant in ttv_dict_get(). */
#define TTV_DICT_DONT_STRDUP_KEY 4   /**< Take ownership of a key that's been
                                         allocated with ttv_malloc() or another memory allocation function. */
#define TTV_DICT_DONT_STRDUP_VAL 8   /**< Take ownership of a value that's been
                                         allocated with ttv_malloc() or another memory allocation function. */
#define TTV_DICT_DONT_OVERWRITE 16   ///< Don't overwrite existing entries.
#define TTV_DICT_APPEND         32   /**< If the entry already exists, append to it.  Note that no
                                      delimiter is added, the strings are simply concatenated. */

typedef struct AVDictionaryEntry {
    char *key;
    char *value;
} AVDictionaryEntry;

typedef struct TTDictionary TTDictionary;

/**
 * Get a dictionary entry with matching key.
 *
 * The returned entry key or value must not be changed, or it will
 * cause undefined behavior.
 *
 * To iterate through all the dictionary entries, you can set the matching key
 * to the null string "" and set the TTV_DICT_IGNORE_SUFFIX flag.
 *
 * @param prev Set to the previous matching element to find the next.
 *             If set to NULL the first matching element is returned.
 * @param key matching key
 * @param flags a collection of TTV_DICT_* flags controlling how the entry is retrieved
 * @return found entry or NULL in case no matching entry was found in the dictionary
 */
AVDictionaryEntry *ttv_dict_get(TT_CONST_AVUTIL53 TTDictionary *m, const char *key,
                               const AVDictionaryEntry *prev, int flags);


/**
 * Set the given entry in *pm, overwriting an existing entry.
 *
 * Note: If TTV_DICT_DONT_STRDUP_KEY or TTV_DICT_DONT_STRDUP_VAL is set,
 * these arguments will be freed on error.
 *
 * @param pm pointer to a pointer to a dictionary struct. If *pm is NULL
 * a dictionary struct is allocated and put in *pm.
 * @param key entry key to add to *pm (will be ttv_strduped depending on flags)
 * @param value entry value to add to *pm (will be ttv_strduped depending on flags).
 *        Passing a NULL value will cause an existing entry to be deleted.
 * @return >= 0 on success otherwise an error code <0
 */
int ttv_dict_set(TTDictionary **pm, const char *key, const char *value, int flags);



/**
 * Copy entries from one TTDictionary struct into another.
 * @param dst pointer to a pointer to a TTDictionary struct. If *dst is NULL,
 *            this function will allocate a struct for you and put it in *dst
 * @param src pointer to source TTDictionary struct
 * @param flags flags to use when setting entries in *dst
 * @note metadata is read using the TTV_DICT_IGNORE_SUFFIX flag
 */
void ttv_dict_copy(TTDictionary **dst, TT_CONST_AVUTIL53 TTDictionary *src, int flags);

/**
 * Free all the memory allocated for an TTDictionary struct
 * and all keys and values.
 */
void ttv_dict_free(TTDictionary **m);

/**
 * @}
 */

#endif /* __TTPOD_TT_DICT_H_ */
