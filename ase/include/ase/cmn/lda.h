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

enum ase_lda_walk_t
{
	ASE_LDA_WALK_STOP     = 0,
	ASE_LDA_WALK_FORWARD  = 1,
	ASE_LDA_WALK_BACKWARD = 2
};

typedef struct ase_lda_t      ase_lda_t;
typedef struct ase_lda_node_t ase_lda_node_t;
typedef enum   ase_lda_walk_t ase_lda_walk_t;

#define ASE_LDA_COPIER_SIMPLE  ((ase_lda_copier_t)1)
#define ASE_LDA_COPIER_INLINE  ((ase_lda_copier_t)2)

#define ASE_LDA_INVALID ((ase_size_t)-1)

#define ASE_LDA_SIZE(lda)        ((lda)->size)
#define ASE_LDA_CAPA(lda)        ((lda)->capa)

#define ASE_LDA_NODE(lda,index)  ((lda)->node[index])
#define ASE_LDA_DPTR(lda,index)  ((lda)->node[index]->dptr)
#define ASE_LDA_DLEN(lda,index)  ((lda)->node[index]->dlen)

#define ASE_LDA_MMGR(lda)        ((lda)->mmgr)
#define ASE_LDA_COPIER(lda)      ((lda)->copier)
#define ASE_LDA_FREEER(lda)      ((lda)->freeer)
#define ASE_LDA_COMPER(lda)      ((lda)->comper)
#define ASE_LDA_KEEPER(lda)      ((lda)->keeper)
#define ASE_LDA_SIZER(lda)       ((lda)->sizer)

#define ASE_LDA_EXTENSION(lda)   ((void*)(((ase_lda_t*)lda) + 1))


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
	ase_lda_t* lda    /* a lda */,
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
	ase_lda_t* lda    /* a lda */,
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

/****t* ase.cmn.lda/ase_lda_keeper_t
 * NAME
 *  ase_lda_keeper_t - define a value keeper
 *
 * DESCRIPTION
 *  The ase_lda_keeper_t type defines a value keeper that is called when 
 *  a value is retained in the context that it should be destroyed because
 *  it is identical to a new value. Two values are identical if their beginning
 *  pointers and their lengths are equal.
 *
 * SYNOPSIS
 */
typedef void (*ase_lda_keeper_t) (
	ase_lda_t* lda     /* a lda */,
	void* vptr         /* the pointer to a value */,
	ase_size_t vlen    /* the length of a value */	
);
/******/

/****t* ase.cmn.lda/ase_lda_sizer_t
 * NAME
 *  ase_lda_sizer_t - define an array size calculator
 *
 * DESCRIPTION
 *  The ase_lda_sizer_t type defines an array size claculator that is called
 *  when the array needs to be resized. 
 *
 * SYNOPSIS
 */
typedef ase_size_t (*ase_lda_sizer_t) (
        ase_lda_t* lda,  /* a linear dynamic array */
        ase_size_t hint  /* a sizing hint */
);
/******/

typedef ase_lda_walk_t (*ase_lda_walker_t) (
        ase_lda_t*      lda   /* a linear dynamic array */,
	ase_size_t      index /* the index to the visited node */,
        void*           arg   /* user-defined data */
);

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
	ase_lda_keeper_t keeper; /* data keeper */
	ase_lda_sizer_t  sizer;  /* size calculator */
	ase_byte_t       scale;  /* scale factor */
	ase_size_t       size;   /* the number of items */
	ase_size_t       capa;   /* capacity */
	ase_lda_node_t** node;
};
/******/

/****s*
 * NAME
 *  ase_lda_node_t - define a linear dynamic array node
 *
 * SYNOPSIS
 */
struct ase_lda_node_t
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
	ase_size_t  ext,
	ase_size_t  capa
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
	ase_lda_t*  lda,
	ase_mmgr_t* mmgr,
	ase_size_t  capa
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

int ase_lda_getscale (
	ase_lda_t* lda   /* a lda */
);

/****f* ase.cmn.lda/ase_lda_setscale
 * NAME
 *  ase_lda_setscale - set the scale factor
 *
 * DESCRIPTION 
 *  The ase_lda_setscale() function sets the scale factor of the length
 *  of a key and a value. A scale factor determines the actual length of
 *  a key and a value in bytes. A lda is created with a scale factor of 1.
 *  The scale factor should be larger than 0 and less than 256.
 *
 * NOTES
 *  It is a bad idea to change the scale factor when a lda is not empty.
 *  
 * SYNOPSIS
 */
