/*
 * $Id: lda.h 363 2008-09-04 10:58:08Z baconevi $
 *
 * {License}
 */

#ifndef _ASE_CMN_LDA_H_
#define _ASE_CMN_LDA_H_

#include <ase/types.h>
#include <ase/macros.h>

/****o* ase.cmn.lda/linear dynamic array
 * DESCRIPTION
 *  <ase/cmn/lda.h> provides a linear dynamic array. It grows as more items 
 *  are added. T
 *
 *  #include <ase/cmn/lda.h>
 ******
 */

typedef struct ase_lda_t ase_lda_t;
typedef struct ase_lda_cell_t ase_lda_cell_t;

#define ASE_LDA_COPIER_INLINE  ase_lda_copyinline

#define ASE_LDA_INVALID ((ase_size_t)-1)

#define ASE_LDA_SIZE(lda)       ((lda)->size)
#define ASE_LDA_CAPA(lda)       ((lda)->capa)
#define ASE_LDA_CELL(lda,index) ((lda)->cell[index])

#define ASE_LDA_DPTR(cell)      ((cell)->dptr)
#define ASE_LDA_DLEN(cell)      ((cell)->dlen)

#define ASE_LDA_MMGR(lda)       ((lda)->mmgr)
#define ASE_LDA_COPIER(lda)     ((lda)->copier)
#define ASE_LDA_FREEER(lda)     ((lda)->freeer)
#define ASE_LDA_COMPER(lda)     ((lda)->comper)


/****b* ase.cmn.lda/ase_lda_copier_t
 * NAME
 *  ase_lda_copier_t - define a node contruction callback
 *
 * DESCRIPTION
 *  The ase_lda_copier_t defines a callback function for node construction.
 *  A node is contructed when a user adds data to a list. The user can
 *  define how the data to add can be maintained in the list. A singly
 *  linked list not specified with any copiers stores the data pointer and
 *  the data length into a node. A special copier ASE_LDA_COPIER_INLINE copies 
 *  the contents of the data a user provided into the node. You can use the
 *  ase_lda_setcopier() function to change the copier. 
 * 
 *  A copier should return the pointer to the copied data. If it fails to copy
 *  data, it may return ASE_NULL. You need to set a proper freeer to free up
 *  memory allocated for copy.
 *
 * SEE ALSO
 *  ase_lda_setcopier, ase_lda_getcopier, ASE_LDA_COPIER
 *
 * SYNOPSIS
 */
typedef void* (*ase_lda_copier_t) (
	ase_lda_t* lda    /* a map */,
	void*      dptr   /* the pointer to data to copy */,
	ase_size_t dlen   /* the length of data to copy */
);
/******/

/****b* ase.cmn.lda/ase_lda_freeer_t
 * NAME
 *  ase_lda_freeer_t - define a node destruction callback
 * SYNOPSIS
 */
typedef void (*ase_lda_freeer_t) (
	ase_lda_t* lda    /* a map */,
	void*      dptr   /* the pointer to data to free */,
	ase_size_t dlen   /* the length of data to free */
);
/******/

/****t* ase.cmn.lda/ase_lda_comper_t
 * NAME
 *  ase_lda_comper_t - define a data comparator
 *
 * DESCRIPTION
 *  The ase_lda_comper_t type defines a key comparator that is called when
 *  the list needs to compare data. A linear dynamic array is created with a
 *  default comparator that performs bitwise comparison.
 *
 *  The comparator should return 0 if the data are the same and a non-zero
 *  integer otherwise.
 *
 * SYNOPSIS
 */
typedef int (*ase_lda_comper_t) (
	ase_lda_t*  lda    /* a linear dynamic array */, 
	const void* dptr1  /* a data pointer */,
	ase_size_t  dlen1  /* a data length */,
	const void* dptr2  /* a data pointer */,
	ase_size_t  dlen2  /* a data length */
);
/******/

/****s* ase.cmn.lda/ase_lda_t
 * NAME
 *  ase_lda_t - define a linear dynamic array
 *
 * SYNOPSIS
 */
struct ase_lda_t
{
	ase_mmgr_t*      mmgr;   /* memory manager */

	ase_lda_copier_t copier; /* data copier */
	ase_lda_freeer_t freeer; /* data freeer */
	ase_lda_comper_t comper; /* data comparator */
	ase_byte_t       scale;  /* scale factor */

	ase_size_t       size;   /* the number of items */
	ase_size_t       capa;   /* capacity */

	ase_lda_cell_t** cell;
};
/******/

/****s*
 * NAME
 *  ase_lda_cell_t - define a linear dynamic array cell
 *
 * SYNOPSIS
 */
