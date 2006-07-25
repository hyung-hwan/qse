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

	//ptn = XP_T("|");
	//ptn = XP_T("^he.llo(jo(in|kk)s|com)+h*e{1,40}abc|[^abc][de-f]|^he.llo(jo(in|kk)s|com)+h*e{1,40}abc|[^abc][de-f]");
	//ptn = XP_T("^he.llo(jo(in|kk)s|com)+h*e{1,40}abc|[^abc][de-f]");
	//ptn = XP_T("^he.llo(jo(in|kk)s|com)|[^x[:space:][:alpha:]j][^abc][de-f]|^he.llo(jo(in|kk)s|com)|[^x[:space:][:alpha:]j][^abc][de-f]");
	ptn = XP_T("^.{0,2}.z[^[:space:]]+(abc|zzz){1,2}khg");
	//
	//ptn = XP_T("a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}"); //a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}a{1,10}");
	if (xp_awk_rex_compile (rex, ptn, xp_strlen(ptn)) == -1)
	{
		xp_printf (XP_T("cannot compile pattern - %s\n"), xp_awk_rex_geterrstr(rex));
		xp_awk_rex_close (rex);
		return -1;
	}

	xp_awk_rex_print (rex);

	{
		int n;
		const xp_char_t* str = XP_T("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
		const xp_char_t* match_ptr;
		xp_size_t match_len;
		//if (xp_awk_rex_match (rex, XP_T("azhabc"), 6, &match_ptr, &match_len) == 0)
		n = xp_awk_rex_match (rex, XP_T("azhzzzabckhgi"), 13, &match_ptr, &match_len);
		//if (xp_awk_rex_match (rex, str, xp_strlen(str), &match_ptr, &match_len) == 0)
		if (n == 1)
		{
			xp_printf (XP_T("match = %s, match length = %u\n"), match_ptr, match_len);
		}
		else if (n == 0)
		{
			xp_printf (XP_T("go to hell\n"));
		}
		else /* if (n == -1) */
		{
			xp_printf (XP_T("ERROR: %s\n"), xp_awk_rex_geterrstr(rex));
		}
	}
	xp_awk_rex_close (rex);
	return 0;
}
