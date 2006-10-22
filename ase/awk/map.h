/*
 * $Id: map.h,v 1.16 2006-10-22 11:34:53 bacon Exp $
 */

#ifndef _SSE_AWK_MAP_H_
#define _SSE_AWK_MAP_H_

#ifndef _SSE_AWK_AWK_H_
#error Never include this file directly. Include <sse/awk/awk.h> instead
#endif

typedef struct sse_awk_map_t sse_awk_map_t;
typedef struct sse_awk_pair_t sse_awk_pair_t;

struct sse_awk_pair_t
{
	sse_char_t* key;
	sse_size_t key_len;
	void* val;
	sse_awk_pair_t* next;
};

struct sse_awk_map_t
{
	void* owner;
	sse_size_t size;
	sse_size_t capa;
	sse_awk_pair_t** buck;
	void (*freeval) (void*,void*);
	sse_awk_t* awk;
	sse_bool_t __dynamic;
};

#ifdef __cplusplus
extern "C" {
#endif

sse_awk_map_t* sse_awk_map_open (
	sse_awk_map_t* map, void* owner, sse_size_t capa, 
	void(*freeval)(void*,void*), sse_awk_t* awk);

void sse_awk_map_close (sse_awk_map_t* map);

void sse_awk_map_clear (sse_awk_map_t* map);

sse_awk_pair_t* sse_awk_map_get (
	sse_awk_map_t* map, const sse_char_t* key, sse_size_t key_len);

sse_awk_pair_t* sse_awk_map_put (
	sse_awk_map_t* map, sse_char_t* key, sse_size_t key_len, void* val);

int sse_awk_map_putx (
	sse_awk_map_t* map, sse_char_t* key, sse_size_t key_len,
	void* val, sse_awk_pair_t** px);

sse_awk_pair_t* sse_awk_map_set (
	sse_awk_map_t* map, sse_char_t* key, sse_size_t key_len, void* val);

sse_awk_pair_t* sse_awk_map_getpair (
	sse_awk_map_t* map, const sse_char_t* key, sse_size_t key_len, void** val);

sse_awk_pair_t* sse_awk_map_setpair (
	sse_awk_map_t* map, sse_awk_pair_t* pair, void* val);

int sse_awk_map_remove (sse_awk_map_t* map, sse_char_t* key, sse_size_t key_len);

int sse_awk_map_walk (sse_awk_map_t* map, 
	int (*walker)(sse_awk_pair_t*,void*), void* arg);

#ifdef __cplusplus
}
#endif

#endif
