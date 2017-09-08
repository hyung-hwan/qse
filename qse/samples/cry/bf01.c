#include <qse/si/sio.h>
#include <qse/cry/blowfish.h>
#include <qse/cry/kseed.h>
#include <string.h>

#define R(f) \
	do { \
		qse_printf (QSE_T("== %s ==\n"), QSE_T(#f)); \
		if (f() == -1) return -1; \
	} while (0)

static int test1 ()
{
	static qse_uint8_t test_data[][QSE_BLOWFISH_BLOCK_SIZE] = 
	{
		{ 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07},
		{ 0xFF,0xFF,0xFF,0xAA,0xBB,0xCC,0xDD,0xEE},
		{ 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H' }
	};

	qse_blowfish_t bf;
	qse_size_t i;

	qse_blowfish_initialize (&bf, "ABCD",4); 

	for (i = 0; i < QSE_COUNTOF(test_data); i++)
	{
		qse_uint8_t tmp[16];
		memcpy (tmp, test_data[i], QSE_BLOWFISH_BLOCK_SIZE);
		qse_blowfish_encrypt_block (&bf, tmp);

		qse_printf (QSE_T("%05d ENC => "), i);
		if (memcmp (tmp, test_data[i], QSE_BLOWFISH_BLOCK_SIZE) == 0)
		{
			qse_printf (QSE_T("FAILURE\n"));
		}
		else
		{
			qse_printf (QSE_T("OK\n"));
		}

		qse_blowfish_decrypt_block (&bf, tmp);
		qse_printf (QSE_T("%05d DEC => "), i);
		if (memcmp (tmp, test_data[i], QSE_BLOWFISH_BLOCK_SIZE) != 0)
		{
			qse_printf (QSE_T("FAILURE\n"));
		}
		else
		{
			qse_printf (QSE_T("OK\n"));
		}
	}

	return 0;
}


static int test2 ()
{
	static qse_uint8_t test_data[][QSE_KSEED_BLOCK_SIZE] = 
	{
		{ 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F },
		{ 0xFF,0xFF,0xFF,0xAA,0xBB,0xCC,0xDD,0xEE,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08 },
		{ 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P'  }
	};

	qse_kseed_t bf;
	qse_size_t i;

	//qse_kseed_initialize (&bf, "\xFF\xFF\xFF\x00",4); // <<-----
	qse_kseed_initialize (&bf, "ABCDEFGHIJKLMNOPQRSTUVWXYZ",16); // <<-----

	for (i = 0; i < QSE_COUNTOF(test_data); i++)
	{
		qse_uint8_t tmp[16];
		memcpy (tmp, test_data[i], QSE_KSEED_BLOCK_SIZE);
		qse_kseed_encrypt_block (&bf, tmp);

		qse_printf (QSE_T("%05d ENC => "), i);
		if (memcmp (tmp, test_data[i], QSE_KSEED_BLOCK_SIZE) == 0)
		{
			qse_printf (QSE_T("FAILURE\n"));
		}
		else
		{
			qse_printf (QSE_T("OK\n"));
		}

		qse_kseed_decrypt_block (&bf, tmp);
		qse_printf (QSE_T("%05d DEC => "), i);
		if (memcmp (tmp, test_data[i], QSE_KSEED_BLOCK_SIZE) != 0)
		{
			qse_printf (QSE_T("FAILURE\n"));
		}
		else
		{
			qse_printf (QSE_T("OK\n"));
		}
	}

	return 0;
}

int main ()
{
	qse_openstdsios ();
	R (test1);
	R (test2);
	qse_closestdsios ();
	return 0;
}
