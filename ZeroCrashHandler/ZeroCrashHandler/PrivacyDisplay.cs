using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace ZeroCrashHandler
{
  public partial class PrivacyDisplay : Form
  {
    public PrivacyDisplay()
    {
      InitializeComponent();
    }

    private void button1_Click(object sender, EventArgs e)
    {
      this.Close();
    }

    private void Privacy_Load(object sender, EventArgs e)
    {
      var dataList = new List<Data>();
      dataList.Add(new Data() { Name = "Username", Value = "We collect your zerohub username so we can contact you if we need more details." });
      dataList.Add(new Data() { Name = "CrashDump", Value = "The last known state of your application. We use this to attempt to identify and fix your crash." });
      dataList.Add(new Data() { Name = "LogFile", Value = "The log of anything printed out during application run. We use this to attempt to identify and fix your crash." });
      dataList.Add(new Data() { Name = "ScriptFiles", Value = "Any script files that may be relevant to the crash. We use this to attempt to identify and fix your crash." });
      this.dataGridView1.DataSource = dataList;
      this.dataGridView1.DefaultCellStyle.WrapMode = DataGridViewTriState.True;
      this.dataGridView1.Columns[1].AutoSizeMode = DataGridViewAutoSizeColumnMode.Fill;
    }

    private void What_TextChanged(object sender, EventArgs e)
    {

    }

    private void richTextBox1_LinkClicked(object sender, LinkClickedEventArgs e)
    {
      ProcessStartInfo info = new ProcessStartInfo();
      info.UseShellExecute = true;
      info.FileName = e.LinkText;
      Process.Start(info);
    }
  }

  class Data
  {
    public String Name { get; set; }
    public String Value { get; set; }
  }

}
