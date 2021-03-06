using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace SourceIndexer
{
  class GitIndexer
  {
    // Get the revision id of the depo.
    // This is used to know what revision we want to revert each file back to.
    static String GetCurrentRevision(String depoPath)
    {
      ProcessStartInfo info = new ProcessStartInfo();
      info.Arguments = "-C \"" + depoPath + "\" log -1 --pretty=%H";
      info.FileName = "git.exe";
      info.RedirectStandardOutput = true;
      info.UseShellExecute = false;
      Process process = Process.Start(info);
      String currentRevision = process.StandardOutput.ReadToEnd();
      process.WaitForExit();

      return currentRevision.Trim();
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
      builder.AppendLine("VERCTRL=GIT");
      builder.AppendLine("DATETIME=" + dateString);
      builder.AppendLine("SRCSRV: variables ------------------------------------------");
      builder.AppendLine(@"SRCLOCATION=%ZERO_SOURCE_GIT%");
      // The target location to output the file to is whatever root VS decides
      // then our revision number then the original relative path.
      builder.AppendLine(@"SRCSRVTRG=%targ%\%var2%\%var3%");
      // Grab the file of the desired revision from the local depo at SRCLOCATION
      // and output that to the source target directory location.
      // Unfortunately mercurial is unable to execute commands on remote repositories...
      // Also, mercurial is unable to check out a file from a sub depo using the main depo's path. 
      // Because of this var4 is added for zero to access a sub depo which is located underneath
      // the main depo. Due to how the expansion of %fnvar% works I think this line must be
      // altered at the top here, not on each individual file's line.
      builder.AppendLine("SRCSRVCMD=git.exe -C \"%fnvar%(SRCLOCATION)\" show %var2%:%var4% > %SRCSRVTRG%");
      builder.AppendLine("SRCSRV: source files ---------------------------------------");

      // Iterate over each depo and write out their respective files
      foreach (var depo in depos)
      {
        // Get the depo's current revision
        String currentRevision = GetCurrentRevision(depo.DepoPath);
        if (isSilent == false)
          Console.WriteLine("Embedding revision {0}", currentRevision);

        // Disable sub-repos as they cause issues
        // Quick and dirty hack to detect if this depo is zilch and if so add extra info to the file's paths.
        //var extraDepo = "";
        //var lastSlash = depo.DepoPath.LastIndexOf('\\');
        //var depoName = depo.DepoPath.Substring(lastSlash).Trim('\\');
        //if (depoName == "Zilch")
        //  extraDepo += "\\Zilch";
        //else if (depoName == "ExtensionLibraries")
        //  extraDepo += "\\ExtensionLibraries";
        //else if (depoName == "StandardLibraries")
        //  extraDepo += "\\ExtensionLibraries\\StandardLibraries";
        //else if (depoName == "AudioEngine")
        //  extraDepo += "\\AudioEngine";

        // Now output the format string for the file
        foreach (var file in depo.Files)
        {
          // Instead of figuring out where each file is supposed to go with the original depo
          // paths, just put each separate depo in its own folder.
          String relativePath = file.RelativePath;
          // The linux path (forward-slash) has to be used for git commands, but the windows
          // path needs backslashes in order to create paths so we have to save both
          String relativePathUnix = relativePath.Replace("\\", "/"); ;
          builder.AppendLine(String.Format("{0}*{1}*{2}*{3}", file.FullPath, currentRevision, relativePath, relativePathUnix));
        }
      }
      builder.AppendLine("SRCSRV: end ------------------------------------------------");

      return builder.ToString();
    }
  }
}
