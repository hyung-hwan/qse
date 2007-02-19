/*
 * $Id: pack.h,v 1.4 2007-02-19 06:13:03 bacon Exp $
 *
 * {License}
 */

#if defined(__GNUC__)
	#pragma pack(push,1)
#elif defined(__HP_aCC) || defined(__HP_cc)
	#pragma PACK 1
#elif defined(_MSC_VER) || defined(__BORLANDC__)
	#pragma pack(push,1)
#elif defined(__DECC)
	#pragma pack(push,1)
#endif
