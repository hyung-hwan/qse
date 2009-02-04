/*
 * $Id: lda.h 363 2008-09-04 10:58:08Z baconevi $
 *
   Copyright 2006-2009 Chung, Hyung-Hwan.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#ifndef _QSE_CMN_LDA_H_
#define _QSE_CMN_LDA_H_

#include <qse/types.h>
#include <qse/macros.h>

/****o* Common/linear dynamic array
 * DESCRIPTION
 *  <Common.h> provides a linear dynamic array. It grows as more items 
 *  are added.
 *
 *  #include <Common.h>
 ******
 */

enum qse_lda_walk_t
{
	QSE_LDA_WALK_STOP     = 0,
	QSE_LDA_WALK_FORWARD  = 1,
	QSE_LDA_WALK_BACKWARD = 2
};

typedef struct qse_lda_t      qse_lda_t;
typedef struct qse_lda_node_t qse_lda_node_t;
typedef enum   qse_lda_walk_t qse_lda_walk_t;

#define QSE_LDA_COPIER_SIMPLE  ((qse_lda_copier_t)1)
#define QSE_LDA_COPIER_INLINE  ((qse_lda_copier_t)2)

#define QSE_LDA_NIL ((qse_size_t)-1)

#define QSE_LDA_SIZE(lda)        ((lda)->size)
#define QSE_LDA_CAPA(lda)        ((lda)->capa)

#define QSE_LDA_NODE(lda,index)  ((lda)->node[index])
#define QSE_LDA_DPTR(lda,index)  ((lda)->node[index]->dptr)
#define QSE_LDA_DLEN(lda,index)  ((lda)->node[index]->dlen)

#define QSE_LDA_COPIER(lda)      ((lda)->copier)
#define QSE_LDA_FREEER(lda)      ((lda)->freeer)
#define QSE_LDA_COMPER(lda)      ((lda)->comper)
#define QSE_LDA_KEEPER(lda)      ((lda)->keeper)
#define QSE_LDA_SIZER(lda)       ((lda)->sizer)



/****t* Common/qse_lda_copier_t
 * NAME
 *  qse_lda_copier_t - define a node contruction callback
 *
 * DESCRIPTION
 *  The qse_lda_copier_t defines a callback function for node construction.
 *  A node is contructed when a user adds data to a list. The user can
 *  define how the data to add can be maintained in the list. A singly
 *  linked list not specified with any copiers stores the data pointer and
 *  the data length into a node. A special copier QSE_LDA_COPIER_INLINE copies 
 *  the contents of the data a user provided into the node. You can use the
 *  qse_lda_setcopier() function to change the copier. 
 * 
 *  A copier should return the pointer to the copied data. If it fails to copy
 *  data, it may return QSE_NULL. You need to set a proper freeer to free up
 *  memory allocated for copy.
 *
 * SEE ALSO
 *  qse_lda_setcopier, qse_lda_getcopier, QSE_LDA_COPIER
 *
 * SYNOPSIS
 */
typedef void* (*qse_lda_copier_t) (
	qse_lda_t* lda    /* a lda */,
	void*      dptr   /* the pointer to data to copy */,
	qse_size_t dlen   /* the length of data to copy */
);
/******/

/****t* Common/qse_lda_freeer_t
 * NAME
 *  qse_lda_freeer_t - define a node destruction callback
 * SYNOPSIS
 */
typedef void (*qse_lda_freeer_t) (
	qse_lda_t* lda    /* a lda */,
	void*      dptr   /* the pointer to data to free */,
	qse_size_t dlen   /* the length of data to free */
);
/******/

/****t* Common/qse_lda_comper_t
 * NAME
 *  qse_lda_comper_t - define a data comparator
 *
 * DESCRIPTION
 *  The qse_lda_comper_t type defines a key comparator that is called when
 *  the list needs to compare data. A linear dynamic array is created with a
 *  default comparator that performs bitwise comparison.
 *
 *  The comparator should return 0 if the data are the same and a non-zero
 *  integer otherwise.
 *
 * SYNOPSIS
 */
typedef int (*qse_lda_comper_t) (
	qse_lda_t*  lda    /* a linear dynamic array */, 
	const void* dptr1  /* a data pointer */,
	qse_size_t  dlen1  /* a data length */,
	const void* dptr2  /* a data pointer */,
	qse_size_t  dlen2  /* a data length */
);
/******/

