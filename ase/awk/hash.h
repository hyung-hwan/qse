/*
 * $Id: hash.h,v 1.2 2006-01-29 18:28:14 bacon Exp $
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
typedef struct xp_awk_hashdatum_t xp_awk_hashdatum_t;

struct xp_awk_hashdatum_t
{
	xp_char_t* key;
	void* value;
};

struct xp_awk_hash_t
{
	xp_size_t size;
	xp_size_t capa;
	xp_bool_t __dynamic;
};

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
}
#endif

#endif
