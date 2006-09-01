/*
 * $Id: map.h,v 1.15 2006-09-01 03:44:16 bacon Exp $
 */

#ifndef _XP_AWK_MAP_H_
#define _XP_AWK_MAP_H_

#ifndef _XP_AWK_AWK_H_
#error Never include this file directly. Include <xp/awk/awk.h> instead
#endif

typedef struct xp_awk_map_t xp_awk_map_t;
typedef struct xp_awk_pair_t xp_awk_pair_t;

struct xp_awk_pair_t
{
	xp_char_t* key;
	xp_size_t key_len;
	void* val;
	xp_awk_pair_t* next;
};

struct xp_awk_map_t
{
	void* owner;
	xp_size_t size;
	xp_size_t capa;
	xp_awk_pair_t** buck;
	void (*freeval) (void*,void*);
	xp_awk_t* awk;
	xp_bool_t __dynamic;
};

#ifdef __cplusplus
extern "C" {
#endif

xp_awk_map_t* xp_awk_map_open (
	xp_awk_map_t* map, void* owner, xp_size_t capa, 
	void(*freeval)(void*,void*), xp_awk_t* awk);

void xp_awk_map_close (xp_awk_map_t* map);

void xp_awk_map_clear (xp_awk_map_t* map);

xp_awk_pair_t* xp_awk_map_get (
	xp_awk_map_t* map, const xp_char_t* key, xp_size_t key_len);

xp_awk_pair_t* xp_awk_map_put (
	xp_awk_map_t* map, xp_char_t* key, xp_size_t key_len, void* val);

int xp_awk_map_putx (
	xp_awk_map_t* map, xp_char_t* key, xp_size_t key_len,
	void* val, xp_awk_pair_t** px);

xp_awk_pair_t* xp_awk_map_set (
	xp_awk_map_t* map, xp_char_t* key, xp_size_t key_len, void* val);

xp_awk_pair_t* xp_awk_map_getpair (
	xp_awk_map_t* map, const xp_char_t* key, xp_size_t key_len, void** val);

xp_awk_pair_t* xp_awk_map_setpair (
	xp_awk_map_t* map, xp_awk_pair_t* pair, void* val);

int xp_awk_map_remove (xp_awk_map_t* map, xp_char_t* key, xp_size_t key_len);

int xp_awk_map_walk (xp_awk_map_t* map, 
	int (*walker)(xp_awk_pair_t*,void*), void* arg);

#ifdef __cplusplus
}
#endif

#endif
