namespace ZeroCrashHandler
{
    partial class Main
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
      System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(Main));
      this.pictureBox1 = new System.Windows.Forms.PictureBox();
      this.CrashMessageLabel = new System.Windows.Forms.Label();
      this.Send = new System.Windows.Forms.Button();
      this.Message = new System.Windows.Forms.Label();
      this.WhatHappened = new System.Windows.Forms.TextBox();
      this.label2 = new System.Windows.Forms.Label();
      this.Username = new System.Windows.Forms.TextBox();
      this.RestartEngine = new System.Windows.Forms.RadioButton();
      this.RestartEngineSafe = new System.Windows.Forms.RadioButton();
      this.DoNotRestart = new System.Windows.Forms.RadioButton();
      this.ShowStack = new System.Windows.Forms.Button();
      this.ShowLog = new System.Windows.Forms.Button();
      this.ShowScripts = new System.Windows.Forms.Button();
      this.ShowFilesPanel = new System.Windows.Forms.Panel();
      this.mAcceptCheckBox = new System.Windows.Forms.CheckBox();
      this.linkLabel1 = new System.Windows.Forms.LinkLabel();
      ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
      this.ShowFilesPanel.SuspendLayout();
      this.SuspendLayout();
      // 
      // pictureBox1
      // 
      this.pictureBox1.ErrorImage = ((System.Drawing.Image)(resources.GetObject("pictureBox1.ErrorImage")));
      this.pictureBox1.Image = ((System.Drawing.Image)(resources.GetObject("pictureBox1.Image")));
      this.pictureBox1.InitialImage = ((System.Drawing.Image)(resources.GetObject("pictureBox1.InitialImage")));
      this.pictureBox1.Location = new System.Drawing.Point(11, 11);
      this.pictureBox1.Name = "pictureBox1";
      this.pictureBox1.Size = new System.Drawing.Size(64, 64);
      this.pictureBox1.SizeMode = System.Windows.Forms.PictureBoxSizeMode.AutoSize;
      this.pictureBox1.TabIndex = 9;
      this.pictureBox1.TabStop = false;
      // 
      // CrashMessageLabel
      // 
      this.CrashMessageLabel.Location = new System.Drawing.Point(82, 12);
      this.CrashMessageLabel.Name = "CrashMessageLabel";
      this.CrashMessageLabel.Size = new System.Drawing.Size(339, 63);
      this.CrashMessageLabel.TabIndex = 10;
      this.CrashMessageLabel.Text = "{0} crashed, and we\'re really sorry about that! Send the error report, seriously," +
    " we actually work on it if you send it. ";
      // 
      // Send
      // 
      this.Send.Enabled = false;
      this.Send.Location = new System.Drawing.Point(261, 253);
      this.Send.Name = "Send";
      this.Send.Size = new System.Drawing.Size(159, 28);
      this.Send.TabIndex = 2;
      this.Send.Text = "Send and Close";
      this.Send.UseVisualStyleBackColor = true;
      this.Send.Click += new System.EventHandler(this.Send_Click);
      // 
      // Message
      // 
      this.Message.AutoSize = true;
      this.Message.Location = new System.Drawing.Point(8, 84);
      this.Message.Name = "Message";
      this.Message.Size = new System.Drawing.Size(89, 13);
      this.Message.TabIndex = 13;
      this.Message.Text = "What Happened:";
      // 
      // WhatHappened
      // 
      this.WhatHappened.Location = new System.Drawing.Point(11, 100);
      this.WhatHappened.Multiline = true;
      this.WhatHappened.Name = "WhatHappened";
      this.WhatHappened.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
      this.WhatHappened.Size = new System.Drawing.Size(409, 57);
      this.WhatHappened.TabIndex = 0;
      this.WhatHappened.Text = "Please fill this in!";
      // 
      // label2
      // 
      this.label2.AutoSize = true;
      this.label2.Location = new System.Drawing.Point(8, 163);
      this.label2.Name = "label2";
      this.label2.Size = new System.Drawing.Size(100, 13);
      this.label2.TabIndex = 14;
      this.label2.Text = "ZeroHub Username";
      // 
      // Username
      // 
      this.Username.Location = new System.Drawing.Point(114, 160);
      this.Username.Name = "Username";
      this.Username.Size = new System.Drawing.Size(307, 20);
      this.Username.TabIndex = 1;
      // 
      // RestartEngine
      // 
      this.RestartEngine.AutoSize = true;
      this.RestartEngine.Checked = true;
      this.RestartEngine.Location = new System.Drawing.Point(11, 186);
      this.RestartEngine.Name = "RestartEngine";
      this.RestartEngine.Size = new System.Drawing.Size(95, 17);
      this.RestartEngine.TabIndex = 4;
      this.RestartEngine.TabStop = true;
      this.RestartEngine.Text = "Restart Engine";
      this.RestartEngine.UseVisualStyleBackColor = true;
      // 
      // RestartEngineSafe
      // 
      this.RestartEngineSafe.AutoSize = true;
      this.RestartEngineSafe.Location = new System.Drawing.Point(11, 205);
      this.RestartEngineSafe.Name = "RestartEngineSafe";
      this.RestartEngineSafe.Size = new System.Drawing.Size(156, 17);
      this.RestartEngineSafe.TabIndex = 4;
      this.RestartEngineSafe.TabStop = true;
      this.RestartEngineSafe.Text = "Restart Engine (Safe Mode)";
      this.RestartEngineSafe.UseVisualStyleBackColor = true;
      // 
      // DoNotRestart
      // 
      this.DoNotRestart.AutoSize = true;
      this.DoNotRestart.Location = new System.Drawing.Point(11, 224);
      this.DoNotRestart.Name = "DoNotRestart";
      this.DoNotRestart.Size = new System.Drawing.Size(87, 17);
      this.DoNotRestart.TabIndex = 4;
      this.DoNotRestart.TabStop = true;
      this.DoNotRestart.Text = "Don\'t Restart";
      this.DoNotRestart.UseVisualStyleBackColor = true;
      // 
      // ShowStack
      // 
      this.ShowStack.Location = new System.Drawing.Point(3, 2);
      this.ShowStack.Name = "ShowStack";
      this.ShowStack.Size = new System.Drawing.Size(73, 28);
      this.ShowStack.TabIndex = 2;
      this.ShowStack.Text = "Show Stack";
      this.ShowStack.UseVisualStyleBackColor = true;
      this.ShowStack.Click += new System.EventHandler(this.ShowStack_Click);
      // 
      // ShowLog
      // 
      this.ShowLog.Location = new System.Drawing.Point(79, 2);
      this.ShowLog.Name = "ShowLog";
      this.ShowLog.Size = new System.Drawing.Size(73, 28);
      this.ShowLog.TabIndex = 2;
      this.ShowLog.Text = "Show Log";
      this.ShowLog.UseVisualStyleBackColor = true;
      this.ShowLog.Click += new System.EventHandler(this.ShowLog_Click);
      // 
      // ShowScripts
      // 
      this.ShowScripts.Location = new System.Drawing.Point(154, 2);
      this.ShowScripts.Name = "ShowScripts";
      this.ShowScripts.Size = new System.Drawing.Size(77, 28);
      this.ShowScripts.TabIndex = 2;
      this.ShowScripts.Text = "Show Scripts";
      this.ShowScripts.UseVisualStyleBackColor = true;
      this.ShowScripts.Click += new System.EventHandler(this.ShowScripts_Click);
      // 
      // ShowFilesPanel
      // 
      this.ShowFilesPanel.Controls.Add(this.ShowStack);
      this.ShowFilesPanel.Controls.Add(this.ShowLog);
      this.ShowFilesPanel.Controls.Add(this.ShowScripts);
      this.ShowFilesPanel.Location = new System.Drawing.Point(182, 184);
      this.ShowFilesPanel.Name = "ShowFilesPanel";
      this.ShowFilesPanel.Size = new System.Drawing.Size(239, 29);
      this.ShowFilesPanel.TabIndex = 15;
      // 
      // mAcceptCheckBox
      // 
      this.mAcceptCheckBox.AutoSize = true;
      this.mAcceptCheckBox.Location = new System.Drawing.Point(11, 253);
      this.mAcceptCheckBox.Name = "mAcceptCheckBox";
      this.mAcceptCheckBox.Size = new System.Drawing.Size(203, 17);
      this.mAcceptCheckBox.TabIndex = 16;
      this.mAcceptCheckBox.Text = "I consent to having my data collected";
      this.mAcceptCheckBox.UseVisualStyleBackColor = true;
      this.mAcceptCheckBox.CheckedChanged += new System.EventHandler(this.mAcceptCheckBox_CheckedChanged);
      // 
      // linkLabel1
      // 
      this.linkLabel1.AutoSize = true;
      this.linkLabel1.Location = new System.Drawing.Point(8, 273);
      this.linkLabel1.Name = "linkLabel1";
      this.linkLabel1.Size = new System.Drawing.Size(127, 13);
      this.linkLabel1.TabIndex = 17;
      this.linkLabel1.TabStop = true;
      this.linkLabel1.Text = "See what data we collect";
      this.linkLabel1.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.linkLabel1_LinkClicked);
      // 
      // Main
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.ClientSize = new System.Drawing.Size(429, 296);
      this.Controls.Add(this.linkLabel1);
      this.Controls.Add(this.mAcceptCheckBox);
      this.Controls.Add(this.ShowFilesPanel);
      this.Controls.Add(this.WhatHappened);
      this.Controls.Add(this.DoNotRestart);
      this.Controls.Add(this.RestartEngineSafe);
      this.Controls.Add(this.RestartEngine);
      this.Controls.Add(this.Username);
      this.Controls.Add(this.label2);
      this.Controls.Add(this.Message);
      this.Controls.Add(this.Send);
      this.Controls.Add(this.CrashMessageLabel);
      this.Controls.Add(this.pictureBox1);
      this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
      this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
      this.MaximizeBox = false;
      this.MinimizeBox = false;
      this.Name = "Main";
      this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
      this.Text = "Crash Reporter";
      this.TopMost = true;
      this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.Main_FormClosing);
      this.Load += new System.EventHandler(this.Main_Load);
      this.Shown += new System.EventHandler(this.Main_Shown);
      ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
      this.ShowFilesPanel.ResumeLayout(false);
      this.ResumeLayout(false);
      this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.PictureBox pictureBox1;
        private System.Windows.Forms.Label CrashMessageLabel;
        private System.Windows.Forms.Button Send;
        private System.Windows.Forms.Label Message;
        private System.Windows.Forms.TextBox WhatHappened;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.TextBox Username;
        private System.Windows.Forms.RadioButton RestartEngine;
        private System.Windows.Forms.RadioButton RestartEngineSafe;
        private System.Windows.Forms.RadioButton DoNotRestart;
        private System.Windows.Forms.Button ShowStack;
        private System.Windows.Forms.Button ShowLog;
        private System.Windows.Forms.Button ShowScripts;
        private System.Windows.Forms.Panel ShowFilesPanel;
    private System.Windows.Forms.CheckBox mAcceptCheckBox;
    private System.Windows.Forms.LinkLabel linkLabel1;
  }
}

