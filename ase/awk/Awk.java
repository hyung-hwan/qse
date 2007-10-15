/*
 * $Id: Awk.java,v 1.19 2007/10/14 05:28:26 bacon Exp $
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

	protected final static Reader stdin = 
		new BufferedReader (new InputStreamReader (System.in));

	protected final static Writer stdout =
		new BufferedWriter (new OutputStreamWriter (System.out));

	private long handle;

	public Awk () throws Exception
	{
		this.handle = 0;
		this.functionTable = new HashMap ();
		open ();
	}

	/* == just in case == */
	protected void finalize () throws Throwable
	{
		if (handle != 0) close ();
		super.finalize ();
	}

	/* == native methods == */
	private native void open () throws Exception;
	public  native void close ();
	public  native void parse () throws Exception;
	public  native void run (String main, String[] args) throws Exception;
	public  native void stop ();

	private native int getmaxdepth (int id);
	private native void setmaxdepth (int id, int depth);

	private native int getoption ();
	private native void setoption (int opt);

	private native boolean getdebug ();
	private native void setdebug (boolean debug);

	private native void setword (String ow, String nw);

	private native void addfunc (
		String name, int min_args, int max_args) throws Exception;
	private native void delfunc (String name) throws Exception;

	native void setfilename (
		long runid, String name) throws Exception;
	native void setofilename (
		long runid, String name) throws Exception;

	private native Object strtonum (
		long runid, String str) throws Exception;
	private native String valtostr (
		long runid, Object obj) throws Exception;

	protected native String strftime (String fmt, long sec);
	protected native String strfgmtime (String fmt, long sec);
	protected native int system (String cmd);

	/* == simpler run methods == */
	public void run (String main) throws Exception
	{
		run (main, null);
	}

	public void run (String[] args) throws Exception
	{
		run (null, args);
	}

	public void run () throws Exception
	{
		run (null, null);
	}

	/* == builtin functions == */
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

	protected long builtinFunctionArgumentToLong (
		long runid, Object obj) throws Exception
	{
		long n;

		if (obj == null) n = 0;
		else
		{
			if (obj instanceof String)
				obj = strtonum (runid, (String)obj);

			if (obj instanceof Long)
			{
				n = ((Long)obj).longValue ();
			}
			else if (obj instanceof Double)
			{
				n = ((Double)obj).longValue ();
			}
			else if (obj instanceof Integer)
			{
				n = ((Integer)obj).longValue ();
			}
			else if (obj instanceof Short)
			{
				n = ((Short)obj).longValue ();
			}
			else if (obj instanceof Float)
			{
				n = ((Float)obj).longValue ();
			}
			else n = 0;
		}

		return n;
	}

	protected double builtinFunctionArgumentToDouble (
		long runid, Object obj) throws Exception
	{
		double n;

		if (obj == null) n = 0.0;
		else
		{
			if (obj instanceof String)
				obj = strtonum (runid, (String)obj);

			if (obj instanceof Long)
			{
				n = ((Long)obj).doubleValue ();
			}
			else if (obj instanceof Double)
			{
				n = ((Double)obj).doubleValue ();
			}
			else if (obj instanceof Integer)
			{
				n = ((Integer)obj).doubleValue ();
			}
			else if (obj instanceof Short)
			{
				n = ((Short)obj).doubleValue ();
			}
			else if (obj instanceof Float)
			{
				n = ((Float)obj).doubleValue ();
			}
			else n = 0.0;
		}

		return n;
	}

	protected String builtinFunctionArgumentToString (
		long runid, Object obj) throws Exception
	{
		String str;

		if (obj == null) str = "";
		else if (obj instanceof String) str = (String)obj;
		else str = valtostr (runid, obj);

		return str;
	}

	/* == console name setters == */
	protected void setConsoleInputName (
		Extio extio, String name) throws Exception
	{
		/* TODO: setfilename is not safe. for example, it can 
		 * crash the program if runid is invalid. so this wrapper
		 * needs to do some sanity check. */
		setfilename (extio.getRunId(), name);
	}

	protected void setConsoleOutputName (
		Extio extio, String name) throws Exception
	{
		setofilename (extio.getRunId(), name);
	}

	/* == depth limiting == */
	public int getMaxDepth (int id)
	{
		return getmaxdepth (id);
	}

	public void setMaxDepth (int ids, int depth)
	{
		setmaxdepth (ids, depth);
	}
	
	/* == option == */
	public int getOption ()
	{
		return getoption ();
	}

	public void setOption (int opt)
	{
		setoption (opt);
	}

	/* == debug == */
	public boolean getDebug ()
	{
		return getdebug ();
	}

	public void setDebug (boolean debug)
	{
		setdebug (debug);
	}

	public void setWord (String ow, String nw)
	{
		setword (ow, nw);
	}

	public void unsetWord (String ow)
	{
		setword (ow, null);
	}

	public void unsetAllWords ()
	{
		setword (null, null);
	}

	/* == intrinsic function handling == */
	protected Object handleFunction (
		long run, String name, Object args[]) throws java.lang.Exception
	{
		String mn = (String)functionTable.get(name);
		// name should always be found in this table.
		// otherwise, there is something wrong with this program. 

		Class c = this.getClass ();
		Class[] a = { Context.class, String.class, Object[].class };

		// TODO: remove new Context ....
		Method m = c.getMethod (mn, a);
		return m.invoke (this, 
			new Object[] { new Context(run), name, args}) ;
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
}
