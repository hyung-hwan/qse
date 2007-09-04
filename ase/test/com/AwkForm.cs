using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;

namespace ase.com
{
	public partial class AwkForm : Form
	{
		public AwkForm()
		{
			InitializeComponent();
		}

		private void btnRun_Click(object sender, EventArgs e)
		{
			Awk awk = new StdAwk ();
			
			//System.Text.Encoding.Default
			awk.SourceInputStream = new MemoryStream (UnicodeEncoding.UTF8.GetBytes(tbxSourceInput.Text));
			awk.SourceOutputStream = new MemoryStream();

			awk.ConsoleInputStream = new MemoryStream(UnicodeEncoding.UTF8.GetBytes(tbxConsoleInput.Text));
			awk.ConsoleOutputStream = new MemoryStream();

			tbxSourceOutput.Text = "";
			tbxConsoleOutput.Text = "";

			if (!awk.Parse())
			{
				MessageBox.Show(awk.ErrorMessage);
			}
			else
			{
				MemoryStream s = (MemoryStream)awk.SourceOutputStream;
				tbxSourceOutput.Text = UnicodeEncoding.UTF8.GetString(s.GetBuffer());

				awk.EntryPoint = cbxEntryPoint.Text;
				awk.ArgumentsToEntryPoint = chkPassArgumentsToEntryPoint.Checked;

				bool n;
				int nargs = lbxArguments.Items.Count;
				if (nargs > 0)
				{
					string[] args = new string[nargs];
					for (int i = 0; i < nargs; i++)
						args[i] = lbxArguments.Items[i].ToString();
					n = awk.Run(args);
				}
				else n = awk.Run();
				
				if (!n)
				{
					MessageBox.Show(awk.ErrorMessage);
				}
				else
				{
					MemoryStream c = (MemoryStream)awk.ConsoleOutputStream;
					tbxConsoleOutput.Text = UnicodeEncoding.UTF8.GetString(c.GetBuffer());
				}
			}

			//awk.Close();
		}

	
		private void btnAddArgument_Click(object sender, EventArgs e)
		{
			if (tbxArgument.Text.Length > 0)
			{
				lbxArguments.Items.Add(tbxArgument.Text);
				tbxArgument.Text = "";
				tbxArgument.Focus();
			}
		}

		private void btnClearAllArguments_Click(object sender, EventArgs e)
		{
			lbxArguments.Items.Clear();
		}

	}
}
