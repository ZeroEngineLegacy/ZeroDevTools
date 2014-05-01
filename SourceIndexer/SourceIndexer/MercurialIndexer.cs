using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace SourceIndexer
{
  class MercurialIndexer
  {
    // Get the revision id of the depo.
    // This is used to know what revision we want to revert each file back to.
    static String GetCurrentRevision(String depoPath)
    {
      ProcessStartInfo info = new ProcessStartInfo();
      info.Arguments = "--cwd " + depoPath + " tip --template {node}";
      info.FileName = "hg.exe";
      info.RedirectStandardOutput = true;
      info.UseShellExecute = false;
      Process process = Process.Start(info);
      String currentRevision = process.StandardOutput.ReadToEnd();
      process.WaitForExit();

      return currentRevision;
    }

    public static String BuildSourceIndex(List<DepoInfo> depos, String pdbPath, String pdbStrPath, bool isSilent)
    {
      // Get the current date and time to put into the pdb.
      String dateString;
      DateTime date = DateTime.Now;
      dateString = date.ToString();

      StringBuilder builder = new StringBuilder();
      builder.AppendLine("SRCSRV: ini ------------------------------------------------");
      builder.AppendLine("VERSION=1");
      builder.AppendLine("INDEXVERSION=2");
      builder.AppendLine("VERCTRL=HG");
      builder.AppendLine("DATETIME=" + dateString);
      builder.AppendLine("SRCSRV: variables ------------------------------------------");
      builder.AppendLine(@"SRCLOCATION=%ZERO_SOURCE%");
      // The target location to output the file to is whatever root VS decides
      // then our revision number then the original relative path.
      builder.AppendLine(@"SRCSRVTRG=%targ%\%var2%\%var3%");
      // Grab the file of the desired revision from the local depo at SRCLOCATION
      // and output that to the source target directory location.
      // Unfortunately mercurial is unable to execute commands on remote repositories...
      builder.AppendLine("SRCSRVCMD=hg.exe --cwd \"%fnvar%(SRCLOCATION)\" -y cat %var3% -r %var2% > %SRCSRVTRG%");
      builder.AppendLine("SRCSRV: source files ---------------------------------------");

      // Iterate over each depo and write out their respective files
      foreach(var depo in depos)
      {
        // Get the depo's current revision
        String currentRevision = GetCurrentRevision(depo.DepoPath);
        if (isSilent == false)
          Console.WriteLine("Embedding revision {0}", currentRevision);  

        // Now output the format string for the file
        foreach(var file in depo.Files)
        {
          builder.AppendLine(String.Format("{0}*{1}*{2}", file.FullPath, currentRevision, file.RelativePath));
        }
      }
      builder.AppendLine("SRCSRV: end ------------------------------------------------");

      return builder.ToString();
    }
  }
}
