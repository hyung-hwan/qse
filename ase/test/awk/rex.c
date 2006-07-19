#include <xp/awk/awk.h>

#ifdef __STAND_ALONE
	#define xp_printf xp_awk_printf
	extern int xp_awk_printf (const xp_char_t* fmt, ...); 
	#define xp_strcmp xp_awk_strcmp
	extern int xp_awk_strcmp (const xp_char_t* s1, const xp_char_t* s2);
	#define xp_strlen xp_awk_strlen
	extern int xp_awk_strlen (const xp_char_t* s);
#endif

int xp_main (int argc, const xp_char_t* argv[])
{

	xp_awk_rex_t* rex;
	const xp_char_t* ptn;

	rex = xp_awk_rex_open (XP_NULL);
	if (rex == XP_NULL)
	{
		xp_printf (XP_T("rex open failed\n"));
		return -1;
	}


	ptn = XP_T("^he.llo");
	if (xp_awk_rex_compile (rex, ptn, xp_strlen(ptn)) == -1)
	{
		xp_printf (XP_T("cannot compile pattern...\n"));
		xp_awk_rex_close (rex);
		return -1;
	}

	xp_awk_rex_close (rex);
	return 0;
}
