using System;
using System.Collections.Generic;
using System.Linq;
using BuildMaker;
using System.IO;

namespace BuildInstall
{
  class Program
  {
    static void Main(string[] args)
    {
      InstallBuilder ib = new InstallBuilder();
      String installerDirectory = ib.Run();
      ReleaseNoteBuilder rnb = new ReleaseNoteBuilder();
      rnb.Run(installerDirectory);
    }
  }
}
