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

namespace BuildMaker
{
  class InstallBuilder
  {

    public String GetRevisionNumber(String cZeroSource, String branch)
    {
        //Get the build number.
        ProcessStartInfo revisionNumberInfo = new ProcessStartInfo();
        revisionNumberInfo.Arguments = String.Format(@"heads -q -r {0}", branch);
        revisionNumberInfo.FileName = "hg";
        revisionNumberInfo.WorkingDirectory = cZeroSource;
        revisionNumberInfo.RedirectStandardOutput = true;
        revisionNumberInfo.UseShellExecute = false;
        Process revisionNumber = Process.Start(revisionNumberInfo);
        String revNum = revisionNumber.StandardOutput.ReadToEnd();
        String buildNumber = (revNum.Split(':'))[0];
        revisionNumber.WaitForExit();

        return buildNumber;
    }

    public String GetRevisionDate(String cZeroSource, String branch)
    {
      //Get the build number.
      ProcessStartInfo procInfo = new ProcessStartInfo();
      procInfo.Arguments = String.Format("heads -q -r {0} --template \"{{date|shortdate}}\"", branch);
      procInfo.FileName = "hg";
      procInfo.WorkingDirectory = cZeroSource;
      procInfo.RedirectStandardOutput = true;
      procInfo.UseShellExecute = false;
      Process process = Process.Start(procInfo);
      String revNum = process.StandardOutput.ReadToEnd();
      String buildDate = (revNum.Split(':'))[0];
      process.WaitForExit();

      return buildDate.Replace('-', '.');

      return buildDate;
    }

    public void OutputBuildInfo(String buildFilePath, String buildVersion, String fileName)
    {
      StringBuilder builder = new StringBuilder();
      builder.Append("{");
      builder.Append(String.Format("\"BuildVersion\": \"{0}\",", buildVersion));
      builder.Append(String.Format("\"InstallerName\": \"{0}\"", fileName));
      builder.Append("}");

      String fileData = builder.ToString();
      File.WriteAllText(buildFilePath, fileData);
    }

    public String Run(String installerPrefix, String zeroEditorOutputSuffix, String branch)
    {
      String cZeroSource = Environment.ExpandEnvironmentVariables("%ZERO_SOURCE%");
      String cBuildOutput = Path.Combine(cZeroSource, "Build");

      String buildNumber = this.GetRevisionNumber(cZeroSource, branch);

      //Print a nice message signifying the start of the install build.
      Console.WriteLine("Building the latest Zero Engine installer...");

      //Generate the Zero Engine installer using Inno Setup.
      ProcessStartInfo innoSetupInfo = new ProcessStartInfo();
      innoSetupInfo.Arguments = Path.Combine(cBuildOutput, "ZeroEngineInstall.iss") + 
                                String.Format(" /DMyAppVersion=\"{0}\" /DZeroEditorOutputSuffix=\"{1}\"", buildNumber, zeroEditorOutputSuffix);
      innoSetupInfo.FileName = @"C:\Program Files (x86)\Inno Setup 5\iscc.exe";
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
      //String innoError = innoSetup.StandardOutput.ReadToEnd();
      innoSetup.WaitForExit();

      

      //Get the date from the last commit
      String date = "." + GetRevisionDate(cZeroSource, branch) + ".";

      //Rename the install executable.
      String newFileName = installerPrefix + date + buildNumber + ".exe";
      File.Copy(Path.Combine(cBuildOutput, "Output", "ZeroEngineSetup.exe"),
                Path.Combine(cBuildOutput, "Output", newFileName), true);
      File.Delete(Path.Combine(cBuildOutput, "Output", "ZeroEngineSetup.exe"));

      String buildFilePath = Path.Combine(cBuildOutput, "Output", "BuildInfo.data");
      this.OutputBuildInfo(buildFilePath, buildNumber, newFileName);
      //Open the folder
      //Process.Start("explorer.exe", 
      //              "/select, " + Path.Combine(cBuildOutput, "Output", newFileName));
      return Path.Combine(cBuildOutput, "Output");
    }
  }
}
