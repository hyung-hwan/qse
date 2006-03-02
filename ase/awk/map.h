/*
 * $Id: map.h,v 1.1 2006-03-02 15:10:59 bacon Exp $
 */

#ifndef _XP_AWK_MAP_H_
#define _XP_AWK_MAP_H_

#ifdef __STAND_ALONE
#include <xp/awk/sa.h>
#else
#include <xp/types.h>
#include <xp/macros.h>
#endif

typedef struct xp_awk_map_t xp_awk_map_t;
typedef struct xp_awk_pair_t xp_awk_pair_t;

struct xp_awk_pair_t
{
	xp_char_t* key;
	void* value;
	xp_awk_pair_t* next;
};

struct xp_awk_map_t
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

xp_awk_map_t* xp_awk_map_open (
	xp_awk_map_t* map, xp_size_t capa, void (*free_value) (void*));
void xp_awk_map_close (xp_awk_map_t* map);

void xp_awk_map_clear (xp_awk_map_t* map);

xp_awk_pair_t* xp_awk_map_get (xp_awk_map_t* map, xp_char_t* key);
xp_awk_pair_t* xp_awk_map_put (xp_awk_map_t* map, xp_char_t* key, void* value);
xp_awk_pair_t* xp_awk_map_set (xp_awk_map_t* map, xp_char_t* key, void* value);

int xp_awk_map_remove (xp_awk_map_t* map, xp_char_t* key);
int xp_awk_map_walk (xp_awk_map_t* map, int (*walker) (xp_awk_pair_t*));

#ifdef __cplusplus
}
#endif

#endif
