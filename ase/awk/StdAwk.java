/*
 * $Id: StdAwk.java,v 1.2 2006-11-24 15:04:23 bacon Exp $
 */

package ase.awk;

import java.io.*;

public abstract class StdAwk extends Awk
{
	private String[] cin;
	private int cin_no;
	private String[] cout;
	private int cout_no;

	public StdAwk () throws Exception
	{
		super ();

		cin = getInputConsoleNames ();
		cout = getOutputConsoleNames ();
		cin_no = 0;
		cout_no = 0;
	}

	/* ===== standard console names ===== */
	protected abstract String[] getInputConsoleNames ();
	protected abstract String[] getOutputConsoleNames ();

	/* ===== console ===== */
	protected int open_console (Extio extio)
	{
		System.err.println ("[open_console called.... name: " + extio.getName() + " mode: " + extio.getMode());

		int mode = extio.getMode ();

		if (mode == Extio.MODE_CONSOLE_READ)
		{
			InputStreamReader isr;
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

	protected int close_console (Extio extio)
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

	protected int read_console (Extio extio, char[] buf, int len)
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

	protected int write_console (Extio extio, char[] buf, int len) 
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

	protected int flush_console (Extio extio)
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

	protected int next_console (Extio extio)
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

			/* TODO: selective close the stream...
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

	/* ===== file ===== */
	public int open_file (Extio extio)
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
	
	public int close_file (Extio extio)
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

	protected int read_file (Extio extio, char[] buf, int len) 
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

	protected int write_file (Extio extio, char[] buf, int len) 
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

	protected int flush_file (Extio extio)
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

	/* ===== pipe ===== */
	public int open_pipe (Extio extio)
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
	
	public int close_pipe (Extio extio)
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

	protected int read_pipe (Extio extio, char[] buf, int len) 
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

	protected int write_pipe (Extio extio, char[] buf, int len) 
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

	protected int flush_pipe (Extio extio)
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
