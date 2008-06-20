/*
 * $Id: unpack.h 194 2008-06-06 13:00:39Z baconevi $
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
