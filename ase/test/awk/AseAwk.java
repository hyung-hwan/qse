/*
 * $Id: AseAwk.java,v 1.4 2007-04-12 10:08:07 bacon Exp $
 */

import java.awt.*;
import java.awt.event.*;

public class AseAwk extends ase.awk.StdAwk
{
	public AseAwk () throws ase.awk.Exception
	{
		super ();
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
		// AWT mode 
		if (args.length == 0)
		{
			final Frame frame = new Frame ();

			frame.setLayout (new BorderLayout());
			frame.setTitle (AseAwk.class.getName());
			frame.setSize (640, 480);
			frame.addWindowListener (new WindowListener ()
			{
				public void windowActivated (WindowEvent e) {}
				public void windowClosed (WindowEvent e) {}
				public void windowClosing (WindowEvent e) { frame.dispose (); }
				public void windowDeactivated (WindowEvent e) {}
				public void windowDeiconified (WindowEvent e) {}
				public void windowIconified (WindowEvent e) {}
				public void windowOpened (WindowEvent e) {}
			});

			frame.add (new AseAwkPanel(), BorderLayout.CENTER);
			frame.setVisible (true);
			return;
		}

		// console mode 
		AseAwk awk = null;

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
