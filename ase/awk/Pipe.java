/*
 * $Id: Pipe.java,v 1.3 2007/11/02 05:49:19 bacon Exp $
 *
 * {License}
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
