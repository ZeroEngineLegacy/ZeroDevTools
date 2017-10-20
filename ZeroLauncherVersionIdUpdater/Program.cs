﻿using CommandLine;
using CommandLine.Text;
using SharedBuildHelpers;
using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Net;
using System.Text;
using System.Threading.Tasks;

namespace ZeroLauncherVersionIdUpdater
{
    class Program
  {
    static String BaseUrl = "https://builds.zeroengine.io";

    static String QueryMajorId(String exePath, String sourcePath, int maxTimeout)
    {
      // The launcher will always end up using the patched version if it is installed on
      // the computer. To get around this, force it to use the local version by writing
      // out a dummy version id file with a very large version id.
      String exeDirectory = Path.GetDirectoryName(exePath);
      String outFileName = Path.Combine(exeDirectory, "ZeroLauncherVersionId.txt");
      File.WriteAllText(outFileName, "9000000");

      BuildMeta meta = new BuildMeta();
      meta.SetupLauncher();
      meta.GetBuildInfoFromExe(exePath, sourcePath, maxTimeout);

      // Delete the dummy file we created
      File.Delete(outFileName);

      return meta.GetProperty("LauncherMajorVersion");
    }

    static void CreateVersionIdFile(String outDir, int majorId)
    {
      String minorIdQuery = BaseUrl + "?Commands=CheckPatchId&MajorId=" + majorId;
      String versionId = HttpDownloadHelpers.GetStringData(minorIdQuery);

      int id = int.Parse(versionId);
      ++id;
      Console.Write(id);

      String outFileName = Path.Combine(outDir, "ZeroLauncherVersionId.txt");
      File.WriteAllText(outFileName, id.ToString());
    }

    static void ZipPackage(String packageDir, String packagePath)
    {
      if (File.Exists(packagePath))
        File.Delete(packagePath);
      ZipFile.CreateFromDirectory(packageDir, packagePath);
    }

    static void CopyDirectory(String sourceDir, String destDir)
    {
      Directory.CreateDirectory(destDir);

      foreach (var file in Directory.GetFiles(sourceDir))
        File.Copy(file, Path.Combine(destDir, Path.GetFileName(file)), true);

      foreach (var dir in Directory.GetDirectories(sourceDir))
        CopyDirectory(dir, Path.Combine(destDir, Path.GetFileName(dir)));
    }

    static void DeleteDirectory(String sourceDir)
    {
      if (Directory.Exists(sourceDir) == false)
        return;

      foreach (var file in Directory.GetFiles(sourceDir))
        File.Delete(file);

      foreach (var dir in Directory.GetDirectories(sourceDir))
        DeleteDirectory(dir);

      try { Directory.Delete(sourceDir); }
      catch (Exception) { }
    }

    static void CopyTools(String toolsDir, String outDir)
    {
      Directory.CreateDirectory(outDir);

      //currently we only need these files from the tools directory, don't copy more as it bloats the package size
      string[] files = {"ZeroCrashHandler.exe", "CommandLine.dll", "Newtonsoft.Json.dll"};

      foreach (var file in files)
        File.Copy(Path.Combine(toolsDir, file), Path.Combine(outDir, file));

      CopyDirectory(Path.Combine(toolsDir, "GeometryProcessor"), Path.Combine(outDir, "GeometryProcessor"));
      CopyDirectory(Path.Combine(toolsDir, "ImageProcessor"), Path.Combine(outDir, "ImageProcessor"));
    }

    static void CreatePackage(String sourceDir, String zeroOutDir, String outDir, String packageOutDir)
    {
      String dllName = "ZeroLauncherDll.dll";
      String pdbName = "ZeroLauncherDll.pdb";
      File.Copy(Path.Combine(zeroOutDir, dllName), Path.Combine(packageOutDir, dllName), true);
      File.Copy(Path.Combine(zeroOutDir, pdbName), Path.Combine(packageOutDir, pdbName), true);
      File.Copy(Path.Combine(outDir, "ZeroLauncherVersionId.txt"), Path.Combine(packageOutDir, "ZeroLauncherVersionId.txt"), true);

      CopyTools(Path.Combine(sourceDir, "Tools"), Path.Combine(packageOutDir, "Tools"));

      CopyDirectory(Path.Combine(sourceDir, "Data"), Path.Combine(packageOutDir, "Data"));
      CopyDirectory(Path.Combine(sourceDir, "Resources", "ZeroLauncherResources"), Path.Combine(packageOutDir, "Resources", "ZeroLauncherResources"));
      CopyDirectory(Path.Combine(sourceDir, "Resources", "Loading"), Path.Combine(packageOutDir, "Resources", "Loading"));
      CopyDirectory(Path.Combine(sourceDir, "Resources", "ZeroCore"), Path.Combine(packageOutDir, "Resources", "ZeroCore"));
      CopyDirectory(Path.Combine(sourceDir, "Resources", "FragmentCore"), Path.Combine(packageOutDir, "Resources", "FragmentCore"));
      File.Copy(Path.Combine(sourceDir, "Data", "ZeroLauncherEula.txt"), Path.Combine(packageOutDir, "ZeroLauncherEula.txt"));

      // Copy the default templates we install with over too
      CopyDirectory(Path.Combine(sourceDir, "Projects", "LauncherTemplates"), Path.Combine(packageOutDir, "Templates"));
    }