struct ase_lda_cell_t
{
	void*      dptr;
	ase_size_t dlen;
};
/******/

#ifdef __cplusplus
extern "C" {
#endif

/****f* ase.cmn.lda/ase_lda_open
 * NAME
 *  ase_lda_open - create a linear dynamic array
 *
 * SYNOPSIS
 */
ase_lda_t* ase_lda_open (
	ase_mmgr_t* lda,
	ase_size_t ext,
	ase_size_t capa
);
/******/

/****f* ase.cmn.lda/ase_lda_close
 * NAME
 *  ase_lda_close - destroy a linear dynamic array
 *
 * SYNOPSIS
 */
void ase_lda_close (
	ase_lda_t* lda
);
/******/

/****f* ase.cmn.lda/ase_lda_init
 * NAME
 *  ase_lda_init - initialize a linear dynamic array
 *
 * SYNOPSIS
 */
ase_lda_t* ase_lda_init (
	ase_lda_t* lda,
	ase_mmgr_t* mmgr,
	ase_size_t capa
);
/******/

/****f* ase.cmn.lda/ase_lda_fini
 * NAME
 *  ase_lda_fini - deinitialize a linear dynamic array
 *
 * SYNOPSIS
 */
void ase_lda_fini (
	ase_lda_t* lda
);
/******/

/****f* ase.cmn.lda/ase_lda_getextension
 * NAME
 *  ase_lda_getextension - get the pointer to the extension
 *
 * DESCRIPTION
 *  The ase_lda_getextension() function returns the pointer to the extension.
 *
 * SYNOPSIS
 */
void* ase_lda_getextension (
	ase_lda_t* lda  /* a linear dynamic array */
);
/******/

/****f* ase.cmn.lda/ase_lda_getmmgr
 * NAME
 *  ase_lda_getmmgr - get the memory manager
 *
 * SYNOPSIS
 */
ase_mmgr_t* ase_lda_getmmgr (
	ase_lda_t* lda  /* a linear dynamic array */
);
/******/

/****f* ase.cmn.lda/ase_lda_setmmgr
 * NAME
 *  ase_lda_setmmgr - set the memory manager
 *
 * SYNOPSIS
 */
void ase_lda_setmmgr (
	ase_lda_t*  lda   /* a linear dynamic array */,
	ase_mmgr_t* mmgr  /* a memory manager */
);
/******/

ase_size_t ase_lda_getsize (
	ase_lda_t* lda
);

ase_size_t ase_lda_getcapa (
	ase_lda_t* lda
);

ase_lda_t* ase_lda_setcapa (
	ase_lda_t* lda,
	ase_size_t capa
);

ase_size_t ase_lda_insert (
	ase_lda_t* lda,
	ase_size_t index, 
	void* dptr,
	ase_size_t dlen
);

ase_size_t ase_lda_delete (
	ase_lda_t* lda,
	ase_size_t index,
	ase_size_t count
);

ase_size_t ase_lda_add (
	ase_lda_t* lda, ase_char_t* str, ase_size_t len);
ase_size_t ase_lda_adduniq (
	ase_lda_t* lda, ase_char_t* str, ase_size_t len);

ase_size_t ase_lda_find (
	ase_lda_t* lda, ase_size_t index,
	const ase_char_t* str, ase_size_t len);
ase_size_t ase_lda_rfind (
	ase_lda_t* lda, ase_size_t index,
	const ase_char_t* str, ase_size_t len);
ase_size_t ase_lda_rrfind (
	ase_lda_t* lda, ase_size_t index,
	const ase_char_t* str, ase_size_t len);

ase_size_t ase_lda_findx (
	ase_lda_t* lda, ase_size_t index,
	const ase_char_t* str, ase_size_t len, 
	void(*transform)(ase_size_t, ase_cstr_t*,void*), void* arg);
ase_size_t ase_lda_rfindx (
	ase_lda_t* lda, ase_size_t index,
	const ase_char_t* str, ase_size_t len, 
	void(*transform)(ase_size_t, ase_cstr_t*,void*), void* arg);
ase_size_t ase_lda_rrfindx (
	ase_lda_t* lda, ase_size_t index,
	const ase_char_t* str, ase_size_t len, 
	void(*transform)(ase_size_t, ase_cstr_t*,void*), void* arg);

void ase_lda_clear (ase_lda_t* lda);


void* ase_lda_copyinline (
	ase_lda_t* lda   /* a linear dynamic array */,
	void*      data  /* pointer to data to copy */ ,
	ase_size_t len   /* length of data in bytes */
);

#ifdef __cplusplus
}
#endif

#endif
