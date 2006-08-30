#include <xp/awk/rex.h>

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
	void* rex;
	const xp_char_t* ptn;
	int errnum;

	//ptn = XP_T("|");
	//ptn = XP_T("^he.llo(jo(in|kk)s|com)+h*e{1,40}abc|[^abc][de-f]|^he.llo(jo(in|kk)s|com)+h*e{1,40}abc|[^abc][de-f]");
	//ptn = XP_T("^he.llo(jo(in|kk)s|com)+h*e{1,40}abc|[^abc][de-f]");
	//ptn = XP_T("^he.llo(jo(in|kk)s|com)|[^x[:space:][:alpha:]j][^abc][de-f]|^he.llo(jo(in|kk)s|com)|[^x[:space:][:alpha:]j][^abc][de-f]");
	//ptn = XP_T("^.{0,2}.z[^[:space:]]+(abc|zzz){1,2}khg");
	ptn = XP_T("");

	rex = xp_awk_buildrex (ptn, xp_strlen(ptn), &errnum);
	if (rex == XP_NULL)
	{
		xp_printf (XP_T("cannot compile pattern\n"));
		return -1;
	}

	xp_printf (XP_T("isemptyrex => %d\n"), xp_awk_isemptyrex (rex));
	xp_printf (XP_T("NA: %u\n"), (unsigned int)XP_AWK_REX_NA(rex));
	xp_printf (XP_T("LEN: %u\n"), (unsigned int)XP_AWK_REX_LEN(rex));
	xp_awk_printrex (rex);

	{
		int n;
		const xp_char_t* match_ptr;
		xp_size_t match_len;

		//if (xp_awk_matchrex (rex, XP_T("azhabc"), 6, &match_ptr, &match_len) == 0)
		n = xp_awk_matchrex (rex, XP_T("azhzzzabckhgi"), 13, &match_ptr, &match_len, &errnum);
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
			xp_printf (XP_T("ERROR: in match\n"));
		}
	}
	return 0;
}
