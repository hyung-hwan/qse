/*
 * $Id: Awk.java,v 1.2 2006-10-24 06:03:14 bacon Exp $
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

	public Awk () throws ase.awk.Exception
	{
		open ();
	}

	public native void close ();
	public native void parse () throws ase.awk.Exception;
	public native void run () throws ase.awk.Exception;
	private native void open () throws ase.awk.Exception;

	/*
	protected native void set_extio (long extio, Object obj);
	protected native Object get_extio (long extio);
	*/

	/* abstrace methods */
	protected abstract int open_source (int mode);
	protected abstract int close_source (int mode);
	protected abstract int read_source (char[] buf, int len);
	protected abstract int write_source (char[] buf, int len);

	protected abstract int open_console ();
	protected abstract int close_console ();
	protected abstract int read_console (char[] buf, int len);
	protected abstract int write_console (char[] buf, int len); 
	protected abstract int next_console (char[] buf, int len);

	protected abstract int open_file (Extio extio);
	protected abstract int close_file (String name);
}
