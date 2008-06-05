/*
 * $Id: pack.h 182 2008-06-03 08:17:42Z baconevi $
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
#else
	#pragma pack(1)
#endif
