/*
 * $Id: prim_let.c,v 1.11 2007-02-10 13:52:23 bacon Exp $
 *
 * {License}
 */

#include <ase/lsp/lsp_i.h>

static ase_lsp_obj_t* __prim_let (
	ase_lsp_t* lsp, ase_lsp_obj_t* args, int sequential)
{
	ase_lsp_frame_t* frame;
	ase_lsp_obj_t* assoc;
	ase_lsp_obj_t* body;
	ase_lsp_obj_t* value;

	/* create a new frameq */
	frame = ase_lsp_newframe (lsp);
	if (frame == ASE_NULL) return ASE_NULL;
	/*frame->link = lsp->mem->frame;*/

	if (sequential) 
	{
		frame->link = lsp->mem->frame;
		lsp->mem->frame = frame;
	}
	else 
	{
		frame->link = lsp->mem->brooding_frame;
		lsp->mem->brooding_frame = frame;
	}

	assoc = ASE_LSP_CAR(args);

	/*while (assoc != lsp->mem->nil) {*/
	while (ASE_LSP_TYPE(assoc) == ASE_LSP_OBJ_CONS) 
	{
		ase_lsp_obj_t* ass = ASE_LSP_CAR(assoc);
		if (ASE_LSP_TYPE(ass) == ASE_LSP_OBJ_CONS) 
		{
			ase_lsp_obj_t* n = ASE_LSP_CAR(ass);
			ase_lsp_obj_t* v = ASE_LSP_CDR(ass);

			if (ASE_LSP_TYPE(n) != ASE_LSP_OBJ_SYM) 
			{
				lsp->errnum = ASE_LSP_EARGBAD; 
				if (sequential) lsp->mem->frame = frame->link;
				else lsp->mem->brooding_frame = frame->link;
				ase_lsp_freeframe (lsp, frame);
				return ASE_NULL;
			}

			if (v != lsp->mem->nil) 
			{
				if (ASE_LSP_CDR(v) != lsp->mem->nil) 
				{
					lsp->errnum = ASE_LSP_EARGMANY;
					if (sequential) lsp->mem->frame = frame->link;
					else lsp->mem->brooding_frame = frame->link;
					ase_lsp_freeframe (lsp, frame);
					return ASE_NULL;
				}
				if ((v = ase_lsp_eval(lsp, ASE_LSP_CAR(v))) == ASE_NULL) 
				{
					if (sequential) lsp->mem->frame = frame->link;
					else lsp->mem->brooding_frame = frame->link;
					ase_lsp_freeframe (lsp, frame);
					return ASE_NULL;
				}
			}

			if (ase_lsp_lookupinframe (lsp, frame, n) != ASE_NULL) 
			{
				lsp->errnum = ASE_LSP_EDUPFML;
				if (sequential) lsp->mem->frame = frame->link;
				else lsp->mem->brooding_frame = frame->link;
				ase_lsp_freeframe (lsp, frame);
				return ASE_NULL;
			}
			if (ase_lsp_insvalueintoframe (lsp, frame, n, v) == ASE_NULL) 
			{
				if (sequential) lsp->mem->frame = frame->link;
				else lsp->mem->brooding_frame = frame->link;
				ase_lsp_freeframe (lsp, frame);
				return ASE_NULL;
			}
		}
		else if (ASE_LSP_TYPE(ass) == ASE_LSP_OBJ_SYM) 
		{
			if (ase_lsp_lookupinframe (lsp, frame, ass) != ASE_NULL)
			{
				lsp->errnum = ASE_LSP_EDUPFML;
				if (sequential) lsp->mem->frame = frame->link;
				else lsp->mem->brooding_frame = frame->link;
				ase_lsp_freeframe (lsp, frame);
				return ASE_NULL;
			}
			if (ase_lsp_insvalueintoframe (lsp, frame, ass, lsp->mem->nil) == ASE_NULL) 
			{
				if (sequential) lsp->mem->frame = frame->link;
				else lsp->mem->brooding_frame = frame->link;
				ase_lsp_freeframe (lsp, frame);
				return ASE_NULL;
			}
		}
		else 
		{
			lsp->errnum = ASE_LSP_EARGBAD;		
			if (sequential) lsp->mem->frame = frame->link;
			else lsp->mem->brooding_frame = frame->link;
			ase_lsp_freeframe (lsp, frame);
			return ASE_NULL;
		}

		assoc = ASE_LSP_CDR(assoc);
	}

	if (assoc != lsp->mem->nil) 
	{
		lsp->errnum = ASE_LSP_EARGBAD;	
		if (sequential) lsp->mem->frame = frame->link;
		else lsp->mem->brooding_frame = frame->link;
		ase_lsp_freeframe (lsp, frame);
		return ASE_NULL;
	}

	/* push the frame */
	if (!sequential) 
	{
		lsp->mem->brooding_frame = frame->link;
		frame->link = lsp->mem->frame;
		lsp->mem->frame = frame;
	}

	/* evaluate forms in the body */
	value = lsp->mem->nil;
	body = ASE_LSP_CDR(args);
	while (body != lsp->mem->nil) 
	{
		value = ase_lsp_eval (lsp, ASE_LSP_CAR(body));
		if (value == ASE_NULL) 
		{
			lsp->mem->frame = frame->link;
			ase_lsp_freeframe (lsp, frame);
			return ASE_NULL;
		}
		body = ASE_LSP_CDR(body);
	}

	/* pop the frame */
	lsp->mem->frame = frame->link;

	/* destroy the frame */
	ase_lsp_freeframe (lsp, frame);
	return value;
}

ase_lsp_obj_t* ase_lsp_prim_let (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	/*
	 * (defun x (x y) 
	 *     (let ((temp1 10) (temp2 20)) 
	 *          (+ x y temp1 temp2)))
	 * (x 40 50)
	 * temp1 
	 */
	return __prim_let (lsp, args, 0);
}

ase_lsp_obj_t* ase_lsp_prim_letx (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	return __prim_let (lsp, args, 1);
}
