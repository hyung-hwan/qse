/*
 * $Id: AseAwkPanel.java,v 1.4 2007-04-12 11:23:49 bacon Exp $
 */

import java.awt.*;
import java.awt.event.*;

import java.net.URL;
import java.io.File;
import java.io.IOException;
import java.io.StringReader;
import java.io.StringWriter;

import ase.awk.StdAwk;
import ase.awk.Extio;

public class AseAwkPanel extends Panel
{
	/* MsgBox taken from http://www.rgagnon.com/javadetails/java-0242.html */
	class MsgBox extends Dialog implements ActionListener 
	{
		boolean id = false;
		Button ok,can;

		MsgBox (Frame frame, String msg, boolean okcan)
		{
			super (frame, "Message", true);
			setLayout(new BorderLayout());
			add("Center",new Label(msg));
			addOKCancelPanel(okcan);
			createFrame();
			pack();
			setVisible(true);
		}

		void addOKCancelPanel( boolean okcan ) 
		{
			Panel p = new Panel();
			p.setLayout(new FlowLayout());
			createOKButton( p );
			if (okcan == true) createCancelButton( p );
			add("South",p);
		}

		void createOKButton(Panel p) 
		{
			p.add(ok = new Button("OK"));
			ok.addActionListener(this); 
		}

		void createCancelButton(Panel p) 
		{
			p.add(can = new Button("Cancel"));
			can.addActionListener(this);
		}

		void createFrame() 
		{
			Dimension d = getToolkit().getScreenSize();
			setLocation(d.width/3,d.height/3);
		}

		public void actionPerformed(ActionEvent ae)
		{
			if(ae.getSource() == ok) 
			{
				id = true;
				setVisible(false);
			}
			else if(ae.getSource() == can) 
			{
				setVisible(false);
			}
		}
	}
	
	class Awk extends StdAwk
	{
		private AseAwkPanel awkPanel;
	
		private StringReader srcIn;
		private StringWriter srcOut;

		private StringReader conIn;
		private StringWriter conOut;
	
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
			try 
			{
				int n = srcIn.read (buf, 0, len); 
				if (n == -1) n = 0;
				return n;
			}
			catch (IOException e) { return -1; }
		}
	
		protected int writeSource (char[] buf, int len)
		{
			srcOut.write (buf, 0, len);
			return len;
		}

		protected int openConsole (Extio extio)
		{
			int mode = extio.getMode ();

			if (mode == Extio.MODE_CONSOLE_READ)
			{
				conIn = new StringReader (awkPanel.getConsoleInput());	
				return 1;
			}
			else if (mode == Extio.MODE_CONSOLE_WRITE)
			{
				conOut = new StringWriter ();
				return 1;
			}

			return -1;

		}
	
		protected int closeConsole (Extio extio)
		{
			int mode = extio.getMode ();

			if (mode == Extio.MODE_CONSOLE_READ)
			{
				conIn.close ();
				return 0;
			}
			else if (mode == Extio.MODE_CONSOLE_WRITE)
			{
				awkPanel.setConsoleOutput (conOut.toString());

				try { conOut.close (); }
				catch (IOException e) { return -1; }
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
					int n = conIn.read (buf, 0, len); 
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
				conOut.write (buf, 0, len);
				return len;
			}

			return -1;
		}

		protected int flushConsole (Extio extio)
		{
			int mode = extio.getMode ();

			if (mode == Extio.MODE_CONSOLE_WRITE)
			{
				return 0;
			}

			return -1;
		}

		protected int nextConsole (Extio extio)
		{
			int mode = extio.getMode ();

			if (mode == Extio.MODE_CONSOLE_READ)
			{
			}
			else if (mode == Extio.MODE_CONSOLE_WRITE)
			{
			}

			return -1;
		}
	}

	private TextArea srcIn;
	private TextArea srcOut;
	private TextArea conIn;
	private TextArea conOut;
	private TextField jniLib;

	private boolean jniLibLoaded = false;

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

		URL url = this.getClass().getResource (
			this.getClass().getName() + ".class");
		File file = new File (url.getFile());
		
		String osname = System.getProperty ("os.name").toLowerCase();
		String aseBase = file.getParentFile().getParentFile().getParent();

		if (osname.startsWith ("windows"))
		{
			String path = aseBase + "\\lib\\aseawk_jni.dll";
			jniLib.setText (path.substring(6));
		}
		else if (osname.startsWith ("mac"))
		{
			String path = aseBase + "/lib/.libs/libaseawk_jni.dylib";
			jniLib.setText (path.substring(5));
		}
		else
		{
			String path = aseBase + "/lib/.libs/libaseawk_jni.so";
			jniLib.setText (path.substring(5));
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

	public String getConsoleInput ()
	{
		return conIn.getText ();
	}

	public void setConsoleOutput (String output)
	{
		conOut.setText (output);
	}

	private void runAwk ()
	{
		Awk awk = null;

		if (!jniLibLoaded)
		{
			try
			{
				System.load (jniLib.getText());
				jniLib.setEnabled (false);
				jniLibLoaded = true;
			}
			catch (UnsatisfiedLinkError e)
			{
				showMessage ("Cannot load library - " + e.getMessage());
				return;
			}
			catch (Exception e)
			{
				showMessage ("Cannot load library - " + e.getMessage());
				return;
			}
		}

		try
		{
			try
			{
				awk = new Awk (this);
			}
			catch (Exception e)
			{
				showMessage ("Cannot instantiate awk - " + e.getMessage());
				return;
			}

			awk.parse ();
			awk.run ();
		}
		catch (ase.awk.Exception e)
		{
			showMessage ("An exception occurred - " + e.getMessage());
			return;
		}
		finally
		{
			if (awk != null) awk.close ();
		}
	}

	private void showMessage (String msg)
	{
		Frame tmp = new Frame ("");
		MsgBox message = new MsgBox (tmp, msg, false);
		requestFocus ();
		message.dispose ();
		tmp.dispose ();
	}
}
