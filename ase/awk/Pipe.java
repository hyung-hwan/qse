/*
 * $Id: Pipe.java,v 1.2 2007/05/26 10:52:48 bacon Exp $
 */

package ase.awk;

public class Pipe extends IO
{
	public static final int MODE_READ = Extio.MODE_PIPE_READ;
	public static final int MODE_WRITE = Extio.MODE_PIPE_WRITE;

	protected Pipe (Awk awk, Extio extio)
	{
		super (awk, extio);
	}
}
