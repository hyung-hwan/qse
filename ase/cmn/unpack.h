/*
 * $Id: unpack.h,v 1.2 2007-03-22 06:30:46 bacon Exp $
 *
 * {License}
 */

#if defined(__GNUC__)
	#pragma pack()
#elif defined(__HP_aCC) || defined(__HP_cc)
	#pragma PACK
#elif defined(_MSC_VER) || defined(__BORLANDC__)
	#pragma pack(pop)
#elif defined(__DECC)
	#pragma pack(pop)
#else
	#pragma pack()
#endif
