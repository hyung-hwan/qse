/*
 * $Id: Context.java,v 1.7 2007/10/29 15:20:13 bacon Exp $
 */

package ase.awk;

public class Context
{
	public static int GLOBAL_ARGC = 0;
	public static int GLOBAL_ARGV = 1;
	public static int GLOBAL_CONVFMT = 2;
	public static int GLOBAL_FILENAME = 3;
	public static int GLOBAL_FNR = 4;
	public static int GLOBAL_FS = 5;
	public static int GLOBAL_IGNORECASE = 6;
	public static int GLOBAL_NF = 7;
	public static int GLOBAL_NR = 8;
	public static int GLOBAL_OFILENAME = 9;
	public static int GLOBAL_OFMT = 10;
	public static int GLOBAL_OFS = 11;
	public static int GLOBAL_ORS = 12;
	public static int GLOBAL_RLENGTH = 13;
	public static int GLOBAL_RS = 14;
	public static int GLOBAL_RSTART = 15;
	public static int GLOBAL_SUBSEP = 16;

	private Awk awk;
	private long runid;
	private Object custom;

	Context (Awk awk)
	{
		this.awk = awk;
		this.runid = 0;
		this.custom = null;
	}

	public Awk getAwk ()
	{
		return awk;
	}

	public long getId ()
	{
		return this.runid;
	}

	public void setCustom (Object custom)
	{
		this.custom = custom;
	}

	public Object getCustom ()
	{
		return this.custom;
	}

	public void setConsoleInputName (String name) throws Exception
	{
		awk.setfilename (this.runid, name);
	}

	public void setConsoleOutputName (String name) throws Exception
	{
		awk.setofilename (this.runid, name);
	}

	public void stop ()
	{
		stop (this.runid);
	}

	public void setGlobal (int id, Return ret) throws Exception
	{
		// regardless of the result, the value field
		// of the return object is reset to 0 by setglobal.
		setglobal (this.runid, id, ret);
	}

	/*
	public void getGlobal (int id, Argument arg) throws Exception
	{
	}
	*/

	protected native void stop (long runid);
	protected native void setglobal (long runid, int id, Return ret);

	// TODO:
	// setGlobal
	// getGlobal
	// setError
	// getError
}
