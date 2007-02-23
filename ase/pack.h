/*
 * $Id: pack.h,v 1.5 2007-02-23 10:33:20 bacon Exp $
 *
 * {License}
 */

#if defined(__GNUC__)
	#pragma pack(1)
#elif defined(__HP_aCC) || defined(__HP_cc)
	#pragma PACK 1
#elif defined(_MSC_VER) || defined(__BORLANDC__)
	#pragma pack(push,1)
#elif defined(__DECC)
	#pragma pack(push,1)
#endif
