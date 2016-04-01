using System;
using System.Diagnostics;
using System.Globalization;
using System.Runtime.InteropServices;
using System.ComponentModel.Design;
using Microsoft.Win32;
using Microsoft.VisualStudio;
using Microsoft.VisualStudio.Shell.Interop;
using Microsoft.VisualStudio.OLE.Interop;
using Microsoft.VisualStudio.Shell;
using EnvDTE80;
using EnvDTE;
using System.IO;
using Microsoft.VisualStudio.CommandBars;
using System.Collections.Generic;

namespace DigiPenInstituteofTechnology.ZeroZilchPlugins
{
	/// <summary>
	/// This is the class that implements the package exposed by this assembly.
	///
	/// The minimum requirement for a class to be considered a valid package for Visual Studio
	/// is to implement the IVsPackage interface and register itself with the shell.
	/// This package uses the helper classes defined inside the Managed Package Framework (MPF)
	/// to do it: it derives from the Package class that provides the implementation of the 
	/// IVsPackage interface and uses the registration attributes defined in the framework to 
	/// register itself and its components with the shell.
	/// </summary>
	// This attribute tells the PkgDef creation utility (CreatePkgDef.exe) that this class is
	// a package.
	[PackageRegistration(UseManagedResourcesOnly = true)]
	// This attribute is used to register the informations needed to show the this package
	// in the Help/About dialog of Visual Studio.
	[InstalledProductRegistration("#110", "#112", "1.0", IconResourceID = 400)]
	// This attribute is needed to let the shell know that this package exposes some menus.
	[ProvideMenuResource("Menus.ctmenu", 1)]
	[Guid(GuidList.guidZeroZilchPluginsPkgString)]
	[ProvideAutoLoad("ADFC4E64-0397-11D1-9F4E-00A0C911004F")]
	public sealed class ZeroZilchPluginsPackage : Package, IVsShellPropertyEvents
	{
		private uint EventSinkCookie;

		/// <summary>
		/// Default constructor of the package.
		/// Inside this method you can place any initialization code that does not require 
		/// any Visual Studio service because at this point the package object is created but 
		/// not sited yet inside Visual Studio environment. The place to do all the other 
		/// initialization is the Initialize method.
		/// </summary>
		public ZeroZilchPluginsPackage()
		{
			Trace.WriteLine(string.Format(CultureInfo.CurrentCulture, "Entering constructor for: {0}", this.ToString()));
		}

		/// <summary>
		/// Initialization of the package; this method is called right after the package is sited, so this is the place
		/// where you can put all the initilaization code that rely on services provided by VisualStudio.
		/// </summary>
		protected override void Initialize()
		{
			Trace.WriteLine(string.Format(CultureInfo.CurrentCulture, "Entering Initialize() of: {0}", this.ToString()));
			base.Initialize();

			// Add our command handlers for menu (commands must exist in the .vsct file)
			OleMenuCommandService mcs = GetService(typeof(IMenuCommandService)) as OleMenuCommandService;
			if (null != mcs)
			{
				// Create the command for the menu item.
				CommandID menuCommandID = new CommandID(GuidList.guidZeroZilchPluginsCmdSet, (int)PkgCmdIDList.cmdidAttachToZero);
				OleMenuCommand menuItem = new OleMenuCommand(MenuItemCallback, menuCommandID);
				menuItem.BeforeQueryStatus += (sender, e) =>
				{
					var enabled = true;
					var dte = this.GetDTE();

					// We must be in design mode to see
					enabled &= (dte.Debugger.CurrentMode == dbgDebugMode.dbgDesignMode);

					// Only show the button if this is a Zero plugin solution
					enabled &= this.IsZeroPluginSolution();

					menuItem.Enabled = enabled;
				};
				mcs.AddCommand(menuItem);
			}

			// Set an event listener for shell property changes
			var shellService = GetService(typeof(SVsShell)) as IVsShell;
			if (shellService != null)
			{
				ErrorHandler.ThrowOnFailure(shellService.
				  AdviseShellPropertyChanges(this, out this.EventSinkCookie));
			}
		}

		public Boolean IsZeroPluginSolution()
		{
			// I would love to cache this information instead of querying it every time
			// But it appears solutions get delay loaded, so this callback gets
			// called literally in the middle of files being loaded in
			var dte = this.GetDTE();
			if (dte.Solution == null)
				return false;

			try
			{
				// The solution must contain a ZeroEngine file
				foreach (Project project in dte.Solution.Projects)
				{
					foreach (ProjectItem item in project.ProjectItems)
					{
						if (Path.GetFileNameWithoutExtension(item.Name) == "ZeroEngine")
						{
							return true;
						}
					}
				}
			}
			catch
			{
			}
			return false;
		}

		private String GetZeroProjectName(String projectDirectory)
		{
			String projectDirectoryName = Path.GetFileName(projectDirectory);

			String possibleProjectName = null;

			// There might be multiple .zeroproj files here, so the best one is most likely going
			// to be named the same as the project directory name
			foreach (var file in Directory.EnumerateFiles(projectDirectory))
			{
				if (Path.GetExtension(file) == ".zeroproj")
				{
					var name = Path.GetFileNameWithoutExtension(file);
					if (name == projectDirectoryName)
						return name;
					possibleProjectName = name;
				}
			}

			return possibleProjectName;
		}

