/*
 * $Id: Context.java,v 1.2 2007/10/12 16:13:34 bacon Exp $
 */

package ase.awk;

public class Context
{
	private long run;
	private Object custom;

	Context (long run)
	{
		this.run = run;
	}

	public long getId ()
	{
		return this.run;
	}

	public void setCustom (Object custom)
	{
		this.custom = custom;
	}

	public Object getCustom ()
	{
		return this.custom;
	}
}
