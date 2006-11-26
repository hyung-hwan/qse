/*
 * $Id: StdAwk.java,v 1.5 2006-11-26 15:55:43 bacon Exp $
 */

package ase.awk;

import java.io.*;

public abstract class StdAwk extends Awk
{
	private InputStreamReader src_in   = null;
	private OutputStreamWriter src_out = null;

	private String[] sin  = null;
	private int sin_no    = 0;
	private String sout   = null;

	private String[] cin  = null;
	private int cin_no    = 0;
	private String[] cout = null;
	private int cout_no   = 0;

	public StdAwk () throws Exception
	{
		super ();
	}

	/* == major methods == */
	public void parse () throws Exception
	{
		sin = getSourceNames (); sin_no = 0;
		sout = getDeparsedSourceName ();
		super.parse ();
	}

	public void run () throws Exception
	{
		cin = getInputConsoleNames (); cin_no = 0;
		cout = getOutputConsoleNames (); cout_no = 0;
		super.run ();
	}

	/* == source code names == */
	protected abstract String[] getSourceNames ();
	protected String getDeparsedSourceName () { return null; }

	/* == console names == */
	protected abstract String[] getInputConsoleNames ();
	protected abstract String[] getOutputConsoleNames ();

	/* == source code == */
	protected int openSource (int mode)
	{
		if (mode == SOURCE_READ)
		{
			InputStreamReader isr;
			sin_no = 0;

			if (sin_no >= sin.length) return 0;
			isr = get_input_stream (sin[sin_no]);
			if (isr == null) return -1;

			src_in = isr;
			sin_no++;
			return 1;
		}
		else if (mode == SOURCE_WRITE)
		{
			OutputStreamWriter osw;
			if (sout == null) return 1;
			osw = get_output_stream (sout);
			if (osw == null) return -1;
			src_out = osw;
			return 1;
		}

		return -1;
	}

