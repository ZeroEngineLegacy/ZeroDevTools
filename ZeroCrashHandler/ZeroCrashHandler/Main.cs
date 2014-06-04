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
using System.DirectoryServices.AccountManagement;
using ICSharpCode.SharpZipLib.Zip;
using System.Collections.Specialized;

namespace ZeroCrashHandler
{
	public partial class Main : Form
	{
		// How would we like to report bugs?
		BugReportMode Mode = BugReportMode.Php;

		// Fog bugz settings data
		const String URL = "---";
		const String FBUsername = "---";
		const String FBPassword = "---";
		const String Title = "Crash";
		const String Project = "---";
		const String Priority = "1 - Must Fix";

		// Email settings data
		const String EUsername = "---";
		const String EPassword = "---";
		const String From = "---";
		const String To = "---";
		const String Host = "---";
		const int Port = 587;
		const bool SSL = true;

		// Php settings data
		const String PhpRedirectUrl = "https://www.digipen.edu/zerocrash/crash.php";
		const String PhpFileId = "userfile";
		const String PhpEmailId = "email";
		const String PhpBodyId = "body";
		const String PhpVersionId = "version";

		// Was the file sent?
		bool WasSent = false;

		// Additional text to be attached to the email
		StringBuilder AdditionalText = new StringBuilder();

		// The version number of the engine
		String Version = String.Empty;

		// Store all the files that need to be uploaded (dump files and whatnot)
		List<String> Files;

		HashSet<String> UserInfoValues = new HashSet<String>();

		// The bug reporting mechanism
		enum BugReportMode
		{
			FogBugz,
			Email,
			Php,
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
		private void Close_Click(object sender, EventArgs e)
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
			//DoEngineRestart();

			// Close out of this application
			//CloseCrashHandler();

			Send_Click(sender, e);
		}

		private void AddUserInfo(StringBuilder builder, String category, String value)
		{
			if (value == null || value.Trim() == String.Empty)
			{
				return;
			}

			if (UserInfoValues.Contains(value) == false)
			{
				UserInfoValues.Add(value);

				builder.AppendLine(category + ": " + value);
			}
		}

		private void Send_Click(object sender, EventArgs e)
		{
			// An error was sent
			WasSent = true;

			// Restart the engine if specified
			DoEngineRestart();

			// Hide the form (it will close when its done sending)
			this.Hide();

			// Assume they did not fill it in
			String whatHappened = String.Empty;

			StringBuilder userInfo = new StringBuilder();

			userInfo.AppendLine("ProvidedEmail: " + Email.Text);

			try { AddUserInfo(userInfo, "RegisteredOrganization", (String)Microsoft.Win32.Registry.GetValue(@"HKEY_LOCAL_MACHINE\Software\Microsoft\Windows NT\CurrentVersion", "RegisteredOrganization", "")); } catch {}
			try { AddUserInfo(userInfo, "RegisteredOwner", (String)Microsoft.Win32.Registry.GetValue(@"HKEY_LOCAL_MACHINE\Software\Microsoft\Windows NT\CurrentVersion", "RegisteredOwner", "")); } catch {}
			try { AddUserInfo(userInfo, "DisplayName", UserPrincipal.Current.DisplayName); } catch {}
			try { AddUserInfo(userInfo, "DistinguishedName", UserPrincipal.Current.DistinguishedName); } catch {}
			try { AddUserInfo(userInfo, "DomainEmailAddress", UserPrincipal.Current.EmailAddress); } catch {}
			try { AddUserInfo(userInfo, "EmployeeId", UserPrincipal.Current.EmployeeId); } catch {}
			try { AddUserInfo(userInfo, "GivenName", UserPrincipal.Current.GivenName); } catch {}
			try { AddUserInfo(userInfo, "UserPrincipalName", UserPrincipal.Current.UserPrincipalName); } catch {}
			try { AddUserInfo(userInfo, "HostName", Dns.GetHostName()); } catch {}
			try { AddUserInfo(userInfo, "MachineName", Environment.MachineName); } catch {}
			try { AddUserInfo(userInfo, "UserName", Environment.UserName); } catch {}
			try { AddUserInfo(userInfo, "UserDomainName", Environment.UserDomainName); } catch {}

			// If they actually filled in the "what happened" field...
			whatHappened = userInfo.ToString() + "\r\n\r\n" + WhatHappened.Text;

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
			else if (Mode == BugReportMode.Php)
			{
				// Create the bug report
				CreatePhpReport(whatHappened, Email.Text);
			}
		}

		// Create the email message
		private void SendEmail(String whatHappened, String email, bool doAttachments)
		{
			// Create a new email client
			SmtpClient emailClient = new SmtpClient();

			// Set the host and port to be gmail
			emailClient.Host = Host;
			emailClient.Port = Port;

			// Enable SSL since gmail requires it
			emailClient.EnableSsl = SSL;

			// Set the credentials
			emailClient.Credentials = new NetworkCredential(EUsername, EPassword);


			// Create a message that will be sent
			MailMessage message = new MailMessage();

			// Setup the message
			message.From = new MailAddress(From);
			message.To.Add(To);
			message.Subject = "Zero Engine Crash - [" + Version + "]";
			message.Body = whatHappened;

			// Should we attach anything to the email?
			if (doAttachments)
			{
				// Loop through all the files
				for (int i = 0; i < Files.Count; i++)
				{
					try
					{
						// Add the attachment to the message
						message.Attachments.Add(new Attachment(Files[i], "application/octet-stream"));
					}
					catch
					{
					}
				}
			}

			// Send the message
			emailClient.Send(message);
		}

