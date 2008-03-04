/*
 * $Id: Pipe.java 115 2008-03-03 11:13:15Z baconevi $
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