	protected int closeSource (int mode)
	{
		if (mode == SOURCE_READ)
		{
			try { src_in.close (); }
			catch (IOException e) { return -1; }
			return 0;
		}
		else if (mode == SOURCE_WRITE)
		{
			if (src_out == null) return 0;
			try { src_out.close (); }
			catch (IOException e) { return -1; }
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
				InputStreamReader isr;
				if (sin_no >= sin.length) return 0;

				isr = get_input_stream (sin[sin_no]);
				if (isr == null) return -1;

				try { src_in.close (); }
				catch (IOException ec) { /* ignore */ }

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
		try { src_out.write (buf, 0, len); }
		catch (IOException e) { return -1; }
		return len;
	}

	/* == console interface == */
	protected int openConsole (Extio extio)
	{
		System.err.println ("[open_console called.... name: " + extio.getName() + " mode: " + extio.getMode());

		int mode = extio.getMode ();

		if (mode == Extio.MODE_CONSOLE_READ)
		{
			InputStreamReader isr;
			cin_no = 0;

			if (cin_no >= cin.length) return 0;
			isr = get_input_stream (cin[cin_no]);
			if (isr == null) return -1;

			extio.setHandle (isr);
			setInputConsoleName (extio, cin[cin_no]);

			cin_no++;
			return 1;
		}
		else if (mode == Extio.MODE_CONSOLE_WRITE)
		{
			OutputStreamWriter osw;
			cout_no = 0;
		       
			if (cout_no >= cout.length) return 0;
			osw = get_output_stream (cout[cout_no]);
			if (osw == null) return -1;

			extio.setHandle (osw);
			setOutputConsoleName (extio, cout[cout_no]);

			cout_no++;
			return 1;
		}

		return -1;
	}

	protected int closeConsole (Extio extio)
	{
		System.err.println ("[close_console called.... name: " + extio.getName() + " mode: " + extio.getMode());

		int mode = extio.getMode ();

		if (mode == Extio.MODE_CONSOLE_READ)
		{
			InputStreamReader isr = (InputStreamReader)extio.getHandle ();
			try { isr.close (); }
			catch (IOException e) { return -1; }
			return 0;
		}
		else if (mode == Extio.MODE_CONSOLE_WRITE)
		{
			OutputStreamWriter osw = (OutputStreamWriter)extio.getHandle ();
			/* TODO: selective close the stream...
			 * system.out should not be closed??? */
			try { osw.close (); }
			catch (IOException e) { return -1; }
			return 0;
		}

		return -1;
	}

	protected int readConsole (Extio extio, char[] buf, int len)
	{
		int mode = extio.getMode ();

		if (mode == Extio.MODE_CONSOLE_READ)
		{
			InputStreamReader isr, tmp;
			int n;
		       
			isr = (InputStreamReader)extio.getHandle ();

			try { n = isr.read (buf, 0, len); }
			catch  (IOException e) { return -1; }

			while (n == -1)
			{
				if (cin_no >= cin.length) return 0;
				tmp = get_input_stream (cin[cin_no]);
				if (tmp == null) return -1;

				try { isr.close (); }
				catch (IOException e) { /* ignore */ }

				extio.setHandle (tmp);
				setInputConsoleName (extio, cin[cin_no]);
				isr = (InputStreamReader)extio.getHandle ();
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

		if (mode == Extio.MODE_CONSOLE_WRITE)
		{
			OutputStreamWriter osw;
			osw = (OutputStreamWriter)extio.getHandle ();
			// as the write operation below doesn't indicate 
			// if it has reached the end, console can't be
			// switched here unlike read_console.
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
			OutputStreamWriter osw;
			osw = (OutputStreamWriter)extio.getHandle ();
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
			InputStreamReader isr, tmp;

			isr = (InputStreamReader)extio.getHandle ();

			if (cin_no >= cin.length) return 0;
			tmp = get_input_stream (cin[cin_no]);
			if (tmp == null) return -1;

			try { isr.close (); }
			catch (IOException e) { /* ignore */ }

			extio.setHandle (tmp);
			setInputConsoleName (extio, cin[cin_no]);

			cin_no++;
			return 1;
		}
		else if (mode == Extio.MODE_CONSOLE_WRITE)
		{
			OutputStreamWriter osw, tmp;

			osw = (OutputStreamWriter)extio.getHandle ();

			if (cout_no >= cout.length) return 0;
			tmp = get_output_stream (cout[cout_no]);
			if (tmp == null) return -1;

			/* TODO: selectively close the stream...
			 * system.out should not be closed??? */
			try { osw.close (); }
			catch (IOException e) { /* ignore */ }

			extio.setHandle (tmp);
			setOutputConsoleName (extio, cout[cout_no]);

			cout_no++;
			return 1;
		}

		return -1;
	}

	private InputStreamReader get_input_stream (String name)
	{
		InputStreamReader isr;

		if (name == null || name.length() == 0)
		{
			isr = new InputStreamReader (System.in);
		}
		else
		{
			FileInputStream fis;
			try { fis = new FileInputStream (name); }
			catch (IOException e) { return null; }
			isr = new InputStreamReader (fis);
		}

		return isr;
	}

	private OutputStreamWriter get_output_stream (String name)
	{
		OutputStreamWriter osw;

		if (name == null || name.length() == 0)
		{
			osw = new OutputStreamWriter (System.out);
		}
		else
		{
			FileOutputStream fos;
			try { fos = new FileOutputStream (name); }
			catch (IOException e) { return null; }
			osw = new OutputStreamWriter (fos);
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
			InputStreamReader isr;

			try { fis = new FileInputStream (extio.getName()); }
			catch (IOException e) { return -1; }

			isr = new InputStreamReader (fis); 
			extio.setHandle (isr);
			return 1;
		}
		else if (mode == Extio.MODE_FILE_WRITE)
		{
			FileOutputStream fos;
			OutputStreamWriter osw;

			try { fos = new FileOutputStream (extio.getName()); }
			catch (IOException e) { return -1; }

			osw = new OutputStreamWriter (fos);
			extio.setHandle (osw);
			return 1;
		}
		else if (mode == Extio.MODE_FILE_APPEND)
		{
			FileOutputStream fos;
			OutputStreamWriter osw;

			try { fos = new FileOutputStream (extio.getName(), true); }
			catch (IOException e) { return -1; }

			osw = new OutputStreamWriter (fos); 
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
			InputStreamReader isr;
			isr = (InputStreamReader)extio.getHandle();
			try { isr.close (); }
			catch (IOException e) { return -1; }
			return 0;
		}
		else if (mode == Extio.MODE_FILE_WRITE)
		{
			OutputStreamWriter osw;
			osw = (OutputStreamWriter)extio.getHandle();
			try { osw.close (); }
			catch (IOException e) { return -1; }
			return 0;
		}
		else if (mode == Extio.MODE_FILE_APPEND)
		{
			OutputStreamWriter osw;
			osw = (OutputStreamWriter)extio.getHandle();
			try { osw.close (); }
			catch (IOException e) { return -1; }
			return 0;
		}
		
		return -1;
	}

	protected int readFile (Extio extio, char[] buf, int len) 
	{
		int mode = extio.getMode();

		if (mode == Extio.MODE_FILE_READ)
		{
			InputStreamReader isr;
			isr = (InputStreamReader)extio.getHandle();

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
			OutputStreamWriter osw;
			osw = (OutputStreamWriter)extio.getHandle();
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
			OutputStreamWriter osw;
			osw = (OutputStreamWriter)extio.getHandle ();
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
			InputStreamReader isr;
		       
			try { proc = Runtime.getRuntime().exec (extio.getName()); }
			catch (IOException e) { return -1; }
			isr = new InputStreamReader (proc.getInputStream()); 
			extio.setHandle (isr);
			return 1;
		}
		else if (mode == Extio.MODE_PIPE_WRITE)
		{
			Process proc;
			OutputStreamWriter osw;

			try { proc = Runtime.getRuntime().exec (extio.getName()); }
			catch (IOException e) { return -1; }
			osw = new OutputStreamWriter (proc.getOutputStream());
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
			InputStreamReader isr;
			isr = (InputStreamReader)extio.getHandle();
			try { isr.close (); }
			catch (IOException e) { return -1; }
			return 0;
		}
		else if (mode == Extio.MODE_PIPE_WRITE)
		{
			OutputStreamWriter osw;
			osw = (OutputStreamWriter)extio.getHandle();
			try { osw.close (); }
			catch (IOException e) { return -1; }
			return 0;
		}
		
		return -1;
	}

	protected int readPipe (Extio extio, char[] buf, int len) 
	{
		int mode = extio.getMode();

		if (mode == Extio.MODE_PIPE_READ)
		{
			InputStreamReader isr;
			isr = (InputStreamReader)extio.getHandle();

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
			OutputStreamWriter osw;
			osw = (OutputStreamWriter)extio.getHandle();
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
			OutputStreamWriter osw;
			osw = (OutputStreamWriter)extio.getHandle ();
			try { osw.flush (); }
			catch (IOException e) { return -1; }
			return 0;
		}

		return -1;
	}

}
