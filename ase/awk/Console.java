/*
 * $Id: Console.java,v 1.3 2007/10/15 16:10:09 bacon Exp $
 *
 * {License}
 */

package ase.awk;

public class Console extends IO
{
	public static final int MODE_READ = Extio.MODE_CONSOLE_READ;
	public static final int MODE_WRITE = Extio.MODE_CONSOLE_WRITE;

	protected Console (Awk awk, Extio extio)
	{
		super (awk, extio);
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
