/*
 * $Id: Extio.java 115 2008-03-03 11:13:15Z baconevi $
 *
 * {License}
 */

package ase.awk;

public class Extio
{
	protected static final int TYPE_PIPE = 0;
	protected static final int TYPE_COPROC = 1;
	protected static final int TYPE_FILE = 2;
	protected static final int TYPE_CONSOLE = 3;

	/* PROBLEMS WITH GCJ 3.4.6 if these fields are protected.
	 * 
	protected static final int MODE_PIPE_READ = 0;
	protected static final int MODE_PIPE_WRITE = 1;

	protected static final int MODE_FILE_READ = 0;
	protected static final int MODE_FILE_WRITE = 1;
	protected static final int MODE_FILE_APPEND = 2;

	protected static final int MODE_CONSOLE_READ = 0;
	protected static final int MODE_CONSOLE_WRITE = 1;
	*/
	public static final int MODE_PIPE_READ = 0;
	public static final int MODE_PIPE_WRITE = 1;
	public static final int MODE_FILE_READ = 0;
	public static final int MODE_FILE_WRITE = 1;
	public static final int MODE_FILE_APPEND = 2;
	public static final int MODE_CONSOLE_READ = 0;
	public static final int MODE_CONSOLE_WRITE = 1;

	private String name;
	private int type;
	private int mode;
	private long run_id;
	private Object handle;

	protected Extio (String name, int type, int mode, long run_id)
	{
		this.name = name;
		this.type = type;
		this.mode = mode;
		this.run_id = run_id;
		this.handle = null;
	}

	public String getName ()
	{
		return this.name;
	}

	public int getType ()
	{
		return this.type;
	}

	public int getMode ()
	{
		return this.mode;
	}

	public long getRunId ()
	{
		return this.run_id;
	}

	public void setHandle (Object handle)
	{
		this.handle = handle;
	}

	public Object getHandle ()
	{
		return this.handle;
	}
};
