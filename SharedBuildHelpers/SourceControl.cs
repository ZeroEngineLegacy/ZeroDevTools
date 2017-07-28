using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;

namespace SharedBuildHelpers
{
  public class Mercurial
  {
    public static bool Exists(String sourcePath)
    {
      ProcessStartInfo processInfo = new ProcessStartInfo();
      processInfo.FileName = "hg";
      processInfo.Arguments = "status";
      processInfo.WorkingDirectory = sourcePath;
      processInfo.RedirectStandardError = true;
      processInfo.UseShellExecute = false;
      Process process = Process.Start(processInfo);
      String result = process.StandardError.ReadToEnd();
      process.WaitForExit();
      return result.Length == 0;
    }

    public static String GetRevisionInfo(String sourcePath)
    {
      // Build up a string to get as much information from mercurial as
      // possible for old versions (no major/minor/etc... version numbers)
      var builder = new StringBuilder();
      builder.Append("log -q -r \"max(parents())\" ");
      builder.Append("--template \"RevisionId {rev}\\n");
      builder.Append("BuildId \\\"0.0.0.{rev}\\\"\\n");
      builder.Append("ChangeSet \\\"{node}\\\"\\n");
      builder.Append("ShortChangeSet \\\"{node|short}\\\"\\n");
      builder.Append("ChangeSetDate \\\"{date|shortdate}\\\"\"");

      ProcessStartInfo processInfo = new ProcessStartInfo();
      processInfo.FileName = "hg";
      processInfo.Arguments = builder.ToString();
      processInfo.WorkingDirectory = sourcePath;
      processInfo.RedirectStandardOutput = true;
      processInfo.UseShellExecute = false;
      Process process = Process.Start(processInfo);
      String result = process.StandardOutput.ReadToEnd();
      process.WaitForExit();
      return result;
    }
  }

  public class Git
  {
    public static bool Exists(String sourcePath)
    {
      ProcessStartInfo processInfo = new ProcessStartInfo();
      processInfo.FileName = "git";
      processInfo.Arguments = "status";
      processInfo.WorkingDirectory = sourcePath;
      processInfo.RedirectStandardError = true;
      processInfo.UseShellExecute = false;
      Process process = Process.Start(processInfo);
      String result = process.StandardError.ReadToEnd();
      process.WaitForExit();
      return result.Length == 0;
    }

    public static String GetRevisionInfo(String sourcePath)
    {
      // Build up a string to get as much information from git as
      // possible for old versions (no major/minor/etc... version numbers)
      var builder = new StringBuilder();
      builder.Append("log -1 --abbrev=12 --date=format:'%Y-%m-%d' ");
      builder.Append("--pretty=\"format:");
      builder.Append("RevisionId " + Git.GetRevisionId(sourcePath));
      builder.Append("ZeroShortChangeSet %h%n");
      builder.Append("ZeroChangeSet %H%n");
      builder.Append("ZeroChangSetDate %ad%n");

      ProcessStartInfo processInfo = new ProcessStartInfo();
      processInfo.FileName = "git";
      processInfo.Arguments = builder.ToString();
      processInfo.WorkingDirectory = sourcePath;
      processInfo.RedirectStandardOutput = true;
      processInfo.UseShellExecute = false;
      Process process = Process.Start(processInfo);
      String result = process.StandardOutput.ReadToEnd();
      process.WaitForExit();
      return result;
    }

    public static String GetRevisionId(String sourcePath)
    {
      ProcessStartInfo processInfo = new ProcessStartInfo();
      processInfo.FileName = "git";
      processInfo.Arguments = "rev-list --count HEAD";
      processInfo.WorkingDirectory = sourcePath;
      processInfo.RedirectStandardOutput = true;
      processInfo.UseShellExecute = false;
      Process process = Process.Start(processInfo);
      String result = process.StandardOutput.ReadToEnd();
      process.WaitForExit();
      return result;
    }
  }
}
