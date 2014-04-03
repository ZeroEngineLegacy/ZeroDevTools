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
    static void Main(string[] args)
    {
      InstallBuilder ib = new InstallBuilder();
      String installerDirectory = ib.Run();
      //ReleaseNoteBuilder rnb = new ReleaseNoteBuilder();
      //rnb.Run(installerDirectory);
    }
  }
}
