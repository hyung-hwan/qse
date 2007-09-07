using System;
using System.Collections.Generic;
using System.Text;

namespace ase.net
{
	public class Awk : ASE.Net.StdAwk
	{
		System.Windows.Forms.TextBox sourceInput;
		System.Windows.Forms.TextBox sourceOutput;

		System.Windows.Forms.TextBox consoleInput;
		System.Windows.Forms.TextBox consoleOutput;

		System.ComponentModel.ISynchronizeInvoke si;

		public Awk(System.ComponentModel.ISynchronizeInvoke si)
		{
			this.si = si;
			SetSourceOutputHandlers += SetSourceOutput;
			SetConsoleOutputHandlers += SetConsoleOutput;

			AddFunction("sleep", 1, 1, Sleep);
		}

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
			System.Windows.Forms.TextBox consoleOutput,
			System.String main, System.String[] args)
		{
			this.consoleInput = consoleInput;
			this.consoleOutput = consoleOutput;
			return base.Run(main, args);
		}

		protected bool Sleep(string name, Argument[] args, Return ret)
		{
			System.Threading.Thread.Sleep((int)(args[0].LongValue*1000));	
			ret.LongValue = 0;
			return true;
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

		public delegate void SetSourceOutputHandler(string text);
		public delegate void SetConsoleOutputHandler(string text);

		public event SetSourceOutputHandler SetSourceOutputHandlers;
		public event SetConsoleOutputHandler SetConsoleOutputHandlers;
		
		private void SetSourceOutput(string text)
		{
			sourceOutput.Text = text;
		}

		private void SetConsoleOutput(string text)
		{
			consoleOutput.Text = text;
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
				sw.Close();

				// MSDN: This method(GetBuffer) works when the memory stream is closed.
				//sourceOutput.Text = UnicodeEncoding.UTF8.GetString(ms.GetBuffer());
				if (si != null && si.InvokeRequired)
					si.Invoke(SetSourceOutputHandlers, new object[] { UnicodeEncoding.UTF8.GetString(ms.GetBuffer()) });
				else SetSourceOutput (UnicodeEncoding.UTF8.GetString(ms.GetBuffer()));

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
				sw.Close();
				
				// MSDN: This method(GetBuffer) works when the memory stream is closed.
				//consoleOutput.Text = UnicodeEncoding.UTF8.GetString(ms.GetBuffer());
				if (si != null && si.InvokeRequired)
					si.Invoke(SetConsoleOutputHandlers, new object[] { UnicodeEncoding.UTF8.GetString(ms.GetBuffer()) });
				else SetConsoleOutput(UnicodeEncoding.UTF8.GetString(ms.GetBuffer()));

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
			sw.Flush();
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
