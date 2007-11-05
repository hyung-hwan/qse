/*
 * $Id: AseAwkPanel.java,v 1.22 2007/11/04 05:01:41 bacon Exp $
 */

import java.awt.*;
import java.awt.event.*;
import java.awt.dnd.*;
import java.awt.datatransfer.*;

import java.net.URL;
import java.net.URLConnection;
import java.io.File;
import java.io.IOException;
import java.io.FileNotFoundException;
import java.io.StringReader;
import java.io.StringWriter;
import java.io.Reader;
import java.io.Writer;
import java.io.InputStream;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.security.MessageDigest;
import java.util.List;
import java.util.Iterator;

import ase.awk.StdAwk;
import ase.awk.Console;
import ase.awk.Context;
import ase.awk.Argument;
import ase.awk.Return;

public class AseAwkPanel extends Panel implements DropTargetListener
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
	
	public class Awk extends StdAwk
	{
		private AseAwkPanel awkPanel;
	
		private StringReader srcIn;
		private StringWriter srcOut;

		public Awk (AseAwkPanel awkPanel) throws Exception
		{
			super ();
			this.awkPanel = awkPanel;

			addFunction ("sleep", 1, 1);
			/*
			setWord ("sin", "cain");
			setWord ("length", "len");
			setWord ("OFMT", "ofmt");
			*/
		}
	
		public void sleep (Context ctx, String name, Return ret, Argument[] args) throws ase.awk.Exception
		{
			Argument t = args[0];
			//if (args[0].isIndexed()) t = args[0].getIndexed(0);
			
			try { Thread.sleep (t.getIntValue() * 1000); }
			catch (InterruptedException e) {}

			ret.setIntValue (0);
		/*
			ret.setIndexedRealValue (1, 111.23);
			ret.setIndexedStringValue (2, "1111111");
			ret.setIndexedStringValue (3, "22222222");
			ret.setIndexedIntValue (4, 444);
			ret.setIndexedIntValue (5, 55555);

			Return r = new Return (ctx);
			r.setStringValue ("[[%.6f]]");
			Return r2 = new Return (ctx);
			r2.setStringValue ("[[%.6f]]");

			//ctx.setGlobal (Context.GLOBAL_CONVFMT, ret);
			Argument g = ctx.getGlobal (Context.GLOBAL_CONVFMT);
			ctx.setGlobal (Context.GLOBAL_CONVFMT, r2);
	System.out.println (g.getStringValue());
			g = ctx.getGlobal (Context.GLOBAL_CONVFMT);
		System.out.println (g.getStringValue());
		*/
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

		protected int openConsole (Console con)
		{
			int mode = con.getMode ();

			if (mode == Console.MODE_READ)
			{
				con.setHandle (new StringReader (awkPanel.getConsoleInput()));
				return 1;
			}
			else if (mode == Console.MODE_WRITE)
			{
				con.setHandle (new StringWriter ());
				return 1;
			}

			return -1;

		}
	
		protected int closeConsole (Console con)
		{
			int mode = con.getMode ();

			if (mode == Console.MODE_READ)
			{
				Reader rd = (Reader)con.getHandle();
				try { rd.close (); }
				catch (IOException e) { return -1; }
				return 0;
			}
			else if (mode == Console.MODE_WRITE)
			{
				Writer wr = (Writer)con.getHandle();
				awkPanel.setConsoleOutput (wr.toString());
				try { wr.close (); }
				catch (IOException e) { return -1; }
				return 0;
			}

			return -1;
		}
	
		protected int readConsole (Console con, char[] buf, int len)
		{
			int mode = con.getMode ();

			if (mode == Console.MODE_READ)
			{
				Reader rd = (Reader)con.getHandle();

				try 
				{ 
					int n = rd.read (buf, 0, len); 
					if (n == -1) n = 0;
					return n;
				}
				catch (IOException e) { return -1; }
			}

			return -1;
		}
	
		protected int writeConsole (Console con, char[] buf, int len)
		{
			int mode = con.getMode ();

			if (mode == Console.MODE_WRITE)
			{
				Writer wr = (Writer)con.getHandle();
				try { wr.write (buf, 0, len); }
				catch (IOException e) { return -1; }
				return len;
			}

			return -1;
		}

		protected int flushConsole (Console con)
		{
			int mode = con.getMode ();

			if (mode == Console.MODE_WRITE)
			{
				return 0;
			}

			return -1;
		}

		protected int nextConsole (Console con)
		{
			int mode = con.getMode ();

			if (mode == Console.MODE_READ)
			{
				return 0;
			}
			else if (mode == Console.MODE_WRITE)
			{
				return 0;
			}

			return -1;
		}
	}

	private TextArea srcIn;
	private TextArea srcOut;
	private TextArea conIn;
	private TextArea conOut;
	private TextField entryPoint;
	private TextField jniLib;
	private Label statusLabel;

	private DropTarget srcInDropTarget;
	private DropTarget conInDropTarget;

	private boolean jniLibLoaded = false;

	private class Option
	{
		private String name;
		private int value;
		private boolean state;

		public Option (String name, int value, boolean state)
		{
			this.name = name;
			this.value = value;
			this.state = state;
		}

		public String getName()
		{
			return this.name;
		}

		public int getValue()
		{
			return this.value;
		}

		public boolean getState()
		{
			return this.state;
		}

		public void setState (boolean state)
		{
			this.state = state;
		}
	}

	protected Option[] options = new Option[]
	{
		new Option("IMPLICIT", AseAwk.OPTION_IMPLICIT, true),
		new Option("EXPLICIT", AseAwk.OPTION_EXPLICIT, false),
		new Option("UNIQUEFN", AseAwk.OPTION_UNIQUEFN, false),
		new Option("SHADING", AseAwk.OPTION_SHADING, true),
		new Option("SHIFT", AseAwk.OPTION_SHIFT, false),
		new Option("IDIV", AseAwk.OPTION_IDIV, false),
		new Option("STRCONCAT", AseAwk.OPTION_STRCONCAT, false),
		new Option("EXTIO", AseAwk.OPTION_EXTIO, true),
		new Option("BLOCKLESS", AseAwk.OPTION_BLOCKLESS, true),
		new Option("BASEONE", AseAwk.OPTION_BASEONE, true),
		new Option("STRIPSPACES", AseAwk.OPTION_STRIPSPACES, false),
		new Option("NEXTOFILE", AseAwk.OPTION_NEXTOFILE, false),
		//new Option("CRLF", AseAwk.OPTION_CRLF, false),
		new Option("ARGSTOMAIN", AseAwk.OPTION_ARGSTOMAIN, false),
		new Option("RESET", AseAwk.OPTION_RESET, false),
		new Option("MAPTOVAR", AseAwk.OPTION_MAPTOVAR, false),
		new Option("PABLOCK", AseAwk.OPTION_PABLOCK, true)
	};

	public AseAwkPanel () 
	{
		prepareUserInterface ();
		prepareNativeInterface ();
	}

	private void prepareUserInterface ()
	{
		jniLib = new TextField ();

		Font font = new Font ("Monospaced", Font.PLAIN, 14);

		srcIn = new TextArea ();
		srcOut = new TextArea ();
		conIn = new TextArea ();
		conOut = new TextArea ();

		srcIn.setFont (font);
		srcOut.setFont (font);
		conIn.setFont (font);
		conOut.setFont (font);

		Panel srcInPanel = new Panel();
		srcInPanel.setLayout (new BorderLayout());
		srcInPanel.add (new Label("Source Input"), BorderLayout.NORTH);
		srcInPanel.add (srcIn, BorderLayout.CENTER);

		Panel srcOutPanel = new Panel();
		srcOutPanel.setLayout (new BorderLayout());
		srcOutPanel.add (new Label("Source Output"), BorderLayout.NORTH);
		srcOutPanel.add (srcOut, BorderLayout.CENTER);

		Panel conInPanel = new Panel();
		conInPanel.setLayout (new BorderLayout());
		conInPanel.add (new Label("Console Input"), BorderLayout.NORTH);
		conInPanel.add (conIn, BorderLayout.CENTER);

		Panel conOutPanel = new Panel();
		conOutPanel.setLayout (new BorderLayout());
		conOutPanel.add (new Label("Console Output"), BorderLayout.NORTH);
		conOutPanel.add (conOut, BorderLayout.CENTER);

		Button runBtn = new Button ("Run Awk");

		runBtn.addActionListener (new ActionListener ()
		{
			public void actionPerformed (ActionEvent e)
			{
				runAwk ();
			}
		});

		entryPoint = new TextField();

		Panel entryPanel = new Panel();
		entryPanel.setLayout (new BorderLayout());
		entryPanel.add (new Label("Main:"), BorderLayout.WEST);
		entryPanel.add (entryPoint, BorderLayout.CENTER);

		Panel leftPanel = new Panel();
		leftPanel.setLayout (new BorderLayout());
		leftPanel.add (runBtn, BorderLayout.SOUTH);

		Panel optPanel = new Panel();
		optPanel.setBackground (Color.YELLOW);
		optPanel.setLayout (new GridLayout(options.length, 1));
		for (int i = 0; i < options.length; i++)
		{
			Checkbox cb = new Checkbox(options[i].getName(), options[i].getState());

			cb.addItemListener (new ItemListener ()
			{
				public void itemStateChanged (ItemEvent e)
				{
					String name = (String)e.getItem();
					for (int i = 0; i < options.length; i++)
					{
						if (options[i].getName().equals(name))
						{
							options[i].setState (e.getStateChange() == ItemEvent.SELECTED);
						}
					}
				}
			});

			optPanel.add (cb);
		}
		leftPanel.add (entryPanel, BorderLayout.NORTH);
		leftPanel.add (optPanel, BorderLayout.CENTER);

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

		centerPanel.add (srcInPanel);
		centerPanel.add (srcOutPanel);
		centerPanel.add (conInPanel);
		centerPanel.add (conOutPanel);

		BorderLayout mainLayout = new BorderLayout ();
		mainLayout.setHgap (2);
		mainLayout.setVgap (2);

		setLayout (mainLayout);
		statusLabel = new Label ("Ready");
		statusLabel.setBackground (Color.GREEN);

		add (topPanel, BorderLayout.NORTH);
		add (centerPanel, BorderLayout.CENTER);
		add (leftPanel, BorderLayout.WEST);
		add (statusLabel, BorderLayout.SOUTH);

		srcInDropTarget = new DropTarget (srcIn, this);
		conInDropTarget = new DropTarget (conIn, this);
	}

	public void prepareNativeInterface ()
	{
		String osname = System.getProperty ("os.name").toLowerCase();

		URL url = this.getClass().getResource (
			this.getClass().getName() + ".class");
		String protocol = url.getProtocol ();

		boolean isHttp = url.getPath().startsWith ("http://");
		File file = new File (isHttp? url.getPath():url.getFile());

		String base = protocol.equals("jar")?
			file.getParentFile().getParentFile().getParent():
			file.getParentFile().getParent();

		if (osname.startsWith ("windows"))
		{
			String path;
			if (isHttp)
			{
				base = "http://" + base.substring(6).replace('\\', '/');
				String jniUrl = base + "/lib/aseawk_jni.dll";
				String md5Url = jniUrl + ".md5";

				String userHome = System.getProperty("user.home");
				path = userHome + "\\aseawk_jni.dll";

				try
				{
					downloadNative (md5Url, jniUrl, path);
				}
				catch (Exception e)
				{
					showMessage ("Cannot download native library - " + e.getMessage());
					path = "ERROR - Not Available";
				}
			}
			else 
			{
				path = base + "\\lib\\aseawk_jni.dll";
				if (protocol.equals("jar")) path = path.substring(6);
			}
			jniLib.setText (path);
		}
		else if (osname.startsWith ("mac"))
		{
			String path = base + "/lib/.libs/libaseawk_jni.dylib";
			if (!isHttp && protocol.equals("jar")) path = path.substring(5);
			jniLib.setText (path);
		}
		else
		{
			String path = base + "/lib/.libs/libaseawk_jni.so";
			if (!isHttp && protocol.equals("jar")) path = path.substring(5);
			jniLib.setText (path);
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

		srcOut.setText ("");
		conOut.setText ("");

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

			for (int i = 0; i < options.length; i++)
			{
				if (options[i].getState())
				{
					awk.setOption (awk.getOption() | options[i].getValue());
				}
				else
				{
					awk.setOption (awk.getOption() & ~options[i].getValue());
				}
			}

			statusLabel.setText ("Parsing...");
			awk.parse ();

			String main = entryPoint.getText().trim();

			statusLabel.setText ("Running...");
			if (main.length() > 0) awk.run (main);
			else awk.run ();

			statusLabel.setText ("Done...");
		}
		catch (ase.awk.Exception e)
		{
			String msg;
			int line = e.getLine();
			int code = e.getCode();

			if (line <= 0)
				msg = "An exception occurred - [" + code + "] " + e.getMessage();
			else
				msg = "An exception occurred - [" + code + "] " + e.getMessage() + " at line " + line;

			statusLabel.setText (msg);
			showMessage (msg);
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


	private String getFileMD5 (String file) throws Exception
	{
		MessageDigest md = MessageDigest.getInstance("MD5");
		FileInputStream fis = null;
		
		try
		{
			fis = new FileInputStream (file);

			int n;
			byte[] b = new byte[1024];
			while ((n = fis.read(b)) != -1)
			{
				md.update (b, 0, n);
			}
		}
		catch (FileNotFoundException e) { return ""; }
		catch (IOException e) { throw e; }
		finally
		{
			if (fis != null) 
			{
				try { fis.close (); }
				catch (IOException e) {}
				fis = null;
			}
		}

		StringBuffer buf = new StringBuffer ();
		byte[] d = md.digest ();
		for (int i = 0; i < d.length; i++)
		{
			String x = Integer.toHexString((d[i] & 0x00FF));
			if (x.length() == 1) buf.append ('0');
			buf.append (x);
		}
		return buf.toString();
	}

	private void downloadNative (String md5URL, String sourceURL, String destFile) throws Exception
	{
		InputStream is = null;
		FileOutputStream fos = null;
		String sumRemote = null;

		/* download the checksum file */
		try
		{
			URL url = new URL (md5URL);
			URLConnection conn = url.openConnection ();

			is = url.openStream ();

			int n, total = 0;
			byte[] b = new byte[32];
			while ((n = is.read(b, total, 32-total)) != -1)
			{
				total += n;
				if (total >= 32) 
				{
					sumRemote = new String (b);
					break;
				}
			}
		}
		catch (IOException e) { throw e; }
		finally 
		{
			if (is != null) 
			{
				try { is.close (); }
				catch (IOException e) {}
				is = null;
			}
		}

		if (sumRemote != null)
		{
			/* if the checksum matches the checksum of the local file,
			 * the native library file doesn't have to be downloaded */
			String sumLocal = getFileMD5 (destFile);
			if (sumRemote.equalsIgnoreCase(sumLocal)) return;
		}

		/* download the actual file */
		try
		{
			URL url = new URL(sourceURL);
			URLConnection conn = url.openConnection();

			is = url.openStream();
			fos = new FileOutputStream(destFile);

			int n;
			byte[] b = new byte[1024];
			while ((n = is.read(b)) != -1)
			{
				fos.write(b, 0, n);
			}
		}
		catch (IOException e) { throw e; }
		finally
		{
			if (is != null) 
			{
				try { is.close (); }
				catch (IOException e) {}
				is = null;
			}
			if (fos != null) 
			{
				try { fos.close (); }
				catch (IOException e) {}
				fos = null;
			}
		}
	}

	public void dragEnter(DropTargetDragEvent dtde) { }
	public void dragExit(DropTargetEvent dte) { }
	public void dragOver(DropTargetDragEvent dtde) { }
	public void dropActionChanged(DropTargetDragEvent dtde) { }

	public void drop (DropTargetDropEvent dtde) 
	{
		DropTarget dropTarget = dtde.getDropTargetContext().getDropTarget();

		if (dropTarget != srcInDropTarget &&
		    dropTarget != conInDropTarget)
		{
			dtde.rejectDrop ();
			return;
		}

		Transferable tr = dtde.getTransferable ();	
		DataFlavor[] flavors = tr.getTransferDataFlavors();
		for (int i = 0; i < flavors.length; i++) 
		{
  			//System.out.println("Possible flavor: " + flavors[i].getMimeType());

			if (flavors[i].isFlavorJavaFileListType())
			{
				TextArea t = (TextArea)dropTarget.getComponent();
				t.setText ("");

				try
				{
					dtde.acceptDrop (DnDConstants.ACTION_COPY_OR_MOVE);
					List files = (List)tr.getTransferData(flavors[i]);
					Iterator x = files.iterator ();
					while (x.hasNext())
					{
						File file = (File)x.next ();
						loadFileTo (file, t);
					}
					dtde.dropComplete (true);
					return;
    				}
				catch (UnsupportedFlavorException e) 
				{ 
					dtde.rejectDrop ();
					return;
				}
				catch (IOException e)
				{
					dtde.rejectDrop ();
					return;
				}
			}			
			else if (flavors[i].isFlavorSerializedObjectType()) 
			{
				TextArea t = (TextArea)dropTarget.getComponent();
				try
				{
					dtde.acceptDrop (DnDConstants.ACTION_COPY_OR_MOVE);
					Object o = tr.getTransferData(flavors[i]);
					t.replaceText (o.toString(), t.getSelectionStart(), t.getSelectionEnd());
					dtde.dropComplete(true);
					return;
				}
				catch (UnsupportedFlavorException e) 
				{ 
					dtde.rejectDrop ();
					return;
				}
				catch (IOException e)
				{
					dtde.rejectDrop ();
					return;
				}
			}
		}

		dtde.rejectDrop ();
	}

	private void loadFileTo (File file, TextArea textArea) throws IOException
	{
		FileReader fr = null;
		StringBuffer fb = new StringBuffer(textArea.getText());
		
		try
		{
			fr = new FileReader (file);

			int n;
			char[] b = new char[1024];
			while ((n = fr.read (b)) != -1) fb.append (b, 0, n);	
		}
		catch (IOException e) { throw e; }
		finally
		{
			if (fr != null) 
			{
				try { fr.close (); }
				catch (IOException e) {}
				fr = null;
			}
		}

		textArea.setText (fb.toString());
	}
	
}
