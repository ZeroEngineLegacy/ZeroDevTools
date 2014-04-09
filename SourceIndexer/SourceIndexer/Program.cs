using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ConsoleApplication1
{
  class FileInfo
  {
    public String FullPath;
    public String RelativePath;
  }

  class Program
  {
    static String[] GetFileListing(String pdbLocation, String srcsrvLocation)
    {
      ProcessStartInfo info = new ProcessStartInfo();
      info.Arguments = pdbLocation + " -r";
      info.WorkingDirectory = srcsrvLocation;
      info.FileName = Path.Combine(srcsrvLocation, "srctool.exe");
      info.RedirectStandardOutput = true;
      info.UseShellExecute = false;
      Process process = Process.Start(info);
      String files = process.StandardOutput.ReadToEnd();
      process.WaitForExit();

      return files.Split('\n');
    }

    static List<String> PruneFileListing(String[] files, String rootPath)
    {
      rootPath = rootPath.ToLower();
      List<String> results = new List<String>();
      for(uint i = 0; i < files.Length; ++i)
      {
        if (files[i].ToLower().StartsWith(rootPath))
        {
          results.Add(files[i]);
        }
      
      }
      return results;
    }

    static String GetCurrentRevision(String depoPath)
    {
      ProcessStartInfo info = new ProcessStartInfo();
      info.Arguments = "--cwd " + depoPath + " tip --template {node}";
      info.FileName = "hg";
      info.RedirectStandardOutput = true;
      info.UseShellExecute = false;
      Process process = Process.Start(info);
      String currentRevision = process.StandardOutput.ReadToEnd();
      process.WaitForExit();

      return currentRevision;
    }

    public static string GetExactPathName(string pathName)
    {
      if (!(File.Exists(pathName) || Directory.Exists(pathName)))
        return pathName;

      var di = new DirectoryInfo(pathName);

      if (di.Parent != null)
      {
        return Path.Combine(
            GetExactPathName(di.Parent.FullName),
            di.Parent.GetFileSystemInfos(di.Name)[0].Name);
      }
      else
      {
        return di.Name.ToUpper();
      }
    }

    static List<FileInfo> GetRelativePaths(List<String> files, String depoPath)
    {
      List<FileInfo> relativePaths = new List<FileInfo>();

      for(int i = 0; i < files.Count; ++i)
      {
        String fileName = files[i].Trim();
        String caseSensitiveFileName = GetExactPathName(fileName);
        String relativeFile = caseSensitiveFileName.Substring(depoPath.Length);
        relativeFile = relativeFile.TrimStart('\\');

        FileInfo info = new FileInfo();
        info.FullPath = caseSensitiveFileName;
        info.RelativePath = relativeFile;
        relativePaths.Add(info);
      }

      return relativePaths;
    }

    static Dictionary<String, String> ParseCommandLineArgs(string[] args)
    {
      Dictionary<String, String> parsedArgs = new Dictionary<String, String>();

      int i = 0;
      while(i < args.Length)
      {
        String flag = args[i];
        if (flag.StartsWith("-"))
        {
          int nextIndex = i + 1;
          String val = "";
          if (nextIndex < args.Length && !args[nextIndex].StartsWith("-"))
            val = args[nextIndex];

          parsedArgs.Add(flag, val);
          i += 2;
        }
        else
          ++i;
      }

      return parsedArgs;
    }

    static void Main(string[] args)
    {
      Dictionary<String, String> parsedArgs = ParseCommandLineArgs(args);

      String depoPath = @"C:\BuildBot\slave\Zero\ZeroCore";
      String pdbLocation = @"C:\BuildBot\slave\Zero\ZeroBuildOut\Out\Win32\Release\ZeroEditor\ZeroEditor.pdb";
      String srcSrvLocation = @"C:\Program Files (x86)\Windows Kits\8.1\Debuggers\x86\srcsrv";

      String outLocation = "";
      if(parsedArgs.ContainsKey("-depo"))
        depoPath =parsedArgs["-depo"];
      if (parsedArgs.ContainsKey("-pdb"))
        pdbLocation = parsedArgs["-pdb"];
      if (parsedArgs.ContainsKey("-srcsrv"))
        srcSrvLocation = parsedArgs["-srcsrv"];

      if (parsedArgs.ContainsKey("-out"))
        outLocation = parsedArgs["-out"];
      else
      {
        String fullPath = Path.GetDirectoryName(pdbLocation);
        outLocation = Path.Combine(fullPath, "PdbData.txt");
      }

      String psbStrLocation = Path.Combine(srcSrvLocation, "pdbstr.exe");

      Console.WriteLine("DepoPath: {0}", depoPath);
      Console.WriteLine("PdbPath: {0}", pdbLocation);
      Console.WriteLine("SrcSrvLocation: {0}", srcSrvLocation);
      Console.WriteLine("OutLocation: {0}", outLocation);

      String currentRevision = GetCurrentRevision(depoPath);
      String[] allFiles = GetFileListing(pdbLocation, srcSrvLocation);
      List<String> files = PruneFileListing(allFiles, depoPath);
      List<FileInfo> relativeFiles = GetRelativePaths(files, depoPath);
      

      StringBuilder builder = new StringBuilder();

      String dateString;
      DateTime date = DateTime.Now;
      dateString = date.ToString();

      builder.AppendLine("SRCSRV: ini ------------------------------------------------");
      builder.AppendLine("VERSION=1");
      builder.AppendLine("INDEXVERSION=2");
      builder.AppendLine("VERCTRL=HG");
      builder.AppendLine("DATETIME=" + dateString);
      builder.AppendLine("SRCSRV: variables ------------------------------------------");
      builder.AppendLine(@"SRCLOCATION=%ZERO_SOURCE%");
      builder.AppendLine(@"SRCSRVTRG=%targ%\%var2%\%var3%");
      builder.AppendLine("SRCSRVCMD=hg.exe --cwd \"%fnvar%(SRCLOCATION)\" -y cat %var3% -r %var2% > %SRCSRVTRG%");
      builder.AppendLine("SRCSRV: source files ---------------------------------------");
      for (int i = 0; i < relativeFiles.Count; ++i)
      {
        builder.AppendLine(String.Format("{0}*{1}*{2}", relativeFiles[i].FullPath, currentRevision, relativeFiles[i].RelativePath));
      }
      builder.AppendLine("SRCSRV: end ------------------------------------------------");

      File.WriteAllText(outLocation, builder.ToString());



      ProcessStartInfo info = new ProcessStartInfo();
      info.Arguments = String.Format("-w -p:\"{0}\" -s:srcsrv -i:\"{1}\"", pdbLocation, outLocation);
      info.FileName = psbStrLocation;
      info.RedirectStandardOutput = true;
      info.RedirectStandardError = true;
      info.UseShellExecute = false;
      Process process = Process.Start(info);
      String fileRevision = process.StandardOutput.ReadToEnd();
      String errs = process.StandardError.ReadToEnd();
      process.WaitForExit();
    }
  }
}
