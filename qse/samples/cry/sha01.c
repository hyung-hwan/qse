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

#include <qse/cry/sha2.h>
#include <qse/cry/sha1.h>
#include <qse/cry/md5.h>
#include <qse/cry/hmac.h>
#include <qse/cmn/path.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <getopt.h>

#define READ_BUF_SIZE (32768)

static qse_size_t sha512sum (int fd, qse_uint8_t* digest, qse_size_t size)
{
	qse_sha512_t md;
	qse_uint8_t buf[READ_BUF_SIZE];
	ssize_t n;

	qse_sha512_initialize (&md);
	while (1)
	{
		n = read(fd, buf, sizeof(buf));
		if (n <= 0) break;
		qse_sha512_update (&md, buf, n);
	}
	return qse_sha512_digest(&md, digest, size);
}

static qse_size_t sha384sum (int fd, qse_uint8_t* digest, qse_size_t size)
{
	qse_sha384_t md;
	qse_uint8_t buf[READ_BUF_SIZE];
	ssize_t n;

	qse_sha384_initialize (&md);
	while (1)
	{
		n = read(fd, buf, sizeof(buf));
		if (n <= 0) break;
		qse_sha384_update (&md, buf, n);
	}
	return qse_sha384_digest(&md, digest, size);
}

static qse_size_t sha256sum (int fd, qse_uint8_t* digest, qse_size_t size)
{
	qse_sha256_t md;
	qse_uint8_t buf[READ_BUF_SIZE];
	ssize_t n;

	qse_sha256_initialize (&md);
	while (1)
	{
		n = read(fd, buf, sizeof(buf));
		if (n <= 0) break;
		qse_sha256_update (&md, buf, n);
	}
	return qse_sha256_digest(&md, digest, size);
}

static qse_size_t sha1sum (int fd, qse_uint8_t* digest, qse_size_t size)
{
	qse_sha1_t md;
	qse_uint8_t buf[READ_BUF_SIZE];
	ssize_t n;

	qse_sha1_initialize (&md);
	while (1)
	{
		n = read(fd, buf, sizeof(buf));
		if (n <= 0) break;
		qse_sha1_update (&md, buf, n);
	}
	return qse_sha1_digest(&md, digest, size);
}

static qse_size_t md5sum (int fd, qse_uint8_t* digest, qse_size_t size)
{
	qse_md5_t md;
	qse_uint8_t buf[READ_BUF_SIZE];
	ssize_t n;

	qse_md5_initialize (&md);
	while (1)
	{
		n = read(fd, buf, sizeof(buf));
		if (n <= 0) break;
		qse_md5_update (&md, buf, n);
	}
	return qse_md5_digest(&md, digest, size);
}

static qse_size_t hmac (int fd, qse_hmac_sha_type_t sha_type, const qse_uint8_t* key, qse_size_t keylen, qse_uint8_t* digest, qse_size_t size)
{
	qse_hmac_t md;
	qse_uint8_t buf[READ_BUF_SIZE];
	ssize_t n;

	qse_hmac_initialize (&md, sha_type, key, keylen);
	while (1)
	{
		n = read(fd, buf, sizeof(buf));
		if (n <= 0) break;
		qse_hmac_update (&md, buf, n);
	}
	return qse_hmac_digest(&md, digest, size);
}


static void print_usage (const char* argv0)
{
	fprintf (stderr, "Usage: %s [options]\n", argv0);
	fprintf (stderr, "where options are:\n");
	fprintf (stderr, "  -m      md5\n");
	fprintf (stderr, "  -1      sha1\n");
	fprintf (stderr, "  -2      sha256\n");
	fprintf (stderr, "  -3      sha384\n");
	fprintf (stderr, "  -5      sha512\n");
	fprintf (stderr, "  -k      specify hmac key and enable hmac\n");
}

int main (int argc, char* argv[])
{
	qse_uint8_t digest[QSE_HMAC_MAX_DIGEST_LEN];
	qse_size_t digest_len;
	qse_size_t (*sha_func) (int fd, qse_uint8_t* digest, qse_size_t len) = sha512sum;
	const char* hmac_key = NULL;
	qse_hmac_sha_type_t hmac_sha_type = QSE_HMAC_SHA512;
	int i, j;

	while (1)
	{
		int c;

		c = getopt(argc, argv, ":hk:m1235");
		if (c == -1) break;

		switch (c)
		{
			case 'h':
				print_usage (qse_basenameasmbs(argv[0]));
				return 0;

			case 'k':
				hmac_key = optarg;
				break;

			case 'm':
				hmac_sha_type = QSE_HMAC_MD5;
				sha_func = md5sum;
				break;

			case '1':
				hmac_sha_type = QSE_HMAC_SHA1;
				sha_func = sha1sum;
				break;

			case '2':
				hmac_sha_type = QSE_HMAC_SHA256;
				sha_func = sha256sum;
				break;

			case '3':
				hmac_sha_type = QSE_HMAC_SHA384;
				sha_func = sha384sum;
				break;

			case '5':
				hmac_sha_type = QSE_HMAC_SHA512;
				sha_func = sha512sum;
				break;

			case '?':
			case ':':
				print_usage (qse_basenameasmbs(argv[0]));
				return -1;
		}
	}



	if (hmac_key)
	{
		if (optind >= argc)
		{
			QSE_STATIC_ASSERT (QSE_SIZEOF(digest) >= QSE_HMAC_MAX_DIGEST_LEN);
			digest_len = hmac(STDIN_FILENO, hmac_sha_type, (const qse_uint8_t*)hmac_key, strlen(hmac_key), digest, QSE_SIZEOF(digest));
			for (j = 0; j < digest_len; j++)
				printf ("%02x", digest[j]);
			printf ("  -\n");
		}
		else
		{
			int fd;

			for (i = optind; i < argc; i++)
			{
				fd = strcmp(argv[i], "-")? open(argv[i], O_RDONLY): STDIN_FILENO;
				if (fd >= 0)
				{
					digest_len = hmac(fd, hmac_sha_type, (const qse_uint8_t*)hmac_key, strlen(hmac_key), digest, QSE_SIZEOF(digest));
					for (j = 0; j < digest_len; j++)
						printf ("%02x", digest[j]);
					printf ("  %s\n", argv[i]);
					if (strcmp(argv[i], "-")) close (fd);
				}
				else
				{
					fprintf (stderr, "%s: %s\n", argv[i], strerror(errno));
				}
			}
		}
	}
	else
	{
		if (optind >= argc)
		{
			digest_len = sha_func(STDIN_FILENO, digest, QSE_SIZEOF(digest));
			for (j = 0; j < digest_len; j++)
				printf ("%02x", digest[j]);
			printf ("  -\n");
		}
		else
		{
			int fd;

			for (i = optind; i < argc; i++)
			{
				fd = strcmp(argv[i], "-")? open(argv[i], O_RDONLY): STDIN_FILENO;
				if (fd >= 0)
				{
					digest_len = sha_func(fd, digest, QSE_SIZEOF(digest));
					for (j = 0; j < digest_len; j++)
						printf ("%02x", digest[j]);
					printf ("  %s\n", argv[i]);
					if (strcmp(argv[i], "-")) close (fd);
				}
				else
				{
					fprintf (stderr, "%s: %s\n", argv[i], strerror(errno));
				}
			}
		}
	}
	return 0;
}