void ase_lda_setscale (
	ase_lda_t* lda   /* a lda */,
	int scale        /* a scale factor */
);
/******/

ase_lda_copier_t ase_lda_getcopier (
	ase_lda_t* lda   /* a lda */
);

/****f* ase.cmn.lda/ase_lda_setcopier
 * NAME 
 *  ase_lda_setcopier - specify how to clone an element
 *
 * DESCRIPTION
 *  A special copier ASE_LDA_COPIER_INLINE is provided. This copier enables
 *  you to copy the data inline to the internal node. No freeer is invoked
 *  when the node is freeed.
 *
 *  You may set the copier to ASE_NULL to perform no special operation 
 *  when the data pointer is rememebered.
 *
 * SYNOPSIS
 */
void ase_lda_setcopier (
	ase_lda_t* lda           /* a lda */, 
	ase_lda_copier_t copier  /* an element copier */
);
/******/

ase_lda_freeer_t ase_lda_getfreeer (
	ase_lda_t*   lda  /* a lda */
);

/****f* ase.cmn.lda/ase_lda_setfreeer
 * NAME 
 *  ase_lda_setfreeer - specify how to destroy an element
 *
 * DESCRIPTION
 *  The freeer is called when a node containing the element is destroyed.
 *
 * SYNOPSIS
 */
void ase_lda_setfreeer (
	ase_lda_t* lda           /* a lda */,
	ase_lda_freeer_t freeer  /* an element freeer */
);
/******/

ase_lda_keeper_t ase_lda_getkeeper (
        ase_lda_t* lda
);

void ase_lda_setkeeper (
        ase_lda_t* lda,
        ase_lda_keeper_t keeper 
);

ase_lda_sizer_t ase_lda_getsizer (
        ase_lda_t* lda
);

void ase_lda_setsizer (
        ase_lda_t* lda,
        ase_lda_sizer_t sizer
);

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

ase_size_t ase_lda_search (
	ase_lda_t* lda,
	ase_size_t pos,
	const void* dptr,
	ase_size_t dlen
);

ase_size_t ase_lda_rsearch (
	ase_lda_t* lda,
	ase_size_t pos,
	const void* dptr,
	ase_size_t dlen
);

ase_size_t ase_lda_upsert (
	ase_lda_t* lda,
	ase_size_t index, 
	void*      dptr,
	ase_size_t dlen
);

ase_size_t ase_lda_insert (
	ase_lda_t* lda,
	ase_size_t index, 
	void*      dptr,
	ase_size_t dlen
);

ase_size_t ase_lda_update (
	ase_lda_t* lda,
	ase_size_t pos,
	void*      dptr,
	ase_size_t dlen
);

/****f* ase.cmn.lda/ase_lda_delete
 * NAME
 *  ase_lda_delete - delete data
 *
 * DESCRIPTION
 *  The ase_lda_delete() function deletes the as many data as the count 
 *  from the index.
 * 
 * RETURN
 *  The ase_lda_delete() function returns the number of data deleted.
 *
 * SYNOPSIS
 */
ase_size_t ase_lda_delete (
	ase_lda_t* lda,
	ase_size_t index,
	ase_size_t count
);
/******/

/****f* ase.cmn.lda/ase_lda_uplete
 * NAME
 *  ase_lda_uplete - delete data node
 *
 * DESCRIPTION
 *  The ase_lda_uplete() function deletes data node without compaction.
 * 
 * RETURN
 *  The ase_lda_uplete() function returns the number of data affected.
 *
 */
ase_size_t ase_lda_uplete (
	ase_lda_t* lda,
	ase_size_t index,
	ase_size_t count
);
/******/

void ase_lda_clear (
	ase_lda_t* lda
);

void ase_lda_walk (
	ase_lda_t*       lda,
	ase_lda_walker_t walker,
	void*            arg
);

void ase_lda_rwalk (
	ase_lda_t*       lda,
	ase_lda_walker_t walker,
	void*            arg
);

#ifdef __cplusplus
}
#endif

#endif