    static void GenerateInstaller(String sourceDir, String zeroOutDir, int majorId)
    {
      //Generate the Zero Engine installer using Inno Setup.
      ProcessStartInfo innoSetupInfo = new ProcessStartInfo();
      innoSetupInfo.Arguments = Path.Combine(sourceDir, "Build", "ZeroLauncherInstaller.iss ") +
                                  String.Format("/DZeroSource=\"{0}\"" , sourceDir) +
                                  String.Format(" /DZeroLauncherOutputPath=\"{0}\"", zeroOutDir) +
                                  String.Format(" /DMajorId=\"{0}\"", majorId);// +
      innoSetupInfo.FileName = @"C:\Program Files (x86)\Inno Setup 5\iscc.exe";
      innoSetupInfo.RedirectStandardOutput = true;
      innoSetupInfo.RedirectStandardError = true;
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
        return;

      }
      String output = innoSetup.StandardOutput.ReadToEnd();
      String errs = innoSetup.StandardError.ReadToEnd();

      //String innoError = innoSetup.StandardOutput.ReadToEnd();
      innoSetup.WaitForExit();
    }

    static void GeneratePatch(String sourceDir, String outputPackagePath, int majorId, String resultPatchPath)
    {
      //Generate the Zero Engine installer using Inno Setup.
      ProcessStartInfo innoSetupInfo = new ProcessStartInfo();
      innoSetupInfo.Arguments = "ZeroLauncherInstallerPatch.iss " +
                                  String.Format("/DZeroSource=\"{0}\"", sourceDir) +
                                  String.Format(" /DOutputFiles=\"{0}\"", outputPackagePath) +
                                  String.Format(" /DMajorId=\"{0}\"", majorId);// +
      innoSetupInfo.FileName = @"C:\Program Files (x86)\Inno Setup 5\iscc.exe";
      innoSetupInfo.RedirectStandardOutput = true;
      innoSetupInfo.RedirectStandardError = true;
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
        return;

      }
      String output = innoSetup.StandardOutput.ReadToEnd();
      String errs = innoSetup.StandardError.ReadToEnd();

      innoSetup.WaitForExit();

      String outputExeName = "ZeroLauncherPatch.exe";
      String expectedOutputPath = Path.Combine("Output", outputExeName);
      if(File.Exists(expectedOutputPath))
      {
        File.Copy(expectedOutputPath, Path.Combine(resultPatchPath, outputExeName), true);
      }
    }

    static void Main(string[] args)
    {
      var commandArgs = new CommandArgs();

      var settings = new CommandLine.ParserSettings();
      settings.IgnoreUnknownArguments = false;
      CommandLine.Parser parser = new Parser(with => with.HelpWriter = Console.Error);

      // Try to parse the command line arguments
      if (parser.ParseArguments(args, commandArgs) == false)
      {
        // If we failed to parse print out the usage of this program
        Console.Write(commandArgs.GetUsage());

        return;
      }

      bool createPackage = commandArgs.CreatePackage;

      String packageOutDir = Path.Combine(commandArgs.OutDir, "ZeroLauncherPackage");
      //don't delete the old folder if we aren't creating the package (as we're only adding the id file),
      //but otherwise make sure the old content is deleted
      if (createPackage)
        DeleteDirectory(commandArgs.OutDir);

      Directory.CreateDirectory(commandArgs.OutDir);
      if (createPackage)
        Directory.CreateDirectory(packageOutDir);

      String launcherExePath = Path.Combine(commandArgs.ZeroOutDir, "ZeroLauncher.exe");
      if (commandArgs.MajorId == -1)
        commandArgs.MajorId = int.Parse(QueryMajorId(launcherExePath, commandArgs.SourceDir, commandArgs.MaxTimeout));

      //always create the id file
      CreateVersionIdFile(commandArgs.OutDir, commandArgs.MajorId);

      if (createPackage)
      {
        CreatePackage(commandArgs.SourceDir, commandArgs.ZeroOutDir, commandArgs.OutDir, packageOutDir);
        ZipPackage(packageOutDir, Path.Combine(commandArgs.OutDir, "ZeroLauncherPackage.zip"));
      }
      
      if(commandArgs.GenerateInstaller)
      {
        if (createPackage == false)
        {
          Console.WriteLine("Generate installer requires create package");
          return;
        }

        CopyDirectory(packageOutDir, commandArgs.ZeroOutDir);
        GenerateInstaller(commandArgs.SourceDir, commandArgs.ZeroOutDir, commandArgs.MajorId);
      }

      if (commandArgs.CreatePatch)
      {
        if (createPackage == false)
        {
          Console.WriteLine("Generate patch requires create package");
          return;
        }

        GeneratePatch(commandArgs.SourceDir, packageOutDir, commandArgs.MajorId, commandArgs.OutDir);
      }

      //open the package folder
      if (commandArgs.OpenExplorer && Directory.Exists(commandArgs.OutDir))
        Process.Start(commandArgs.OutDir);
    }
  }
}
