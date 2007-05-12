/*
 * $Id: StdAwk.java,v 1.5 2007/05/11 16:25:38 bacon Exp $
 *
 * {License}
 */

package ase.awk;

import java.io.*;

public abstract class StdAwk extends Awk
{
	private Reader src_in  = null;
	private Writer src_out = null;

	private String[] sin  = null;
	private int sin_no    = 0;

	private String[] sout = null;
	private int sout_no = 0;

	private String[] cin  = null;
	private int cin_no    = 0;
	private String[] cout = null;
	private int cout_no   = 0;

	private long seed;
	private java.util.Random random;

	private final static Reader stdin = 
		new BufferedReader (new InputStreamReader (System.in));

	private final static Writer stdout =
		new BufferedWriter (new OutputStreamWriter (System.out));

	/*
	static
	{
		stdin = new BufferedReader (
			new InputStreamReader (System.in));
		stdout = new BufferedWriter ( 
			new OutputStreamWriter (System.out));
	}
	*/

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

		addFunction ("system", 1, 1); 
	}

	/* == major methods == */
	public void parse () throws Exception
	{
		sin = sourceInputNames (); sin_no = 0;
		sout = sourceOutputNames (); sout_no = 0;
		super.parse ();
	}

	public void run (String main, String[] args) throws Exception
	{
		cin = consoleInputNames (); cin_no = 0;
		cout = consoleOutputNames (); cout_no = 0;
		super.run (main, args);
	}

	public void run (String main) throws Exception
	{
		run (main, null);
	}

	public void run (String[] args) throws Exception
	{
		run (null, args);
	}

	public void run () throws Exception
	{
		run (null, null);
	}


	/* == source code names == */
	protected String[] sourceInputNames () { return null; }
	protected String[] sourceOutputNames () { return null; }

	/* == console names == */
	protected String[] consoleInputNames () { return null; }
	protected String[] consoleOutputNames () { return null; }

	/* == source code == */
	protected int openSource (int mode)
	{
		if (mode == SOURCE_READ)
		{
			Reader isr;
			sin_no = 0;

			if (sin == null || sin_no >= sin.length) return 0;

			isr = get_stream_reader (sin[sin_no]);
			if (isr == null) return -1;

			src_in = isr;
			sin_no++;
			return 1;
		}
		else if (mode == SOURCE_WRITE)
		{
			Writer osw;
			sout_no = 0;

			// when sout is null, the source output is treated
			// as if the operation has been successful with 
			// the writeSource and closeSource method.
			if (sout == null) return 1;
			if (sout_no >= sin.length) return 0;

			osw = get_stream_writer (sout[sout_no]);
			if (osw == null) return -1;

			src_out = osw;
			sout_no++;
			return 1;
		}

		return -1;
	}

	protected int closeSource (int mode)
	{
		if (mode == SOURCE_READ)
		{
			if (src_in != StdAwk.stdin)
			{
				try { src_in.close (); }
				catch (IOException e) { return -1; }
			}
			return 0;
		}
		else if (mode == SOURCE_WRITE)
		{
			if (src_out == null) return 0;

			if (src_out != StdAwk.stdout) 
			{
				try { src_out.close (); }
				catch (IOException e) { return -1; }
			}
			return 0;
		}

		return -1;
	}

	protected int readSource (char[] buf, int len)
	{
		try { 
			int n = src_in.read (buf, 0, len); 
			while (n == -1)
			{
				Reader isr;
				if (sin == null || sin_no >= sin.length) return 0;

				isr = get_stream_reader (sin[sin_no]);
				if (isr == null) return -1;

				if (src_in != StdAwk.stdin)
				{
					try { src_in.close (); }
					catch (IOException ec) { /* ignore */ }
				}

				src_in = isr;
				sin_no++;

				n = src_in.read (buf, 0, len); 
			}

			return n;
		}
		catch (IOException e) 
		{ 
			return -1; 
		}
	}

	protected int writeSource (char[] buf, int len)
	{
		if (src_out == null) return len;

		// only the first file is used at the moment.
		// this is because the write message doesn't indicate 
		// the end of the output stream.
		try { src_out.write (buf, 0, len); }
		catch (IOException e) { return -1; }

		return len;
	}

	/* == console interface == */
	protected int openConsole (Extio extio)
	{
		//System.err.println ("[openConsole called.... name: " + extio.getName() + " mode: " + extio.getMode());

		int mode = extio.getMode ();

		if (mode == Extio.MODE_CONSOLE_READ)
		{
			/*InputStream*/Reader isr;
			cin_no = 0;

			if (cin == null || cin_no >= cin.length) return 0;
			isr = get_stream_reader (cin[cin_no]);
			if (isr == null) return -1;

			extio.setHandle (isr);

			try { setConsoleInputName (extio, cin[cin_no]); }
			catch (Exception e) { return -1; }

			cin_no++;
			return 1;
		}
		else if (mode == Extio.MODE_CONSOLE_WRITE)
		{
			Writer osw;
			cout_no = 0;
		       
			if (cout == null || cout_no >= cout.length) return 0;

			osw = get_stream_writer (cout[cout_no]);
			if (osw == null) return -1;

			extio.setHandle (osw);
			try { setConsoleOutputName (extio, cout[cout_no]); }
			catch (Exception e) { return -1; }

			cout_no++;
			return 1;
		}

		return -1;
	}

	protected int closeConsole (Extio extio)
	{
		//System.err.println ("[closeConsole called.... name: " + extio.getName() + " mode: " + extio.getMode());

		int mode = extio.getMode ();

		if (mode == Extio.MODE_CONSOLE_READ)
		{
			Reader isr = (Reader)extio.getHandle ();

			if (isr != StdAwk.stdin)
			{
				try { isr.close (); }
				catch (IOException e) { return -1; }
			}
			return 0;
		}
		else if (mode == Extio.MODE_CONSOLE_WRITE)
		{
			Writer osw = (Writer)extio.getHandle ();
			/* TODO: selective close the stream...
			 * system.out should not be closed??? */

			if (osw != StdAwk.stdout)
			{
				try { osw.close (); }
				catch (IOException e) { return -1; }
			}
			return 0;
		}

		return -1;
	}

	protected int readConsole (Extio extio, char[] buf, int len)
	{
		int mode = extio.getMode ();

		if (mode == Extio.MODE_CONSOLE_READ)
		{
			Reader isr, tmp;
			int n;
		       
			isr = (Reader)extio.getHandle ();

			try { n = isr.read (buf, 0, len); }
			catch  (IOException e) { return -1; }

			while (n == -1)
			{
				if (cin == null || cin_no >= cin.length) return 0;
				tmp = get_stream_reader (cin[cin_no]);
				if (tmp == null) return -1;

				if (isr != StdAwk.stdin) 
				{
					try { isr.close (); }
					catch (IOException e) { /* ignore */ }
				}

				extio.setHandle (tmp);
				try { setConsoleInputName (extio, cin[cin_no]); }
				catch (Exception e) { return -1; }

				isr = (Reader)extio.getHandle ();
				cin_no++;

				try { n = isr.read (buf, 0, len); }
				catch (IOException e) { return -1; }
			}

			return n;
		}

		return -1;
	}

	protected int writeConsole (Extio extio, char[] buf, int len) 
	{
		int mode = extio.getMode ();

		//System.err.println ("[writeConsole called name: " + extio.getName() + " mode: " + extio.getMode());

		if (mode == Extio.MODE_CONSOLE_WRITE)
		{
			Writer osw;

			osw = (Writer)extio.getHandle ();
			// as the write operation below doesn't 
			// indicate if it has reached the end, console 
			// can't be switched here unlike in read_console
			try { osw.write (buf, 0, len); osw.flush (); }
			catch (IOException e) { return -1; }

			return len;
		}

		return -1;
	}

	protected int flushConsole (Extio extio)
	{
		int mode = extio.getMode ();

		if (mode == Extio.MODE_CONSOLE_WRITE)
		{
			Writer osw;

			osw = (Writer)extio.getHandle ();

			try { osw.flush (); }
			catch (IOException e) { return -1; }

			return 0;
		}

		return -1;
	}

	protected int nextConsole (Extio extio)
	{
		int mode = extio.getMode ();

		if (mode == Extio.MODE_CONSOLE_READ)
		{
			Reader isr, tmp;

			isr = (Reader)extio.getHandle ();

			if (cin == null || cin_no >= cin.length) return 0;
			tmp = get_stream_reader (cin[cin_no]);
			if (tmp == null) return -1;

			if (isr != StdAwk.stdin)
			{
				try { isr.close (); }
				catch (IOException e) { /* ignore */ }
			}

			extio.setHandle (tmp);
			try { setConsoleInputName (extio, cin[cin_no]); }
			catch (Exception e) { return -1; }

			cin_no++;
			return 1;
		}
		else if (mode == Extio.MODE_CONSOLE_WRITE)
		{
			Writer osw, tmp;

			osw = (Writer)extio.getHandle ();

			if (cout == null || cout_no >= cout.length) return 0;
			tmp = get_stream_writer (cout[cout_no]);
			if (tmp == null) return -1;

			if (osw != StdAwk.stdout)
			{
				try { osw.close (); }
				catch (IOException e) { /* ignore */ }
			}

			extio.setHandle (tmp);
			try { setConsoleOutputName (extio, cout[cout_no]); }
			catch (Exception e) { return -1;}

			cout_no++;
			return 1;
		}

		return -1;
	}

	private Reader get_stream_reader (String name)
	{
		Reader isr;

		if (name == null || name.length() == 0) isr = StdAwk.stdin;
		else
		{
			FileInputStream fis;
			try { fis = new FileInputStream (name); }
			catch (IOException e) { return null; }
			isr = new BufferedReader(new InputStreamReader (fis));
		}

		return isr;
	}

	private Writer get_stream_writer (String name)
	{
		Writer osw;

		if (name == null || name.length() == 0) osw = StdAwk.stdout;
		else
		{
			FileOutputStream fos;
			try { fos = new FileOutputStream (name); }
			catch (IOException e) { return null; }
			osw = new BufferedWriter (new OutputStreamWriter (fos));
		}

		return osw;
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
			if (isr != StdAwk.stdin)
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
			if (osw != StdAwk.stdout)
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
			if (osw != StdAwk.stdout)
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
			if (isr != StdAwk.stdin)
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
			if (osw != StdAwk.stdout)
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

	public Object bfn_strftime (long runid, Object[] args)
	{
		// TODO: implement this...
		return null;
	}

	/* miscellaneous built-in functions */
	public Object bfn_system (long runid, Object[] args) throws Exception
	{
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
