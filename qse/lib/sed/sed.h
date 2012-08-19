/*
 * $Id$
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _QSE_LIB_SED_SED_H_
#define _QSE_LIB_SED_SED_H_

#include <qse/sed/sed.h>
#include <qse/cmn/str.h>

#define QSE_MAP_AS_RBT
#include <qse/cmn/map.h>

/* 
 * Define USE_REX to use rex.h on behalf of tre.h 
 * rex.h currently does not support backreference.
 */
#ifdef USE_REX
enum qse_sed_depth_t
{
     QSE_SED_DEPTH_REX_BUILD = (1 << 0),
     QSE_SED_DEPTH_REX_MATCH = (1 << 1)
};
typedef enum qse_sed_depth_t qse_sed_depth_t;
#endif

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
	QSE_DEFINE_COMMON_FIELDS (sed)

	qse_sed_errstr_t errstr; /**< error string getter */
	qse_sed_errnum_t errnum; /**< stores an error number */
	qse_char_t errmsg[128];  /**< error message holder */
	qse_sed_loc_t errloc;    /**< location of the last error */

	int option;              /**< stores options */

	qse_sed_lformatter_t lformatter;

	struct
	{
		struct
		{
			qse_size_t build;
			qse_size_t match; 
		} rex;
	} depth;

	/** source text pointers */
	struct
	{
		qse_sed_io_fun_t  fun; /**< input stream handler */
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
			qse_sed_io_fun_t fun; /**< an output handler */
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
			qse_sed_io_fun_t fun; /**< an input handler */
			qse_sed_io_arg_t arg; /**< input handling data */

			qse_char_t xbuf[1]; /**< a read-ahead buffer */
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

#ifdef QSE_ENABLE_SEDTRACER
		/** trace function */
		qse_sed_exec_tracer_t tracer;
#endif
	} e;
};

#ifdef __cplusplus
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

#ifdef USE_REX
/**
 * The qse_sed_getmaxdepth() gets the maximum processing depth.
 */
qse_size_t qse_sed_getmaxdepth (
	const qse_sed_t* sed, /**< stream editor */
	qse_sed_depth_t  id   /**< one of qse_sed_depth_t values */
);

/**
 * The qse_sed_setmaxdepth() sets the maximum processing depth.
 */
void qse_sed_setmaxdepth (
	qse_sed_t* sed,  /**< stream editor */
	int        ids,  /**< 0 or a number OR'ed of #qse_sed_depth_t values */
	qse_size_t depth /**< maximum depth level */
);
#endif

#ifdef __cplusplus
}
#endif

#endif
