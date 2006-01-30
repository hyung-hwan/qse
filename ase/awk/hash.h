/*
 * $Id: hash.h,v 1.4 2006-01-30 14:34:47 bacon Exp $
 */

#ifndef _XP_AWK_HASH_H_
#define _XP_AWK_HASH_H_

#ifdef __STAND_ALONE
#include <xp/awk/sa.h>
#else
#include <xp/types.h>
#include <xp/macros.h>
#endif

typedef struct xp_awk_hash_t xp_awk_hash_t;
typedef struct xp_awk_pair_t xp_awk_pair_t;

struct xp_awk_pair_t
{
	xp_char_t* key;
	void* value;
	xp_awk_pair_t* next;
};

struct xp_awk_hash_t
{
	xp_size_t size;
	xp_size_t capa;
	xp_awk_pair_t** buck;
	void (*free_value) (void*);
	xp_bool_t __dynamic;
};

#ifdef __cplusplus
extern "C" {
#endif

xp_awk_hash_t* xp_awk_hash_open (
	xp_awk_hash_t* hash, xp_size_t capa, void (*free_value) (void*));
void xp_awk_hash_close (xp_awk_hash_t* hash);

void xp_awk_hash_clear (xp_awk_hash_t* hash);

xp_awk_pair_t* xp_awk_hash_get (xp_awk_hash_t* hash, xp_char_t* key);
xp_awk_pair_t* xp_awk_hash_put (xp_awk_hash_t* hash, xp_char_t* key, void* value);
xp_awk_pair_t* xp_awk_hash_set (xp_awk_hash_t* hash, xp_char_t* key, void* value);

int xp_awk_hash_remove (xp_awk_hash_t* hash, xp_char_t* key);
int xp_awk_hash_walk (xp_awk_hash_t* hash, int (*walker) (xp_awk_pair_t*));

#ifdef __cplusplus
}
#endif

#endif
