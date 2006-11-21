package ase.awk;

public class Extio
{
	public static final int TYPE_PIPE = 0;
	public static final int TYPE_COPROC = 1;
	public static final int TYPE_FILE = 2;
	public static final int TYPE_CONSOLE = 3;

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
	private Object handle;

	protected Extio (String name, int type, int mode)
	{
		this.name = name;
		this.type = type;
		this.mode = mode;
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

	public void setHandle (Object handle)
	{
		this.handle = handle;
	}

	public Object getHandle ()
	{
		return this.handle;
	}

	protected void finalize ()
	{
		System.out.println ("Extio being finalized....");
	}
};
