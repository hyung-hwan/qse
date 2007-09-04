/*
 * $Id: Awk.cs,v 1.1 2007/09/03 03:50:39 bacon Exp $
 */

using System;
using System.Collections.Generic;
using System.Text;
using System.IO;

using COM = System.Runtime.InteropServices.ComTypes;

namespace ase.com
{
	public class Awk : ASECOM.IAwkEvents
	{
		private ASECOM.Awk awk;

		private int cookie = -1;
		private COM.IConnectionPoint icp;

		private Stream sourceInputStream = null;
		private Stream sourceOutputStream = null;
		private StreamReader sourceInputReader;
		private StreamWriter sourceOutputWriter;

		private Stream consoleInputStream = null;
		private Stream consoleOutputStream = null;
		private StreamReader consoleInputReader;
		private StreamWriter consoleOutputWriter;

		public delegate object FunctionHandler (object[] args);
		private System.Collections.Hashtable funcTable;

		char[] consoleInputBuffer = new char[1024];

		public Awk()
		{
			this.funcTable = new System.Collections.Hashtable();

			this.awk = new ASECOM.Awk();
			this.awk.UseLongLong = true;
			//this.awk.UseCrlf = true;
			
			COM.IConnectionPointContainer icpc = 
				(COM.IConnectionPointContainer)awk;
			Guid g = typeof(ASECOM.IAwkEvents).GUID;

			try
			{
				icpc.FindConnectionPoint(ref g, out icp);
				icp.Advise(this, out this.cookie);
			}
			catch (System.Runtime.InteropServices.COMException ex)
			{
				this.cookie = -1;
				//System.Windows.Forms.MessageBox.Show(ex.Message);
			}
		}

		/*~Awk()
		{
			  if (cookie != -1 && icp != null)
			  {
				   try
				   {
						icp.Unadvise(cookie);
						cookie = -1;
				   }
				   catch (System.Runtime.InteropServices.COMException ex)
				   {
						System.Windows.Forms.MessageBox.Show(ex.Message);
				   }	
			  }
		}*/

		public int ErrorCode
		{
			get { return awk.ErrorCode; }
		}

		public int ErrorLine
		{
			get { return awk.ErrorLine; }
		}

		public string ErrorMessage
		{
			get { return awk.ErrorMessage; }
		}

		public bool ImplicitVariable
		{
			get { return awk.ImplicitVariable; }
			set { awk.ImplicitVariable = value; }
		}

		public bool ExplicitVariable
		{
			get { return awk.ExplicitVariable; }
			set { awk.ExplicitVariable = value; }
		}

		public bool UniqueFunction
		{
			get { return awk.UniqueFunction; }
			set { awk.UniqueFunction = value; }
		}

		public bool VariableShading
		{
			get { return awk.VariableShading; }
			set { awk.VariableShading = value; }
		}

		public bool ShiftOperators
		{
			get { return awk.ShiftOperators; }
			set { awk.ShiftOperators = value; }
		}

		public bool IdivOperator
		{
			get { return awk.IdivOperator; }
			set { awk.IdivOperator = value; }
		}

		public bool ConcatString
		{
			get { return awk.ConcatString; }
			set { awk.ConcatString = value; }
		}

		public bool SupportExtio
		{
			get { return awk.SupportExtio; }
			set { awk.SupportExtio = value; }
		}

		public bool SupportBlockless
		{
			get { return awk.SupportBlockless; }
			set { awk.SupportBlockless = value; }
		}

		public bool StringBaseOne
		{
			get { return awk.StringBaseOne; }
			set { awk.StringBaseOne = value; }
		}

		public bool StripSpaces
		{
			get { return awk.StripSpaces; }
			set { awk.StripSpaces = value; }
		}

		public bool Nextofile
		{
			get { return awk.Nextofile; }
			set { awk.Nextofile = value; }
		}

		public bool Usecrlf
		{
			get { return awk.UseCrlf; }
			set { awk.UseCrlf = value; }
		}

		public string EntryPoint
		{
			get { return awk.EntryPoint; }
			set { awk.EntryPoint = value; }
		}

		public bool ArgumentsToEntryPoint
		{
			get { return awk.ArgumentsToEntryPoint; }
			set { awk.ArgumentsToEntryPoint = value; }
		}

		public bool Debug
		{
			get { return awk.Debug; }
			set { awk.Debug = value; }
		}

		/* this property doesn't need to be available to the public
		 * as it can be always true in .NET environment. However, 
		 * it is kept private here for reference */
		private bool UseLongLong
		{
			get { return awk.UseLongLong; }
			set { awk.UseLongLong = value; }
		}

		public int MaxDepthForBlockParse
		{
			get { return awk.MaxDepthForBlockParse; }
			set { awk.MaxDepthForBlockParse = value; }
		}

		public int MaxDepthForBlockRun
		{
			get { return awk.MaxDepthForBlockRun; }
			set { awk.MaxDepthForBlockRun = value; }
		}

		public int MaxDepthForExprParse
		{
			get { return awk.MaxDepthForExprParse; }
			set { awk.MaxDepthForExprParse = value; }
		}

		public int MaxDepthForExprRun
		{
			get { return awk.MaxDepthForExprRun; }
			set { awk.MaxDepthForExprRun = value; }
		}

		public int MaxDepthForRexBuild
		{
			get { return awk.MaxDepthForRexBuild; }
			set { awk.MaxDepthForRexBuild = value; }
		}

