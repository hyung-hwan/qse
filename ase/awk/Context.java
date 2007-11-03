/*
 * $Id: Context.java,v 1.10 2007/11/02 05:49:19 bacon Exp $
 *
 * {License}
 */

package ase.awk;

import java.util.Stack;

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

	protected Awk awk;
	protected long runid;
	protected Object custom;
	protected Stack clearableStack;

	Context (Awk awk)
	{
		this.awk = awk;
		this.runid = 0;
		this.custom = null;
		this.clearableStack = new Stack ();
	}

	void clear ()
	{
		Clearable obj;
		while ((obj = popClearable()) != null) obj.clear ();
	}

	void pushClearable (Clearable obj)
	{
		clearableStack.push (obj);
	}

	Clearable popClearable ()
	{
		if (clearableStack.empty()) return null;
		return (Clearable)clearableStack.pop ();
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

	public Argument getGlobal (int id) throws Exception
	{
		return getglobal (this.runid, id);
	}

	protected native void stop (long runid);
	protected native void setglobal (long runid, int id, Return ret);
	protected native Argument getglobal (long runid, int id);

	// TODO:
	// setError
	// getError
}
