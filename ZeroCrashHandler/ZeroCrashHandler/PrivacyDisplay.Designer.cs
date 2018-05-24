namespace ZeroCrashHandler
{
  partial class PrivacyDisplay
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
      System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle1 = new System.Windows.Forms.DataGridViewCellStyle();
      System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(PrivacyDisplay));
      this.button1 = new System.Windows.Forms.Button();
      this.dataGridView1 = new System.Windows.Forms.DataGridView();
      this.panel1 = new System.Windows.Forms.Panel();
      this.panel2 = new System.Windows.Forms.Panel();
      this.richTextBox2 = new System.Windows.Forms.RichTextBox();
      this.panel3 = new System.Windows.Forms.Panel();
      this.richTextBox1 = new System.Windows.Forms.RichTextBox();
      ((System.ComponentModel.ISupportInitialize)(this.dataGridView1)).BeginInit();
      this.panel1.SuspendLayout();
      this.panel2.SuspendLayout();
      this.panel3.SuspendLayout();
      this.SuspendLayout();
      // 
      // button1
      // 
      this.button1.Location = new System.Drawing.Point(227, 4);
      this.button1.Name = "button1";
      this.button1.Size = new System.Drawing.Size(75, 23);
      this.button1.TabIndex = 0;
      this.button1.Text = "Close";
      this.button1.UseVisualStyleBackColor = true;
      this.button1.Click += new System.EventHandler(this.button1_Click);
      // 
      // dataGridView1
      // 
      dataGridViewCellStyle1.WrapMode = System.Windows.Forms.DataGridViewTriState.True;
      this.dataGridView1.AlternatingRowsDefaultCellStyle = dataGridViewCellStyle1;
      this.dataGridView1.AutoSizeRowsMode = System.Windows.Forms.DataGridViewAutoSizeRowsMode.AllCells;
      this.dataGridView1.BackgroundColor = System.Drawing.SystemColors.Control;
      this.dataGridView1.BorderStyle = System.Windows.Forms.BorderStyle.None;
      this.dataGridView1.ColumnHeadersVisible = false;
      this.dataGridView1.Dock = System.Windows.Forms.DockStyle.Fill;
      this.dataGridView1.Location = new System.Drawing.Point(0, 33);
      this.dataGridView1.Name = "dataGridView1";
      this.dataGridView1.RowHeadersVisible = false;
      this.dataGridView1.RowTemplate.ReadOnly = true;
      this.dataGridView1.Size = new System.Drawing.Size(534, 113);
      this.dataGridView1.TabIndex = 4;
      // 
      // panel1
      // 
      this.panel1.Controls.Add(this.button1);
      this.panel1.Dock = System.Windows.Forms.DockStyle.Bottom;
      this.panel1.Location = new System.Drawing.Point(0, 168);
      this.panel1.Name = "panel1";
      this.panel1.Size = new System.Drawing.Size(534, 31);
      this.panel1.TabIndex = 5;
      // 
      // panel2
      // 
      this.panel2.Controls.Add(this.richTextBox2);
      this.panel2.Dock = System.Windows.Forms.DockStyle.Top;
      this.panel2.Location = new System.Drawing.Point(0, 0);
      this.panel2.Name = "panel2";
      this.panel2.Size = new System.Drawing.Size(534, 33);
      this.panel2.TabIndex = 6;
      // 
      // richTextBox2
      // 
      this.richTextBox2.BackColor = System.Drawing.SystemColors.Control;
      this.richTextBox2.BorderStyle = System.Windows.Forms.BorderStyle.None;
      this.richTextBox2.Dock = System.Windows.Forms.DockStyle.Fill;
      this.richTextBox2.Location = new System.Drawing.Point(0, 0);
      this.richTextBox2.Name = "richTextBox2";
      this.richTextBox2.ReadOnly = true;
      this.richTextBox2.Size = new System.Drawing.Size(534, 33);
      this.richTextBox2.TabIndex = 1;
      this.richTextBox2.Text = "We collect your data so that we can help improve the stability of the engine. We\'" +
    "ve minimized what data we need to collect to protect your privacy.\n";
      // 
      // panel3
      // 
      this.panel3.Controls.Add(this.richTextBox1);
      this.panel3.Dock = System.Windows.Forms.DockStyle.Bottom;
      this.panel3.Location = new System.Drawing.Point(0, 146);
      this.panel3.Name = "panel3";
      this.panel3.Size = new System.Drawing.Size(534, 22);
      this.panel3.TabIndex = 7;
      // 
      // richTextBox1
      // 
      this.richTextBox1.BackColor = System.Drawing.SystemColors.Control;
      this.richTextBox1.BorderStyle = System.Windows.Forms.BorderStyle.None;
      this.richTextBox1.Dock = System.Windows.Forms.DockStyle.Fill;
      this.richTextBox1.Location = new System.Drawing.Point(0, 0);
      this.richTextBox1.Name = "richTextBox1";
      this.richTextBox1.ReadOnly = true;
      this.richTextBox1.Size = new System.Drawing.Size(534, 22);
      this.richTextBox1.TabIndex = 1;
      this.richTextBox1.Text = "For details regarding your personal data rights visit: https://dev.zeroengine.io/" +
    "u/PrivacyInfo";
      this.richTextBox1.LinkClicked += new System.Windows.Forms.LinkClickedEventHandler(this.richTextBox1_LinkClicked);
      // 
      // PrivacyDisplay
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.ClientSize = new System.Drawing.Size(534, 199);
      this.Controls.Add(this.dataGridView1);
      this.Controls.Add(this.panel3);
      this.Controls.Add(this.panel2);
      this.Controls.Add(this.panel1);
      this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
      this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
      this.Name = "PrivacyDisplay";
      this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
      this.Text = "Data Collection";
      this.Load += new System.EventHandler(this.Privacy_Load);
      ((System.ComponentModel.ISupportInitialize)(this.dataGridView1)).EndInit();
      this.panel1.ResumeLayout(false);
      this.panel2.ResumeLayout(false);
      this.panel3.ResumeLayout(false);
      this.ResumeLayout(false);

    }

    #endregion

    private System.Windows.Forms.Button button1;
    private System.Windows.Forms.DataGridView dataGridView1;
    private System.Windows.Forms.Panel panel1;
    private System.Windows.Forms.Panel panel2;
    private System.Windows.Forms.RichTextBox richTextBox2;
    private System.Windows.Forms.Panel panel3;
    private System.Windows.Forms.RichTextBox richTextBox1;
  }
}