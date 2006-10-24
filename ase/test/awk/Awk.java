/*
 * $Id: Awk.java,v 1.1 2006-10-24 06:03:14 bacon Exp $
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
			try { insrc = new FileReader ("test.awk"); }
			catch (IOException e) { return -1; }
			return 1;
		}
		else if (mode == SOURCE_WRITE)
		{
			try { outsrc = new FileWriter ("test.out"); }
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

	protected int open_console ()
	{
		System.err.println ("[open_console called....]");
		return 1;
	}

	protected int close_console ()
	{
		System.err.println ("[close_console called....]");
		return 1;
	}

	protected int read_console (char[] buf, int len)
	{
		return 0;
	}

	protected int write_console (char[] buf, int len) 
	{
		System.out.print (new String (buf, 0, len));
		return len;
	}

	protected int next_console (char[] buf, int len)
	{
		return 0;
	}

	public int open_file (ase.awk.Extio extio)
	{
		System.out.print ("opening file [");
		//System.out.print (extio.name());
		System.out.println ("]");

		/*
		FileInputStream f = new FileInputStream (extio.name());
		extio.setHandle (f);
		*/
		return 1;
	}
	
	/*
	public int open_file (String name)
	{
		System.out.print ("opening file [");
		System.out.print (name);
		System.out.println ("]");
		return 1;
	}
	*/

	public int close_file (String name)
	{
		System.out.print ("closing file [");
		System.out.print (name);
		System.out.println ("]");
		return 0;
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
