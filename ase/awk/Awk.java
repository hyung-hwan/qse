/*
 * $Id: Awk.java,v 1.28 2007/10/21 13:58:47 bacon Exp $
 *
 * {License}
 */

package ase.awk;

import java.io.*;
import java.util.HashMap;
import java.lang.reflect.Method;
import java.lang.reflect.InvocationTargetException;

public abstract class Awk
{
	private HashMap functionTable;

	// mode for open_source & close_source 
	public static final int SOURCE_READ = 1;
	public static final int SOURCE_WRITE = 2;

	// depth id
	public static final int DEPTH_BLOCK_PARSE = (1 << 0);
	public static final int DEPTH_BLOCK_RUN   = (1 << 1);
	public static final int DEPTH_EXPR_PARSE  = (1 << 2);
	public static final int DEPTH_EXPR_RUN    = (1 << 3);
	public static final int DEPTH_REX_BUILD   = (1 << 4);
	public static final int DEPTH_REX_MATCH   = (1 << 5);

	// options
	public static final int OPTION_IMPLICIT    = (1 << 0);
	public static final int OPTION_EXPLICIT    = (1 << 1);
	public static final int OPTION_UNIQUEFN    = (1 << 2);
	public static final int OPTION_SHADING     = (1 << 3);
	public static final int OPTION_SHIFT       = (1 << 4);
	public static final int OPTION_IDIV        = (1 << 5);
	public static final int OPTION_STRCONCAT   = (1 << 6);
	public static final int OPTION_EXTIO       = (1 << 7);
	public static final int OPTION_COPROC      = (1 << 8);
	public static final int OPTION_BLOCKLESS   = (1 << 9);
	public static final int OPTION_ASEONE      = (1 << 10);
	public static final int OPTION_STRIPSPACES = (1 << 11);
	public static final int OPTION_NEXTOFILE   = (1 << 12);
	public static final int OPTION_CRLF        = (1 << 13);
	public static final int OPTION_ARGSTOMAIN  = (1 << 14);
	public static final int OPTION_RESET       = (1 << 15);
	public static final int OPTION_MAPTOVAR    = (1 << 16);
	public static final int OPTION_PABLOCK     = (1 << 17);

	protected final static Reader stdin = new BufferedReader (new InputStreamReader (System.in));
	protected final static Writer stdout = new BufferedWriter (new OutputStreamWriter (System.out));

	private long awkid;

	public Awk () throws Exception
	{
		this.awkid = 0;
		this.functionTable = new HashMap ();
		open ();
	}

	/* == just in case == */
	protected void finalize () throws Throwable
	{
		close ();
		super.finalize ();
	}

	public void close ()
	{
		if (this.awkid != 0)
		{
			close (this.awkid);
			this.awkid = 0;
		}
	}

	public void parse () throws Exception
	{
		parse (this.awkid);
	}

	public void run (String main, String[] args) throws Exception
	{
		run (this.awkid, main, args);
	}

	public void run (String main) throws Exception
	{
		run (this.awkid, main, null);
	}

	public void run (String[] args) throws Exception
	{
		run (this.awkid, null, args);
	}

	public void run () throws Exception
	{
		run (this.awkid, null, null);
	}

	public void stop () throws Exception
	{
		stop (this.awkid);
	}

	/* == native methods == */
	private   native void    open () throws Exception;
	protected native void    close (long awkid);
	protected native void    parse (long awkid) throws Exception;
	protected native void    run (long awkid, String main, String[] args) throws Exception;
	protected native void    stop (long awkid) throws Exception;
	protected native int     getmaxdepth (long awkid, int id) throws Exception;
	protected native void    setmaxdepth (long awkid, int id, int depth) throws Exception;
	protected native int     getoption (long awkid) throws Exception;
	protected native void    setoption (long awkid, int opt) throws Exception;
	protected native boolean getdebug (long awkid) throws Exception;
	protected native void    setdebug (long awkid, boolean debug) throws Exception;
	protected native void    setword (long awkid, String ow, String nw) throws Exception;


