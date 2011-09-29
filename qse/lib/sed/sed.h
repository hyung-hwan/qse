/*
 * $Id: sed.h 576 2011-09-23 14:52:22Z hyunghwan.chung $
 *
    Copyright 2006-2011 Chung, Hyung-Hwan.
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

#define QSE_SED_CMD_NOOP            QSE_T('\0')
#define QSE_SED_CMD_QUIT            QSE_T('q')

typedef struct qse_sed_adr_t qse_sed_adr_t; 
typedef struct qse_sed_cmd_blk_t qse_sed_cmd_blk_t;

struct qse_sed_adr_t
{
	enum
	{
		QSE_SED_ADR_NONE,     /* no address */
		QSE_SED_ADR_DOL,      /* $ - last line */
		QSE_SED_ADR_LINE,     /* specified line */
		QSE_SED_ADR_REX,      /* lines matching regular expression */
		QSE_SED_ADR_STEP,     /* line steps - only in the second address */
		QSE_SED_ADR_RELLINE,  /* relative line - only in second address */
		QSE_SED_ADR_RELLINEM  /* relative line in the multiples - only in second address */
	} type;

	union 
	{
		qse_size_t lno;
		void*      rex;
	} u;
};

typedef struct qse_sed_app_t qse_sed_app_t;
struct qse_sed_app_t
{
	qse_sed_cmd_t* cmd;
	qse_sed_app_t* next;
};

#define QSE_SED_CMD_QUIT_QUIET      QSE_T('Q')
#define QSE_SED_CMD_APPEND          QSE_T('a')
#define QSE_SED_CMD_INSERT          QSE_T('i')
#define QSE_SED_CMD_CHANGE          QSE_T('c')
#define QSE_SED_CMD_DELETE          QSE_T('d')
#define QSE_SED_CMD_DELETE_FIRSTLN  QSE_T('D')
#define QSE_SED_CMD_PRINT_LNNUM     QSE_T('=')
#define QSE_SED_CMD_PRINT           QSE_T('p')
#define QSE_SED_CMD_PRINT_FIRSTLN   QSE_T('P')
#define QSE_SED_CMD_PRINT_CLEARLY   QSE_T('l')
#define QSE_SED_CMD_HOLD            QSE_T('h')
#define QSE_SED_CMD_HOLD_APPEND     QSE_T('H')
#define QSE_SED_CMD_RELEASE         QSE_T('g')
#define QSE_SED_CMD_RELEASE_APPEND  QSE_T('G')
#define QSE_SED_CMD_EXCHANGE        QSE_T('x') 
#define QSE_SED_CMD_NEXT            QSE_T('n')
#define QSE_SED_CMD_NEXT_APPEND     QSE_T('N')
#define QSE_SED_CMD_READ_FILE       QSE_T('r')
#define QSE_SED_CMD_READ_FILELN     QSE_T('R')
#define QSE_SED_CMD_WRITE_FILE      QSE_T('w')
#define QSE_SED_CMD_WRITE_FILELN    QSE_T('W')
#define QSE_SED_CMD_BRANCH          QSE_T('b') 
#define QSE_SED_CMD_BRANCH_COND     QSE_T('t')
#define QSE_SED_CMD_SUBSTITUTE      QSE_T('s')
#define QSE_SED_CMD_TRANSLATE       QSE_T('y')
#define QSE_SED_CMD_CLEAR_PATTERN   QSE_T('z')

struct qse_sed_cmd_t
{
	qse_char_t type;
	qse_sed_loc_t loc;

	int negated;

	qse_sed_adr_t a1; /* optional start address */
	qse_sed_adr_t a2; /* optional end address */

	union
	{
		/* text for the a, i, c commands */
		qse_xstr_t text;  

		/* file name for r, w, R, W */
		qse_xstr_t file;

		/* data for the s command */
		struct
		{
			void* rex; /* regular expression */
			qse_xstr_t rpl;  /* replacement */

			/* flags */
			qse_xstr_t file; /* file name for w */
			unsigned short occ;
			unsigned short g: 1; /* global */
			unsigned short p: 1; /* print */
			unsigned short i: 1; /* case insensitive */
		} subst;

		/* translation set for the y command */
		qse_xstr_t transet;

		/* branch target for b and t */
		struct
		{
			qse_xstr_t label;
			qse_sed_cmd_t* target;
		} branch;
	} u;	

	struct
	{
		int a1_matched;
		qse_size_t a1_match_line;

		int c_ready;

		/* points to the next command for fast traversal and 
		 * fast random jumps */
		qse_sed_cmd_t* next; 
	} state;
};

struct qse_sed_cmd_blk_t
{
	qse_size_t         len;	
	qse_sed_cmd_t      buf[256];
	qse_sed_cmd_blk_t* next;
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
		qse_sed_loc_t loc;     /**< location */
		qse_cint_t cc;         /**< last character read */
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
			qse_str_t subst;
		} txt;

		/** indicates if a successful substitution has been made 
		 *  since the last read on the input stream. */
		int subst_done;
		void* last_rex;	

		/** stop requested */
		int stopreq;

		/** hook function */
		qse_sed_exec_hook_t hook;
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
	qse_sed_t* sed, 
	qse_sed_errnum_t errnum
);


#ifdef USE_REX
/**
 * The qse_sed_getmaxdepth() gets the maximum processing depth.
 */
qse_size_t qse_sed_getmaxdepth (
	qse_sed_t*      sed, /**< stream editor */
	qse_sed_depth_t id   /**< one of qse_sed_depth_t values */
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
