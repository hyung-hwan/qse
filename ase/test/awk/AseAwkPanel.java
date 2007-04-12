/*
 * $Id: AseAwkPanel.java,v 1.1 2007-04-12 10:08:08 bacon Exp $
 */

import java.awt.*;
import java.awt.event.*;

import java.net.URL;
import java.io.File;
import java.io.IOException;
import java.io.StringReader;
import java.io.StringWriter;

import ase.awk.StdAwk;

public class AseAwkPanel extends Panel
{
	class Awk extends StdAwk
	{
		private AseAwkPanel awkPanel;
	
		private StringReader srcIn;
		private StringWriter srcOut;
	
		public Awk (AseAwkPanel awkPanel) throws Exception
		{
			super ();
			this.awkPanel = awkPanel;
		}
	
		protected int openSource (int mode)
		{
			if (mode == SOURCE_READ)
			{
				srcIn = new StringReader (awkPanel.getSourceInput());	
				return 1;
			}
			else if (mode == SOURCE_WRITE)
			{
				srcOut = new StringWriter ();
				return 1;
			}
	
			return -1;
		}
	
		protected int closeSource (int mode)
		{
			if (mode == SOURCE_READ)
			{
				srcIn.close ();
				return 0;
			}
			else if (mode == SOURCE_WRITE)
			{
				awkPanel.setSourceOutput (srcOut.toString());
	
				try { srcOut.close (); }
				catch (IOException e) { return -1; }
				return 0;
			}
	
			return -1;
		}
	
		protected int readSource (char[] buf, int len)
		{
			try { return srcIn.read (buf, 0, len); }
			catch (IOException e) { return -1; }
		}
	
		protected int writeSource (char[] buf, int len)
		{
			srcOut.write (buf, 0, len);
			return len;
		}
	}

	private TextArea srcIn;
	private TextArea srcOut;
	private TextArea conIn;
	private TextArea conOut;
	private TextField jniLib;

	public AseAwkPanel () 
	{
		jniLib = new TextField ();

		srcIn = new TextArea ();
		srcOut = new TextArea ();
		conIn = new TextArea ();
		conOut = new TextArea ();

		Button runBtn = new Button ("Run Awk");

		runBtn.addActionListener (new ActionListener ()
		{
			public void actionPerformed (ActionEvent e)
			{
				runAwk ();
			}
		});

		Panel topPanel = new Panel ();
		BorderLayout topPanelLayout = new BorderLayout ();
		topPanel.setLayout (topPanelLayout);

		topPanelLayout.setHgap (2);
		topPanelLayout.setVgap (2);
		topPanel.add (new Label ("JNI Library: "), BorderLayout.WEST);
		topPanel.add (jniLib, BorderLayout.CENTER);

		Panel centerPanel = new Panel ();
		GridLayout centerPanelLayout = new GridLayout (2, 2);

		centerPanel.setLayout (centerPanelLayout);

		centerPanelLayout.setHgap (2);
		centerPanelLayout.setVgap (2);

		centerPanel.add (srcIn);
		centerPanel.add (srcOut);
		centerPanel.add (conIn);
		centerPanel.add (conOut);

		BorderLayout mainLayout = new BorderLayout ();
		mainLayout.setHgap (2);
		mainLayout.setVgap (2);

		setLayout (mainLayout);
		
		add (topPanel, BorderLayout.NORTH);
		add (centerPanel, BorderLayout.CENTER);
		add (runBtn, BorderLayout.SOUTH);

		URL url = this.getClass().getResource ("AseAwkApplet.class");
		File file = new File (url.getFile());
		
		if (System.getProperty ("os.name").toLowerCase().startsWith ("windows"))
		{
			jniLib.setText (file.getParentFile().getParentFile().getParent() + "/lib/aseawk_jni.dll");
		}
		else
		{
			jniLib.setText (file.getParentFile().getParentFile().getParent() + "/lib/.libs/libaseawk_jni.dylib");
		}
	}

	public String getSourceInput ()
	{
		return srcIn.getText ();
	}

	public void setSourceOutput (String output)
	{
		srcOut.setText (output);
	}

	private void runAwk ()
	{
		Awk awk = null;

		try
		{
			try
			{
				System.load (jniLib.getText());
			}
			catch (Exception e)
			{
				System.err.println ("xxx fuck you - cannot load library: " + e.getMessage());
				return;
			}


			try
			{
				awk = new Awk (this);
			}
			catch (Exception e)
			{
				System.err.println ("cannot create awk - " + e.getMessage());
			}

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
