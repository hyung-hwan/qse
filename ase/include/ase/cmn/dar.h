/*
 * $Id: dar.h 363 2008-09-04 10:58:08Z baconevi $
 *
 * {License}
 */

#ifndef _ASE_CMN_DAR_H_
#define _ASE_CMN_DAR_H_

#include <ase/types.h>
#include <ase/macros.h>

/****o* ase.cmn.dar/dynamic array
 * DESCRIPTION
 *  A dynamic array grows as more items are added.
 *
 *  #include <ase/cmn/dar.h>
 ******
 */

typedef struct ase_dar_t ase_dar_t;

/****s* ase.cmn.dar/ase_dar_t
 * NAME
 *  ase_dar_t - define a dynamic array
 *
 * SYNOPSIS
 */
struct ase_dar_t
{
	ase_mmgr_t*      mmgr;   /* memory manager */

	ase_sll_copier_t copier; /* data copier */
	ase_sll_freeer_t freeer; /* data freeer */
	ase_sll_comper_t comper; /* data comparator */
	ase_byte_t       scale;  /* scale factor */

	ase_size_t       size;   /* the number of items */
	ase_size_t       capa;   /* capacity */

	struct
	{
		void*      dptr;
		ase_size_t dlen;
	}* buf;
};
/******/

#ifdef __cplusplus
extern "C" {
#endif

/****f* ase.cmn.dar/ase_dar_open
 * NAME
 *  ase_dar_open - create a dynamic array
 *
 * SYNOPSIS
 */
ase_dar_t* ase_dar_open (
	ase_mmgr_t* dar,
	ase_size_t ext,
	ase_size_t capa
);
/******/

/****f* ase.cmn.dar/ase_dar_close
 * NAME
 *  ase_dar_close - destroy a dynamic array
 *
 * SYNOPSIS
 */
void ase_dar_close (
	ase_dar_t* dar
);
/******/

/****f* ase.cmn.dar/ase_dar_init
 * NAME
 *  ase_dar_init - initialize a dynamic array
 *
 * SYNOPSIS
 */
ase_dar_t* ase_dar_init (
	ase_dar_t* dar,
	ase_mmgr_t* mmgr,
	ase_size_t capa
);
/******/

/****f* ase.cmn.dar/ase_dar_fini
 * NAME
 *  ase_dar_fini - deinitialize a dynamic array
 *
 * SYNOPSIS
 */
void ase_dar_fini (
	ase_dar_t* dar
);
/******/

ase_size_t ase_dar_getsize (ase_dar_t* dar);
ase_size_t ase_dar_getcapa (ase_dar_t* dar);
ase_dar_t* ase_dar_setcapa (ase_dar_t* dar, ase_size_t capa);

ase_size_t ase_dar_insert (
	ase_dar_t* dar,
	ase_size_t index, 
	const ase_char_t* str,
	ase_size_t len
);

ase_size_t ase_dar_delete (
	ase_dar_t* dar,
	ase_size_t index,
	ase_size_t count
);

ase_size_t ase_dar_add (
	ase_dar_t* dar, const ase_char_t* str, ase_size_t len);
ase_size_t ase_dar_adduniq (
	ase_dar_t* dar, const ase_char_t* str, ase_size_t len);

ase_size_t ase_dar_find (
	ase_dar_t* dar, ase_size_t index,
	const ase_char_t* str, ase_size_t len);
ase_size_t ase_dar_rfind (
	ase_dar_t* dar, ase_size_t index,
	const ase_char_t* str, ase_size_t len);
ase_size_t ase_dar_rrfind (
	ase_dar_t* dar, ase_size_t index,
	const ase_char_t* str, ase_size_t len);

ase_size_t ase_dar_findx (
	ase_dar_t* dar, ase_size_t index,
	const ase_char_t* str, ase_size_t len, 
	void(*transform)(ase_size_t, ase_cstr_t*,void*), void* arg);
ase_size_t ase_dar_rfindx (
	ase_dar_t* dar, ase_size_t index,
	const ase_char_t* str, ase_size_t len, 
	void(*transform)(ase_size_t, ase_cstr_t*,void*), void* arg);
ase_size_t ase_dar_rrfindx (
	ase_dar_t* dar, ase_size_t index,
	const ase_char_t* str, ase_size_t len, 
	void(*transform)(ase_size_t, ase_cstr_t*,void*), void* arg);

void ase_dar_clear (ase_dar_t* dar);

#ifdef __cplusplus
}
#endif

#endif
