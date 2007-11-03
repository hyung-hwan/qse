/*
 * $Id: IO.java,v 1.2 2007/11/02 05:49:19 bacon Exp $
 *
 * {License}
 */

package ase.awk;

class IO 
{
	protected Awk awk;
	protected Extio extio;
	protected Object handle;

	protected IO (Awk awk, Extio extio)
	{
		this.awk = awk;
		this.extio = extio;
	}

	public String getName ()
	{
		return extio.getName();
	}

	public int getMode ()
	{
		return extio.getMode();
	}

	public long getRunId ()
	{
		return extio.getRunId();
	}

	public void setHandle (Object handle)
	{
		this.handle = handle;
	}

	public Object getHandle ()
	{
		return handle;
	}

}
