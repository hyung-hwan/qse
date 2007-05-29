/*
 * $Id: Console.java,v 1.2 2007/05/26 10:52:48 bacon Exp $
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
