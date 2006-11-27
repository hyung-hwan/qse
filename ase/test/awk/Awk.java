/*
 * $Id: Awk.java,v 1.12 2006-11-27 04:33:22 bacon Exp $
 */

package ase.test.awk;

public class Awk extends ase.awk.StdAwk
{
	private String[] cin;
	private int cin_no;

	private String[] cout;
	private int cout_no;

	public Awk () throws ase.awk.Exception
	{
		super ();

		addbfn (bfn_sin, 1, 3);
	}

	public void bfn_sin ()
	{
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

	protected String[] getSourceNames ()
	{
		String[] cout = new String[1];
		cout[0] = "t.awk";
		return cout;
	}

	/*
	protected String getDeparsedSourceName ()
	{
		return "";
	}
	*/
	protected int getMaxParseDepth ()
	{
		return 50;
	}

	protected int getMaxRunDepth ()
	{
		return 50;
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
