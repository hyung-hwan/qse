#ifndef _XP_AWK_EXTIO_H_
#define _XP_AWK_EXTIO_H_

#ifndef _XP_AWK_AWK_H_
#error Never include this file directly. Include <xp/awk/awk.h> instead
#endif


#ifdef __cplusplus
extern "C"
#endif

int xp_awk_readextio (
	xp_awk_run_t* run, int type, const xp_char_t* name, int* errnum);
int xp_awk_closeextio (
	xp_awk_run_t* run, const xp_char_t* name, int* errnum);

#ifdef __cplusplus
}
#endif

#endif
