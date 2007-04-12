/*
 * $Id: AseAwkApplet.java,v 1.4 2007-04-12 10:08:08 bacon Exp $
 */

import java.applet.*;
import java.awt.*;
import java.awt.event.*;

public class AseAwkApplet extends Applet
{
	AseAwkPanel awkPanel;

	public void init () 
	{
		awkPanel = new AseAwkPanel ();

		setLayout (new BorderLayout ());
		add (awkPanel, BorderLayout.CENTER);	
	}

	public void stop () {}
	public void paint (Graphics g) {}
}
