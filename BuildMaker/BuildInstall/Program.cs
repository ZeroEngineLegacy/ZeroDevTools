///////////////////////////////////////////////////////////////////////////////
///
/// \file Program.cs
/// Where the InstallBuilder and ReleaseNoteBuilder are run from.
/// 
/// Authors: Benjamin Strukus
/// Copyright 2010-2013, DigiPen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////
using System;
using System.Collections.Generic;
using System.Linq;
using BuildMaker;
using System.IO;

namespace BuildInstall
{
  class Program
  {
    // Simple helper to turn command line arguments into a dictionary lookup.
    static Dictionary<String, List<String>> ParseCommandLineArgs(string[] args)
    {
      Dictionary<String, List<String>> parsedArgs = new Dictionary<String, List<String>>();

      int i = 0;
      while (i < args.Length)
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

          // To support multiple depos this was turned into a multi-map. Unfortunately this
          // means other parameters can get more than one key, but oh well...
          if (parsedArgs.ContainsKey(flag) == false)
            parsedArgs.Add(flag, new List<String>());
          parsedArgs[flag].Add(val);
          i += 2;
        }
        else
          ++i;
      }

      return parsedArgs;
    }

    static void Main(string[] args)
    {
      Dictionary<String, List<String>> parsedArgs = ParseCommandLineArgs(args);

      String installerPrefix = "ZeroEngineSetup";
      String zeroEditorOutputSuffix = @"\Out\Win32\Release\ZeroEditor";
      String branch = "default";

      if (parsedArgs.ContainsKey("-prefix"))
        installerPrefix = parsedArgs["-prefix"][0];
      if (parsedArgs.ContainsKey("-outputSuffix"))
        zeroEditorOutputSuffix = parsedArgs["-outputSuffix"][0];
      if (parsedArgs.ContainsKey("-branch"))
        branch = parsedArgs["-branch"][0];

      InstallBuilder ib = new InstallBuilder();
      String installerDirectory = ib.Run(installerPrefix, zeroEditorOutputSuffix, branch);
      //ReleaseNoteBuilder rnb = new ReleaseNoteBuilder();
      //rnb.Run(installerDirectory);
    }
  }
}
