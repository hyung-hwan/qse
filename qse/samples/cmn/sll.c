#include <qse/cmn/mem.h>
#include <qse/cmn/str.h>
#include <qse/cmn/sll.h>
#include <qse/cmn/stdio.h>


#define R(f) \
	do { \
		qse_printf (QSE_T("== %s ==\n"), QSE_T(#f)); \
		if (f() == -1) return -1; \
	} while (0)

static qse_sll_walk_t walk_sll (qse_sll_t* sll, qse_sll_node_t* n, void* arg)
{
	qse_printf (QSE_T("[%.*s]\n"), (int)QSE_SLL_DLEN(n), QSE_SLL_DPTR(n));
	return QSE_SLL_WALK_FORWARD;
}

static int test1 ()
{
	qse_sll_t* s1;
	qse_sll_node_t* p;
	qse_char_t* x[] =
	{
		QSE_T("this is so good"),
		QSE_T("what the hack"),
		QSE_T("do you like it?")
	};
	int i;

	s1 = qse_sll_open (QSE_MMGR_GETDFL(), 0);	
	if (s1 == QSE_NULL)
	{
		qse_printf (QSE_T("cannot open a string\n"));
		return -1;
	}

	qse_sll_setcopier (s1, QSE_SLL_COPIER_INLINE);
	qse_sll_setscale (s1, QSE_SIZEOF(qse_char_t));

	for (i = 0; i < QSE_COUNTOF(x); i++)
	{
		qse_sll_pushtail (s1, x[i], qse_strlen(x[i]));
	}
	qse_printf (QSE_T("s1 holding [%d] nodes\n"), QSE_SLL_SIZE(s1));
	qse_sll_walk (s1, walk_sll, QSE_NULL);
		

	p = qse_sll_search (s1, QSE_NULL, x[0], qse_strlen(x[0]));
	if (p != QSE_NULL)
	{
		qse_sll_delete (s1, p);
	}
	qse_printf (QSE_T("s1 holding [%d] nodes\n"), QSE_SLL_SIZE(s1));
	qse_sll_walk (s1, walk_sll, QSE_NULL);

	qse_sll_close (s1);
	return 0;
}

int main ()
{
	R (test1);
	return 0;
}
