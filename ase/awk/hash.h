/*
 * $Id: hash.h,v 1.3 2006-01-30 13:25:26 bacon Exp $
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
	const xp_char_t* key;
	void* value;
	xp_awk_pair_t* next;
};

struct xp_awk_hash_t
{
	xp_size_t size;
	xp_size_t capa;
	xp_awk_pair_t** buck;
	xp_bool_t __dynamic;
};

#ifdef __cplusplus
extern "C" {
#endif

xp_awk_hash_t* xp_awk_hash_open (xp_awk_hash_t* hash, xp_size_t capa);
void xp_awk_hash_close (xp_awk_hash_t* hash);

void xp_awk_hash_clear (xp_awk_hash_t* hash);

xp_awk_pair_t* xp_awk_hash_get (xp_awk_hash_t* hash, const xp_char_t* key);
xp_awk_pair_t* xp_awk_hash_put (xp_awk_hash_t* hash, const xp_char_t* key, void* value);
xp_awk_pair_t* xp_awk_hash_set (xp_awk_hash_t* hash, const xp_char_t* key, void* value);

int xp_awk_hash_remove (xp_awk_hash_t* hash, const xp_char_t* key);

#ifdef __cplusplus
}
#endif

#endif
