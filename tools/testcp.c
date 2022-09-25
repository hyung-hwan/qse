#include <stdio.h>

typedef unsigned short qse_uint16_t;
#define QSE_COUNTOF(x) (sizeof(x) / sizeof(x[0]))

#include "x.h"

int main ()
{
	qse_uint16_t mb;
	for (mb = 0; mb <= 127; mb++)
	{
		printf ("0x%04x 0x%04x\n", mb, mb);
	}
	for (mb = 128; mb < 0xFFFF; mb++)
	{
		qse_uint16_t wc = mbtowc(mb);
		printf ("0x%04x 0x%04x", mb, wc);
		if (wc != 0xFFFF)
		{
			qse_uint16_t xmb = wctomb(wc);
			if (xmb != mb) printf (" (ERROR xmb=0x%04x)", xmb);
			
		}
		printf ("\n");
		
	}
	return 0;
}
