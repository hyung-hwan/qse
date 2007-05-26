/*
 * $Id: StdAwk.java,v 1.9 2007/05/25 14:41:48 bacon Exp $
 *
 * {License}
 */

package ase.awk;

import java.io.*;

public abstract class StdAwk extends Awk
{
	private long seed;
	private java.util.Random random;

	public StdAwk () throws Exception
	{
		super ();

		seed = System.currentTimeMillis();
		random = new java.util.Random (seed);

		addFunction ("sin", 1, 1); 
		addFunction ("cos", 1, 1); 
		addFunction ("tan", 1, 1); 
		addFunction ("atan2", 1, 1); 
		addFunction ("log", 1, 1); 
		addFunction ("exp", 1, 1); 
		addFunction ("sqrt", 1, 1); 
		addFunction ("int", 1, 1); 

		addFunction ("srand", 0, 1); 
		addFunction ("rand", 0, 0); 

		addFunction ("systime", 0, 0);
		addFunction ("strftime", 0, 2);
		addFunction ("strfgmtime", 0, 2);

		addFunction ("system", 1, 1); 
	}

	/* == file interface == */
	protected int openFile (Extio extio)
	{
		int mode = extio.getMode();

		if (mode == Extio.MODE_FILE_READ)
		{
			FileInputStream fis;
			Reader isr;

			try { fis = new FileInputStream (extio.getName()); }
			catch (IOException e) { return -1; }

			isr = new BufferedReader (new InputStreamReader (fis)); 
			extio.setHandle (isr);
			return 1;
		}
		else if (mode == Extio.MODE_FILE_WRITE)
		{
			FileOutputStream fos;
			Writer osw;

			try { fos = new FileOutputStream (extio.getName()); }
			catch (IOException e) { return -1; }

			osw = new BufferedWriter (new OutputStreamWriter (fos));
			extio.setHandle (osw);
			return 1;
		}
		else if (mode == Extio.MODE_FILE_APPEND)
		{
			FileOutputStream fos;
			Writer osw;

			try { fos = new FileOutputStream (extio.getName(), true); }
			catch (IOException e) { return -1; }

			osw = new BufferedWriter (new OutputStreamWriter (fos));
			extio.setHandle (osw);
			return 1;
		}

		return -1;
	}
	
	protected int closeFile (Extio extio)
	{
		int mode = extio.getMode();

		if (mode == Extio.MODE_FILE_READ)
		{
			Reader isr;
			isr = (Reader)extio.getHandle();
			if (isr != null)
			{
				try { isr.close (); }
				catch (IOException e) { return -1; }
			}
			return 0;
		}
		else if (mode == Extio.MODE_FILE_WRITE)
		{
			Writer osw;
			osw = (Writer)extio.getHandle();
			if (osw != null)
			{
				try { osw.close (); }
				catch (IOException e) { return -1; }
			}
			return 0;
		}
		else if (mode == Extio.MODE_FILE_APPEND)
		{
			Writer osw;
			osw = (Writer)extio.getHandle();
			if (osw != null)
			{
				try { osw.close (); }
				catch (IOException e) { return -1; }
			}
			return 0;
		}
		
		return -1;
	}

	protected int readFile (Extio extio, char[] buf, int len) 
	{
		int mode = extio.getMode();

		if (mode == Extio.MODE_FILE_READ)
		{
			Reader isr;
			isr = (Reader)extio.getHandle();

			try 
			{
				len = isr.read (buf, 0, len);
				if (len == -1) len = 0;
			}
			catch (IOException e) { len = -1; }
			return len; 
		}

		return -1;
	}

	protected int writeFile (Extio extio, char[] buf, int len) 
	{
		int mode = extio.getMode();

		if (mode == Extio.MODE_FILE_WRITE ||
		    mode == Extio.MODE_FILE_APPEND)
		{
			Writer osw;
			osw = (Writer)extio.getHandle();
			try { osw.write (buf, 0, len); }
			catch (IOException e) { len = -1; }
			return len;
		}

		return -1;
	}

	protected int flushFile (Extio extio)
	{
		int mode = extio.getMode ();

		if (mode == Extio.MODE_FILE_WRITE ||
		    mode == Extio.MODE_FILE_APPEND)
		{
			Writer osw;
			osw = (Writer)extio.getHandle ();
			try { osw.flush (); }
			catch (IOException e) { return -1; }
			return 0;
		}

		return -1;
	}

	/* == pipe interface == */
	protected int openPipe (Extio extio)
	{
		int mode = extio.getMode();

		if (mode == Extio.MODE_PIPE_READ)
		{

			Process proc;
			Reader isr;
		       
			try { proc = popen (extio.getName()); }
			catch (IOException e) { return -1; }

			isr = new BufferedReader (new InputStreamReader (proc.getInputStream())); 
			extio.setHandle (isr);
			return 1;
		}
		else if (mode == Extio.MODE_PIPE_WRITE)
		{
			Process proc;
			Writer osw;

			try { proc = popen (extio.getName()); }
			catch (IOException e) { return -1; }

			osw = new BufferedWriter (new OutputStreamWriter (proc.getOutputStream()));
			extio.setHandle (osw);
			return 1;
		}

		return -1;
	}
	
	protected int closePipe (Extio extio)
	{
		int mode = extio.getMode();

		if (mode == Extio.MODE_PIPE_READ)
		{
			Reader isr;
			isr = (Reader)extio.getHandle();
			if (isr != null)
			{
				try { isr.close (); }
				catch (IOException e) { return -1; }
			}
			return 0;
		}
		else if (mode == Extio.MODE_PIPE_WRITE)
		{
			Writer osw;
			osw = (Writer)extio.getHandle();
			if (osw != null)
			{
				try { osw.close (); }
				catch (IOException e) { return -1; }
			}
			return 0;
		}
		
		return -1;
	}

