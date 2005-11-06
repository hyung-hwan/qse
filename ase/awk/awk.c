/*
 * $Id: awk.c,v 1.1 2005-11-06 12:01:29 bacon Exp $
 */

#include <xp/awk/awk.h>
#include <xp/bas/memory.h>

xp_awk_t* xp_awk_open (xp_awk_t* awk)
{
	if (awk == XP_NULL) {
		awk = (xp_awk_t*) xp_malloc (xp_sizeof(awk));
		if (awk == XP_NULL) return XP_NULL;
		awk->__malloced = xp_true;
	}
	else awk->__malloced = xp_false;

	awk->errnum = XP_AWK_ENOERR;
	return awk;
}

int xp_awk_close (xp_awk_t* awk)
{
	if (awk->__malloced) xp_free (awk);
	return 0;
}
