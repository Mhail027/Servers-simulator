// Copyright Necula Mihail 313CAa 2023-2024
#ifndef LRU_CACHE_H
#define LRU_CACHE_H

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "list.h"
#include "hash_map.h"
#include "utils.h"

/******************************
 * Structure to save the informtions
 * of a document.
*******************************/
typedef struct doc_t{
    /* Name of the document. */
    char *name;
    /* Content of the document. */
    char *content;
} doc_t;

/******************************
 * LRU CACHE is implemented using 1 circular
 * doubly linking list and 1 hashmap.
 * The pair key-value (name of document - 
 * pointer to the entire document) is saved using
 * those 2 structures.
 * value -> is saved in a node from list
 * key with the address of the node in which is stored
 * the value -> are saved in pair in hashtable
 * The name of docs from list should be stored in the
 * order of their use, so:
 * head - the least recent document used
 * tail - the most recent document used
*******************************/
typedef struct lru_cache_t {
    /* The list with the values. */
    cdll_t *list_docs;
    /* The hashtable with the keys. */
    hashtable_t *ht_docs;
    /* The current number of elements from cache. */
    u_int size;
    /* The maximum number of elements from cache. */
    u_int max_size;
} lru_cache_t;


/******************************
 * init_lru_cache() - Create and initialize a cache.
 *
 * @param cache_capacity: The maximum size of the cache. / The 
 *      maximum number of elements which can be stored in cache.
 *
 * @return - The created cache.
*******************************/
lru_cache_t *init_lru_cache(unsigned int cache_capacity);

/******************************
 * lru_cache_is_full() - Verify if the cache is full.
 *
 * @param cache: Cache which will be verified.
 *
 * @return - TRUE if the cache is full,
 *           FALSE if isn't.
*******************************/
bool lru_cache_is_full(lru_cache_t *cache);

/******************************
 * free_lru_cache() - Free the memory allocated for a cache.
 * 
 * @param cache: Addreess which points at the cache's address.
*******************************/
void free_lru_cache(lru_cache_t **cache);

/******************************
* lru_cache_has_key() - Verify if a key / doc is in a cache.
*
* @param cache: Cache with which we work.
* @param key: The key which will be searched.
*
* @return 1, if the key is in the cache
*         0, if isn't
*******************************/
u_int lru_cache_has_key(lru_cache_t *cache, void *key);

/******************************
 * lru_cache_put() - Adds a new pair in our cache.
 * 
 * @param cache: Cache where the key-value pair will be stored.
 * @param key: Key of the pair.
 * @param value: Value of the pair.
 * @param evicted_key: The function will RETURN via this parameter the
 *      key removed from cache if the cache was full.
*******************************/
void lru_cache_put(lru_cache_t *cache, void *key, void *value,
                   void **evicted_key);

/******************************
 * lru_cache_get() - Retrieves the value associated with a key.
 * 
 * @param cache: Cache where the key-value pair is stored.
 * @param key: Key of the pair.
 * 
 * @return - The value associated with the key,
 *           or NULL if the key is not found.
*******************************/
void *lru_cache_get(lru_cache_t *cache, void *key);

/******************************
 * lru_cache_remove() - Removes a key-value pair from the cache.
 * 
 * @param cache: Cache where the key-value pair is stored.
 * @param key: Key of the pair.
 *
 * @return - TRUE if the remove operation can take place,
 *           FALSE if can not take place.
*******************************/
bool lru_cache_remove(lru_cache_t *cache, void *key);

#endif
