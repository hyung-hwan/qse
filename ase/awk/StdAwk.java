/*
 * $Id: StdAwk.java,v 1.11 2007/08/26 14:33:38 bacon Exp $
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
		addFunction ("atan", 1, 1); 
		addFunction ("atan2", 2, 2); 
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
	protected int openFile (File file)
	{
		int mode = file.getMode();

		if (mode == File.MODE_READ)
		{
			FileInputStream fis;

			try { fis = new FileInputStream (file.getName()); }
			catch (IOException e) { return -1; }

			Reader rd = new BufferedReader (
				new InputStreamReader (fis)); 
			file.setHandle (rd);
			return 1;
		}
		else if (mode == File.MODE_WRITE)
		{
			FileOutputStream fos;

			try { fos = new FileOutputStream (file.getName()); }
			catch (IOException e) { return -1; }

			Writer wr = new BufferedWriter (
				new OutputStreamWriter (fos));
			file.setHandle (wr);
			return 1;
		}
		else if (mode == File.MODE_APPEND)
		{
			FileOutputStream fos;

			try { fos = new FileOutputStream (file.getName(), true); }
			catch (IOException e) { return -1; }

			Writer wr = new BufferedWriter (
				new OutputStreamWriter (fos));
			file.setHandle (wr);
			return 1;
		}

		return -1;
	}
	
	protected int closeFile (File file)
	{
		int mode = file.getMode();

		if (mode == File.MODE_READ)
		{
			Reader isr = (Reader)file.getHandle();
			try { isr.close (); }
			catch (IOException e) { return -1; }
			return 0;
		}
		else if (mode == File.MODE_WRITE)
		{
			Writer osw = (Writer)file.getHandle();
			try { osw.close (); }
			catch (IOException e) { return -1; }
			return 0;
		}
		else if (mode == File.MODE_APPEND)
		{
			Writer osw = (Writer)file.getHandle();
			try { osw.close (); }
			catch (IOException e) { return -1; }
			return 0;
		}
		
		return -1;
	}

	protected int readFile (File file, char[] buf, int len) 
	{
		int mode = file.getMode();

		if (mode == File.MODE_READ)
		{
			Reader rd = (Reader)file.getHandle();

			try 
			{
				len = rd.read (buf, 0, len);
				if (len == -1) return 0;
			}
			catch (IOException e) { return -1; }
			return len; 
		}

		return -1;
	}

	protected int writeFile (File file, char[] buf, int len) 
	{
		int mode = file.getMode();

		if (mode == File.MODE_WRITE ||
		    mode == File.MODE_APPEND)
		{
			Writer wr = (Writer)file.getHandle();
			try { wr.write (buf, 0, len); }
			catch (IOException e) { return -1; }
			return len;
		}

		return -1;
	}

	protected int flushFile (File file)
	{
		int mode = file.getMode ();

		if (mode == File.MODE_WRITE ||
		    mode == File.MODE_APPEND)
		{
			Writer wr = (Writer)file.getHandle ();
			try { wr.flush (); }
			catch (IOException e) { return -1; }
			return 0;
		}

		return -1;
	}

	private class RWE
	{
		public Writer wr;
		public Reader rd;
		public Reader er;

		public RWE (Writer wr, Reader rd, Reader er)
		{
			this.wr = wr;
			this.rd = rd;
			this.er = er;
		}
	};

	/* == pipe interface == */
	protected int openPipe (Pipe pipe)
	{
		int mode = pipe.getMode();

		if (mode == Pipe.MODE_READ)
		{
			Process proc;
		       
			try { proc = popen (pipe.getName()); }
			catch (IOException e) { return -1; }

			Reader rd = new BufferedReader (
				new InputStreamReader (proc.getInputStream())); 

			pipe.setHandle (rd);
			return 1;
		}
		else if (mode == Pipe.MODE_WRITE)
		{
			Process proc;

			try { proc = popen (pipe.getName()); }
			catch (IOException e) { return -1; }

			Writer wr = new BufferedWriter (
				new OutputStreamWriter (proc.getOutputStream()));
			Reader rd = new BufferedReader (
				new InputStreamReader (proc.getInputStream()));
			Reader er = new BufferedReader (
				new InputStreamReader (proc.getErrorStream()));

			pipe.setHandle (new RWE (wr, rd, er));
			return 1;
		}

		return -1;
	}
	
	protected int closePipe (Pipe pipe)
	{
		int mode = pipe.getMode();

		if (mode == Pipe.MODE_READ)
		{
			Reader rd = (Reader)pipe.getHandle();
			try { rd.close (); }
			catch (IOException e) { return -1; }
			return 0;
		}
		else if (mode == Pipe.MODE_WRITE)
		{
			//Writer wr = (Writer)pipe.getHandle();
			RWE rwe = (RWE)pipe.getHandle();

			try { rwe.wr.close (); }
			catch (IOException e) { return -1; }

			char[] buf = new char[256];

			try 
			{
				while (true)
				{
					int len = rwe.rd.read (buf, 0, buf.length);
					if (len == -1) break;
					System.out.print (new String (buf, 0, len));
				}

				System.out.flush ();
			}
			catch (IOException e) {}

			try
			{
				while (true)
				{
					int len = rwe.er.read (buf, 0, buf.length);
					if (len == -1) break;
					System.err.print (new String (buf, 0, len));
				}

				System.err.flush ();
			}
			catch (IOException e) {}

			try { rwe.rd.close (); } catch (IOException e) {}
			try { rwe.er.close (); } catch (IOException e) {}

			pipe.setHandle (null);
			return 0;
		}
		
		return -1;
	}

	protected int readPipe (Pipe pipe, char[] buf, int len) 
	{
		int mode = pipe.getMode();

		if (mode == Pipe.MODE_READ)
		{
			Reader rd = (Reader)pipe.getHandle();
			try 
			{
				len = rd.read (buf, 0, len);
				if (len == -1) len = 0;
			}
			catch (IOException e) { len = -1; }
			return len; 
		}

		return -1;
	}

	protected int writePipe (Pipe pipe, char[] buf, int len) 
	{
		int mode = pipe.getMode();

		if (mode == Pipe.MODE_WRITE)
		{
			//Writer wr = (Writer)pipe.getHandle ();
			RWE rw = (RWE)pipe.getHandle();
			try 
			{ 
				rw.wr.write (buf, 0, len); 
				rw.wr.flush ();
			}
			catch (IOException e) { return -1; }
			return len;
		}

		return -1;
	}

	protected int flushPipe (Pipe pipe)
	{
		int mode = pipe.getMode ();

		if (mode == Pipe.MODE_WRITE)
		{
			Writer wr = (Writer)pipe.getHandle ();
			try { wr.flush (); }
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

	public Object bfn_atan (long runid, Object[] args) throws Exception
	{
		double x = builtinFunctionArgumentToDouble (runid, args[0]);
		return new Double (Math.atan(x));
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
