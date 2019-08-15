/*
 * $Id$
 *
    Copyright (c) 2006-2019 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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
		qse_uint8_t tmp[QSE_BLOWFISH_BLOCK_SIZE];
		memcpy (tmp, test_data[i], QSE_BLOWFISH_BLOCK_SIZE);
		qse_blowfish_encrypt_block (&bf, &tmp);

		qse_printf (QSE_T("%05d ENC => "), i);
		if (memcmp (tmp, test_data[i], QSE_BLOWFISH_BLOCK_SIZE) == 0)
		{
			qse_printf (QSE_T("FAILURE\n"));
		}
		else
		{
			qse_printf (QSE_T("OK\n"));
		}

		qse_blowfish_decrypt_block (&bf, &tmp);
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
		qse_uint8_t tmp[QSE_KSEED_BLOCK_SIZE];
		memcpy (tmp, test_data[i], QSE_KSEED_BLOCK_SIZE);
		qse_kseed_encrypt_block (&bf, &tmp);

		qse_printf (QSE_T("%05d ENC => "), i);
		if (memcmp (tmp, test_data[i], QSE_KSEED_BLOCK_SIZE) == 0)
		{
			qse_printf (QSE_T("FAILURE\n"));
		}
		else
		{
			qse_printf (QSE_T("OK\n"));
		}

		qse_kseed_decrypt_block (&bf, &tmp);
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
	qse_open_stdsios ();
	R (test1);
	R (test2);
	qse_close_stdsios ();
	return 0;
}
