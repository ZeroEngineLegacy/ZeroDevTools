using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.IO;
using UBR.Products.TimeTrakker.Client.Lib.FogBugz;
using System.Configuration;
using System.Xml;
using System.Security.Permissions;
using Microsoft.Win32;
using System.Net.Mail;
using System.Net;

namespace ZeroCrashHandler
{
    public partial class Main : Form
    {
        // How would we like to report bugs?
        BugReportMode Mode = BugReportMode.Email;

        // Fog bugz settings data
        const String URL = "http://zeroengine0.digipen.edu/api.asp";
        const String FBUsername = "leveldesigner2d";
        const String FBPassword = "letmein";
        const String Title = "Crash";
        const String Project = "Gat250";
        const String Priority = "1 - Must Fix";

        // Email settings data
        const String EUsername = "ZeroEngineTeam";
        const String EPassword = "lotsofforloops";
        const String From = "crash@zeroengine0.digipen.edu";
        const String To = "ZeroEngineTeam@gmail.com";
        const String Host = "smtp.gmail.com";
        const int Port = 587;
        const bool SSL = true;

        // Additional text to be attached to the email
        StringBuilder AdditionalText = new StringBuilder();

        // The version number of the engine
        String Version = String.Empty;

        // Store all the files that need to be uploaded (dump files and whatnot)
        List<String> Files;

        // The bug reporting mechanism
        enum BugReportMode
        {
            FogBugz,
            Email,
        }

        // Main entrypoint into our application
        public Main(String version, List<String> files)
        {
            // The rest of the arguments are all files
            Files = files;

            // Store the engine version (the first argument
            Version = version;

            // Process any text files
            ProcessTextFiles();

            // Initialize all the form components
            InitializeComponent();
        }

        // Load any text files into the "additional text" field
        void ProcessTextFiles()
        {
            // Loop through all the files
            foreach (String fileName in Files)
            {
                // Get the extension from the current file (if it's a text file...)
                if (Path.GetExtension(fileName) == ".txt")
                {
                    // Append a line of text to the message
                    AdditionalText.AppendLine("\r\n\r\n" + "---------------------" + Path.GetFileNameWithoutExtension(fileName) + "---------------------");
                    AdditionalText.Append(File.ReadAllText(fileName));
                }
            }
        }

        // When the form loads...
        private void Main_Load(object sender, EventArgs e)
        {
            // Create the tooltips
            new ToolTip().SetToolTip(this.RestartEngine, "Restarts the engine and loads the last project you had set");
            new ToolTip().SetToolTip(this.RestartEngineSafe, "Restarts the engine, erases your config, and doesn't load anything extra");
            new ToolTip().SetToolTip(this.DoNotRestart, "Doesn't restart the engine at all");

            // Get the email that we might already have saved away
            String email = Application.UserAppDataRegistry.GetValue("Email") as String;

            // If we found a saved away email...
            if (email != null)
            {
                // Set the email field
                Email.Text = email;
            }
            else
            {
                // Set the email field
                Email.Text = Environment.UserName.Replace(" ", "").ToLower() + "@digipen.edu";
            }

            // Get the restart mode
            String restartMode = Application.UserAppDataRegistry.GetValue("RestartMode") as String;

            // Do we have a restart mode?
            if (restartMode != null)
            {
                // Select the restart engine control
                if (restartMode == "RestartEngine")
                {
                    RestartEngine.Select();
                }
                // Select the restart engine safe control
                else if (restartMode == "RestartEngineSafe")
                {
                    RestartEngineSafe.Select();
                }
                // Select the do not restart control
                else if (restartMode == "DoNotRestart")
                {
                    DoNotRestart.Select();
                }
            }
        }

        // When the form is shown...
        private void Main_Shown(object sender, EventArgs e)
        {
            // Select the "What happened" text
            WhatHappened.Focus();
            WhatHappened.SelectionStart = 0;
            WhatHappened.SelectionLength = WhatHappened.Text.Length;
        }

        // Do the functionality for engine restarting (if it's specified)
        private void DoEngineRestart()
        {
            try
            {
                // If we should restart the engine...
                if (RestartEngine.Checked)
                {
                    // Start the process
                    Application.UserAppDataRegistry.SetValue("RestartMode", "RestartEngine");
                    System.Diagnostics.Process.Start("ZeroEditor.exe", @"/crashed");
                }
                // If we should restart the engine in safe mode...
                else if (RestartEngineSafe.Checked)
                {
                    // Start the process with the "safe" flag
                    Application.UserAppDataRegistry.SetValue("RestartMode", "RestartEngineSafe");
                    System.Diagnostics.Process.Start("ZeroEditor.exe", @"/crashed /safe");
                }
                else if (DoNotRestart.Checked)
                {
                    // Save the settings
                    Application.UserAppDataRegistry.SetValue("RestartMode", "DoNotRestart");
                }
            }
            catch
            {
            }
        }

        // Close the crash handler
        private void CloseCrashHandler()
        {
            // Close out of this application
            Application.Exit();
        }

        // If the user clicks "Don't Send"...
        private void DontSend_Click(object sender, EventArgs e)
        {
            //// Make a dialog that makes sure that they want to ignore the bug report
            //DialogResult result = MessageBox.Show("Are you sure?", ":(", MessageBoxButtons.YesNo, MessageBoxIcon.Warning, MessageBoxDefaultButton.Button2);

            //// If the result was indeed yes...
            //if (result == DialogResult.Yes)
            //{
            //    // Close out of this application
            //    CloseCrashHandler();
            //}

            // Restart the engine if specified
            DoEngineRestart();

            // Close out of this application
            CloseCrashHandler();
        }

