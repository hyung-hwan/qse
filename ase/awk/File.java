/*
 * $Id: File.java 115 2008-03-03 11:13:15Z baconevi $
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
