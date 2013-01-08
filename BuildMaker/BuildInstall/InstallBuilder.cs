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
    public void Run()
    {
      string cZeroSource = Environment.ExpandEnvironmentVariables("%ZERO_SOURCE%");
      string cBuildOutput = Path.Combine(cZeroSource, "Build");

      //Print a nice message signifying the start of the install build.
      Console.WriteLine("Building the latest Zero Engine installer...");

      //Generate the Zero Engine installer using Inno Setup.
      ProcessStartInfo innoSetupInfo = new ProcessStartInfo();
      innoSetupInfo.Arguments = Path.Combine(cBuildOutput, "ZeroEngineInstall.iss");
      innoSetupInfo.FileName = @"C:\Program Files (x86)\Inno Setup 5\iscc.exe";
      Process innoSetup;
      try
      {
        innoSetup = Process.Start(innoSetupInfo);
      }
      catch (Win32Exception win32)
      {
        Console.WriteLine("Inno Setup not found. Please install it and rerun " +
                          "the build install maker.");
        Console.WriteLine("Press any key to continue...");
        Console.ReadKey();
        return;
      }
      innoSetup.WaitForExit();

      //Get the build number.
      ProcessStartInfo revisionNumberInfo = new ProcessStartInfo();
      revisionNumberInfo.Arguments = "tip -q";
      revisionNumberInfo.FileName = "hg";
      revisionNumberInfo.WorkingDirectory = cZeroSource;
      revisionNumberInfo.RedirectStandardOutput = true;
      revisionNumberInfo.UseShellExecute = false;
      Process revisionNumber = Process.Start(revisionNumberInfo);
      string revNum = revisionNumber.StandardOutput.ReadToEnd();
      string buildNumber = (revNum.Split(':'))[0];
      revisionNumber.WaitForExit();

      //Get the date.
      DateTime theDate = DateTime.Now;
      string date = theDate.ToString(".yyyy.MM.dd.");

      //Rename the install executable.
      string newFileName = "ZeroEngineSetup" + date + buildNumber + ".exe";
      File.Copy(Path.Combine(cBuildOutput, "Output", "ZeroEngineSetup.exe"),
                Path.Combine(cBuildOutput, "Output", newFileName), true);
      File.Delete(Path.Combine(cBuildOutput, "Output", "ZeroEngineSetup.exe"));

      //Open the folder
      Process.Start("explorer.exe", 
                    "/select, " + Path.Combine(cBuildOutput, "Output", newFileName));
    }
  }
}
