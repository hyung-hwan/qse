/*
 * $Id: hash.c,v 1.3 2006-01-30 14:34:47 bacon Exp $
 */

#include <xp/awk/hash.h>

// TODO: improve the entire hash routines.
//       support automatic bucket resizing and rehashing, etc.

static xp_size_t __hash (const xp_char_t* key);

#define FREE_PAIR(hash,pair) \
	do { \
		xp_free ((pair)->key); \
		if ((hash)->free_value != XP_NULL) \
			(hash)->free_value ((pair)->value); \
		xp_free (pair); \
	} while (0)

xp_awk_hash_t* xp_awk_hash_open (
	xp_awk_hash_t* hash, xp_size_t capa, void (*free_value) (void*))
{
	if (hash == XP_NULL) {
		hash = (xp_awk_hash_t*)	xp_malloc (xp_sizeof(xp_awk_hash_t));
		if (hash == XP_NULL) return XP_NULL;
		hash->__dynamic = xp_true;
	}
	else hash->__dynamic = xp_false;

	hash->buck = (xp_awk_pair_t**) xp_malloc (xp_sizeof(xp_awk_pair_t*) * capa);
	if (hash->buck == XP_NULL) {
		if (hash->__dynamic) xp_free (hash);
		return XP_NULL;	
	}

	hash->capa = capa;
	hash->size = 0;
	hash->free_value = free_value;
	while (capa > 0) hash->buck[--capa] = XP_NULL;

	return hash;
}

void xp_awk_hash_close (xp_awk_hash_t* hash)
{
	xp_awk_hash_clear (hash);
	xp_free (hash->buck);
	if (hash->__dynamic) xp_free (hash);
}

void xp_awk_hash_clear (xp_awk_hash_t* hash)
{
	xp_size_t i;
	xp_awk_pair_t* pair, * next;

	for (i = 0; i < hash->capa; i++) {
		pair = hash->buck[i];

		while (pair != XP_NULL) {
			next = pair->next;

			FREE_PAIR (hash, pair);
			hash->size--;

			pair = next;
		}

		hash->buck[i] = XP_NULL;
	}

	xp_assert (hash->size == 0);
}

xp_awk_pair_t* xp_awk_hash_get (xp_awk_hash_t* hash, xp_char_t* key)
{
	xp_awk_pair_t* pair;
	xp_size_t hc;

	hc = __hash(key) % hash->capa;
	pair = hash->buck[hc];

	while (pair != XP_NULL) {
		if (xp_strcmp(pair->key,key) == 0) return pair;
		pair = pair->next;
	}

	return XP_NULL;
}

xp_awk_pair_t* xp_awk_hash_put (xp_awk_hash_t* hash, xp_char_t* key, void* value)
{
	xp_awk_pair_t* pair;
	xp_size_t hc;

	hc = __hash(key) % hash->capa;
	pair = hash->buck[hc];

	while (pair != XP_NULL) {
		if (xp_strcmp(pair->key,key) == 0) {

			if (pair->key != key) {
				xp_free (pair->key);
				pair->key = key;
			}
			if (hash->free_value != XP_NULL) {
				hash->free_value (pair->value);
			}
			pair->value = value;

			return pair;
		}
		pair = pair->next;
	}

	pair = (xp_awk_pair_t*) xp_malloc (xp_sizeof(xp_awk_pair_t));
	if (pair == XP_NULL) return XP_NULL;

	pair->key = key;
	pair->value = value;
	pair->next = hash->buck[hc];
	hash->buck[hc] = pair;
	hash->size++;

	return pair;
}

xp_awk_pair_t* xp_awk_hash_set (xp_awk_hash_t* hash, xp_char_t* key, void* value)
{
	xp_awk_pair_t* pair;
	xp_size_t hc;

	hc = __hash(key) % hash->capa;
	pair = hash->buck[hc];

	while (pair != XP_NULL) {
		if (xp_strcmp(pair->key,key) == 0) {
			if (pair->key != key) {
				xp_free (pair->key);
				pair->key = key;
			}

			if (hash->free_value != XP_NULL) {
				hash->free_value (pair->value);
			}
			pair->value = value;

			return pair;
		}
		pair = pair->next;
	}

	return XP_NULL;
}

int xp_awk_hash_remove (xp_awk_hash_t* hash, xp_char_t* key)
{
	xp_awk_pair_t* pair, * prev;
	xp_size_t hc;

	hc = __hash(key) % hash->capa;
	pair = hash->buck[hc];
	prev = XP_NULL;

	while (pair != XP_NULL) {
		if (xp_strcmp(pair->key,key) == 0) {
			if (prev == XP_NULL) 
				hash->buck[hc] = pair->next;
			else prev->next = pair->next;

			FREE_PAIR (hash, pair);
			hash->size--;

			return 0;
		}

		prev = pair;
		pair = pair->next;
	}

	return -1;
}

int xp_awk_hash_walk (xp_awk_hash_t* hash, int (*walker) (xp_awk_pair_t*))
{
	xp_size_t i;
	xp_awk_pair_t* pair, * next;

	for (i = 0; i < hash->capa; i++) {
		pair = hash->buck[i];

		while (pair != XP_NULL) {
			next = pair->next;
			if (walker(pair) == -1) return -1;
			pair = next;
		}
	}

	return 0;
}

static xp_size_t __hash (const xp_char_t* key)
{
	xp_size_t n = 0, i;

	while (*key != XP_CHAR('\0')) {
		xp_byte_t* bp = (xp_byte_t*)key;
		for (i = 0; i < xp_sizeof(*key); i++) n = n * 31 + *bp++;
		key++;
	}	

	return n;
}