		DTE GetDTE()
		{
			try
			{
				return Package.GetGlobalService(typeof(DTE)) as DTE;
			}
			catch (COMException)
			{
				this.MessageBox("Visual Studio was not found");
				return null;
			}
		}

		/// <summary>
		/// This function is the callback used to execute a command when the a menu item is clicked.
		/// See the Initialize method to see how the menu item is associated to this function using
		/// the OleMenuCommandService service and the MenuCommand class.
		/// </summary>
		private void MenuItemCallback(object sender, EventArgs e)
		{
			// Reference Visual Studio core
			var dte = this.GetDTE();
			EnvDTE.Process possibleZeroProcess = null;
			EnvDTE.Process betterZeroProcess = null;
			EnvDTE.Process bestZeroProcess = null;

			Processes processes = dte.Debugger.LocalProcesses;
			foreach (EnvDTE.Process vsProcess in dte.Debugger.LocalProcesses)
			{
				if (Path.GetFileName(vsProcess.Name) == "ZeroEditor.exe")
				{
					possibleZeroProcess = vsProcess;

					var process = System.Diagnostics.Process.GetProcessById(vsProcess.ProcessID);

					// Within the solution path we should generally be able to find the project name
					// For example: C:\Users\Trevor\Documents\ZeroProjects\Test123\Plugins\MyPlugin\MyPlugin.sln
					// Test123 would be the project name

					var solutionName = dte.Solution.FullName;
					if (String.IsNullOrWhiteSpace(solutionName) == false)
					{
						var pluginDir = Path.GetDirectoryName(solutionName);
						var pluginsDir = Path.GetDirectoryName(pluginDir);
						var projectDir = Path.GetDirectoryName(pluginsDir);

						var projectName = this.GetZeroProjectName(projectDir);

						if (process.MainWindowTitle == "Zero Editor - " + projectName)
						{
							bestZeroProcess = vsProcess;
							break;
						}
						else if (process.MainWindowTitle.EndsWith(projectName))
						{
							betterZeroProcess = vsProcess;
						}
					}
				}
			}

			var attachingZeroProcess = bestZeroProcess;
			if (attachingZeroProcess == null)
				attachingZeroProcess = betterZeroProcess;
			if (attachingZeroProcess == null)
				attachingZeroProcess = possibleZeroProcess;

			var possiblyAttached = false;
			if (attachingZeroProcess != null)
			{
				// Visual Studio may not attach on the first try, so spend up to 5 seconds attempting to connect
				var tryCount = 5;
				while (tryCount > 0)
				{
					try
					{
						attachingZeroProcess.Attach();

						// For some reason, just because we got here does not mean that we actually attached
						possiblyAttached = true;
						break;
					}
					catch
					{
						System.Threading.Thread.Sleep(1000);
					}

					--tryCount;
				}
			}

			// As long as we 'possibly attached' and we're not in design mode then consider us fully attached
			// CurrentProcess appears to always be null on the Debugger, even after Attach...
			var attached = possiblyAttached && dte.Debugger.CurrentMode != dbgDebugMode.dbgDesignMode;

			if (!attached)
			{
				this.MessageBox("Unable to attach to Zero. Make sure the ZeroEdtior.exe process is running.");
			}
		}

		public void MessageBox(String text)
		{
			IVsUIShell uiShell = (IVsUIShell)GetService(typeof(SVsUIShell));
			Guid clsid = Guid.Empty;
			int result;
			Microsoft.VisualStudio.ErrorHandler.ThrowOnFailure(uiShell.ShowMessageBox(
					0,
					ref clsid,
					"ZeroZilchPlugins",
					text,
					string.Empty,
					0,
					OLEMSGBUTTON.OLEMSGBUTTON_OK,
					OLEMSGDEFBUTTON.OLEMSGDEFBUTTON_FIRST,
					OLEMSGICON.OLEMSGICON_INFO,
					0,
					out result));
		}

		public int OnShellPropertyChange(int propid, object propValue)
		{
			// We handle the event if zombie state changes from true to false
			if ((int)__VSSPROPID.VSSPROPID_Zombie == propid)
			{
				if ((bool)propValue == false)
				{
					// Show the commandbar
					var dte = GetService(typeof(DTE)) as DTE2;
					var cbs = ((CommandBars)dte.CommandBars);
					CommandBar cb = cbs["Zero Engine"];
					cb.Visible = true;

					// Unsubscribe from events
					var shellService = GetService(typeof(SVsShell)) as IVsShell;
					if (shellService != null)
					{
						ErrorHandler.ThrowOnFailure(shellService.
						  UnadviseShellPropertyChanges(this.EventSinkCookie));
					}
					this.EventSinkCookie = 0;
				}
			}
			return VSConstants.S_OK;
		}
	}
}
