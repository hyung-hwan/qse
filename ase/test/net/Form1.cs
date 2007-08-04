using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace asetestnet
{

	public partial class Form1 : Form
	{

		public class Awk : ASE.Net.StdAwk
		{

			protected override int OpenSource(ASE.Net.StdAwk.Source source)
			{
				System.IO.FileMode mode;
				System.IO.FileAccess access;
				System.IO.FileStream fs;

				if (source.Mode.Equals(ASE.Net.StdAwk.Source.MODE.READ))
				{
					mode = System.IO.FileMode.Open;
					access = System.IO.FileAccess.Read;

					fs = new System.IO.FileStream ("t.awk", mode, access);
					source.Handle = new System.IO.StreamReader (fs);
					return 1;
				}
				else if (source.Mode.Equals(ASE.Net.StdAwk.Source.MODE.WRITE))
				{
					mode = System.IO.FileMode.Create;
					access = System.IO.FileAccess.Write;

					fs = new System.IO.FileStream("t.out", mode, access);
					source.Handle = new System.IO.StreamWriter(fs);
					return 1;
				}

				return -1;
			}

			protected override int CloseSource(ASE.Net.StdAwk.Source source)
			{
				if (source.Mode.Equals(ASE.Net.StdAwk.Source.MODE.READ))
				{
					System.IO.StreamReader sr = (System.IO.StreamReader)source.Handle;
					sr.Close ();
					return 0;
				}
				else if (source.Mode.Equals(ASE.Net.StdAwk.Source.MODE.WRITE))
				{
					System.IO.StreamWriter sw = (System.IO.StreamWriter)source.Handle;
					sw.Close ();
					return 0;
				}

				return -1;
			}

			protected override int ReadSource(ASE.Net.StdAwk.Source source, char[] buf, int len)
			{
				System.IO.StreamReader sr = (System.IO.StreamReader)source.Handle;
				return sr.Read (buf, 0, len);
			}

			protected override int WriteSource(ASE.Net.StdAwk.Source source, char[] buf, int len)
			{
				System.IO.StreamWriter sw = (System.IO.StreamWriter)source.Handle;
				sw.Write(buf, 0, len);
				return len;
			}

			protected override int OpenConsole(ASE.Net.StdAwk.Console console)
			{
				return -1;
			}

			protected override int CloseConsole(ASE.Net.StdAwk.Console console)
			{
				return -1;
			}

			protected override int ReadConsole(ASE.Net.StdAwk.Console console, char[] buf, int len)
			{
				return -1;
			}

			protected override int WriteConsole(ASE.Net.StdAwk.Console console, char[] buf, int len)
			{
				return -1;
			}

			protected override int FlushConsole(ASE.Net.StdAwk.Console console)
			{
				return -1;
			}

			protected override int NextConsole(ASE.Net.StdAwk.Console console)
			{
				return -1;
			}
		}

		public Form1()
		{
			InitializeComponent();
		}

		private void button1_Click(object sender, EventArgs e)
		{
			ASE.Net.Awk awk = new Awk();
			
			/*awk.OpenFileHandler += new ASE.Net.Awk.OpenFile (OpenFile);
			awk.CloseFileHandler += CloseFile;*/

			//awk.Open();
			//awk.SourceInputStream = new System.IO.FileStream("t.awk", System.IO.FileMode.Open, System.IO.FileAccess.Read);
			//awk.SourceOutputStream = new System.IO.FileStream("t.out", System.IO.FileMode.Create, System.IO.FileAccess.Write);

			awk.Parse();
			awk.Run();
		}

		/*
		private int OpenFile(ASE.Net.Awk.File file)
		{
			MessageBox.Show("OpenFile");
			file.Handle = "abc";
			return 1;
		}

		private int CloseFile(ASE.Net.Awk.File file)
		{
			MessageBox.Show("CloseFile" + (string)file.Handle);
			return 0;
		}*/

	}
}