	protected native void    addfunc (String name, int min_args, int max_args) throws Exception;
	protected native void    delfunc (String name) throws Exception;
	          native void    setfilename (long runid, String name) throws Exception;
	          native void    setofilename (long runid, String name) throws Exception;
	protected native String  strftime (String fmt, long sec);
	protected native String  strfgmtime (String fmt, long sec);
	protected native int     system (String cmd);


	/* == intrinsic functions == */
	public void addFunction (String name, int min_args, int max_args) throws Exception
	{
		addFunction (name, min_args, max_args, name);
	}

	public void addFunction (String name, int min_args, int max_args, String method) throws Exception
	{
		if (functionTable.containsKey (name))
		{
			throw new Exception (
				"cannot add existing function '" + name + "'", 
				Exception.EXIST);
		}

		functionTable.put (name, method);
		try { addfunc (name, min_args, max_args); }
		catch (Exception e)
		{
			functionTable.remove (name);
			throw e;
		}
	}

	public void deleteFunction (String name) throws Exception
	{
		delfunc (name); 
		functionTable.remove (name);
	}

	protected Object handleFunction (
		Context ctx, String name, Argument[] args) throws Exception
	{
		String mn = (String)functionTable.get(name);
		// name should always be found in this table.
		// otherwise, there is something wrong with this program. 

		Class c = this.getClass ();
		Class[] a = { Context.class, String.class, Argument[].class };

		try
		{
			Method m = c.getMethod (mn, a);
			return m.invoke (this, /*new Object[] {*/ ctx, name, args/*}*/) ;
		}
		catch (java.lang.reflect.InvocationTargetException e)
		{
			/* the underlying method has throw an exception */
			Throwable t = e.getCause();
			if (t == null) 
			{
				throw new Exception (null, Exception.BFNIMPL);
			}
			else if (t instanceof Exception) 
			{
				throw (Exception)t;
			}
			else 
			{
				throw new Exception (
					t.getMessage(), Exception.BFNIMPL);
			}
		}
		catch (java.lang.Exception e)
		{
			throw new Exception (e.getMessage(), Exception.BFNIMPL);
		}
	}

	/* == depth limiting == */
	public int getMaxDepth (int id) throws Exception
	{
		return getmaxdepth (this.awkid, id);
	}

	public void setMaxDepth (int ids, int depth) throws Exception
	{
		setmaxdepth (this.awkid, ids, depth);
	}
	
	/* == option == */
	public int getOption () throws Exception
	{
		return getoption (this.awkid);
	}

	public void setOption (int opt) throws Exception
	{
		setoption (this.awkid, opt);
	}

	/* == debug == */
	public boolean getDebug () throws Exception
	{
		return getdebug (this.awkid);
	}

	public void setDebug (boolean debug) throws Exception
	{
		setdebug (this.awkid, debug);
	}

	/* == word replacement == */
	public void setWord (String ow, String nw) throws Exception
	{
		setword (this.awkid, ow, nw);
	}

	public void unsetWord (String ow) throws Exception
	{
		setword (this.awkid, ow, null);
	}

	public void unsetAllWords () throws Exception
	{
		setword (this.awkid, null, null);
	}

	/* == source code management == */
	protected abstract int openSource (int mode);
	protected abstract int closeSource (int mode);
	protected abstract int readSource (char[] buf, int len);
	protected abstract int writeSource (char[] buf, int len);

	/* == external io interface == */
	protected int openExtio (Extio extio)
	{
		switch (extio.getType())
		{
			case Extio.TYPE_CONSOLE:
			{
				Console con = new Console (this, extio);
				int n = openConsole (con);
				extio.setHandle (con);
				return n;
			}

			case Extio.TYPE_FILE:
			{
				File file = new File (this, extio);
				int n = openFile (file);
				extio.setHandle (file);
				return n;
			}

			case Extio.TYPE_PIPE:
			{
				Pipe pipe = new Pipe (this, extio);
				int n = openPipe (pipe);
				extio.setHandle (pipe);
				return n;
			}
		}

		return -1;
	}

