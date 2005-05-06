/*
 * $Id: object.h,v 1.1 2005-05-06 17:17:59 bacon Exp $
 */

#ifndef _XP_STX_OBJECT_H_
#define _XP_STX_OBJECT_H_

#include <xp/stx/memory.h>

#define XP_STX_IS_SMALLINT(x)   (((x) & 0x01) == 0x01)
#define XP_STX_TO_SMALLINT(x)   ((x) << 1) | 0x01)
#define XP_STX_FROM_SMALLINT(x) ((x) >> 1)

#endif
