package ase.test.awk;

import java.applet.*;
import java.awt.*;
import java.awt.event.*;


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