		private void CreatePhpReport(String whatHappened, String email)
		{
			var zipFile = CreateZipFile(Files);

			var collection = new NameValueCollection();
			collection.Add(PhpBodyId, whatHappened);
			collection.Add(PhpEmailId, email);
			collection.Add(PhpVersionId, Version);

			HttpUploadFile(PhpRedirectUrl, zipFile, PhpFileId, "application/octet-stream", collection);
		}

		private void CreateEmailReport(String whatHappened, String email)
		{
			try
			{
				SendEmail(whatHappened, email, true);
			}
			catch
			{
				try
				{
					SendEmail(whatHappened, email, false);
				}
				catch
				{
				}
			}
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

		private void Main_FormClosing(object sender, FormClosingEventArgs e)
		{
			if (WasSent == false)
			{
				e.Cancel = true;
			}
		}

		public static String CreateZipFile(List<String> filenames)
		{
			var zipFilePath = Path.GetTempFileName() + ".zip";
			var zipFile = File.Create(zipFilePath);
			// 'using' statements guarantee the stream is closed properly which is a big source
			// of problems otherwise.  Its exception safe as well which is great.
			using (ZipOutputStream s = new ZipOutputStream(zipFile))
			{

				s.SetLevel(9); // 0 - store only to 9 - means best compression

				byte[] buffer = new byte[4096];

				foreach (string file in filenames)
				{

					// Using GetFileName makes the result compatible with XP
					// as the resulting path is not absolute.
					ZipEntry entry = new ZipEntry(Path.GetFileName(file));

					// Setup the entry data as required.

					// Crc and size are handled by the library for seakable streams
					// so no need to do them here.

					// Could also use the last write time or similar for the file.
					entry.DateTime = DateTime.Now;
					s.PutNextEntry(entry);

					using (FileStream fs = File.OpenRead(file))
					{

						// Using a fixed size buffer here makes no noticeable difference for output
						// but keeps a lid on memory usage.
						int sourceBytes;
						do
						{
							sourceBytes = fs.Read(buffer, 0, buffer.Length);
							s.Write(buffer, 0, sourceBytes);
						} while (sourceBytes > 0);
					}
				}

				// Finish/Close arent needed strictly as the using statement does this automatically

				// Finish is important to ensure trailing information for a Zip file is appended.  Without this
				// the created file would be invalid.
				s.Finish();

				// Close is important to wrap things up and unlock the file.
				s.Close();
			}

			return zipFilePath;
		}

		public static Uri GetRedirectUrl(string redirectUrl)
		{
			var request = (HttpWebRequest)WebRequest.Create(redirectUrl);
			var response = request.GetResponse();
			return response.ResponseUri;
		}

		public static void HttpUploadFile(string redirectUrl, string file, string paramName, string contentType, NameValueCollection nvc)
		{
			var url = GetRedirectUrl(redirectUrl);

			string boundary = "---------------------------" + DateTime.Now.Ticks.ToString("x");
			byte[] boundarybytes = System.Text.Encoding.ASCII.GetBytes("\r\n--" + boundary + "\r\n");

			HttpWebRequest wr = (HttpWebRequest)WebRequest.Create(url);
			wr.ContentType = "multipart/form-data; boundary=" + boundary;
			wr.Method = "POST";
			wr.KeepAlive = true;
			wr.Credentials = System.Net.CredentialCache.DefaultCredentials;

			Stream rs = wr.GetRequestStream();

			string formdataTemplate = "Content-Disposition: form-data; name=\"{0}\"\r\n\r\n{1}";
			foreach (string key in nvc.Keys)
			{
				rs.Write(boundarybytes, 0, boundarybytes.Length);
				string formitem = string.Format(formdataTemplate, key, nvc[key]);
				byte[] formitembytes = System.Text.Encoding.UTF8.GetBytes(formitem);
				rs.Write(formitembytes, 0, formitembytes.Length);
			}
			rs.Write(boundarybytes, 0, boundarybytes.Length);

			string headerTemplate = "Content-Disposition: form-data; name=\"{0}\"; filename=\"{1}\"\r\nContent-Type: {2}\r\n\r\n";
			string header = string.Format(headerTemplate, paramName, file, contentType);
			byte[] headerbytes = System.Text.Encoding.UTF8.GetBytes(header);
			rs.Write(headerbytes, 0, headerbytes.Length);

			FileStream fileStream = new FileStream(file, FileMode.Open, FileAccess.Read);
			byte[] buffer = new byte[4096];
			int bytesRead = 0;
			while ((bytesRead = fileStream.Read(buffer, 0, buffer.Length)) != 0)
			{
				rs.Write(buffer, 0, bytesRead);
			}
			fileStream.Close();

			byte[] trailer = System.Text.Encoding.ASCII.GetBytes("\r\n--" + boundary + "--\r\n");
			rs.Write(trailer, 0, trailer.Length);
			rs.Close();

			WebResponse wresp = null;
			try
			{
				wresp = wr.GetResponse();
				Stream stream2 = wresp.GetResponseStream();
				StreamReader reader2 = new StreamReader(stream2);
				var result = reader2.ReadToEnd();
				result = result;
			}
			catch (Exception ex)
			{
				if (wresp != null)
				{
					wresp.Close();
					wresp = null;
				}
			}
			finally
			{
				wr = null;
			}
		}
	}
}
