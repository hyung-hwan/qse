#include <qse/cry/sha2.h>
#include <qse/cry/sha1.h>
#include <qse/cry/md5.h>
#include <qse/cmn/path.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <getopt.h>

#define READ_BUF_SIZE (32768)

static void sha512sum (int fd, unsigned char digest[QSE_SHA512_DIGEST_LEN])
{
	qse_sha512_t md;
	unsigned char buf[READ_BUF_SIZE];
	ssize_t n;

	qse_sha512_initialize (&md);
	while (1)
	{
		n = read(fd, buf, sizeof(buf));
		if (n <= 0) break;
		qse_sha512_update (&md, buf, n);
	}
	qse_sha512_digest (&md, digest, QSE_SHA512_DIGEST_LEN);
}

static void sha384sum (int fd, unsigned char digest[QSE_SHA384_DIGEST_LEN])
{
	qse_sha384_t md;
	unsigned char buf[READ_BUF_SIZE];
	ssize_t n;

	qse_sha384_initialize (&md);
	while (1)
	{
		n = read(fd, buf, sizeof(buf));
		if (n <= 0) break;
		qse_sha384_update (&md, buf, n);
	}
	qse_sha384_digest (&md, digest, QSE_SHA384_DIGEST_LEN);
}

static void sha256sum (int fd, unsigned char digest[QSE_SHA256_DIGEST_LEN])
{
	qse_sha256_t md;
	unsigned char buf[READ_BUF_SIZE];
	ssize_t n;

	qse_sha256_initialize (&md);
	while (1)
	{
		n = read(fd, buf, sizeof(buf));
		if (n <= 0) break;
		qse_sha256_update (&md, buf, n);
	}
	qse_sha256_digest (&md, digest, QSE_SHA256_DIGEST_LEN);
}

static void sha1sum (int fd, unsigned char digest[QSE_SHA1_DIGEST_LEN])
{
	qse_sha1_t md;
	unsigned char buf[READ_BUF_SIZE];
	ssize_t n;

	qse_sha1_initialize (&md);
	while (1)
	{
		n = read(fd, buf, sizeof(buf));
		if (n <= 0) break;
		qse_sha1_update (&md, buf, n);
	}
	qse_sha1_digest (&md, digest, QSE_SHA1_DIGEST_LEN);
}

static void md5sum (int fd, unsigned char digest[QSE_MD5_DIGEST_LEN])
{
	qse_md5_t md;
	unsigned char buf[READ_BUF_SIZE];
	ssize_t n;

	qse_md5_initialize (&md);
	while (1)
	{
		n = read(fd, buf, sizeof(buf));
		if (n <= 0) break;
		qse_md5_update (&md, buf, n);
	}
	qse_md5_digest (&md, digest, QSE_MD5_DIGEST_LEN);
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
}

int main (int argc, char* argv[])
{
	unsigned char digest[QSE_SHA512_DIGEST_LEN];
	int digest_len = QSE_SHA512_DIGEST_LEN;
	void (*sha_func) (int fd, unsigned char* digest) = sha512sum;
	int i, j;

	while (1)
	{
		int c;

		c = getopt(argc, argv, ":hm1235");
		if (c == -1) break;

		switch (c)
		{
			case 'h':
				print_usage (qse_mbsbasename(argv[0]));
				return 0;

			case 'm':
				digest_len = QSE_MD5_DIGEST_LEN;
				sha_func = md5sum;
				break;

			case '1':
				digest_len = QSE_SHA1_DIGEST_LEN;
				sha_func = sha1sum;
				break;

			case '2':
				digest_len = QSE_SHA256_DIGEST_LEN;
				sha_func = sha256sum;
				break;

			case '3':
				digest_len = QSE_SHA384_DIGEST_LEN;
				sha_func = sha384sum;
				break;

			case '5':
				digest_len = QSE_SHA512_DIGEST_LEN;
				sha_func = sha512sum;
				break;

			case '?':
			case ':':
				print_usage (qse_mbsbasename(argv[0]));
				return -1;
		}
	}



	if (optind >= argc)
	{
		sha_func (STDIN_FILENO, digest);
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
				sha_func (fd, digest);
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

	return 0;
}