	protected int readPipe (Extio extio, char[] buf, int len) 
	{
		int mode = extio.getMode();

		if (mode == Extio.MODE_PIPE_READ)
		{
			Reader isr;
			isr = (Reader)extio.getHandle();

			try 
			{
				len = isr.read (buf, 0, len);
				if (len == -1) len = 0;
			}
			catch (IOException e) { len = -1; }
			return len; 
		}

		return -1;
	}

	protected int writePipe (Extio extio, char[] buf, int len) 
	{
		int mode = extio.getMode();

		if (mode == Extio.MODE_PIPE_WRITE)
		{
			Writer osw;
			osw = (Writer)extio.getHandle();
			try { osw.write (buf, 0, len); }
			catch (IOException e) { len = -1; }
			return len;
		}

		return -1;
	}

	protected int flushPipe (Extio extio)
	{
		int mode = extio.getMode ();

		if (mode == Extio.MODE_PIPE_WRITE)
		{
			Writer osw;
			osw = (Writer)extio.getHandle ();
			try { osw.flush (); }
			catch (IOException e) { return -1; }
			return 0;
		}

		return -1;
	}


	/* == arithmetic built-in functions */
	public Object bfn_sin (long runid, Object[] args)  throws Exception
	{
		double x = builtinFunctionArgumentToDouble (runid, args[0]);
		return new Double (Math.sin(x));
	}

	public Object bfn_cos (long runid, Object[] args) throws Exception
	{
		double x = builtinFunctionArgumentToDouble (runid, args[0]);
		return new Double (Math.cos(x));
	}

	public Object bfn_tan (long runid, Object[] args) throws Exception
	{
		double x = builtinFunctionArgumentToDouble (runid, args[0]);
		return new Double (Math.tan(x));
	}

	public Object bfn_atan2 (long runid, Object[] args) throws Exception
	{
		double y = builtinFunctionArgumentToDouble (runid, args[0]);
		double x = builtinFunctionArgumentToDouble (runid, args[1]);
		return new Double (Math.atan2(y,x));
	}

	public Object bfn_log (long runid, Object[] args) throws Exception
	{
		double x = builtinFunctionArgumentToDouble (runid, args[0]);
		return new Double (Math.log(x));
	}

	public Object bfn_exp (long runid, Object[] args) throws Exception
	{
		double x = builtinFunctionArgumentToDouble (runid, args[0]);
		return new Double (Math.exp(x));
	}

	public Object bfn_sqrt (long runid, Object[] args) throws Exception
	{
		double x = builtinFunctionArgumentToDouble (runid, args[0]);
		return new Double (Math.sqrt(x));
	}

	public Object bfn_int (long runid, Object[] args) throws Exception
	{
		long x = builtinFunctionArgumentToLong (runid, args[0]);
		return new Long (x);
	}

	public Object bfn_rand (long runid, Object[] args)
	{
		return new Double (random.nextDouble ());
	}

	public Object bfn_srand (long runid, Object[] args) throws Exception
	{
		long prev_seed = seed;

		seed = (args == null || args.length == 0)?
			System.currentTimeMillis ():
			builtinFunctionArgumentToLong (runid, args[0]);

		random.setSeed (seed);
		return new Long (prev_seed);
	}

	public Object bfn_systime (long runid, Object[] args) 
	{
		long msec = System.currentTimeMillis ();
		return new Long (msec / 1000);
	}

	public Object bfn_strftime (long runid, Object[] args) throws Exception
	{
		String fmt = (args.length<1)? "%c": builtinFunctionArgumentToString (runid, args[0]);
		long t = (args.length<2)? (System.currentTimeMillis()/1000): builtinFunctionArgumentToLong (runid, args[1]);
		return strftime (fmt, t);
	}

	public Object bfn_strfgmtime (long runid, Object[] args) throws Exception
	{
		String fmt = (args.length<1)? "%c": builtinFunctionArgumentToString (runid, args[0]);
		long t = (args.length<2)? (System.currentTimeMillis()/1000): builtinFunctionArgumentToLong (runid, args[1]);
		return strfgmtime (fmt, t);
	}

	/* miscellaneous built-in functions */
	public Object bfn_system (long runid, Object[] args) throws Exception
	{
		String str = builtinFunctionArgumentToString (runid, args[0]);
		return system (str);

		/*
		String str = builtinFunctionArgumentToString (runid, args[0]);
		Process proc = null;
		int n = 0;

		str = builtinFunctionArgumentToString (runid, args[0]);

		try { proc = popen (str); }
		catch (IOException e) { n = -1; }

		if (proc != null)
		{
			InputStream is;
			byte[] buf = new byte[1024];

			is = proc.getInputStream(); 

			// TODO; better error handling... program execution.. io redirection??? 
			try { while (is.read (buf) != -1) ; } 
			catch (IOException e) { n = -1; };

			try { n = proc.waitFor (); } 
			catch (InterruptedException e) 
			{ 
				proc.destroy (); 
				n = -1; 
			}
		}

		return new Long (n);	
		*/
	}

	/* == utility functions == */
	private Process popen (String command) throws IOException
	{
		String full;

		/* TODO: consider OS names and versions */
		full = System.getenv ("ComSpec");
		if (full != null)
		{	
			full = full + " /c " + command;
		}
		else
		{
			full = System.getenv ("SHELL");
			if (full != null)
			{
				full = "/bin/sh -c \"" + command + "\"";
			}
			else full = command;
		}

		return Runtime.getRuntime().exec (full);
	}
}
