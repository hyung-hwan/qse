using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;

namespace asetestnet
{
	public partial class AwkForm : Form
	{
		public AwkForm()
		{
			InitializeComponent();
		}

		private void btnRun_Click(object sender, EventArgs e)
		{
			//using (Awk awk = new Awk())
			{
			Awk awk = new Awk();
				tbxSourceOutput.Text = "";
				tbxConsoleOutput.Text = "";

				if (!awk.Parse(tbxSourceInput, tbxSourceOutput))
				{
					MessageBox.Show("Parse error");
					//MessageBox.Show(awk.ErrorMessage);
				}
				else
				{

					/*
					awk.EntryPoint = cbxEntryPoint.Text;
					awk.ArgumentsToEntryPoint = chkPassArgumentsToEntryPoint.Checked;*/

					bool n;
					/*int nargs = lbxArguments.Items.Count;
					if (nargs > 0)
					{
						string[] args = new string[nargs];
						for (int i = 0; i < nargs; i++)
							args[i] = lbxArguments.Items[i].ToString();
						n = awk.Run(args);
					}
					else*/
					n = awk.Run(tbxConsoleInput, tbxConsoleOutput);

					if (!n)
					{
						//MessageBox.Show(awk.ErrorMessage);
						MessageBox.Show("Run Error");
					}

				}

				awk.Close();
			}
			//awk.Dispose(); 
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
