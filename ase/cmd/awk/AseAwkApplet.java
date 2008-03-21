/*
 * $Id: AseAwkApplet.java,v 1.1 2007/04/30 08:32:41 bacon Exp $
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

	public void setConsoleInput (String str)
	{
		awkPanel.setConsoleInput (str);
	}

	public void setSourceInput (String str)
	{
		awkPanel.setSourceInput (str);
	}

	public void clear ()
	{
		awkPanel.clear ();
	}
}
