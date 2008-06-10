/*
 * $Id: IO.java 183 2008-06-03 08:18:55Z baconevi $
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
