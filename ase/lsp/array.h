/*
 * $Id: array.h,v 1.1 2005-02-04 15:39:11 bacon Exp $
 */

#ifndef _RBL_ARRAY_H_
#define _RBL_ARRAY_H_

#include <rb/types.h>

struct xp_lisp_array_t {
	void** buffer;
	xp_size_t size;
	xp_size_t capacity;
};

typedef struct xp_lisp_array_t xp_lisp_array_t;

#ifdef __cplusplus
extern "C" {
#endif

xp_lisp_array_t* xp_lisp_array_new      (xp_size_t capacity);
void         xp_lisp_array_free     (xp_lisp_array_t* array);
int          xp_lisp_array_add_item (xp_lisp_array_t* array, void* item);
int          xp_lisp_array_insert   (xp_lisp_array_t* array, xp_size_t index, void* value);
void         xp_lisp_array_delete   (xp_lisp_array_t* array, xp_size_t index);
void         xp_lisp_array_clear    (xp_lisp_array_t* array);
void**       xp_lisp_array_transfer (xp_lisp_array_t* array, xp_size_t capacity);

#ifdef __cplusplus
}
#endif

#endif
