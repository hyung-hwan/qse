namespace ase.net
{
	partial class AwkForm
	{
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.IContainer components = null;

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		/// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
		protected override void Dispose(bool disposing)
		{
			if (disposing && (components != null))
			{
				components.Dispose();
			}
			base.Dispose(disposing);
		}

		#region Windows Form Designer generated code

		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.tbxSourceInput = new System.Windows.Forms.TextBox();
			this.btnRun = new System.Windows.Forms.Button();
			this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
			this.panel1 = new System.Windows.Forms.Panel();
			this.label1 = new System.Windows.Forms.Label();
			this.panel3 = new System.Windows.Forms.Panel();
			this.tbxSourceOutput = new System.Windows.Forms.TextBox();
			this.label2 = new System.Windows.Forms.Label();
			this.panel4 = new System.Windows.Forms.Panel();
			this.tbxConsoleInput = new System.Windows.Forms.TextBox();
			this.label3 = new System.Windows.Forms.Label();
			this.panel5 = new System.Windows.Forms.Panel();
			this.tbxConsoleOutput = new System.Windows.Forms.TextBox();
			this.label4 = new System.Windows.Forms.Label();
			this.statusStrip1 = new System.Windows.Forms.StatusStrip();
			this.cbxEntryPoint = new System.Windows.Forms.ComboBox();
			this.panel2 = new System.Windows.Forms.Panel();
			this.clbOptions = new System.Windows.Forms.CheckedListBox();
			this.groupBox2 = new System.Windows.Forms.GroupBox();
			this.groupBox1 = new System.Windows.Forms.GroupBox();
			this.btnClearAllArguments = new System.Windows.Forms.Button();
			this.btnAddArgument = new System.Windows.Forms.Button();
			this.tbxArgument = new System.Windows.Forms.TextBox();
			this.lbxArguments = new System.Windows.Forms.ListBox();
			this.tableLayoutPanel1.SuspendLayout();
			this.panel1.SuspendLayout();
			this.panel3.SuspendLayout();
			this.panel4.SuspendLayout();
			this.panel5.SuspendLayout();
			this.panel2.SuspendLayout();
			this.groupBox2.SuspendLayout();
			this.groupBox1.SuspendLayout();
			this.SuspendLayout();
			// 
			// tbxSourceInput
			// 
			this.tbxSourceInput.Dock = System.Windows.Forms.DockStyle.Fill;
			this.tbxSourceInput.Font = new System.Drawing.Font("Courier New", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.tbxSourceInput.Location = new System.Drawing.Point(0, 19);
			this.tbxSourceInput.Multiline = true;
			this.tbxSourceInput.Name = "tbxSourceInput";
			this.tbxSourceInput.ScrollBars = System.Windows.Forms.ScrollBars.Both;
			this.tbxSourceInput.Size = new System.Drawing.Size(240, 230);
			this.tbxSourceInput.TabIndex = 1;
			this.tbxSourceInput.WordWrap = false;
			// 
			// btnRun
			// 
			this.btnRun.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
			this.btnRun.Location = new System.Drawing.Point(78, 484);
			this.btnRun.Name = "btnRun";
			this.btnRun.Size = new System.Drawing.Size(75, 23);
			this.btnRun.TabIndex = 2;
			this.btnRun.Text = "Run";
			this.btnRun.UseVisualStyleBackColor = true;
			this.btnRun.Click += new System.EventHandler(this.btnRun_Click);
			// 
			// tableLayoutPanel1
			// 
			this.tableLayoutPanel1.ColumnCount = 2;
			this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
			this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
			this.tableLayoutPanel1.Controls.Add(this.panel1, 0, 0);
			this.tableLayoutPanel1.Controls.Add(this.panel3, 1, 0);
			this.tableLayoutPanel1.Controls.Add(this.panel4, 0, 1);
			this.tableLayoutPanel1.Controls.Add(this.panel5, 1, 1);
			this.tableLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
			this.tableLayoutPanel1.Location = new System.Drawing.Point(157, 0);
			this.tableLayoutPanel1.Name = "tableLayoutPanel1";
			this.tableLayoutPanel1.RowCount = 2;
			this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 50F));
			this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 50F));
			this.tableLayoutPanel1.Size = new System.Drawing.Size(492, 510);
			this.tableLayoutPanel1.TabIndex = 2;
			// 
			// panel1
			// 
			this.panel1.Controls.Add(this.tbxSourceInput);
			this.panel1.Controls.Add(this.label1);
			this.panel1.Dock = System.Windows.Forms.DockStyle.Fill;
			this.panel1.Location = new System.Drawing.Point(3, 3);
			this.panel1.Name = "panel1";
			this.panel1.Size = new System.Drawing.Size(240, 249);
			this.panel1.TabIndex = 5;
			// 
			// label1
			// 
			this.label1.AutoSize = true;
			this.label1.Dock = System.Windows.Forms.DockStyle.Top;
			this.label1.Location = new System.Drawing.Point(0, 0);
			this.label1.Name = "label1";
			this.label1.Padding = new System.Windows.Forms.Padding(0, 3, 0, 3);
			this.label1.Size = new System.Drawing.Size(68, 19);
			this.label1.TabIndex = 2;
			this.label1.Text = "Source Input";
			// 
			// panel3
			// 
			this.panel3.Controls.Add(this.tbxSourceOutput);
			this.panel3.Controls.Add(this.label2);
			this.panel3.Dock = System.Windows.Forms.DockStyle.Fill;
			this.panel3.Location = new System.Drawing.Point(249, 3);
			this.panel3.Name = "panel3";
			this.panel3.Size = new System.Drawing.Size(240, 249);
			this.panel3.TabIndex = 6;
			// 
			// tbxSourceOutput
			// 
			this.tbxSourceOutput.Dock = System.Windows.Forms.DockStyle.Fill;
			this.tbxSourceOutput.Font = new System.Drawing.Font("Courier New", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.tbxSourceOutput.Location = new System.Drawing.Point(0, 19);
			this.tbxSourceOutput.Multiline = true;
			this.tbxSourceOutput.Name = "tbxSourceOutput";
			this.tbxSourceOutput.ReadOnly = true;
			this.tbxSourceOutput.ScrollBars = System.Windows.Forms.ScrollBars.Both;
			this.tbxSourceOutput.Size = new System.Drawing.Size(240, 230);
			this.tbxSourceOutput.TabIndex = 2;
			this.tbxSourceOutput.WordWrap = false;
			// 
			// label2
			// 
			this.label2.AutoSize = true;
			this.label2.Dock = System.Windows.Forms.DockStyle.Top;
			this.label2.Location = new System.Drawing.Point(0, 0);
			this.label2.Name = "label2";
			this.label2.Padding = new System.Windows.Forms.Padding(0, 3, 0, 3);
			this.label2.Size = new System.Drawing.Size(76, 19);
			this.label2.TabIndex = 0;
			this.label2.Text = "Source Output";
			// 
			// panel4
			// 
			this.panel4.Controls.Add(this.tbxConsoleInput);
			this.panel4.Controls.Add(this.label3);
			this.panel4.Dock = System.Windows.Forms.DockStyle.Fill;
			this.panel4.Location = new System.Drawing.Point(3, 258);
			this.panel4.Name = "panel4";
			this.panel4.Size = new System.Drawing.Size(240, 249);
			this.panel4.TabIndex = 7;
			// 
			// tbxConsoleInput
			// 
			this.tbxConsoleInput.Dock = System.Windows.Forms.DockStyle.Fill;
			this.tbxConsoleInput.Font = new System.Drawing.Font("Courier New", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.tbxConsoleInput.Location = new System.Drawing.Point(0, 19);
			this.tbxConsoleInput.Multiline = true;
			this.tbxConsoleInput.Name = "tbxConsoleInput";
			this.tbxConsoleInput.ScrollBars = System.Windows.Forms.ScrollBars.Both;
			this.tbxConsoleInput.Size = new System.Drawing.Size(240, 230);
			this.tbxConsoleInput.TabIndex = 3;
			this.tbxConsoleInput.WordWrap = false;
			// 
			// label3
			// 
			this.label3.AutoSize = true;
			this.label3.Dock = System.Windows.Forms.DockStyle.Top;
			this.label3.Location = new System.Drawing.Point(0, 0);
			this.label3.Name = "label3";
			this.label3.Padding = new System.Windows.Forms.Padding(0, 3, 0, 3);
			this.label3.Size = new System.Drawing.Size(72, 19);
			this.label3.TabIndex = 0;
			this.label3.Text = "Console Input";
			// 
			// panel5
			// 
			this.panel5.Controls.Add(this.tbxConsoleOutput);
			this.panel5.Controls.Add(this.label4);
			this.panel5.Dock = System.Windows.Forms.DockStyle.Fill;
			this.panel5.Location = new System.Drawing.Point(249, 258);
			this.panel5.Name = "panel5";
			this.panel5.Size = new System.Drawing.Size(240, 249);
			this.panel5.TabIndex = 8;
			// 
			// tbxConsoleOutput
			// 
			this.tbxConsoleOutput.Dock = System.Windows.Forms.DockStyle.Fill;
			this.tbxConsoleOutput.Font = new System.Drawing.Font("Courier New", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.tbxConsoleOutput.Location = new System.Drawing.Point(0, 19);
			this.tbxConsoleOutput.Multiline = true;
			this.tbxConsoleOutput.Name = "tbxConsoleOutput";
			this.tbxConsoleOutput.ReadOnly = true;
			this.tbxConsoleOutput.ScrollBars = System.Windows.Forms.ScrollBars.Both;
			this.tbxConsoleOutput.Size = new System.Drawing.Size(240, 230);
			this.tbxConsoleOutput.TabIndex = 4;
			this.tbxConsoleOutput.WordWrap = false;
			// 
			// label4
			// 
			this.label4.AutoSize = true;
			this.label4.Dock = System.Windows.Forms.DockStyle.Top;
			this.label4.Location = new System.Drawing.Point(0, 0);
			this.label4.Name = "label4";
			this.label4.Padding = new System.Windows.Forms.Padding(0, 3, 0, 3);
			this.label4.Size = new System.Drawing.Size(80, 19);
			this.label4.TabIndex = 0;
			this.label4.Text = "Console Output";
			// 
			// statusStrip1
			// 
			this.statusStrip1.Location = new System.Drawing.Point(0, 510);
			this.statusStrip1.Name = "statusStrip1";
			this.statusStrip1.Size = new System.Drawing.Size(649, 22);
			this.statusStrip1.TabIndex = 3;
			this.statusStrip1.Text = "statusStrip1";
			// 
			// cbxEntryPoint
			// 
			this.cbxEntryPoint.Dock = System.Windows.Forms.DockStyle.Fill;
			this.cbxEntryPoint.FormattingEnabled = true;
			this.cbxEntryPoint.Location = new System.Drawing.Point(3, 16);
			this.cbxEntryPoint.Name = "cbxEntryPoint";
			this.cbxEntryPoint.Size = new System.Drawing.Size(147, 21);
			this.cbxEntryPoint.TabIndex = 1;
			// 
			// panel2
			// 
			this.panel2.AutoScroll = true;
			this.panel2.Controls.Add(this.clbOptions);
			this.panel2.Controls.Add(this.btnRun);
			this.panel2.Controls.Add(this.groupBox2);
			this.panel2.Controls.Add(this.groupBox1);
			this.panel2.Dock = System.Windows.Forms.DockStyle.Left;
			this.panel2.Location = new System.Drawing.Point(0, 0);
			this.panel2.Name = "panel2";
			this.panel2.Size = new System.Drawing.Size(157, 510);
			this.panel2.TabIndex = 5;
			// 
			// clbOptions
			// 
			this.clbOptions.FormattingEnabled = true;
			this.clbOptions.Location = new System.Drawing.Point(0, 279);
			this.clbOptions.Name = "clbOptions";
			this.clbOptions.Size = new System.Drawing.Size(157, 199);
			this.clbOptions.TabIndex = 3;
			// 
			// groupBox2
			// 
			this.groupBox2.Controls.Add(this.cbxEntryPoint);
			this.groupBox2.Location = new System.Drawing.Point(0, 4);
			this.groupBox2.Name = "groupBox2";
			this.groupBox2.Size = new System.Drawing.Size(153, 45);
			this.groupBox2.TabIndex = 1;
			this.groupBox2.TabStop = false;
			this.groupBox2.Text = "Entry Point";
			// 
			// groupBox1
			// 
			this.groupBox1.AutoSize = true;
			this.groupBox1.Controls.Add(this.btnClearAllArguments);
			this.groupBox1.Controls.Add(this.btnAddArgument);
			this.groupBox1.Controls.Add(this.tbxArgument);
			this.groupBox1.Controls.Add(this.lbxArguments);
			this.groupBox1.Location = new System.Drawing.Point(0, 51);
			this.groupBox1.Name = "groupBox1";
			this.groupBox1.Size = new System.Drawing.Size(154, 222);
			this.groupBox1.TabIndex = 0;
			this.groupBox1.TabStop = false;
			this.groupBox1.Text = "Arguments";
			// 
			// btnClearAllArguments
			// 
			this.btnClearAllArguments.Location = new System.Drawing.Point(3, 181);
			this.btnClearAllArguments.Name = "btnClearAllArguments";
			this.btnClearAllArguments.Size = new System.Drawing.Size(145, 22);
			this.btnClearAllArguments.TabIndex = 3;
			this.btnClearAllArguments.Text = "Clear All";
			this.btnClearAllArguments.UseVisualStyleBackColor = true;
			this.btnClearAllArguments.Click += new System.EventHandler(this.btnClearAllArguments_Click);
			// 
			// btnAddArgument
			// 
			this.btnAddArgument.Location = new System.Drawing.Point(87, 154);
			this.btnAddArgument.Name = "btnAddArgument";
			this.btnAddArgument.Size = new System.Drawing.Size(61, 22);
			this.btnAddArgument.TabIndex = 2;
			this.btnAddArgument.Text = "Add";
			this.btnAddArgument.UseVisualStyleBackColor = true;
			this.btnAddArgument.Click += new System.EventHandler(this.btnAddArgument_Click);
			// 
			// tbxArgument
			// 
			this.tbxArgument.Location = new System.Drawing.Point(3, 155);
			this.tbxArgument.Name = "tbxArgument";
			this.tbxArgument.Size = new System.Drawing.Size(83, 20);
			this.tbxArgument.TabIndex = 1;
			// 
			// lbxArguments
			// 
			this.lbxArguments.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.lbxArguments.FormattingEnabled = true;
			this.lbxArguments.Location = new System.Drawing.Point(3, 16);
			this.lbxArguments.Name = "lbxArguments";
			this.lbxArguments.Size = new System.Drawing.Size(147, 134);
			this.lbxArguments.TabIndex = 0;
			// 
			// AwkForm
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(649, 532);
			this.Controls.Add(this.tableLayoutPanel1);
			this.Controls.Add(this.panel2);
			this.Controls.Add(this.statusStrip1);
			this.Name = "AwkForm";
			this.Text = "ASE.NET.AWK";
			this.Load += new System.EventHandler(this.AwkForm_Load);
			this.tableLayoutPanel1.ResumeLayout(false);
			this.panel1.ResumeLayout(false);
			this.panel1.PerformLayout();
			this.panel3.ResumeLayout(false);
			this.panel3.PerformLayout();
			this.panel4.ResumeLayout(false);
			this.panel4.PerformLayout();
			this.panel5.ResumeLayout(false);
			this.panel5.PerformLayout();
			this.panel2.ResumeLayout(false);
			this.panel2.PerformLayout();
			this.groupBox2.ResumeLayout(false);
			this.groupBox1.ResumeLayout(false);
			this.groupBox1.PerformLayout();
			this.ResumeLayout(false);
			this.PerformLayout();

		}

		#endregion

		private System.Windows.Forms.TextBox tbxSourceInput;
		private System.Windows.Forms.Button btnRun;
		private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
		private System.Windows.Forms.TextBox tbxSourceOutput;
		private System.Windows.Forms.TextBox tbxConsoleInput;
		private System.Windows.Forms.TextBox tbxConsoleOutput;
		private System.Windows.Forms.StatusStrip statusStrip1;
		private System.Windows.Forms.ComboBox cbxEntryPoint;
		private System.Windows.Forms.Panel panel2;
		private System.Windows.Forms.GroupBox groupBox1;
		private System.Windows.Forms.Button btnClearAllArguments;
		private System.Windows.Forms.Button btnAddArgument;
		private System.Windows.Forms.TextBox tbxArgument;
		private System.Windows.Forms.ListBox lbxArguments;
		private System.Windows.Forms.GroupBox groupBox2;
		private System.Windows.Forms.Panel panel1;
		private System.Windows.Forms.Label label1;
		private System.Windows.Forms.Panel panel3;
		private System.Windows.Forms.Label label2;
		private System.Windows.Forms.Panel panel4;
		private System.Windows.Forms.Label label3;
		private System.Windows.Forms.Panel panel5;
		private System.Windows.Forms.Label label4;
		private System.Windows.Forms.CheckedListBox clbOptions;
	}
}
