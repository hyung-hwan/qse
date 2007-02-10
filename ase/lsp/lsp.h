/*
 * $Id: lsp.h,v 1.36 2007-02-10 13:52:23 bacon Exp $
 *
 * {License}
 */

#ifndef _ASE_LSP_LSP_H_
#define _ASE_LSP_LSP_H_

#include <ase/types.h>
#include <ase/macros.h>

typedef struct ase_lsp_t ase_lsp_t;
typedef struct ase_lsp_obj_t ase_lsp_obj_t;
typedef struct ase_lsp_prmfns_t ase_lsp_prmfns_t;

typedef ase_ssize_t (*ase_lsp_io_t) (
	int cmd, void* arg, ase_char_t* data, ase_size_t count);

typedef void* (*ase_lsp_malloc_t)  (ase_size_t n, void* custom_data); 
typedef void* (*ase_lsp_realloc_t) (void* ptr, ase_size_t n, void* custom_data);
typedef void  (*ase_lsp_free_t)    (void* ptr, void* custom_data); 
typedef void* (*ase_lsp_memcpy_t)  (void* dst, const void* src, ase_size_t n);
typedef void* (*ase_lsp_memset_t)  (void* dst, int val, ase_size_t n);

typedef ase_bool_t (*ase_lsp_isctype_t) (ase_cint_t c);
typedef ase_cint_t (*ase_lsp_toctype_t) (ase_cint_t c);
typedef ase_real_t (*ase_lsp_pow_t) (ase_real_t x, ase_real_t y);

typedef int (*ase_lsp_sprintf_t) (
	ase_char_t* buf, ase_size_t size, const ase_char_t* fmt, ...);
typedef void (*ase_lsp_aprintf_t) (const ase_char_t* fmt, ...); 
typedef void (*ase_lsp_dprintf_t) (const ase_char_t* fmt, ...); 
typedef void (*ase_lsp_abort_t) (void* custom_data);

struct ase_lsp_prmfns_t
{
	/* memory */
	ase_lsp_malloc_t  malloc;
	ase_lsp_realloc_t realloc;
	ase_lsp_free_t    free;

	ase_lsp_memcpy_t  memcpy;
	ase_lsp_memset_t  memset;

	/* character class */
	ase_lsp_isctype_t is_upper;
	ase_lsp_isctype_t is_lower;
	ase_lsp_isctype_t is_alpha;
	ase_lsp_isctype_t is_digit;
	ase_lsp_isctype_t is_xdigit;
	ase_lsp_isctype_t is_alnum;
	ase_lsp_isctype_t is_space;
	ase_lsp_isctype_t is_print;
	ase_lsp_isctype_t is_graph;
	ase_lsp_isctype_t is_cntrl;
	ase_lsp_isctype_t is_punct;
	ase_lsp_toctype_t to_upper;
	ase_lsp_toctype_t to_lower;

	/* utilities */
	ase_lsp_sprintf_t sprintf;
	ase_lsp_aprintf_t aprintf;
	ase_lsp_dprintf_t dprintf;
	ase_lsp_abort_t abort;

	void* custom_data;
};

/* io function commands */
enum 
{
	ASE_LSP_IO_OPEN   = 0,
	ASE_LSP_IO_CLOSE  = 1,
	ASE_LSP_IO_READ   = 2,
	ASE_LSP_IO_WRITE  = 3
};

/* option code */
enum
{
	ASE_LSP_UNDEFSYMBOL = (1 << 0)
};

/* error code */
enum 
{
	ASE_LSP_ENOERR,
	ASE_LSP_ENOMEM,

	ASE_LSP_EEXIT,
	ASE_LSP_EEND,
	ASE_LSP_EENDSTR,
	ASE_LSP_ERR_INPUT_NOT_ATTACHED,
	ASE_LSP_ERR_INPUT,
	ASE_LSP_ERR_OUTPUT_NOT_ATTACHED,
	ASE_LSP_ERR_OUTPUT,
	ASE_LSP_ESYNTAX,
	ASE_LSP_EARGBAD,
	ASE_LSP_EARGFEW,
	ASE_LSP_EARGMANY,
	ASE_LSP_ERR_UNDEF_FUNC,
	ASE_LSP_ERR_BAD_FUNC,
	ASE_LSP_EDUPFML,
	ASE_LSP_EBADSYM,
	ASE_LSP_ERR_UNDEF_SYMBOL,
	ASE_LSP_EEMPBDY,
	ASE_LSP_EVALBAD,

