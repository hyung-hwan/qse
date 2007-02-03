/*
 * $Id: map.h,v 1.19 2007-02-03 10:47:41 bacon Exp $
 *
 * {License}
 */

#ifndef _ASE_AWK_MAP_H_
#define _ASE_AWK_MAP_H_

#ifndef _ASE_AWK_AWK_H_
#error Never include this file directly. Include <ase/awk/awk.h> instead
#endif

typedef struct ase_awk_map_t ase_awk_map_t;
typedef struct ase_awk_pair_t ase_awk_pair_t;

struct ase_awk_pair_t
{
	ase_char_t* key;
	ase_size_t key_len;
	void* val;
	ase_awk_pair_t* next;
};

struct ase_awk_map_t
{
	void* owner;
	ase_size_t size;
	ase_size_t capa;
	ase_awk_pair_t** buck;
	void (*freeval) (void*,void*);
	ase_awk_t* awk;
	ase_bool_t __dynamic;
};

#ifdef __cplusplus
extern "C" {
#endif

ase_awk_map_t* ase_awk_map_open (
	ase_awk_map_t* map, void* owner, ase_size_t capa, 
	void(*freeval)(void*,void*), ase_awk_t* awk);

void ase_awk_map_close (ase_awk_map_t* map);

void ase_awk_map_clear (ase_awk_map_t* map);

ase_size_t ase_awk_map_getsize (ase_awk_map_t* map);

ase_awk_pair_t* ase_awk_map_get (
	ase_awk_map_t* map, const ase_char_t* key, ase_size_t key_len);

ase_awk_pair_t* ase_awk_map_put (
	ase_awk_map_t* map, ase_char_t* key, ase_size_t key_len, void* val);

int ase_awk_map_putx (
	ase_awk_map_t* map, ase_char_t* key, ase_size_t key_len,
	void* val, ase_awk_pair_t** px);

ase_awk_pair_t* ase_awk_map_set (
	ase_awk_map_t* map, ase_char_t* key, ase_size_t key_len, void* val);

ase_awk_pair_t* ase_awk_map_getpair (
	ase_awk_map_t* map, const ase_char_t* key, ase_size_t key_len, void** val);

ase_awk_pair_t* ase_awk_map_setpair (
	ase_awk_map_t* map, ase_awk_pair_t* pair, void* val);

int ase_awk_map_remove (ase_awk_map_t* map, ase_char_t* key, ase_size_t key_len);

int ase_awk_map_walk (ase_awk_map_t* map, 
	int (*walker)(ase_awk_pair_t*,void*), void* arg);

#ifdef __cplusplus
}
#endif

#endif
