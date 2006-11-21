/*
 * $Id: Awk.java,v 1.4 2006-11-21 15:06:14 bacon Exp $
 */

package ase.awk;

import java.io.*;

public abstract class Awk
{
	// mode for open_source & close_source 
	public static final int SOURCE_READ = 1;
	public static final int SOURCE_WRITE = 2;

	static
	{
		System.load ("c://projects//ase/awk/aseawk.dll");
	}

	private long __awk;

	public Awk () throws Exception
	{
		open ();
	}

	public native void close ();
	public native void parse () throws Exception;
	public native void run () throws Exception;
	private native void open () throws Exception;

	/* abstrace methods */
	protected abstract int open_source (int mode);
	protected abstract int close_source (int mode);
	protected abstract int read_source (char[] buf, int len);
	protected abstract int write_source (char[] buf, int len);

	protected int open_extio (Extio extio)
	{
		int type = extio.getType ();
		if (type == Extio.TYPE_CONSOLE) return open_console (extio);
		if (type == Extio.TYPE_FILE) return open_file (extio);
		/*if (type == Extio.TYPE_PIPE) return open_pipe (extio);
		if (type == Extio.TYPE_COPROC) return open_coproc (extio);*/
		return -1;
	}

	protected int close_extio (Extio extio)
	{
		int type = extio.getType ();
		if (type == Extio.TYPE_CONSOLE) return close_console (extio);
		if (type == Extio.TYPE_FILE) return close_file (extio);
		/*if (type == Extio.TYPE_PIPE) return close_pipe (extio);
		if (type == Extio.TYPE_COPROC) return close_coproc (extio);*/
		return -1;
	}

	protected int read_extio (Extio extio, char[] buf, int len)
	{
		int type = extio.getType ();
		if (type == Extio.TYPE_CONSOLE) 
			return read_console (extio, buf, len);
		if (type == Extio.TYPE_FILE) 
			return read_file (extio, buf, len);
		/*if (type == Extio.TYPE_PIPE)
		 * 	return read_pipe (extio, buf, len);
		if (type == Extio.TYPE_COPROC)
			return read_coproc (extio, buf, len);*/
		return -1;
	}

	protected int write_extio (Extio extio, char[] buf, int len)
	{
		int type = extio.getType ();
		if (type == Extio.TYPE_CONSOLE) 
			return write_console (extio, buf, len);
		if (type == Extio.TYPE_FILE) 
			return write_file (extio, buf, len);
		/*if (type == Extio.TYPE_PIPE) 
		 *	return write_pipe (extio, buf, len);
		if (type == Extio.TYPE_COPROC)
			return write_coproc (extio, buf, len);*/
		return -1;
	}

	protected abstract int open_console (Extio extio);
	protected abstract int close_console (Extio extio);
	protected abstract int read_console (Extio extio, char[] buf, int len);
	protected abstract int write_console (Extio extio, char[] buf, int len); 
	protected abstract int next_console (Extio extio, char[] buf, int len);

	protected abstract int open_file (Extio extio);
	protected abstract int close_file (Extio name);
	protected abstract int read_file (Extio extio, char[] buf, int len);
	protected abstract int write_file (Extio extio, char[] buf, int len); 
}
