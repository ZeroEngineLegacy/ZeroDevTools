///////////////////////////////////////////////////////////////////////////////
///
/// \file InstallBuilder.cs
/// The entirety of the InstallBuilder tool.
/// 
/// Authors: Benjamin Strukus
/// Copyright 2010-2013, DigiPen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Diagnostics;
using System.IO;
using System.ComponentModel;
using SharedBuildHelpers;

namespace BuildMaker
{
  class InstallBuilder
  {
    public void OutputBuildInfo(String buildFilePath, String buildVersion, String metaFileName, String fileName)
    {
      StringBuilder builder = new StringBuilder();
      builder.Append("{");
      builder.Append(String.Format("\"BuildVersion\": \"{0}\",", buildVersion));
      builder.Append(String.Format("\"InstallerName\": \"{0}\",", fileName));
      builder.Append(String.Format("\"MetaFileName\": \"{0}\"", metaFileName));
      builder.Append("}");

      String fileData = builder.ToString();
      File.WriteAllText(buildFilePath, fileData);
    }

    public BuildMeta GetBuildInfo(String sourcePath, String outputPath, String zeroEditorOutputSuffix, int zeroEngineTimeoutSeconds)
    {
      BuildMeta meta = new BuildMeta();
      meta.SetupEditor();

      string exePath = Path.Combine(outputPath, zeroEditorOutputSuffix, "ZeroEditor.exe");
      meta.GetBuildInfoFromExe(exePath, sourcePath, zeroEngineTimeoutSeconds);
      return meta;
    }

    public String Run(String sourcePath, String outputPath, String installerPrefix, String zeroEditorOutputSuffix, int zeroEngineTimeoutSeconds)
    {
      // Get the build's meta information from the current exe
      var buildMeta = this.GetBuildInfo(sourcePath, outputPath, zeroEditorOutputSuffix, zeroEngineTimeoutSeconds);
      
      String cBuildOutput = Path.Combine(sourcePath, "Build");
      String buildId = buildMeta.GetBuildId();
      String newFileNameWithoutExtension = buildMeta.GetBuildNameWithoutExtension(installerPrefix);
      String newFileName = newFileNameWithoutExtension + ".exe";

      //Print a nice message signifying the start of the install build.
      Console.WriteLine("Building the latest Zero Engine installer...");

      //Generate the Zero Engine installer using Inno Setup.
      ProcessStartInfo innoSetupInfo = new ProcessStartInfo();
      StringBuilder definesBuilder = new StringBuilder();
      definesBuilder.Append(" /DMyAppVersion=\"{0}\"");
      definesBuilder.Append(" /DZeroEditorOutputSuffix=\"{1}\"");
      definesBuilder.Append(" /DZeroSource=\"{2}\"");
      definesBuilder.Append(" /DZeroOutput=\"{3}\"");
      String defines = String.Format(definesBuilder.ToString(), buildId, zeroEditorOutputSuffix, sourcePath, outputPath);

      Console.WriteLine(defines);

      innoSetupInfo.Arguments = Path.Combine(cBuildOutput, "ZeroEngineInstall.iss") + defines;
      innoSetupInfo.FileName = @"C:\Program Files (x86)\Inno Setup 5\iscc.exe";
      innoSetupInfo.RedirectStandardError = true;
      innoSetupInfo.RedirectStandardOutput = true;
      innoSetupInfo.UseShellExecute = false;
      
      Process innoSetup;
      try
      {
        innoSetup = Process.Start(innoSetupInfo);
      }
      catch (Win32Exception /*win32*/)
      {
        Console.WriteLine("Inno Setup not found. Please install it and rerun " +
                          "the build install maker.");
        Console.WriteLine("Press any key to continue...");
        Console.ReadKey();
        return null;
      
      }
      innoSetup.WaitForExit();

      string errOut = innoSetup.StandardError.ReadToEnd();

      if (errOut.Length != 0)
      {
        Console.WriteLine(errOut);
        System.Environment.Exit(-1);
      }

      //Rename the install executable.
      File.Copy(Path.Combine(cBuildOutput, "Output", "ZeroEngineSetup.exe"),
                Path.Combine(cBuildOutput, "Output", newFileName), true);
      File.Delete(Path.Combine(cBuildOutput, "Output", "ZeroEngineSetup.exe"));

      String buildFilePath = Path.Combine(cBuildOutput, "Output", "BuildInfo.data");
      String metaFileName = newFileNameWithoutExtension + ".meta";
      var metaFilePath = Path.Combine(cBuildOutput, "Output", metaFileName);
      this.OutputBuildInfo(buildFilePath, buildId, metaFileName, newFileName);

      
      buildMeta.SaveMeta(metaFilePath);
      //Open the folder
      //Process.Start("explorer.exe", 
      //              "/select, " + Path.Combine(cBuildOutput, "Output", newFileName));
      return Path.Combine(cBuildOutput, "Output");
    }
  }
}
