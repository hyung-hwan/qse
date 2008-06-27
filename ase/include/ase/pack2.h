/*
 * $Id: pack.h 214 2008-06-19 09:54:58Z baconevi $
 *
 * {License}
 */

#if defined(__GNUC__)
	#pragma pack(2)
#elif defined(__HP_aCC) || defined(__HP_cc)
	#pragma PACK 2
#elif defined(_MSC_VER) || defined(__BORLANDC__)
	#pragma pack(push,2)
#elif defined(__DECC)
	#pragma pack(push,2)
#else
	#pragma pack(2)
#endif
