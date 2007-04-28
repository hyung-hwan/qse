/*
 * $Id: File.java,v 1.1 2007/05/26 10:23:52 bacon Exp $
 */

package ase.awk;

public class File extends Extio
{
	public static final int MODE_READ = MODE_FILE_READ;
	public static final int MODE_WRITE = MODE_FILE_WRITE;
	public static final int MODE_APPEND = MODE_FILE_APPEND;

	private Awk awk;

	protected File (Awk awk, Extio extio)
	{
		super (extio.getName(), TYPE_FILE, 
		       extio.getMode(), extio.getRunId());
		setHandle (extio.getHandle());
		this.awk = awk;
	}
}
