/*
 * $Id: pack1.h 225 2008-06-26 06:48:38Z baconevi $
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
