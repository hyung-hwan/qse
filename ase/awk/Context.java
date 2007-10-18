/*
 * $Id: Context.java,v 1.6 2007/10/17 14:38:28 bacon Exp $
 */

package ase.awk;

public class Context
{
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

	protected native void stop (long runid);

	// TODO:
	// setGlobal
	// getGlobal
	// setError
	// getError
}
