/*
 * $Id: env.h,v 1.1 2005-02-04 15:39:11 bacon Exp $
 */

#ifndef _RBL_ENV_H_
#define _RBL_ENV_H_

#include "object.h"

struct xp_lisp_assoc_t
{
	xp_lisp_obj_t*          name; // xp_lisp_obj_symbol_t
	xp_lisp_obj_t*          value;
	struct xp_lisp_assoc_t* link;
};

struct xp_lisp_frame_t
{
	struct xp_lisp_assoc_t* assoc;
	struct xp_lisp_frame_t* link;
};

typedef struct xp_lisp_assoc_t xp_lisp_assoc_t;
typedef struct xp_lisp_frame_t xp_lisp_frame_t;

#ifdef __cplusplus
extern "C" {
#endif

xp_lisp_assoc_t* xp_lisp_assoc_new  (xp_lisp_obj_t* name, xp_lisp_obj_t* value);
void         xp_lisp_assoc_free (xp_lisp_assoc_t* assoc);

xp_lisp_frame_t* xp_lisp_frame_new    (void);
void         xp_lisp_frame_free   (xp_lisp_frame_t* frame);
xp_lisp_assoc_t* xp_lisp_frame_lookup (xp_lisp_frame_t* frame, xp_lisp_obj_t* name);
xp_lisp_assoc_t* xp_lisp_frame_insert (xp_lisp_frame_t* frame, xp_lisp_obj_t* name, xp_lisp_obj_t* value);

#ifdef __cplusplus
}
#endif

#endif
