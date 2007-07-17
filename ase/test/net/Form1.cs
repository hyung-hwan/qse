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
		public Form1()
		{
			InitializeComponent();
		}

		private void button1_Click(object sender, EventArgs e)
		{
			ASE.Net.Awk awk = new ASE.Net.StdAwk();

			awk.Open();
			awk.SourceInputStream = new System.IO.FileStream("t.awk", System.IO.FileMode.Open, System.IO.FileAccess.Read);
			awk.SourceOutputStream = new System.IO.FileStream("t.out", System.IO.FileMode.Create, System.IO.FileAccess.Write);

			awk.Parse();
		}
	}
}