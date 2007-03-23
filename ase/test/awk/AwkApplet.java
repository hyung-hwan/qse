//package ase.test.awk;

import java.applet.*;
import java.awt.*;
import java.awt.event.*;
import java.net.URL;

public class AwkApplet extends Applet
{
	public void init () 
	{
		Button btn_run;
		btn_run = new Button ("Run Awk");

		btn_run.addActionListener (new ActionListener ()
		{
			public void actionPerformed (ActionEvent e)
			{
				run_awk ();
			}
		});
		add (btn_run);
	}

	public void stop () {}
	public void paint (Graphics g) {}

	private void run_awk ()
	{
		ase.test.awk.Awk awk = null;

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


			awk = new ase.test.awk.Awk ();
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
