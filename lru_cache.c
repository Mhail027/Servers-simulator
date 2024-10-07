// Copyright Necula Mihail 313CAa 2023-2024
#include "lru_cache.h"

lru_cache_t *init_lru_cache(u_int cache_capacity)
{
	// Allocate memory for the cache's structure.
	lru_cache_t *cache = (lru_cache_t *)malloc(sizeof(lru_cache_t));
	DIE(cache == NULL, "malloc() failed\n");

	// Allocate memory for every complex field from structure
	cache->list_docs = cdll_create(sizeof(doc_t *));
	cache->ht_docs = ht_create(1117, hash_string, compare_function_strings,
					key_val_free_function);

	// Initialize the parameters of the cache.
	cache->size = 0;
	cache->max_size = cache_capacity;

	// Return the creted cache.
	return cache;
}

bool lru_cache_is_full(lru_cache_t *cache)
{
	if (cache->size == cache->max_size)
		return true;
	return false;
}

void free_lru_cache(lru_cache_t **c)
{
	// Get the cache's address.
	lru_cache_t *cache = *c;

	// Verify the cache.
	if (!cache || !cache->list_docs || !cache->ht_docs) {
		fprintf(stderr, "free_lru_cache() - cache isn't valid\n");
		return;
	}

	// Free the memory of the list.
	cdll_free(&cache->list_docs);
	// Freee the memory of the hashtable.
	ht_free(&cache->ht_docs);
	// Free the memory of the cache's structure.
	free(cache);

	// Lose the address of the cache, which
	// doesn't exist anymore.
	*c = NULL;
}

u_int lru_cache_has_key(lru_cache_t *cache, void *key)
{
	// Verify the parameters.
	if (!cache) {
		fprintf(stderr, "lru_cache_has_key() - the cache isn't valid\n");
		return 0;
	}
	if (!key) {
		fprintf(stderr, "lru_cache_has_key() - the key isn't valid\n");
		return 0;
	}

	// Verify if the key is in cache.
	if (ht_has_key(cache->ht_docs, key))
		return 1;
	return 0;
}
void lru_cache_put(lru_cache_t *cache, void *key, void *value,
				   void **evicted_key)
{
	// Verify the parameters.
	if (!cache || !key || !value || !evicted_key) {
		fprintf(stderr, "lru_cache_put() - at least one parameter isn't valid\n");
		return;
	}

	// If the cache is full and the received key isn't already
	// in cache, we must make place for it and to evacute / eliminate
	// the key which was used the least recently.
	if (!lru_cache_has_key(cache, key) && lru_cache_is_full(cache)) {
		// Find the least recent document.
		doc_t *file = *(doc_t **)cache->list_docs->head->data;
		// Save the key which will remove from cache.
		*evicted_key = (char *)malloc(strlen(file->name) + 1);
		DIE(*evicted_key == NULL, "malloc() failed\n");
		strcpy(*evicted_key, file->name);
		// Remove the key from cache.
		lru_cache_remove(cache, file->name);
	} else {
		*evicted_key = NULL;
	}

	// If the current key is already in the cache, we must remove it,
	// before to add it.
	if (lru_cache_has_key(cache, key))
		lru_cache_remove(cache, key);

	// Add the received pair key-value in the given cache.
	cdll_add_nth_node(cache->list_docs, cache->list_docs->size, value);
	dll_node_t *tail = cache->list_docs->head->prev;
	ht_put(cache->ht_docs, key, strlen((char *)key) + 1,  &tail,
		  sizeof(dll_node_t *));

	// Increment the numbers of elements from cache.
	cache->size++;
}

void *lru_cache_get(lru_cache_t *cache, void *key)
{
	// Verify the parameters.
	if (!cache) {
		fprintf(stderr, "lru_cache_remove() - the cache isn't valid\n");
		return NULL;
	}
	if (!cache->size) {
		fprintf(stderr, "lru_cache_remove() - the cache is empty\n");
		return NULL;
	}
	if (!key) {
		fprintf(stderr, "lru_cache_remove() - the key isn't valid\n");
		return NULL;
	}
	// Verify if the received key is in the cache.
	if (!lru_cache_has_key(cache, key))
		return NULL;

	// Find the node in which is stored the value
	// associated with the given key.
	dll_node_t *node = *(dll_node_t **)ht_get(cache->ht_docs, key);

	// Return the value assicated with the key.
	return *(doc_t **)node->data;
}

bool lru_cache_remove(lru_cache_t *cache, void *key)
{
	// Verify the parameters.
	if (!cache) {
		fprintf(stderr, "lru_cache_remove() - the cache isn't valid\n");
		return false;
	}
	if (!cache->size) {
		fprintf(stderr, "lru_cache_remove() - the cache is empty\n");
		return false;
	}
	if (!key) {
		fprintf(stderr, "lru_cache_remove() - the key isn't valid\n");
		return false;
	}

	// Find the node which will remove from cache's list.
	dll_node_t *node = *(dll_node_t **)ht_get(cache->ht_docs, key);

	// Remove the node from the hashtable.
	ht_remove_entry(cache->ht_docs, key);

	// Will remove the node manually from the list.

	// Breaks the links with its neighbors if has.
	if (cache->list_docs->size >= 1) {
		node->prev->next = node->next;
		node->next->prev = node->prev;
	}
	// Update the head if it's the case.
	if (node == cache->list_docs->head) {
		if (cache->list_docs->size >= 1)
			cache->list_docs->head = node->next;
		else
			cache->list_docs->head = NULL;
	}
	// Free the memory of the node.
	free_cdll_node(node);
	// Decrement the size of the list.
	--cache->list_docs->size;

	// Decrement the number of documents from the cache.
	cache->size--;

	// The work finished successfully.
	return true;
}
