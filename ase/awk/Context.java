/*
 * $Id: Context.java,v 1.3 2007/10/14 16:34:57 bacon Exp $
 */

package ase.awk;

public class Context
{
	private long handle;
	private Object custom;

	Context (long handle)
	{
		this.handle = handle;
	}

	public long getId ()
	{
		return this.handle;
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
