/*
 * $Id: Pipe.java 183 2008-06-03 08:18:55Z baconevi $
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
