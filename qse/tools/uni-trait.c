#include <qse/types.h>
#include <locale.h>
#include <wchar.h>
#include <wctype.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#if QSE_SIZEOF_WCHAR_T == QSE_SIZEOF_SHORT
	#define MAX_CHAR 0xFFFF
#else
	/*#define MAX_CHAR 0xE01EF*/
	#define MAX_CHAR 0x10FFFF
#endif

#define TRAIT_PAGE_SIZE 256
#define MAX_TRAIT_PAGE_COUNT ((MAX_CHAR + TRAIT_PAGE_SIZE) / TRAIT_PAGE_SIZE)

typedef struct trait_page_t trait_page_t;
struct trait_page_t
{
	size_t no;
	short traits[TRAIT_PAGE_SIZE];
	trait_page_t* next;
};

size_t trait_page_count = 0;
trait_page_t* trait_pages = NULL;

size_t trait_map_count = 0;
trait_page_t* trait_maps[MAX_TRAIT_PAGE_COUNT];

enum
{
	TRAIT_UPPER  = (1 << 0),
	TRAIT_LOWER  = (1 << 1),
	TRAIT_ALPHA  = (1 << 2),
	TRAIT_DIGIT  = (1 << 3),
	TRAIT_XDIGIT = (1 << 4),
	TRAIT_ALNUM  = (1 << 5),
	TRAIT_SPACE  = (1 << 6),
	TRAIT_PRINT  = (1 << 8),
	TRAIT_GRAPH  = (1 << 9),
	TRAIT_CNTRL  = (1 << 10),
	TRAIT_PUNCT  = (1 << 11),
	TRAIT_BLANK  = (1 << 12)
};

int get_trait (qse_wcint_t code)
{
	int trait = 0;

	if (iswupper(code))    trait |= TRAIT_UPPER;
	if (iswlower(code))    trait |= TRAIT_LOWER;
	if (iswalpha(code))    trait |= TRAIT_ALPHA;
	if (iswdigit(code))    trait |= TRAIT_DIGIT;
	if (iswxdigit(code))   trait |= TRAIT_XDIGIT;
	if (iswalnum(code))    trait |= TRAIT_ALNUM;
	if (iswspace(code))    trait |= TRAIT_SPACE;
	if (iswprint(code))    trait |= TRAIT_PRINT;
	if (iswgraph(code))    trait |= TRAIT_GRAPH;
	if (iswcntrl(code))    trait |= TRAIT_CNTRL;
	if (iswpunct(code))    trait |= TRAIT_PUNCT;
	if (iswblank(code))    trait |= TRAIT_BLANK;
	/*
	if (iswascii(code))    trait |= TRAIT_ASCII;
	if (isphonogram(code)) trait |= TRAIT_PHONO;
	if (isideogram(code))  trait |= TRAIT_IDEOG;
	if (isenglish(code))   trait |= TRAIT_ENGLI;
	*/

	return trait;
}

void make_trait_page (qse_wcint_t start, qse_wcint_t end)
{
	qse_wcint_t code;
	size_t i;
	short traits[TRAIT_PAGE_SIZE];
	trait_page_t* page;

	memset (traits, 0, sizeof(traits));
	for (code = start; code <= end; code++) {
		traits[code - start] = get_trait(code);
	}

	for (page = trait_pages; page != NULL; page = page->next) {
		if (memcmp (traits, page->traits, sizeof(traits)) == 0) {
			trait_maps[trait_map_count++] = page;
			return;
		}
	}

	page = (trait_page_t*)malloc (sizeof(trait_page_t));
	page->no = trait_page_count++;
	memcpy (page->traits, traits, sizeof(traits));
	page->next = trait_pages;

	trait_pages = page;
	trait_maps[trait_map_count++] = page;
}

