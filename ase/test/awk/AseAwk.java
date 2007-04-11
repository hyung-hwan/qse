/*
 * $Id: AseAwk.java,v 1.3 2007-04-11 15:21:24 bacon Exp $
 */

import java.net.URL;

public class AseAwk extends ase.awk.StdAwk
{
	public AseAwk () throws ase.awk.Exception
	{
		super ();

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
		AseAwk awk = null;

		/*
		URL url = ase.awk.Awk.class.getResource ("aseawk_jni.dll");
		if (url == null) url = ase.awk.Awk.class.getResource ("aseawk_jni.so");
		if (url != null) System.load (url.getFile());
		*/

		if (args.length != 1)
		{
			System.err.println ("Usage: " + AseAwk.class.getName() + " jni");
			System.err.println ("Where jni := the full path to the jni library");
			return;
		}

		try
		{
			System.load (args[0]);
		}
		catch (java.lang.UnsatisfiedLinkError e)
		{
			System.err.println ("Error: cannot load the library - " + args[0]);
			return;
		}

		try
		{
			awk = new AseAwk ();
			awk.setMaxDepth (AseAwk.DEPTH_BLOCK_PARSE, 30);
			awk.setDebug (true);
			//awk.setDebug (false);

			//awk.setOption (awk.getOption() | OPTION_STRBASEONE);
			System.out.println ("Option: [" + awk.getOption() + "]");

			awk.parse ();
			
			System.out.println ("about to run");
			String[] aaa = new String[3];
			aaa[0] = "abcdefg";
			aaa[1] = "qwerty";
			aaa[2] = "awk is bad";
			awk.run ("main", aaa);
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
