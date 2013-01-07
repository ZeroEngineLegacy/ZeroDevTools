using System;
using System.Collections.Generic;
using System.Linq;
using BuildMaker;

namespace BuildInstall
{
  class Program
  {
    static void Main(string[] args)
    {
      //InstallBuilder ib = new InstallBuilder();
      //ib.Run();
      ReleaseNoteBuilder rnb = new ReleaseNoteBuilder();
      rnb.Run();
    }
  }
}
