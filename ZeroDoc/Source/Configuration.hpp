#pragma once
#include "Support/StringMap.hpp"
#include "Utility/FilePath.hpp"

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
  config.SourcePath = NormalizePath(config.SourcePath);
  //get the path to the documentation
  config.DocumentationPath = BuildString(config.SourcePath.all(),"\\Projects\\Editor\\");
  config.DocumentationPath = NormalizePath(config.DocumentationPath);
  //load the raw documentation (before merging with doxy) and the documentation file
  config.DocumentationRawFile = BuildString(config.DocumentationPath.c_str(),"DocumentationRaw.data");
  config.DocumentationFile  = BuildString(config.DocumentationPath.c_str(),"Documentation.data");
  //Load the list of events
  config.EventsFile = BuildString(config.DocumentationPath.c_str(), "EventList.data");
  //get the path to the doxygen file
  config.DoxygenPath = GetStringValue<String>(params,"doxyPath","E:\\Zero\\Output");
  config.DoxygenPath = NormalizePath(config.DoxygenPath);
  //get the path to the documentation folder (where the data files for documentation are)
  config.DocumentationRoot = BuildString(config.SourcePath.c_str(),"DevTools\\Documentation\\");

  return config;
}


}