using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace SourceIndexer
{
  // Simple structure that stores the full path and relative path of a file.
  class FileInfo
  {
    public String FullPath;
    public String RelativePath;
  }

  class SourceIndexer
  {
    // The pdb contains a list of what files it uses. This can be extracted using srctool.exe.
    // Note: this list contains files we don't care about (such as new and malloc).
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

    // The file list we get from the pdb contains all files referenced in the project.
    // This includes new and malloc. Prune the file listing to only those that
    // share the same root path as our code.
    static List<String> PruneFileListing(String[] files, String rootPath)
    {
      // Ignore case by just to-lowering everything
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

    // Unfortunately, the pdb changes the case of the path. Mercurial is case
    // sensitive with some commands so this poses problems. Luckily windows can
    // find the file case-insenstive and then reconstruct the original path.
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

    // We need the relative path of the file from the depo's root so that we it
    // can be copied later (during the source indexing from a crash dump) to
    // another directory but with the same relative path from the root.
    static List<FileInfo> GetRelativePaths(List<String> files, String depoPath)
    {
      List<FileInfo> relativePaths = new List<FileInfo>();

      for(int i = 0; i < files.Count; ++i)
      {
        // Make sure to trim out any newlines or other characters at the end
        String fileName = files[i].Trim();
        // Get the exact name so that mercurial can look it up
        String caseSensitiveFileName = GetExactPathName(fileName);
        // Chop of the beginning of the file path that matches the depo path.
        String relativeFile = caseSensitiveFileName.Substring(depoPath.Length);
        relativeFile = relativeFile.TrimStart('\\');

        FileInfo info = new FileInfo();
        info.FullPath = caseSensitiveFileName;
        info.RelativePath = relativeFile;
        relativePaths.Add(info);
      }

      return relativePaths;
    }

    // Simple helper to turn command line arguments into a dictionary lookup.
    static Dictionary<String, String> ParseCommandLineArgs(string[] args)
    {
      Dictionary<String, String> parsedArgs = new Dictionary<String, String>();

      int i = 0;
      while(i < args.Length)
      {
        // If this argument starts with a '-' then check the next argument.
        // If the next argument doesn't have a '-' then it's the value of this argument.
        // If it does have a '-' then assume this is a flag is a boolean being set to true.
        String flag = args[i];
        if (flag.StartsWith("-"))
        {
          int nextIndex = i + 1;
          String val = "true";
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

    // Test function to write each file into the pdb. Unfortunately there's no way to extract the files back out.
    static void WriteEachFileToDataStream(List<FileInfo> relativeFiles, String psbStrLocation, String pdbLocation)
    {
      for (int i = 0; i < relativeFiles.Count; ++i)
      {
        String relativePath = relativeFiles[i].RelativePath;
        String fullPath = relativeFiles[i].FullPath;


        ProcessStartInfo info = new ProcessStartInfo();
        info.Arguments = String.Format("-w -p:\"{0}\" -s:{1} -i:\"{2}\"", pdbLocation, relativePath, fullPath);
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

    static void Main(string[] args)
    {
      Dictionary<String, String> parsedArgs = ParseCommandLineArgs(args);

      String depoPath = @"C:\BuildBot\slave\Zero\ZeroCore";
      String pdbLocation = "ZeroEditor.pdb";
      String srcSrvLocation = "srcsrv";
      String mode = "hg";

      // Should we not print diagnostic messages?
      bool isSilent = false;

      // Parse the arguments we need
      String outLocation = "";
      if (parsedArgs.ContainsKey("-depo"))
        depoPath = parsedArgs["-depo"];
      if (parsedArgs.ContainsKey("-pdb"))
        pdbLocation = parsedArgs["-pdb"];
      if (parsedArgs.ContainsKey("-srcsrv"))
        srcSrvLocation = parsedArgs["-srcsrv"];
      if (parsedArgs.ContainsKey("-mode"))
        mode = parsedArgs["-mode"];
      if (parsedArgs.ContainsKey("-silent"))
        isSilent = true;

      // If the user specifies where to output the text data we need to
      // save then use that path. Otherwise put it next to the pdb.
      if (parsedArgs.ContainsKey("-out"))
        outLocation = parsedArgs["-out"];
      else
      {
        String fullPath = Path.GetDirectoryName(pdbLocation);
        outLocation = Path.Combine(fullPath, "PdbData.txt");
      }

      // Some of the srcsrv tools don't work with relative paths.
      // Convert all of our paths to full paths.
      depoPath = Path.GetFullPath(depoPath);
      pdbLocation = Path.GetFullPath(pdbLocation);
      srcSrvLocation = Path.GetFullPath(srcSrvLocation);
      outLocation = Path.GetFullPath(outLocation);

      // Get the path to the tool used to modify the pdb.
      String pdbStrLocation = Path.Combine(srcSrvLocation, "pdbstr.exe");

      if (isSilent == false)
      {
        // For debugging: output what the directory of each path we need is.
        Console.WriteLine("DepoPath: {0}", depoPath);
        Console.WriteLine("PdbPath: {0}", pdbLocation);
        Console.WriteLine("SrcSrvLocation: {0}", srcSrvLocation);
        Console.WriteLine("OutLocation: {0}", outLocation);
      }

      String[] allFiles = GetFileListing(pdbLocation, srcSrvLocation);
      if (isSilent == false)
        Console.WriteLine("Found {0} files in the pdb", allFiles.Length);

      List<String> files = PruneFileListing(allFiles, depoPath);
      if (isSilent == false)
        Console.WriteLine("Pruned file listing to {0} files", files.Count);

      List<FileInfo> relativeFiles = GetRelativePaths(files, depoPath);

      String pdbData = "";
      if (mode == "hg")
        pdbData = MercurialIndexer.BuildSourceIndex(depoPath, pdbLocation, pdbStrLocation, relativeFiles, isSilent);

      File.WriteAllText(outLocation, pdbData);


      // Write the text file into the pdb
      ProcessStartInfo info = new ProcessStartInfo();
      info.Arguments = String.Format("-w -p:\"{0}\" -s:srcsrv -i:\"{1}\"", pdbLocation, outLocation);
      info.FileName = pdbStrLocation;
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