/****t* Common/qse_lda_keeper_t
 * NAME
 *  qse_lda_keeper_t - define a value keeper
 *
 * DESCRIPTION
 *  The qse_lda_keeper_t type defines a value keeper that is called when 
 *  a value is retained in the context that it should be destroyed because
 *  it is identical to a new value. Two values are identical if their beginning
 *  pointers and their lengths are equal.
 *
 * SYNOPSIS
 */
typedef void (*qse_lda_keeper_t) (
	qse_lda_t* lda     /* a lda */,
	void* vptr         /* the pointer to a value */,
	qse_size_t vlen    /* the length of a value */	
);
/******/

/****t* Common/qse_lda_sizer_t
 * NAME
 *  qse_lda_sizer_t - define an array size calculator
 *
 * DESCRIPTION
 *  The qse_lda_sizer_t type defines an array size claculator that is called
 *  when the array needs to be resized. 
 *
 * SYNOPSIS
 */
typedef qse_size_t (*qse_lda_sizer_t) (
        qse_lda_t* lda,  /* a linear dynamic array */
        qse_size_t hint  /* a sizing hint */
);
/******/

typedef qse_lda_walk_t (*qse_lda_walker_t) (
        qse_lda_t*      lda   /* a linear dynamic array */,
	qse_size_t      index /* the index to the visited node */,
        void*           arg   /* user-defined data */
);

/****s* Common/qse_lda_t
 * NAME
 *  qse_lda_t - define a linear dynamic array
 *
 * SYNOPSIS
 */
struct qse_lda_t
{
	QSE_DEFINE_COMMON_FIELDS (lda)

	qse_lda_copier_t copier; /* data copier */
	qse_lda_freeer_t freeer; /* data freeer */
	qse_lda_comper_t comper; /* data comparator */
	qse_lda_keeper_t keeper; /* data keeper */
	qse_lda_sizer_t  sizer;  /* size calculator */
	qse_byte_t       scale;  /* scale factor */
	qse_size_t       size;   /* the number of items */
	qse_size_t       capa;   /* capacity */
	qse_lda_node_t** node;
};
/******/

/****s*
 * NAME
 *  qse_lda_node_t - define a linear dynamic array node
 *
 * SYNOPSIS
 */
struct qse_lda_node_t
{
	void*      dptr;
	qse_size_t dlen;
};
/******/