	ASE_LSP_EDIVBY0
};

typedef ase_lsp_obj_t* (*ase_lsp_prim_t) (ase_lsp_t* lsp, ase_lsp_obj_t* obj);

/* assertion statement */
#ifdef NDEBUG
	#define ASE_LSP_ASSERT(lsp,expr) ((void)0)
	#define ASE_LSP_ASSERTX(lsp,expr,desc) ((void)0)
#else
	#define ASE_LSP_ASSERT(lsp,expr) (void)((expr) || \
		(ase_lsp_assertfail (lsp, ASE_T(#expr), ASE_NULL, ASE_T(__FILE__), __LINE__), 0))
	#define ASE_LSP_ASSERTX(lsp,expr,desc) (void)((expr) || \
		(ase_lsp_assertfail (lsp, ASE_T(#expr), ASE_T(desc), ASE_T(__FILE__), __LINE__), 0))
#endif

#ifdef __cplusplus
extern "C" {
#endif

ase_lsp_t* ase_lsp_open (
	const ase_lsp_prmfns_t* prmfns,
	ase_size_t mem_ubound, ase_size_t mem_ubound_inc);

void ase_lsp_close (ase_lsp_t* lsp);

int ase_lsp_geterrnum (ase_lsp_t* lsp);

int ase_lsp_attach_input (ase_lsp_t* lsp, ase_lsp_io_t input, void* arg);
int ase_lsp_detach_input (ase_lsp_t* lsp);

int ase_lsp_attach_output (ase_lsp_t* lsp, ase_lsp_io_t output, void* arg);
int ase_lsp_detach_output (ase_lsp_t* lsp);

ase_lsp_obj_t* ase_lsp_read (ase_lsp_t* lsp);
ase_lsp_obj_t* ase_lsp_eval (ase_lsp_t* lsp, ase_lsp_obj_t* obj);
int ase_lsp_print (ase_lsp_t* lsp, const ase_lsp_obj_t* obj);

int ase_lsp_addprim (
	ase_lsp_t* lsp, const ase_char_t* name, ase_size_t name_len, 
	ase_lsp_prim_t prim, ase_size_t min_args, ase_size_t max_args);
int ase_lsp_removeprim (ase_lsp_t* lsp, const ase_char_t* name);

/* string functions exported by lsp.h */
ase_char_t* ase_lsp_strdup (ase_lsp_t* lsp, const ase_char_t* str);
ase_char_t* ase_lsp_strxdup (
	ase_lsp_t* lsp, const ase_char_t* str, ase_size_t len);
ase_char_t* ase_lsp_strxdup2 (
	ase_lsp_t* lsp,
	const ase_char_t* str1, ase_size_t len1,
	const ase_char_t* str2, ase_size_t len2);

ase_size_t ase_lsp_strlen (const ase_char_t* str);
ase_size_t ase_lsp_strcpy (ase_char_t* buf, const ase_char_t* str);
ase_size_t ase_lsp_strncpy (ase_char_t* buf, const ase_char_t* str, ase_size_t len);
int ase_lsp_strcmp (const ase_char_t* s1, const ase_char_t* s2);

int ase_lsp_strxncmp (
	const ase_char_t* s1, ase_size_t len1, 
	const ase_char_t* s2, ase_size_t len2);

int ase_lsp_strxncasecmp (
	ase_lsp_t* lsp,
	const ase_char_t* s1, ase_size_t len1, 
	const ase_char_t* s2, ase_size_t len2);

ase_char_t* ase_lsp_strxnstr (
	const ase_char_t* str, ase_size_t strsz, 
	const ase_char_t* sub, ase_size_t subsz);


/* abort function for assertion. use ASE_LSP_ASSERT instead */
int ase_lsp_assertfail (ase_lsp_t* lsp,
	const ase_char_t* expr, const ase_char_t* desc,
	const ase_char_t* file, int line);

const ase_char_t* ase_lsp_geterrstr (int errnum);

#ifdef __cplusplus
}
#endif

#endif
