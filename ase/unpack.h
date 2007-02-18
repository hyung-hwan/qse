/*
 * $Id: unpack.h,v 1.3 2007-02-18 16:49:03 bacon Exp $
 *
 * {License}
 */

#if defined(__GNUC__)
#pragma pack(pop)
#elif defined(__HP_aCC) || defined(__HP_cc)
#pragma PACK
#elif defined(__MSC_VER) || defined(__BORLANDC__)
#pragma pack(pop)
#endif
