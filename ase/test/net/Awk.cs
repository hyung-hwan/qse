using System;
using System.Collections.Generic;
using System.Text;

namespace asetestnet
{
	public class Awk : ASE.Net.StdAwk
	{
		System.Windows.Forms.TextBox sourceInput;
		System.Windows.Forms.TextBox sourceOutput;

		System.Windows.Forms.TextBox consoleInput;
		System.Windows.Forms.TextBox consoleOutput;

		public bool Parse(
			System.Windows.Forms.TextBox sourceInput, 
			System.Windows.Forms.TextBox sourceOutput)
		{
			this.sourceInput = sourceInput;
			this.sourceOutput = sourceOutput;
			return base.Parse();
		}

		public bool Run(
			System.Windows.Forms.TextBox consoleInput,
			System.Windows.Forms.TextBox consoleOutput)
		{
			this.consoleInput = consoleInput;
			this.consoleOutput = consoleOutput;
			return base.Run();
		}

		protected override int OpenSource(ASE.Net.StdAwk.Source source)
		{
			//System.IO.FileMode mode;
			//System.IO.FileAccess access;

			if (source.Mode.Equals(ASE.Net.StdAwk.Source.MODE.READ))
			{
				//mode = System.IO.FileMode.Open;
				//access = System.IO.FileAccess.Read;
				//fs = new System.IO.FileStream("t.awk", mode, access);

				if (sourceInput == null) return -1;

				System.IO.MemoryStream ms = new System.IO.MemoryStream (UnicodeEncoding.UTF8.GetBytes(sourceInput.Text));
				source.Handle = new System.IO.StreamReader(ms);
				return 1;
			}
			else if (source.Mode.Equals(ASE.Net.StdAwk.Source.MODE.WRITE))
			{
				//mode = System.IO.FileMode.Create;
				//access = System.IO.FileAccess.Write;
				//fs = new System.IO.FileStream("t.out", mode, access);

				if (sourceOutput == null) return -1;
				System.IO.MemoryStream ms = new System.IO.MemoryStream ();
				source.Handle = new System.IO.StreamWriter(ms);
				return 1;
			}

			return -1;
		}

		protected override int CloseSource(ASE.Net.StdAwk.Source source)
		{
			if (source.Mode.Equals(ASE.Net.StdAwk.Source.MODE.READ))
			{
				System.IO.StreamReader sr = (System.IO.StreamReader)source.Handle;
				sr.Close();
				return 0;
			}
			else if (source.Mode.Equals(ASE.Net.StdAwk.Source.MODE.WRITE))
			{
				System.IO.StreamWriter sw = (System.IO.StreamWriter)source.Handle;
				sw.Flush();
				System.IO.MemoryStream ms = (System.IO.MemoryStream)sw.BaseStream;
				sourceOutput.Text = UnicodeEncoding.UTF8.GetString(ms.GetBuffer());
				sw.Close();
				return 0;
			}

			return -1;
		}

		protected override int ReadSource(ASE.Net.StdAwk.Source source, char[] buf, int len)
		{
			System.IO.StreamReader sr = (System.IO.StreamReader)source.Handle;
			return sr.Read(buf, 0, len);
		}

		protected override int WriteSource(ASE.Net.StdAwk.Source source, char[] buf, int len)
		{
			System.IO.StreamWriter sw = (System.IO.StreamWriter)source.Handle;
			sw.Write(buf, 0, len);
			return len;
		}

		protected override int OpenConsole(ASE.Net.StdAwk.Console console)
		{
			if (console.Mode.Equals(ASE.Net.StdAwk.Console.MODE.READ))
			{
				if (consoleInput == null) return -1;

				System.IO.MemoryStream ms = new System.IO.MemoryStream(UnicodeEncoding.UTF8.GetBytes(consoleInput.Text));
				console.Handle = new System.IO.StreamReader(ms);
				return 1;
			}
			else if (console.Mode.Equals(ASE.Net.StdAwk.Console.MODE.WRITE))
			{
				if (consoleOutput == null) return -1;
				System.IO.MemoryStream ms = new System.IO.MemoryStream();
				console.Handle = new System.IO.StreamWriter(ms);
				return 1;
			}

			return -1;
		}

		protected override int CloseConsole(ASE.Net.StdAwk.Console console)
		{
			if (console.Mode.Equals(ASE.Net.StdAwk.Console.MODE.READ))
			{
				System.IO.StreamReader sr = (System.IO.StreamReader)console.Handle;
				sr.Close();
				return 0;
			}
			else if (console.Mode.Equals(ASE.Net.StdAwk.Console.MODE.WRITE))
			{
				System.IO.StreamWriter sw = (System.IO.StreamWriter)console.Handle;
				sw.Flush();
				System.IO.MemoryStream ms = (System.IO.MemoryStream)sw.BaseStream;
				consoleOutput.Text = UnicodeEncoding.UTF8.GetString(ms.GetBuffer());
				sw.Close();
				return 0;
			}

			return -1;
		}

		protected override int ReadConsole(ASE.Net.StdAwk.Console console, char[] buf, int len)
		{
			System.IO.StreamReader sr = (System.IO.StreamReader)console.Handle;
			return sr.Read(buf, 0, len);
		}

		protected override int WriteConsole(ASE.Net.StdAwk.Console console, char[] buf, int len)
		{
			System.IO.StreamWriter sw = (System.IO.StreamWriter)console.Handle;
			sw.Write(buf, 0, len);
			return len;
		}

		protected override int FlushConsole(ASE.Net.StdAwk.Console console)
		{
			System.IO.StreamWriter sw = (System.IO.StreamWriter)console.Handle;
			sw.Flush();
			return 0;
		}

		protected override int NextConsole(ASE.Net.StdAwk.Console console)
		{
			return 0;
		}
	}

}
