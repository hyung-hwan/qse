/*
 * $Id: unpack.h,v 1.5 2007-02-23 10:33:20 bacon Exp $
 *
 * {License}
 */

#if defined(__GNUC__)
	#pragma pack()
#elif defined(__HP_aCC) || defined(__HP_cc)
	#pragma PACK
#elif defined(__MSC_VER) || defined(__BORLANDC__)
	#pragma pack(pop)
#elif defined(__DECC)
	#pragma pack(pop)
#endif