        private void Send_Click(object sender, EventArgs e)
        {
            // Restart the engine if specified
            DoEngineRestart();

            // Hide the form (it will close when its done sending)
            this.Hide();

            // Assume they did not fill it in
            String whatHappened = String.Empty;

            // If they actually filled in the "what happened" field...
            if (WhatHappened.Text != "Please fill this in!")
            {
                whatHappened = Email.Text + ":\r\n\r\n" + WhatHappened.Text;
            }

            // Add the additional text
            whatHappened += AdditionalText.ToString();

            // Create the bug report
            CreateBugReport(whatHappened, Email.Text);

            // Exit out
            CloseCrashHandler();
        }

        private void CreateBugReport(String whatHappened, String email)
        {
            // Save the email that we're using now for the next time this dialog appears
            Application.UserAppDataRegistry.SetValue("Email", email);

            // Depending on the mode...
            if (Mode == BugReportMode.FogBugz)
            {
                // Create the bug report
                CreateFogBugzReport(whatHappened, Email.Text);
            }
            else if (Mode == BugReportMode.Email)
            {
                // Create the bug report
                CreateEmailReport(whatHappened, Email.Text);
            }
        }

        private void CreateEmailReport(String whatHappened, String email)
        {
            // Create a new email client
            SmtpClient emailClient = new SmtpClient();

            // Create a message that will be sent
            MailMessage message = new MailMessage();

            // Setup the message
            message.From = new MailAddress(From);
            message.To.Add(To);
            message.Subject = "Zero Engine Crash - [" + Version + "]";
            message.Body = whatHappened;

            // Loop through all the files
            for (int i = 0; i < Files.Count; i++)
            {
                // Add the attachment to the message
                message.Attachments.Add(new Attachment(Files[i]));
            }

            // Set the host and port to be gmail
            emailClient.Host = Host;
            emailClient.Port = Port;

            // Enable SSL since gmail requires it
            emailClient.EnableSsl = SSL;

            // Set the credentials
            emailClient.Credentials = new NetworkCredential(EUsername, EPassword);

            // Send the message
            emailClient.Send(message);
        }

        private void CreateFogBugzReport(String whatHappened, String email)
        {
            // Set the URL that we'll connect to (why is this a static and not on the FB object?)
            FBApi.Url = URL;

            // Create a fog bugz api object
            FBApi fogBugz = null;

            try
            {
                // Create the object and give it the username and password that we'll login with
                fogBugz = new FBApi(FBUsername, FBPassword);
            }
            catch
            {
                // If it failed, do the email report then exit out
                CreateEmailReport(whatHappened, email);
                return;
            }

            // Create a dictionary 
            Dictionary<String, String> args = new Dictionary<String, String>();

            // Add arguments to the fog bugz data
            args.Add("sTitle", Title);
            args.Add("sProject", Project);
            args.Add("sPriority", Priority);
            args.Add("sVersion", Version);
            args.Add("sComputer", Environment.MachineName + " - " + Environment.UserName);
            args.Add("sEvent", whatHappened);

            // Select the columns we would like to be uploading
            args.Add("cols", "fOpen,sTitle,sProject,sPriority,sVersion,sComputer,sCustomerEmail,dtOpened");

            // Encoding for binary files
            ASCIIEncoding encoding = new ASCIIEncoding();

            // Create a map of strings to loaded files
            Dictionary<String, Byte[]>[] files = null;

            // If any files were specified to be uploaded (hopefully the dump and other project files)
            if (Files.Count != 0)
            {
                // Create the files dictionary (othrewise it would be left null if we had no files)
                files = new Dictionary<String, Byte[]>[Files.Count];

                // Loop through all the files
                for (int i = 0; i < Files.Count; i++)
                {
                    // Create the settings for each file
                    files[i] = new Dictionary<String, Byte[]>();

                    // Set the name, file name, and the content type of each file
                    files[i]["name"] = encoding.GetBytes("File" + (i + 1).ToString());
                    files[i]["filename"] = encoding.GetBytes(Files[i].Substring(Files[i].LastIndexOf("\\") + 1));
                    files[i]["contenttype"] = encoding.GetBytes(GetMIMEType(Files[i]));

                    // Create a stream and binary reader to read the file
                    FileStream fs = new FileStream(Files[i], FileMode.Open);
                    BinaryReader br = new BinaryReader(fs);

                    // Read in all the data and add it to our dictionary under "data"
                    files[i]["data"] = br.ReadBytes((int)fs.Length);

                    // Close the file stream
                    fs.Close();
                }

                // Add the file count to the list
                args.Add("nFileCount", Files.Count.ToString());
            }

            // We're making a new bug, use that command
            fogBugz.Cmd("new", args, files);
        }

        // Get the MIME type of a given file
        // From http://www.codeproject.com/dotnet/ContentType.asp
        private string GetMIMEType(string filepath)
        {
            RegistryPermission regPerm = new RegistryPermission(RegistryPermissionAccess.Read, "\\\\HKEY_CLASSES_ROOT");
            FileInfo fi = new FileInfo(filepath);
            RegistryKey classesRoot = Registry.ClassesRoot;
            String dotExt = fi.Extension.ToLower();
            RegistryKey typeKey = classesRoot.OpenSubKey("MIME\\Database\\Content Type");

            foreach (String keyname in typeKey.GetSubKeyNames())
            {
                RegistryKey curKey = classesRoot.OpenSubKey("MIME\\Database\\Content Type\\" + keyname);
                if (curKey.GetValue("Extension") != null && curKey.GetValue("Extension").ToString().ToLower() == dotExt)
                    return keyname;
            }
            return String.Empty;
        }
    }
}
