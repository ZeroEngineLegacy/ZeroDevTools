//------------------------------------------------------------------------------
// <copyright file="AttachToZero.cs" company="Company">
//     Copyright (c) Company.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

using System;
using System.ComponentModel.Design;
using System.Globalization;
using Microsoft.VisualStudio;
using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.Shell.Interop;
using Microsoft.VisualStudio.OLE.Interop;
using EnvDTE80;
using EnvDTE;
using Microsoft.VisualStudio.CommandBars;
using System.IO;
using System.Runtime.InteropServices;

namespace ZeroZilchPlugins
{
    /// <summary>
    /// Command handler
    /// </summary>
    internal sealed class AttachToZero : IVsShellPropertyEvents
    {
        /// <summary>
        /// Command ID.
        /// </summary>
        public const int CommandId = 0x0100;

        /// <summary>
        /// Command menu group (command set GUID).
        /// </summary>
        public static readonly Guid CommandSet = new Guid("4e252c94-509b-4353-b10d-477f4b64508e");

        /// <summary>
        /// VS Package that provides this command, not null.
        /// </summary>
        private readonly Package package;

        /// <summary>
        /// Used to enable the plugin by default.
        /// </summary>
        private uint EventSinkCookie;

        /// <summary>
        /// When a build finishes, should we attach to Zero?
        /// </summary>
        private bool AttachOnFinishBuild;

        /// <summary>
        /// Initializes a new instance of the <see cref="AttachToZero"/> class.
        /// Adds our command handlers for menu (commands must exist in the command table file)
        /// </summary>
        /// <param name="package">Owner package, not null.</param>
        private AttachToZero(Package package)
        {
            if (package == null)
            {
                throw new ArgumentNullException("package");
            }

            this.package = package;

            OleMenuCommandService commandService = this.ServiceProvider.GetService(typeof(IMenuCommandService)) as OleMenuCommandService;
            if (commandService != null)
            {
                var menuCommandID = new CommandID(CommandSet, CommandId);
                var menuItem = new OleMenuCommand(this.MenuItemCallback, menuCommandID);
                menuItem.BeforeQueryStatus += (sender, e) =>
                {
                    this.AnythingChangedUpdate();

                    var enabled = true;
                    var dte = this.GetDTE();

                    // We must be in design mode to see
                    enabled &= (dte.Debugger.CurrentMode == dbgDebugMode.dbgDesignMode);

                    // Only show the button if this is a Zero plugin solution
                    enabled &= this.IsZeroPluginSolution();

                    menuItem.Enabled = enabled;
                };
                commandService.AddCommand(menuItem);
            }

            // Set an event listener for shell property changes
            var shellService = this.ServiceProvider.GetService(typeof(SVsShell)) as IVsShell;
            if (shellService != null)
            {
                ErrorHandler.ThrowOnFailure(shellService.
                  AdviseShellPropertyChanges(this, out this.EventSinkCookie));
            }
        }

        /// <summary>
        /// Gets the instance of the command.
        /// </summary>
        public static AttachToZero Instance
        {
            get;
            private set;
        }

        /// <summary>
        /// Gets the service provider from the owner package.
        /// </summary>
        private System.IServiceProvider ServiceProvider
        {
            get
            {
                return this.package;
            }
        }

        /// <summary>
        /// Initializes the singleton instance of the command.
        /// </summary>
        /// <param name="package">Owner package, not null.</param>
        public static void Initialize(Package package)
        {
            Instance = new AttachToZero(package);
        }

        public bool FindZeroItem(ProjectItem item)
        {
            if (item.Name == "Zero" || Path.GetFileNameWithoutExtension(item.Name) == "ZeroEngine")
                return true;

            try
            {
                foreach (ProjectItem subItem in item.ProjectItems)
                {
                    this.FindZeroItem(item);
                }
            }
            catch
            {
            }
            return false;
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
                        // See if there is a filter named Zero or a ZeroEngine file
                        if (this.FindZeroItem(item))
                            return true;
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
                return this.ServiceProvider.GetService(typeof(DTE)) as DTE;
            }
            catch (COMException)
            {
                this.MessageBox("Visual Studio was not found", OLEMSGICON.OLEMSGICON_CRITICAL);
                return null;
            }
        }

        /// <summary>
        /// This function is the callback used to execute the command when the menu item is clicked.
        /// See the constructor to see how the menu item is associated with this function using
        /// OleMenuCommandService service and MenuCommand class.
        /// </summary>
        /// <param name="sender">Event sender.</param>
        /// <param name="e">Event args.</param>
        private void MenuItemCallback(object sender, EventArgs e)
        {
            var dte = this.GetDTE();

            var buildState = dte.Solution.SolutionBuild.BuildState;

            if (buildState == vsBuildState.vsBuildStateInProgress)
            {
                this.AttachOnFinishBuild = true;
                return;
            }
            
            // If we haven't tried building, or any projects failed to build...
            var needsRebuild = buildState == vsBuildState.vsBuildStateNotStarted || buildState == vsBuildState.vsBuildStateDone && dte.Solution.SolutionBuild.LastBuildInfo != 0;
            if (needsRebuild)
            {
                dte.ExecuteCommand("Build.BuildSolution");
                this.AttachOnFinishBuild = true;
                return;
            }
            else
            {
                this.AttemptAttach();
                return;
            }
        }

        private void AnythingChangedUpdate()
        {
            var dte = this.GetDTE();
            if (this.AttachOnFinishBuild)
            {
                if (dte.Solution.SolutionBuild.BuildState == vsBuildState.vsBuildStateDone)
                {
                    this.AttachOnFinishBuild = false;

                    if (dte.Solution.SolutionBuild.LastBuildInfo == 0)
                        this.AttemptAttach();
                }
            }
        }

        private void AttemptAttach()
        {
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
            var attached = possiblyAttached && dte.Debugger.CurrentMode != dbgDebugMode.dbgDesignMode && dte.Debugger.DebuggedProcesses.Count > 0;

            if (!attached)
            {
                this.MessageBox("Unable to attach to Zero. Make sure the ZeroEdtior.exe process is running.", OLEMSGICON.OLEMSGICON_WARNING);
            }
        }

        public void MessageBox(String text, OLEMSGICON icon = OLEMSGICON.OLEMSGICON_INFO)
        {
            IVsUIShell uiShell = (IVsUIShell)this.ServiceProvider.GetService(typeof(SVsUIShell));
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
                    icon,
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
                    var dte = this.ServiceProvider.GetService(typeof(DTE)) as DTE2;
                    var cbs = ((CommandBars)dte.CommandBars);
                    CommandBar cb = cbs["Zero Engine"];
                    cb.Visible = true;

                    // Unsubscribe from events
                    var shellService = this.ServiceProvider.GetService(typeof(SVsShell)) as IVsShell;
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
