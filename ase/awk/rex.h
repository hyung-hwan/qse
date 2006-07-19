/*
 * $Id: rex.h,v 1.2 2006-07-19 11:45:23 bacon Exp $
 **/

#ifndef _XP_AWK_REX_H_
#define _XP_AWK_REX_H_

#ifndef _XP_AWK_AWK_H_
#error Never include this file directly. Include <xp/awk/awk.h> instead
#endif

struct xp_awk_rex_t
{
	struct
	{
		const xp_char_t* ptr;
		const xp_char_t* end;
		const xp_char_t* curp;
		xp_char_t curc;
	} ptn;

	struct
	{
		xp_byte_t* buf;
		xp_size_t  size;
		xp_size_t  capa;
	} code;

	xp_bool_t __dynamic;
};

#ifdef __cplusplus
extern "C" {
#endif

xp_awk_rex_t* xp_awk_rex_open (xp_awk_rex_t* rex);
void xp_awk_rex_close (xp_awk_rex_t* rex);
int xp_awk_rex_compile (xp_awk_rex_t* rex, const xp_char_t* ptn, xp_size_t len);

#ifdef __cplusplus
}
#endif

#endif
