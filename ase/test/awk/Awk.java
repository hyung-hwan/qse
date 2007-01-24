/*
 * $Id: Awk.java,v 1.21 2007-01-24 14:21:30 bacon Exp $
 */

package ase.test.awk;

public class Awk extends ase.awk.StdAwk
{
	public Awk () throws ase.awk.Exception
	{
		super ();

		addBuiltinFunction ("sin", 1, 1); 
		addBuiltinFunction ("cos", 1, 1); 
		addBuiltinFunction ("tan", 1, 1); 
		addBuiltinFunction ("srand", 0, 1); 
		addBuiltinFunction ("rand", 0, 0); 

		addBuiltinFunction ("system", 1, 1); 

		addBuiltinFunction ("xxx", 1, 10); 
		//addBuiltinFunction ("xxx", 1, 1); 
		//deleteBuiltinFunction ("sin");
		//deleteBuiltinFunction ("sin");
	}

	public Object xxx (long runid, Object[] args)
	{
		System.out.println ("<<BFN_XXX>>");
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

	protected String[] consoleInputNames ()
	{
		String[] cin = new String[3];
		cin[0] = "c1.txt";
		cin[1] = "c2.txt";
		cin[2] = "c3.txt";
		return cin;
	}	

	protected String[] consoleOutputNames ()
	{
		String[] cout = new String[1];
		cout[0] = "";
		return cout;
/*
		String[] cout = new String[3];
		cout[0] = "c4.txt";
		cout[1] = "c5.txt";
		cout[2] = "";
		return cout;
*/
	}

	protected String[] sourceInputNames ()
	{
		String[] sin = new String[1];
		sin[0] = "t.awk";
		return sin;
	}

	/*
	protected String sourceOutputName ()
	{
		return "";
	}
	*/

	public static void main (String[] args)
	{
		Awk awk = null;

		try
		{
			awk = new Awk ();
			awk.setMaxDepth (Awk.DEPTH_BLOCK_PARSE, 30);

			awk.parse ();
			awk.run ();
		}
		catch (ase.awk.Exception e)
		{
			if (e.getLine() == 0)
			{
				System.out.println ("ase.awk.Exception - " + e.getMessage());
			}
			else
			{
				System.out.println (
					"ase.awk.Exception at line " +
					e.getLine() + " - " + e.getMessage());
			}
		}
		finally
		{
			if (awk != null) 
			{
				awk.close ();
				awk = null;
			}
		}
		System.out.println ("==== end of awk ====");
	}

}
