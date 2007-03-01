/*
 * $Id: unpack.h,v 1.6 2007-03-01 07:44:58 bacon Exp $
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
#else
	#pragma pack()
#endif
