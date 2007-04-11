/*
 * $Id: AseAwkApplet.java,v 1.3 2007-04-11 10:49:34 bacon Exp $
 */

import java.applet.*;
import java.awt.*;
import java.awt.event.*;
import java.net.URL;
import java.io.File;

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

		URL url = this.getClass().getResource ("AseAwk.class");
		File file = new File (url.getFile());
		
		jniLib.setText (file.getParentFile().getParentFile().getParent() + "/lib/.libs/libaseawk_jni.dylib");
	}

	public void stop () {}
	public void paint (Graphics g) {}

	private void runAwk ()
	{
		AseAwk awk = null;

		try
		{
			try
			{
				System.load (jniLib.getText());
			}
			catch (Exception e)
			{
				System.err.println ("xxx fuck you - cannot load library: " + e.getMessage());
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
