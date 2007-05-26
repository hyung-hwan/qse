/*
 * $Id: AseAwk.java,v 1.6 2007/05/25 16:55:02 bacon Exp $
 */

import java.awt.*;
import java.awt.event.*;
import java.io.*;
import java.util.LinkedList;
import java.util.ArrayList;
import java.util.Iterator;
import java.net.URL;
import ase.awk.*;

public class AseAwk extends StdAwk
{

	private static void run_in_awt ()
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
	}

	private static void print_usage ()
	{
		System.out.print ("Usage: ");
		System.out.print (AseAwk.class.getName());

		System.out.println (" [-m main] [-si file]? [-so file]? [-ci file]* [-co file]* [-a arg]*");
		System.out.println ("    -m  main  Specify the main function name\n");
		System.out.println ("    -si file  Specify the input source file\n");
		System.out.println ("              The source code is read from stdin when it is not specified\n");
		System.out.println ("    -so file  Specify the output source file\n");
		System.out.println ("              The deparsed code is not output when is it not specified\n");
		System.out.println ("    -ci file  Specify the input console file\n");
		System.out.println ("    -co file  Specify the output console file\n");
		System.out.println ("    -a  str   Specify an argument\n");
	}

	private static void print_error (String msg)
	{
		System.out.print ("Error: ");
		System.out.println (msg);
	}

	private static String get_dll_name ()
	{
		URL url = AseAwk.class.getResource (
			AseAwk.class.getName() + ".class");
		File file = new File (url.getFile());

		String osname = System.getProperty ("os.name").toLowerCase();
		String aseBase = file.getParentFile().getParentFile().getParent(
);
		String path;

		if (osname.startsWith ("windows"))
		{
			path = aseBase + "\\lib\\aseawk_jni.dll";
			return path.substring(6);
                }
		else if (osname.startsWith ("mac"))
		{
			path = aseBase + "/lib/.libs/libaseawk_jni.dylib";
			return path.substring(5);
		}
		else
		{
			path = aseBase + "/lib/.libs/libaseawk_jni.so";
			return path.substring(5);
		}
	}

	private static void run_in_console (String[] args)
	{
		AseAwk awk = null;

		String dll = get_dll_name();
		try
		{
			System.load (dll);
		}
		catch (java.lang.UnsatisfiedLinkError e)
		{
			print_error ("cannot load the library - " + dll);
			return;
		}

		try
		{
			awk = new AseAwk ();
			awk.setMaxDepth (AseAwk.DEPTH_BLOCK_PARSE, 30);
			awk.setDebug (true);
			//awk.setDebug (false);
		}
		catch (ase.awk.Exception e)
		{
			print_error ("ase.awk.Exception - " + e.getMessage());
		}

		int mode = 0;
		String mainfn = null;
		String srcin = null;
		String srcout = null;
		int nsrcins = 0;
		int nsrcouts = 0;
		ArrayList<String> params = new ArrayList<String> ();

		for (String arg: args)
		{
			if (mode == 0)
			{
				if (arg.equals("-si")) mode = 1;
				else if (arg.equals("-so")) mode = 2;
				else if (arg.equals("-ci")) mode = 3;
				else if (arg.equals("-co")) mode = 4;
				else if (arg.equals("-a")) mode = 5;
				else if (arg.equals("-m")) mode = 6;
				else 
				{
					print_usage ();
					return;
				}
			}
			else
			{
				if (arg.length() >= 1 && arg.charAt(0) == '-')
				{
					print_usage ();
					return;
				}
	
				if (mode == 1) // source input 
				{
					if (nsrcins != 0) 
					{
						print_usage ();
						return;
					}
		
					srcin = arg;
					nsrcins++;
					mode = 0;
				}
				else if (mode == 2) // source output 
				{
					if (nsrcouts != 0) 
					{
						print_usage ();
						return;
					}
		
					srcout = arg;
					nsrcouts++;
					mode = 0;
				}
				else if (mode == 3) // console input
				{
					awk.addConsoleInput (arg);
					mode = 0;
				}
				else if (mode == 4) // console output
				{
					awk.addConsoleOutput (arg);
					mode = 0;
				}
				else if (mode == 5) // argument mode
				{
					params.add (arg);
					mode = 0;
				}
				else if (mode == 6)
				{
					if (mainfn != null) 
					{
						print_usage ();
						return;
					}
	
					mainfn = arg;
					mode = 0;
				}
			}
		}

		if (mode != 0)
		{
			print_usage ();
			return;
		}

		try
		{
			awk.parse (srcin, srcout);
			awk.run (mainfn, params.toArray(new String[0]));
		}
		catch (ase.awk.Exception e)
		{
			if (e.getLine() == 0)
			{
				print_error ("ase.awk.Exception - " + e.getMessage());
			}
			else
			{
				print_error (
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
	}

	public static void main (String[] args)
	{
		if (args.length == 0)
		{
			run_in_awt ();
		}
		else
		{
			run_in_console (args);
		}
	}

	private Reader srcReader;
	private Writer srcWriter;
	private String srcInName;
	private String srcOutName;

	private Reader conReader;
	private Writer conWriter;
	private LinkedList conInNames;
	private LinkedList conOutNames;
	private Iterator conInIter;
	private Iterator conOutIter;

	public AseAwk () throws ase.awk.Exception
	{
		super ();

		srcReader = null;
		srcWriter = null;

		srcInName = null;
		srcOutName = null;

		conReader = null;
		conWriter = null;

		conInNames = new LinkedList ();
		conOutNames = new LinkedList ();

		addFunction ("sleep", 1, 1);
	}

	public Object bfn_sleep (
		long runid, Object[] args) throws ase.awk.Exception
	{
		long x = builtinFunctionArgumentToLong (runid, args[0]);
		try { Thread.sleep (x * 1000); }
		catch (InterruptedException e) {}
		return new Long(0);
	}

	public void parse () throws ase.awk.Exception
	{
		srcInName = null;
		srcOutName = null;
		super.parse ();
	}

	public void parse (String inName) throws ase.awk.Exception
	{
		srcInName = inName;
		srcOutName = null;
		super.parse ();
	}

	public void parse (String inName, String outName) throws ase.awk.Exception
	{
		srcInName = inName;
		srcOutName = outName;
		super.parse ();
	}

	public void run (String main, String[] args) throws ase.awk.Exception
	{
		conInIter = conInNames.iterator();
		conOutIter = conOutNames.iterator();
		super.run (main, args);
	}

	public void addConsoleInput (String name)
	{
		conInNames.addLast (name);	
	}

	public void addConsoleOutput (String name)
	{
		conOutNames.addLast (name);	
	}

	protected int openSource (int mode)
	{
		if (mode == SOURCE_READ)
		{
			Reader isr;

			if (srcInName == null || srcInName.length() == 0)
			{
				isr = stdin;
			}
			else
			{
				FileInputStream fis;
				try { fis = new FileInputStream (srcInName); }
				catch (IOException e) { return -1; }
				isr = new BufferedReader(new InputStreamReader (fis));
			}
			if (isr == null) return -1;

			srcReader = isr;
			return 1;
		}
		else if (mode == SOURCE_WRITE)
		{
			Writer osw;

			if (srcOutName == null)
			{
				return 0;
			}
			else if (srcOutName.length() == 0)
			{
				osw = stdout;
			}
			else
			{
				FileOutputStream fos;
				try { fos = new FileOutputStream (srcOutName); }
				catch (IOException e) { return -1; }
				osw = new BufferedWriter(new OutputStreamWriter (fos));
			}

			srcWriter = osw;
			return 1;
		}

		return -1;
	}

	protected int closeSource (int mode)
	{
		if (mode == SOURCE_READ)
		{
			if (srcReader != null)
			{
				if (srcReader == stdin)
				{
					srcReader = null;
				}
				else
				{
					try 
					{ 
						srcReader.close (); 
						srcReader = null; 
					}
					catch (IOException e) { return -1; }
				}
			}
			return 0;
		}
		else if (mode == SOURCE_WRITE)
		{
			if (srcWriter != null)
			{
				if (srcWriter == stdout)
				{
					try 
					{ 
						srcWriter.flush (); 
						srcWriter = null;
					}
					catch (IOException e) { return -1; }
				}
				else
				{
					try 
					{ 
						srcWriter.close (); 
						srcWriter = null;
					}
					catch (IOException e) { return -1; }
				}
			}
			return 0;
		}

		return -1;
	}

	protected int readSource (char[] buf, int len)
	{
		try 
		{ 
			int n = srcReader.read (buf, 0, len); 
			if (n == -1) n = 0;
			return n;
		}
		catch (IOException e) 
		{ 
			return -1; 
		}
	}

	protected int writeSource (char[] buf, int len)
	{
		if (srcWriter == null) return len;

		try { srcWriter.write (buf, 0, len); }
		catch (IOException e) { return -1; }

		return len;
	}

	protected int openConsole (Extio extio)
	{
		int mode = extio.getMode ();

		if (mode == Extio.MODE_CONSOLE_READ)
		{
			Reader rd;

			if (!conInIter.hasNext()) rd = stdin;
			else
			{
				FileInputStream fis;
				String fn = (String)conInIter.next();
				try 
				{ 
					fis = new FileInputStream (fn);
					rd = new BufferedReader(
						new InputStreamReader (fis));
				}
				catch (IOException e) { return -1; }

				try
				{
					setConsoleInputName (extio, fn);
				}
				catch (ase.awk.Exception e)
				{
					try { rd.close(); }
					catch (IOException e2) {}
					return -1;
				}
			}

			conReader = rd;
			return 1;
		}
		else if (mode == Extio.MODE_CONSOLE_WRITE)
		{
			Writer wr;

			if (!conOutIter.hasNext()) wr = stdout;
			else
			{
				FileOutputStream fos;
				String fn = (String)conOutIter.next();
				try 
				{ 
					fos = new FileOutputStream (fn);
					wr = new BufferedWriter(
						new OutputStreamWriter (fos));
				}
				catch (IOException e) { return -1; }

				try
				{
					setConsoleOutputName (extio, fn);
				}
				catch (ase.awk.Exception e)
				{
					try { wr.close(); }
					catch (IOException e2) {}
					return -1;
				}
			}

			conWriter = wr;
			return 1;
		}

		return -1;

	}

	protected int closeConsole (Extio extio)
	{
		int mode = extio.getMode ();

		if (mode == Extio.MODE_CONSOLE_READ)
		{
			if (conReader != null)
			{
				if (conReader == stdin)
				{
					conReader = null;
				}
				else
				{	
					try
					{
						conReader.close ();
						conReader = null;
					}
					catch (IOException e) { return -1; }
				}
			}

			return 0;
		}
		else if (mode == Extio.MODE_CONSOLE_WRITE)
		{
			if (conWriter != null)
			{
				if (conWriter == stdout)
				{
					try 
					{ 
						conWriter.flush (); 
						conWriter = null;
					}
					catch (IOException e) { return -1; }
				}
				else
				{
					try 
					{ 
						conWriter.close (); 
						conWriter = null;
					}
					catch (IOException e) { return -1; }
				}
			}

			return 0;
		}

		return -1;
	}

	protected int readConsole (Extio extio, char[] buf, int len)
	{
		int mode = extio.getMode ();

		if (mode == Extio.MODE_CONSOLE_READ)
		{
			try 
			{ 
				int n = conReader.read (buf, 0, len); 
				if (n == -1) n = 0;
				return n;
			}
			catch (IOException e) { return -1; }
		}

		return -1;
	}

	protected int writeConsole (Extio extio, char[] buf, int len)
	{
		int mode = extio.getMode ();

		if (mode == Extio.MODE_CONSOLE_WRITE)
		{
			try 
			{ 
				conWriter.write (buf, 0, len); 
				conWriter.flush ();
			}
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
			try { conWriter.flush (); }
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
			if (!conInIter.hasNext()) return 0;
			String fn = (String)conInIter.next();

			Reader rd;
			FileInputStream fis;
			try 
			{ 
				fis = new FileInputStream (fn);
				rd = new BufferedReader(
					new InputStreamReader (fis));
			}
			catch (IOException e) { return -1; }

			try
			{
				setConsoleInputName (extio, fn);
			}
			catch (ase.awk.Exception e)
			{
				try { rd.close(); }
				catch (IOException e2) {}
				return -1;
			}

			try { conReader.close (); }
			catch (IOException e) { return -1; }
			
			conReader = rd;
			return 1;
		}
		else if (mode == Extio.MODE_CONSOLE_WRITE)
		{
			if (!conOutIter.hasNext()) return 0;
			String fn = (String)conOutIter.next();
			
			Writer wr;
			FileOutputStream fos;
			try 
			{ 
				fos = new FileOutputStream (fn);
				wr = new BufferedWriter(
					new OutputStreamWriter (fos));
			}
			catch (IOException e) { return -1; }

			try 
			{
				setConsoleOutputName (extio, fn);
			}
			catch (ase.awk.Exception e)
			{
				try { wr.close(); }
				catch (IOException e2) {}
				return -1;
			}

			try { conWriter.close (); }
			catch (IOException e) { return -1; }

			conWriter = wr;
			return 1;
		}

		return -1;
	}

}
