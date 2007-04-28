/*
 * $Id: Pipe.java,v 1.1 2007/05/26 10:23:52 bacon Exp $
 */

package ase.awk;

public class Pipe extends Extio
{
	public static final int MODE_READ = MODE_PIPE_READ;
	public static final int MODE_WRITE = MODE_PIPE_WRITE;

	private Awk awk;

	protected Pipe (Awk awk, Extio extio)
	{
		super (extio.getName(), TYPE_PIPE, 
		       extio.getMode(), extio.getRunId());
		setHandle (extio.getHandle());
		this.awk = awk;
	}
}
