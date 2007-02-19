/*
 * $Id: unpack.h,v 1.4 2007-02-19 06:13:03 bacon Exp $
 *
 * {License}
 */

#if defined(__GNUC__)
	#pragma pack(pop)
#elif defined(__HP_aCC) || defined(__HP_cc)
	#pragma PACK
#elif defined(__MSC_VER) || defined(__BORLANDC__)
	#pragma pack(pop)
#elif defined(__DECC)
	#pragma pack(pop)
#endif