#ifdef __cplusplus
extern "C" {
#endif

QSE_DEFINE_COMMON_FUNCTIONS (lda)

/****f* Common/qse_lda_open
 * NAME
 *  qse_lda_open - create a linear dynamic array
 *
 * SYNOPSIS
 */
qse_lda_t* qse_lda_open (
	qse_mmgr_t* lda,
	qse_size_t  ext,
	qse_size_t  capa
);
/******/

/****f* Common/qse_lda_close
 * NAME
 *  qse_lda_close - destroy a linear dynamic array
 *
 * SYNOPSIS
 */
void qse_lda_close (
	qse_lda_t* lda
);
/******/

/****f* Common/qse_lda_init
 * NAME
 *  qse_lda_init - initialize a linear dynamic array
 *
 * SYNOPSIS
 */
qse_lda_t* qse_lda_init (
	qse_lda_t*  lda,
	qse_mmgr_t* mmgr,
	qse_size_t  capa
);
/******/

/****f* Common/qse_lda_fini
 * NAME
 *  qse_lda_fini - deinitialize a linear dynamic array
 *
 * SYNOPSIS
 */
void qse_lda_fini (
	qse_lda_t* lda
);
/******/

int qse_lda_getscale (
	qse_lda_t* lda   /* a lda */
);

/****f* Common/qse_lda_setscale
 * NAME
 *  qse_lda_setscale - set the scale factor
 *
 * DESCRIPTION 
 *  The qse_lda_setscale() function sets the scale factor of the length
 *  of a key and a value. A scale factor determines the actual length of
 *  a key and a value in bytes. A lda is created with a scale factor of 1.
 *  The scale factor should be larger than 0 and less than 256.
 *
 * NOTES
 *  It is a bad idea to change the scale factor when a lda is not empty.
 *  
 * SYNOPSIS
 */
void qse_lda_setscale (
	qse_lda_t* lda   /* a lda */,
	int scale        /* a scale factor */
);
/******/

qse_lda_copier_t qse_lda_getcopier (
	qse_lda_t* lda   /* a lda */
);

/****f* Common/qse_lda_setcopier
 * NAME 
 *  qse_lda_setcopier - specify how to clone an element
 *
 * DESCRIPTION
 *  A special copier QSE_LDA_COPIER_INLINE is provided. This copier enables
 *  you to copy the data inline to the internal node. No freeer is invoked
 *  when the node is freeed.
 *
 *  You may set the copier to QSE_NULL to perform no special operation 
 *  when the data pointer is rememebered.
 *
 * SYNOPSIS
 */
void qse_lda_setcopier (
	qse_lda_t* lda           /* a lda */, 
	qse_lda_copier_t copier  /* an element copier */
);
/******/

qse_lda_freeer_t qse_lda_getfreeer (
	qse_lda_t*   lda  /* a lda */
);

/****f* Common/qse_lda_setfreeer
 * NAME 
 *  qse_lda_setfreeer - specify how to destroy an element
 *
 * DESCRIPTION
 *  The freeer is called when a node containing the element is destroyed.
 *
 * SYNOPSIS
 */
void qse_lda_setfreeer (
	qse_lda_t* lda           /* a lda */,
	qse_lda_freeer_t freeer  /* an element freeer */
);
/******/

qse_lda_keeper_t qse_lda_getkeeper (
        qse_lda_t* lda
);

void qse_lda_setkeeper (
        qse_lda_t* lda,
        qse_lda_keeper_t keeper 
);

qse_lda_sizer_t qse_lda_getsizer (
        qse_lda_t* lda
);

void qse_lda_setsizer (
        qse_lda_t* lda,
        qse_lda_sizer_t sizer
);

qse_size_t qse_lda_getsize (
	qse_lda_t* lda
);

qse_size_t qse_lda_getcapa (
	qse_lda_t* lda
);

qse_lda_t* qse_lda_setcapa (
	qse_lda_t* lda,
	qse_size_t capa
);

qse_size_t qse_lda_search (
	qse_lda_t* lda,
	qse_size_t pos,
	const void* dptr,
	qse_size_t dlen
);

qse_size_t qse_lda_rsearch (
	qse_lda_t* lda,
	qse_size_t pos,
	const void* dptr,
	qse_size_t dlen
);

qse_size_t qse_lda_upsert (
	qse_lda_t* lda,
	qse_size_t index, 
	void*      dptr,
	qse_size_t dlen
);

qse_size_t qse_lda_insert (
	qse_lda_t* lda,
	qse_size_t index, 
	void*      dptr,
	qse_size_t dlen
);

qse_size_t qse_lda_update (
	qse_lda_t* lda,
	qse_size_t pos,
	void*      dptr,
	qse_size_t dlen
);

/****f* Common/qse_lda_delete
 * NAME
 *  qse_lda_delete - delete data
 *
 * DESCRIPTION
 *  The qse_lda_delete() function deletes the as many data as the count 
 *  from the index.
 * 
 * RETURN
 *  The qse_lda_delete() function returns the number of data deleted.
 *
 * SYNOPSIS
 */
qse_size_t qse_lda_delete (
	qse_lda_t* lda,
	qse_size_t index,
	qse_size_t count
);
/******/

/****f* Common/qse_lda_uplete
 * NAME
 *  qse_lda_uplete - delete data node
 *
 * DESCRIPTION
 *  The qse_lda_uplete() function deletes data node without compaction.
 * 
 * RETURN
 *  The qse_lda_uplete() function returns the number of data affected.
 *
 */
qse_size_t qse_lda_uplete (
	qse_lda_t* lda,
	qse_size_t index,
	qse_size_t count
);
/******/

void qse_lda_clear (
	qse_lda_t* lda
);

void qse_lda_walk (
	qse_lda_t*       lda,
	qse_lda_walker_t walker,
	void*            arg
);

void qse_lda_rwalk (
	qse_lda_t*       lda,
	qse_lda_walker_t walker,
	void*            arg
);

#ifdef __cplusplus
}
#endif

#endif
