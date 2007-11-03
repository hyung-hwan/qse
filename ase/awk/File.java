/*
 * $Id: File.java,v 1.3 2007/11/02 05:49:19 bacon Exp $
 *
 * {License}
 */

package ase.awk;

public class File extends IO
{
	public static final int MODE_READ = Extio.MODE_FILE_READ;
	public static final int MODE_WRITE = Extio.MODE_FILE_WRITE;
	public static final int MODE_APPEND = Extio.MODE_FILE_APPEND;

	protected File (Awk awk, Extio extio)
	{
		super (awk, extio);
	}

}
