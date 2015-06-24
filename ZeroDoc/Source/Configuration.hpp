#pragma once
#include "Support/StringMap.hpp"
#include "Platform/FilePath.hpp"

namespace Zero
{

struct Configurations
{
  String SourcePath;
  String DocumentationPath;
  String DocumentationRoot;
  String DoxygenPath;
  String DocumentationRawFile;
  String DocumentationFile;
  String EventsFile;

  bool Verbose;
  String Log;
};

inline Configurations LoadConfigurations(StringMap& params)
{
  Configurations config;
  config.Verbose = GetStringValue<bool>(params,"verbose",false);
  config.Log = GetStringValue<String>(params,"log","");

  //get the path to the source
  config.SourcePath = GetStringValue<String>(params,"sourcePath","E:\\Zero\\");
  config.SourcePath = FilePath::Normalize(config.SourcePath);
  //get the path to the documentation
  config.DocumentationPath = FilePath::Combine(config.SourcePath,"Projects", "Editor");
  config.DocumentationPath = FilePath::Normalize(config.DocumentationPath);
  //load the raw documentation (before merging with doxy) and the documentation file
  config.DocumentationRawFile = FilePath::Combine(config.DocumentationPath,"DocumentationRaw.data");
  config.DocumentationFile  = FilePath::Combine(config.SourcePath, "Data","Documentation.data");
  //Load the list of events
  config.EventsFile = FilePath::Combine(config.DocumentationPath, "EventList.data");
  //get the path to the doxygen file
  config.DoxygenPath = GetStringValue<String>(params,"doxyPath","C:\\ZeroDoxygen");
  config.DoxygenPath = FilePath::Normalize(config.DoxygenPath);
  //get the path to the documentation folder (where the data files for documentation are)
  config.DocumentationRoot = FilePath::Combine(config.SourcePath,"ZeroDoc", "Documentation");

  return config;
}


}