		public int MaxDepthForRexMatch
		{
			get { return awk.MaxDepthForRexMatch; }
			set { awk.MaxDepthForRexMatch = value; }
		}

		public virtual bool AddFunction(string name, int minArgs, int maxArgs, FunctionHandler handler)
		{
			if (funcTable.ContainsKey(name)) return false;
			
			funcTable.Add(name, handler);
			if (!awk.AddFunction(name, minArgs, maxArgs))
			{
				funcTable.Remove(name);
				return false;
			}

			return true;
		}

		public virtual bool DeleteFunction(string name)
		{
			if (!funcTable.ContainsKey(name)) return false;

			if (awk.DeleteFunction(name))
			{
				funcTable.Remove(name);
				return true;
			}

			return false;
		}

		public virtual bool Parse()
		{
			return awk.Parse();
		}

		public virtual bool Run ()
		{	
			return awk.Run(null);
		}

		public virtual bool Run(string[] args)
		{
			return awk.Run(args);
		}

		public Stream SourceInputStream
		{
			get { return this.sourceInputStream; }
			set { this.sourceInputStream = value; }
		}

		public Stream SourceOutputStream
		{
			get { return this.sourceOutputStream; }
			set { this.sourceOutputStream = value; }
		}

		public Stream ConsoleInputStream
		{
			get { return this.consoleInputStream; }
			set { this.consoleInputStream = value; }
		}

		public Stream ConsoleOutputStream
		{
			get { return this.consoleOutputStream; }
			set { this.consoleOutputStream = value; }
		}
		
		public virtual int OpenSource(ASECOM.AwkSourceMode mode)
		{
			if (mode == ASECOM.AwkSourceMode.AWK_SOURCE_READ)
			{
				if (this.sourceInputStream == null) return 0;
				this.sourceInputReader = new StreamReader (this.sourceInputStream);
				return 1;
			}
			else if (mode == ASECOM.AwkSourceMode.AWK_SOURCE_WRITE)
			{
				if (this.sourceOutputStream == null) return 0;
				this.sourceOutputWriter = new StreamWriter (this.sourceOutputStream);
				return 1;
			}

			return -1;
		}

		public virtual int CloseSource(ASECOM.AwkSourceMode mode)
		{
			if (mode == ASECOM.AwkSourceMode.AWK_SOURCE_READ)
			{
				this.sourceInputReader.Close ();
				return 0;
			}
			else if (mode == ASECOM.AwkSourceMode.AWK_SOURCE_WRITE)
			{
				this.sourceOutputWriter.Close ();
				return 0;
			}

			return -1;
		}

		public virtual int ReadSource(ASECOM.Buffer buf)
		{
			buf.Value = this.sourceInputReader.ReadLine();
			if (buf.Value == null) return 0;
			return buf.Value.Length;
		}

		public virtual int WriteSource(ASECOM.Buffer buf)
		{
			this.sourceOutputWriter.Write(buf.Value);
			return buf.Value.Length;
		}

		public virtual int OpenExtio(ASECOM.AwkExtio extio)
		{
			if (extio.Mode == ASECOM.AwkExtioMode.AWK_EXTIO_CONSOLE_READ)
			{
				if (this.consoleInputStream == null) return 0;
				this.consoleInputReader = new StreamReader(this.consoleInputStream);
				return 1;
			}
			else if (extio.Mode == ASECOM.AwkExtioMode.AWK_EXTIO_CONSOLE_WRITE)
			{
				if (this.consoleOutputStream == null) return 0;
				this.consoleOutputWriter = new StreamWriter(this.consoleOutputStream);
				return 1;
			}

			return -1;
		}

		public virtual int CloseExtio(ASECOM.AwkExtio extio)
		{
			if (extio.Mode == ASECOM.AwkExtioMode.AWK_EXTIO_CONSOLE_READ)
			{
				this.consoleInputReader.Close();
				return 0;
			}
			else if (extio.Mode == ASECOM.AwkExtioMode.AWK_EXTIO_CONSOLE_WRITE)
			{
				this.consoleOutputWriter.Close();
				return 0;
			}

			return -1;
		}

		public virtual int ReadExtio(ASECOM.AwkExtio extio, ASECOM.Buffer buf)
		{
			if (extio.Mode == ASECOM.AwkExtioMode.AWK_EXTIO_CONSOLE_READ)
			{
				int n = this.consoleInputReader.Read(consoleInputBuffer, 0, consoleInputBuffer.Length);
				if (n == 0) return 0;
				buf.Value = new string(consoleInputBuffer, 0, n);
				return buf.Value.Length;
			}

			return -1;
		}

		public virtual int WriteExtio(ASECOM.AwkExtio extio, ASECOM.Buffer buf)
		{
			if (extio.Mode == ASECOM.AwkExtioMode.AWK_EXTIO_CONSOLE_WRITE)
			{
				this.consoleOutputWriter.Write(buf.Value);
				return buf.Value.Length;
			}

			return -1;
		}

		public virtual int FlushExtio(ASECOM.AwkExtio extio)
		{
			return -1;
		}

		public virtual int NextExtio(ASECOM.AwkExtio extio)
		{
			return 1;
		}
		
		public virtual object HandleFunction(string name, object argarray)
		{
			FunctionHandler handler = (FunctionHandler)funcTable[name];
			return handler((object[])argarray);
		}

	}
}
