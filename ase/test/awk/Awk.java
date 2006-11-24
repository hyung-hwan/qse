/*
 * $Id: Awk.java,v 1.7 2006-11-24 13:25:12 bacon Exp $
 */

package ase.test.awk;

import java.io.*;

public class Awk extends ase.awk.StdAwk
{
	private FileReader insrc;
	private FileWriter outsrc;

	private String[] cin;
	private int cin_no;

	private String[] cout;
	private int cout_no;

	public Awk () throws ase.awk.Exception
	{
		super ();
	}

	protected String[] getInputConsoleNames ()
	{
		String[] cin = new String[3];
		cin[0] = "c1.txt";
		cin[1] = "c2.txt";
		cin[2] = "c3.txt";
		return cin;
	}	

	protected String[] getOutputConsoleNames ()
	{
		String[] cout = new String[3];
		cout[0] = "c4.txt";
		cout[1] = "c5.txt";
		cout[2] = "";
		return cout;
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