void emit_trait_page (trait_page_t* page)
{
	size_t i;
	int trait, need_or;

	printf ("static qse_uint16_t trait_page_%04X[%u] =\n{\n", 
		(unsigned int)page->no, (unsigned int)TRAIT_PAGE_SIZE);

	for (i = 0; i < TRAIT_PAGE_SIZE; i++) {

		need_or = 0;
		trait = page->traits[i];

		if (i != 0) printf (",\n");
		printf ("\t");

		if (trait == 0) {
			printf ("0");
			continue;
		}

		if (trait & TRAIT_UPPER) {
			if (need_or) printf (" | ");
			printf ("TRAIT_UPPER");
			need_or = 1;
		}

		if (trait & TRAIT_LOWER) {
			if (need_or) printf (" | ");
			printf ("TRAIT_LOWER");
			need_or = 1;
		}

		if (trait & TRAIT_ALPHA) {
			if (need_or) printf (" | ");
			printf ("TRAIT_ALPHA");
			need_or = 1;
		}

		if (trait & TRAIT_DIGIT) {
			if (need_or) printf (" | ");
			printf ("TRAIT_DIGIT");
			need_or = 1;
		}

		if (trait & TRAIT_XDIGIT) {
			if (need_or) printf (" | ");
			printf ("TRAIT_XDIGIT");
			need_or = 1;
		}

		if (trait & TRAIT_ALNUM) {
			if (need_or) printf (" | ");
			printf ("TRAIT_ALNUM");
			need_or = 1;
		}

		if (trait & TRAIT_SPACE) {
			if (need_or) printf (" | ");
			printf ("TRAIT_SPACE");
			need_or = 1;
		}

		if (trait & TRAIT_PRINT) {
			if (need_or) printf (" | ");
			printf ("TRAIT_PRINT");
			need_or = 1;
		}

		if (trait & TRAIT_GRAPH) {
			if (need_or) printf (" | ");
			printf ("TRAIT_GRAPH");
			need_or = 1;
		}

		if (trait & TRAIT_CNTRL) {
			if (need_or) printf (" | ");
			printf ("TRAIT_CNTRL");
			need_or = 1;
		}

		if (trait & TRAIT_PUNCT) {
			if (need_or) printf (" | ");
			printf ("TRAIT_PUNCT");
			need_or = 1;
		}

		if (trait & TRAIT_BLANK) {
			if (need_or) printf (" | ");
			printf ("TRAIT_BLANK");
			need_or = 1;
		}


		/*
		if (trait & TRAIT_ASCII) {
			if (need_or) printf (" | ");
			printf ("TRAIT_ASCII");
			need_or = 1;
		}

		if (trait & TRAIT_IDEOG) {
			if (need_or) printf (" | ");
			printf ("TRAIT_IDEOG");
			need_or = 1;
		}

		if (trait & TRAIT_PHONO) {
			if (need_or) printf (" | ");
			printf ("TRAIT_PHONO");
			need_or = 1;
		}

		if (trait & TRAIT_ENGLI) {
			if (need_or) printf (" | ");
			printf ("TRAIT_ENGLI");
			need_or = 1;
		}
		*/

	}

	printf ("\n};\n");
}

void emit_trait_map ()
{
	size_t i;

	printf ("static qse_uint16_t* trait_map[%u] =\n{\n", (unsigned int)trait_map_count);

	for (i = 0; i < trait_map_count; i++) {
		if (i != 0) printf (",\n");
		printf ("\t /* 0x%X-0x%X */ ", 
			i * TRAIT_PAGE_SIZE, (i + 1) * TRAIT_PAGE_SIZE - 1);
		printf ("trait_page_%04X", trait_maps[i]->no);
	}

	printf ("\n};\n");
}

static void emit_trait_macros (void)
{
	printf ("/* generated by tools/uni-trait.c */\n\n");
	printf ("#define TRAIT_MAX 0x%lX\n", (unsigned long)MAX_CHAR);
	printf ("\n");
}

int main ()
{
	qse_wcint_t code;
	trait_page_t* page;
	char* locale;

	locale = setlocale (LC_ALL, "");
	if (locale == NULL ||
	    (strstr(locale, ".utf8") == NULL && strstr(locale, ".UTF8") == NULL &&
	     strstr(locale, ".utf-8") == NULL && strstr(locale, ".UTF-8") == NULL)) {
		fprintf (stderr, "error: the locale should be utf-8 compatible\n");
		return -1;
	}


	for (code = 0; code < MAX_CHAR; code += TRAIT_PAGE_SIZE) {
		make_trait_page (code, code + TRAIT_PAGE_SIZE - 1);
	}

	emit_trait_macros ();

	for (page = trait_pages; page != NULL; page = page->next) {
		emit_trait_page (page);	
		printf ("\n");
	}

	emit_trait_map ();

	return 0;
}

