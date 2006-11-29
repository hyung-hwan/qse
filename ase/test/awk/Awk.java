/*
 * $Id: Awk.java,v 1.17 2006-11-29 14:52:36 bacon Exp $
 */

package ase.test.awk;

public class Awk extends ase.awk.StdAwk
{
	public Awk () throws ase.awk.Exception
	{
		super ();

		addBuiltinFunction ("sin", 1, 10); 
		addBuiltinFunction ("xxx", 1, 1); 
		//addBuiltinFunction ("xxx", 1, 1); 

		//deleteBuiltinFunction ("sin");
		//deleteBuiltinFunction ("sin");
	}

	public Object xxx (Object[] args)
	{
		System.out.println ("BFN_XXX");
		return null;
	}

	public Object sin (Object[] args)
	{
		System.out.println ("<<BFN_SIN>>");
		for (int i = 0; i < args.length; i++)
		{
			System.out.print ("ARG #" + i);
			System.out.print (": ");
			if (args[i] == null) System.out.println ("nil");
			else System.out.println (args[i].toString());
		}
		return null;
		//return new String ("return value");
		//return new Double (1.234);
		//return new Float (1.234);
		//return new Integer (1001);
		//return new Long (1001);
		//return new Short ((short)1001);
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
