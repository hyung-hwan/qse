/*
 * $Id: Awk.java,v 1.4 2006-11-22 05:58:26 bacon Exp $
 */

package ase.test.awk;

import java.io.*;

public class Awk extends ase.awk.Awk
{
	private FileReader insrc;
	private FileWriter outsrc;

	public Awk () throws ase.awk.Exception
	{
		super ();
	}

	protected int open_source (int mode)
	{
		if (mode == SOURCE_READ)
		{
			try { insrc = new FileReader ("t.awk"); }
			catch (IOException e) { return -1; }
			return 1;
		}
		else if (mode == SOURCE_WRITE)
		{
			try { outsrc = new FileWriter ("t.out"); }
			catch (IOException e) { return -1; }
			return 1;
		}

		return -1;
	}

	protected int close_source (int mode)
	{
		if (mode == SOURCE_READ)
		{
			try { insrc.close (); }
			catch (IOException e) { return -1; }
			return 0;
		}
		else if (mode == SOURCE_WRITE)
		{
			try { outsrc.close (); }
			catch (IOException e) { return -1; }
			return 0;
		}

		return -1;
	}

	protected int read_source (char[] buf, int len)
	{
		try { return insrc.read (buf, 0, len); }
		catch (IOException e) { return -1; }
	}

	protected int write_source (char[] buf, int len)
	{
		try { outsrc.write (buf, 0, len); }
		catch (IOException e) { return -1; }
		return len;
	}

	protected int open_console (ase.awk.Extio extio)
	{
		System.err.println ("[open_console called.... name: " + extio.getName() + " mode: " + extio.getMode());

		int mode = extio.getMode ();

		if (mode == ase.awk.Extio.MODE_CONSOLE_READ)
		{
			InputStreamReader isr = 
				new InputStreamReader (System.in);
			extio.setHandle (isr);
			return 1;
		}
		else if (mode == ase.awk.Extio.MODE_CONSOLE_WRITE)
		{
			OutputStreamWriter osw =
				new OutputStreamWriter (System.out);
			extio.setHandle (osw);
			return 1;
		}

		return -1;
	}

	protected int close_console (ase.awk.Extio extio)
	{
		System.err.println ("[close_console called.... name: " + extio.getName() + " mode: " + extio.getMode());

		int mode = extio.getMode ();

		if (mode == ase.awk.Extio.MODE_CONSOLE_READ)
		{
			InputStreamReader isr = (InputStreamReader)extio.getHandle ();
			try { isr.close (); }
			catch (IOException e) { return -1; }
			return 0;
		}
		else if (mode == ase.awk.Extio.MODE_CONSOLE_WRITE)
		{
			OutputStreamWriter osw = (OutputStreamWriter)extio.getHandle ();
			//try { osw.close (); }
			//catch (IOException e) { return -1; }
			return 0;
		}

		return -1;
	}

	protected int read_console (ase.awk.Extio extio, char[] buf, int len)
	{
		int mode = extio.getMode ();

		if (mode == ase.awk.Extio.MODE_CONSOLE_READ)
		{
			InputStreamReader isr = (InputStreamReader)extio.getHandle ();
			try 
			{ 
				len = isr.read (buf, 0, len); 
				if (len == -1) len = 0;
			}
			catch (IOException e) { System.out.println ("EXCEPTIN---"+e.getMessage());return -1; }

			return len;
		}
		else if (mode == ase.awk.Extio.MODE_CONSOLE_WRITE)
		{
			return -1;
		}

		return -1;
	}

	protected int write_console (ase.awk.Extio extio, char[] buf, int len) 
	{
		int mode = extio.getMode ();

		if (mode == ase.awk.Extio.MODE_CONSOLE_READ)
		{
			return -1;
		}
		else if (mode == ase.awk.Extio.MODE_CONSOLE_WRITE)
		{
			OutputStreamWriter osw = (OutputStreamWriter)extio.getHandle ();
			try { osw.write (buf, 0, len); osw.flush (); }
			catch (IOException e) { return -1; }

			return len;
		}
		return -1;
	}

	protected int next_console (ase.awk.Extio extio)
	{
		/* TODO */
		return 0;
	}

	public int open_file (ase.awk.Extio extio)
	{
		int mode = extio.getMode();

		if (mode == ase.awk.Extio.MODE_FILE_READ)
		{
			FileInputStream fis;
			InputStreamReader isr;

			try { fis = new FileInputStream (extio.getName()); }
			catch (IOException e) { return -1; }

			isr = new InputStreamReader (fis); 
			extio.setHandle (isr);
			return 1;
		}
		else if (mode == ase.awk.Extio.MODE_FILE_WRITE)
		{
			FileOutputStream fos;
			OutputStreamWriter osw;

			try { fos = new FileOutputStream (extio.getName()); }
			catch (IOException e) { return -1; }

			osw = new OutputStreamWriter (fos);
			extio.setHandle (osw);
			return 1;
		}
		else if (mode == ase.awk.Extio.MODE_FILE_APPEND)
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
	
	public int close_file (ase.awk.Extio extio)
	{
		int mode = extio.getMode();

		if (mode == ase.awk.Extio.MODE_FILE_READ)
		{
			InputStreamReader isr;
			isr = (InputStreamReader)extio.getHandle();
			try { isr.close (); }
			catch (IOException e) { return -1; }
			return 0;
		}
		else if (mode == ase.awk.Extio.MODE_FILE_WRITE)
		{
			OutputStreamWriter osw;
			osw = (OutputStreamWriter)extio.getHandle();
			try { osw.close (); }
			catch (IOException e) { return -1; }
			return 0;
		}
		else if (mode == ase.awk.Extio.MODE_FILE_APPEND)
		{
			OutputStreamWriter osw;
			osw = (OutputStreamWriter)extio.getHandle();
			try { osw.close (); }
			catch (IOException e) { return -1; }
			return 0;
		}
		
		return -1;
	}

	protected int read_file (ase.awk.Extio extio, char[] buf, int len) 
	{
		int mode = extio.getMode();

		if (mode == ase.awk.Extio.MODE_FILE_READ)
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
	protected int write_file (ase.awk.Extio extio, char[] buf, int len) 
	{
		int mode = extio.getMode();

		if (mode == ase.awk.Extio.MODE_FILE_WRITE ||
		    mode == ase.awk.Extio.MODE_FILE_APPEND)
		{
			OutputStreamWriter osw;
			osw = (OutputStreamWriter)extio.getHandle();
			try { osw.write (buf, 0, len); }
			catch (IOException e) { len = -1; }
			return len;
		}

		return -1;
	}

	public static void main (String[] args)
	{
		Awk awk = null;

		try
		{
			awk = new Awk ();
			awk.parse ();
			awk.run ();
		}
		catch (ase.awk.Exception e)
		{
			System.out.println ("ase.awk.Exception - " + e.getMessage());
		}
		finally
		{
			if (awk != null) awk.close ();
		}
	}

}
