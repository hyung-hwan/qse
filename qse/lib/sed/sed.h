/*
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

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

#ifndef _QSE_LIB_SED_SED_H_
#define _QSE_LIB_SED_SED_H_

#include <qse/sed/sed.h>
#include <qse/cmn/str.h>

#define QSE_MAP_AS_RBT
#include <qse/cmn/map.h>

/* structure to maintain data to append
 * at the end of each cycle, triggered by r, R, and a */
typedef struct qse_sed_app_t qse_sed_app_t;
struct qse_sed_app_t
{
	qse_sed_cmd_t* cmd;
	qse_sed_app_t* next;
};

typedef struct qse_sed_cmd_blk_t qse_sed_cmd_blk_t;
struct qse_sed_cmd_blk_t
{
	qse_size_t         len;	
	qse_sed_cmd_t      buf[256];
	qse_sed_cmd_blk_t* next;
};

/* structure to maintain list of compiliation
 * identifiers */
typedef struct qse_sed_cid_t qse_sed_cid_t;
struct qse_sed_cid_t
{
	qse_sed_cid_t* next;		
};

/* special structure to represent an unknown cid
 * used once the action of setting a new cid fails */
typedef struct qse_sed_unknown_cid_t qse_sed_unknown_cid_t;
struct qse_sed_unknown_cid_t
{
	qse_sed_cid_t* next;		
	qse_char_t buf[1];
};

/** 
 * The qse_sed_t type defines a stream editor 
 */
struct qse_sed_t
{
	qse_mmgr_t* mmgr;

	qse_sed_errstr_t errstr; /**< error string getter */
	qse_sed_errnum_t errnum; /**< stores an error number */
	qse_char_t errmsg[128];  /**< error message holder */
	qse_sed_loc_t errloc;    /**< location of the last error */

	struct 
	{
		int                  trait;	
		qse_sed_tracer_t     tracer;
		qse_sed_lformatter_t lformatter;

		struct
		{
			struct
			{
				qse_size_t build;
				qse_size_t match; 
			} rex;
		} depth; /* useful only for rex.h */
	} opt;

	qse_sed_ecb_t* ecb;

	/** source text pointers */
	struct
	{
		qse_sed_io_impl_t fun; /**< input stream handler */
		qse_sed_io_arg_t  arg;
		qse_char_t        buf[1024];
		int               eof;

		qse_sed_cid_t*         cid;
		qse_sed_unknown_cid_t  unknown_cid;

		qse_sed_loc_t     loc; /**< location */
		qse_cint_t        cc;  /**< last character read */
		const qse_char_t* ptr; /**< beginning of the source text */
		const qse_char_t* end; /**< end of the source text */
		const qse_char_t* cur; /**< current source text pointer */
	} src;

	/** temporary data for compiling */
	struct
	{
		qse_str_t rex; /**< regular expression buffer */
		qse_str_t lab; /**< label name buffer */

		/** data structure to compile command groups */
		struct
		{
			/** current level of command group nesting */
			int level;
			/** keeps track of the begining of nested groups */
			qse_sed_cmd_t* cmd[128];
		} grp;

		/** a table storing labels seen */
		qse_map_t labs; 
	} tmp;

	/** compiled commands */
	struct
	{
		qse_sed_cmd_blk_t  fb; /**< the first block is static */
		qse_sed_cmd_blk_t* lb; /**< points to the last block */

		qse_sed_cmd_t      quit; 
		qse_sed_cmd_t      quit_quiet; 
		qse_sed_cmd_t      again;
		qse_sed_cmd_t      over;
	} cmd;

	/** data for execution */
	struct
	{
		/** data needed for output streams and files */
		struct
		{
			qse_sed_io_impl_t fun; /**< an output handler */
			qse_sed_io_arg_t arg; /**< output handling data */

			qse_char_t buf[2048];
			qse_size_t len;
			int        eof;

			/*****************************************************/
			/* the following two fields are very tightly-coupled.
			 * don't make any partial changes */
			qse_map_t  files;
			qse_sed_t* files_ext;
			/*****************************************************/
		} out;

		/** data needed for input streams */
		struct
		{
			qse_sed_io_impl_t fun; /**< input handler */
			qse_sed_io_arg_t arg; /**< input handling data */

			qse_char_t xbuf[1]; /**< read-ahead buffer */
			int xbuf_len; /**< data length in the buffer */

			qse_char_t buf[2048]; /**< input buffer */
			qse_size_t len; /**< data length in the buffer */
			qse_size_t pos; /**< current position in the buffer */
			int        eof; /**< EOF indicator */

			qse_str_t line; /**< pattern space */
			qse_size_t num; /**< current line number */
		} in;

		struct
		{
			qse_size_t count; /* number of append entries in a static buffer. */
			qse_sed_app_t s[16]; /* handle up to 16 appends in a static buffer */
			struct
			{
				qse_sed_app_t* head;
				qse_sed_app_t* tail;
			} d;
		} append;

		/** text buffers */
		struct
		{
			qse_str_t hold; /* hold space */
			qse_str_t scratch;
		} txt;

		struct
		{
			qse_size_t  nflds; /**< the number of fields */
			qse_size_t  cflds; /**< capacity of flds field */
			qse_cstr_t  sflds[128]; /**< static field buffer */
			qse_cstr_t* flds;
			int delimited;
		} cutf;

		/** indicates if a successful substitution has been made 
		 *  since the last read on the input stream. */
		int subst_done;
		void* last_rex;	

		/** stop requested */
		int stopreq;
	} e;
};

#if defined(__cplusplus)
extern "C" {
#endif

int qse_sed_init (
	qse_sed_t* sed, 
	qse_mmgr_t* mmgr
);

void qse_sed_fini (
	qse_sed_t* sed
);

const qse_char_t* qse_sed_dflerrstr (
	const qse_sed_t* sed, 
	qse_sed_errnum_t errnum
);

#if defined(__cplusplus)
}
#endif

#endif