	protected int closeExtio (Extio extio)
	{
		switch (extio.getType())
		{
			case Extio.TYPE_CONSOLE: 
				return closeConsole (
					(Console)extio.getHandle());

			case Extio.TYPE_FILE:
				return closeFile ((File)extio.getHandle());

			case Extio.TYPE_PIPE:
				return closePipe ((Pipe)extio.getHandle());
		}

		return -1;
	}

	protected int readExtio (Extio extio, char[] buf, int len)
	{
		// this check is needed because 0 is used to indicate
		// the end of the stream. java streams can return 0 
		// if the data given is 0 bytes and it didn't reach 
		// the end of the stream. 
		if (len <= 0) return -1;

		switch (extio.getType())
		{

			case Extio.TYPE_CONSOLE: 
			{
				return readConsole (
					(Console)extio.getHandle(), buf, len);
			}

			case Extio.TYPE_FILE:
			{
				return readFile (
					(File)extio.getHandle(), buf, len);
			}

			case Extio.TYPE_PIPE:
			{
				return readPipe (
					(Pipe)extio.getHandle(), buf, len);
			}
		}

		return -1;
	}

	protected int writeExtio (Extio extio, char[] buf, int len)
	{
		if (len <= 0) return -1;

		switch (extio.getType())
		{
			case Extio.TYPE_CONSOLE: 
			{
				return writeConsole (
					(Console)extio.getHandle(), buf, len);
			}

			case Extio.TYPE_FILE:
			{
				return writeFile (
					(File)extio.getHandle(), buf, len);
			}

			case Extio.TYPE_PIPE:
			{
				return writePipe (
					(Pipe)extio.getHandle(), buf, len);
			}
		}

		return -1;
	}

	protected int flushExtio (Extio extio)
	{
		switch (extio.getType())
		{

			case Extio.TYPE_CONSOLE: 
			{
				return flushConsole ((Console)extio.getHandle());
			}

			case Extio.TYPE_FILE:
			{
				return flushFile ((File)extio.getHandle());
			}

			case Extio.TYPE_PIPE:
			{
				return flushPipe ((Pipe)extio.getHandle());
			}
		}

		return -1;
	}

	protected int nextExtio (Extio extio)
	{
		int type = extio.getType ();

		switch (extio.getType())
		{
			case Extio.TYPE_CONSOLE:
			{
				return nextConsole ((Console)extio.getHandle());
			}
		}

		return -1;
	}

	protected abstract int openConsole (Console con);
	protected abstract int closeConsole (Console con);
	protected abstract int readConsole (Console con, char[] buf, int len);
	protected abstract int writeConsole (Console con, char[] buf, int len);
	protected abstract int flushConsole (Console con);
	protected abstract int nextConsole (Console con);

	protected abstract int openFile (File file);
	protected abstract int closeFile (File file);
	protected abstract int readFile (File file, char[] buf, int len);
	protected abstract int writeFile (File file, char[] buf, int len); 
	protected abstract int flushFile (File file);

	protected abstract int openPipe (Pipe pipe);
	protected abstract int closePipe (Pipe pipe);
	protected abstract int readPipe (Pipe pipe, char[] buf, int len);
	protected abstract int writePipe (Pipe pipe, char[] buf, int len); 
	protected abstract int flushPipe (Pipe pipe);

	/* TODO: ...
	protected void onRunStart (Context ctx) {}
	protected void onRunEnd (Context ctx) {}
	protected void onRunReturn (Context ctx) {}
	protected void onRunStatement (Context ctx) {}
	*/
}
