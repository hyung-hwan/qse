/*
 * $Id: Console.java,v 1.1 2007/05/26 10:23:52 bacon Exp $
 */

package ase.awk;

public class Console extends Extio
{
	public static final int MODE_READ = MODE_CONSOLE_READ;
	public static final int MODE_WRITE = MODE_CONSOLE_WRITE;

	private Awk awk;

	protected Console (Awk awk, Extio extio)
	{
		super (extio.getName(), TYPE_CONSOLE, 
		       extio.getMode(), extio.getRunId());
		setHandle (extio.getHandle());
		this.awk = awk;
	}

	public void setFileName (String name) throws Exception
	{
		if (getMode() == MODE_READ)
		{
			awk.setfilename (getRunId(), name);
		}
		else 
		{
			awk.setofilename (getRunId(), name);
		}
	}
}
