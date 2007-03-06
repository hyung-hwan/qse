/*
 * $Id: lsp.h,v 1.42 2007-03-06 14:58:00 bacon Exp $
 *
 * {License}
 */

#ifndef _ASE_LSP_LSP_H_
#define _ASE_LSP_LSP_H_

#include <ase/cmn/types.h>
#include <ase/cmn/macros.h>

typedef struct ase_lsp_t ase_lsp_t;
typedef struct ase_lsp_obj_t ase_lsp_obj_t;
typedef struct ase_lsp_prmfns_t ase_lsp_prmfns_t;

typedef ase_ssize_t (*ase_lsp_io_t) (
	int cmd, void* arg, ase_char_t* data, ase_size_t count);

typedef ase_real_t (*ase_lsp_pow_t) (
	void* custom, ase_real_t x, ase_real_t y);
typedef int (*ase_lsp_sprintf_t) (
	void* custom, ase_char_t* buf, ase_size_t size, 
	const ase_char_t* fmt, ...);
typedef void (*ase_lsp_dprintf_t) (void* custom, const ase_char_t* fmt, ...); 

struct ase_lsp_prmfns_t
{
	ase_mmgr_t mmgr;
	ase_ccls_t ccls;

	/* utilities */
	struct
	{
		ase_lsp_sprintf_t sprintf;
		ase_lsp_dprintf_t dprintf;
		void* custom_data;
	} misc;
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
	ASE_LSP_ENOINP,
	ASE_LSP_EINPUT,
	ASE_LSP_ENOOUTP,
	ASE_LSP_EOUTPUT,

	ASE_LSP_ESYNTAX,
	ASE_LSP_ERPAREN,
	ASE_LSP_EARGBAD,
	ASE_LSP_EARGFEW,
	ASE_LSP_EARGMANY,
	ASE_LSP_EUNDEFFN,
	ASE_LSP_EBADFN,
	ASE_LSP_EDUPFML,
	ASE_LSP_EBADSYM,
	ASE_LSP_EUNDEFSYM,
	ASE_LSP_EEMPBDY,
	ASE_LSP_EVALBAD,
	ASE_LSP_EDIVBY0
};

typedef ase_lsp_obj_t* (*ase_lsp_prim_t) (ase_lsp_t* lsp, ase_lsp_obj_t* obj);

#ifdef __cplusplus
extern "C" {
#endif

ase_lsp_t* ase_lsp_open (
	const ase_lsp_prmfns_t* prmfns,
	ase_size_t mem_ubound, ase_size_t mem_ubound_inc);

void ase_lsp_close (ase_lsp_t* lsp);

void ase_lsp_geterror (
	ase_lsp_t* lsp, int* errnum, const ase_char_t** errmsg);

void ase_lsp_seterror (
	ase_lsp_t* lsp, int errnum, 
	const ase_char_t** errarg, ase_size_t argcnt);

int ase_lsp_attinput (ase_lsp_t* lsp, ase_lsp_io_t input, void* arg);
int ase_lsp_detinput (ase_lsp_t* lsp);

int ase_lsp_attoutput (ase_lsp_t* lsp, ase_lsp_io_t output, void* arg);
int ase_lsp_detoutput (ase_lsp_t* lsp);

ase_lsp_obj_t* ase_lsp_read (ase_lsp_t* lsp);
ase_lsp_obj_t* ase_lsp_eval (ase_lsp_t* lsp, ase_lsp_obj_t* obj);
int ase_lsp_print (ase_lsp_t* lsp, const ase_lsp_obj_t* obj);

int ase_lsp_addprim (
	ase_lsp_t* lsp, const ase_char_t* name, ase_size_t name_len, 
	ase_lsp_prim_t prim, ase_size_t min_args, ase_size_t max_args);
int ase_lsp_removeprim (ase_lsp_t* lsp, const ase_char_t* name);

/* abort function for assertion. use ASE_LSP_ASSERT instead */
int ase_lsp_assertfail (ase_lsp_t* lsp,
	const ase_char_t* expr, const ase_char_t* desc,
	const ase_char_t* file, int line);


#ifdef __cplusplus
}
#endif

#endif
