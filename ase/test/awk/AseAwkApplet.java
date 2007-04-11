/*
 * $Id: AseAwkApplet.java,v 1.2 2007-04-11 09:39:46 bacon Exp $
 */

import java.applet.*;
import java.awt.*;
import java.awt.event.*;
import java.net.URL;

public class AseAwkApplet extends Applet
{
	private TextArea srcIn;
	private TextArea srcOut;
	private TextArea conIn;
	private TextArea conOut;
	private TextField jniLib;

	public void init () 
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
				run_awk ();
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
	}

	public void stop () {}
	public void paint (Graphics g) {}

	private void run_awk ()
	{
		AseAwk awk = null;

		try
		{
			/*
			URL url = ase.awk.Awk.class.getResource ("aseawk_jni.dll");
			if (url == null) url = ase.awk.Awk.class.getResource ("aseawk_jni.so");

			if (url != null) System.load (url.getFile()); */

			try
			{
				System.load ("c:/projects/ase/test/awk/aseawk_jni.dll");
			}
			catch (Exception e)
			{
				System.err.println ("fuck you");
			}


			awk = new AseAwk ();
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
