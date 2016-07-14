#pragma once
#include "Support/StringMap.hpp"
#include "Platform/FilePath.hpp"
#include "StandardLibraries/Common/String/StringBuilder.hpp"

namespace Zero
{

struct DocGeneratorConfig
{
  ///// Raw Strings /////

  // required if parseDoxygen flag is set
  String mDoxygenPath;

  // optional
  String mZeroDocFile;

  // defaults to file in rawDocDirectory honestly it should always be with a documentation folder
  String mTypedefLibraryFile;

  String mOutputDirectory;

  // defaults to where it should be in the output directory
  String mRawDocDirectory;

  String mIgnoreFile;

  String mIgnoreSkeletonDocFile;

  String mLogFile;

  ///// Trimmed Strings /////
  // defaults to trimdoc.data in the output directory
  String mTrimmedOutput;
  
  String mTrimmedTypedefFile;

  ///// Markup Strings /////
  String mMarkupDirectory;

  String mCommandListFile;

  ///// Raw Bools /////
  bool mReplaceTypes;
  bool mLoadTypedefs;
  bool mLoadTypedefsFromDoxygen;
  bool mVerbose;
  bool mTagAllAsUnbound;
  
  bool mHelp;

  ///// Trimmed Bools /////
  bool mCreateTrimmed;

};

inline DocGeneratorConfig LoadConfigurations(StringMap& params)
{
  DocGeneratorConfig config;

  ///// Load Raw Options /////

  config.mReplaceTypes = GetStringValue<bool>(params, "mReplaceTypes", false);
  config.mLoadTypedefs = GetStringValue<bool>(params, "mLoadTypedefs", false);
  config.mLoadTypedefsFromDoxygen = GetStringValue<bool>(params, "mLoadTypedefsFromDoxygen", false);
  config.mVerbose = GetStringValue<bool>(params, "verbose", false);
  config.mHelp = GetStringValue<bool>(params, "help", false);
  config.mTagAllAsUnbound = GetStringValue<bool>(params, "tagAllAsUnbound", false);

  //get the path to the doxygen file
  config.mDoxygenPath = GetStringValue<String>(params, "doxyPath", "");
  config.mDoxygenPath = FilePath::Normalize(config.mDoxygenPath);

  //load the zero documentation (before merging with doxy) and the documentation file
  config.mZeroDocFile = GetStringValue<String>(params, "zeroDocFile", "");
  config.mZeroDocFile = FilePath::Normalize(config.mZeroDocFile);

  // output directory
  config.mOutputDirectory  = GetStringValue<String>(params, "output", "");
  config.mOutputDirectory = FilePath::Normalize(config.mOutputDirectory);

  // raw doc directory (often just the output directory)
  config.mRawDocDirectory = GetStringValue<String>(params, "rawDocDirectory", config.mOutputDirectory);
  config.mRawDocDirectory = FilePath::Normalize(config.mRawDocDirectory);

  // typedef directory (often just the raw doc directory)
  config.mTypedefLibraryFile = GetStringValue<String>(params, "typedefsFile",
    BuildString(config.mRawDocDirectory, "\\Typedefs.data"));
  config.mTypedefLibraryFile = FilePath::Normalize(config.mTypedefLibraryFile);

  // data file for the list of directories and files to ignore
  config.mIgnoreFile = GetStringValue<String>(params, "ignoreFile", "");
  config.mIgnoreFile = FilePath::Normalize(config.mIgnoreFile);

  // will contain a list of names that we wish to ignore
  config.mIgnoreSkeletonDocFile = GetStringValue<String>(params, "ignoreSkeleFile", "");
  config.mIgnoreSkeletonDocFile = FilePath::Normalize(config.mIgnoreSkeletonDocFile);

  config.mLogFile = GetStringValue<String>(params, "logFile", "");
  config.mLogFile = FilePath::Normalize(config.mLogFile);

  ///// Load Trim Options /////
  config.mCreateTrimmed = GetStringValue<bool>(params, "createTrimmed", false);

  config.mTrimmedOutput = GetStringValue<String>(params, "trimmedOutput",
    BuildString(config.mOutputDirectory, "\\Documentation.data"));
  config.mTrimmedOutput = FilePath::Normalize(config.mTrimmedOutput);

  config.mTrimmedTypedefFile = GetStringValue<String>(params, "trimmedTypedefFile", "");
  config.mTrimmedTypedefFile = FilePath::Normalize(config.mTrimmedTypedefFile);

  ///// Load Markup Options /////
  config.mMarkupDirectory = GetStringValue<String>(params, "markupDirectory", "");
  config.mMarkupDirectory = FilePath::Normalize(config.mMarkupDirectory);

  config.mCommandListFile = GetStringValue<String>(params, "commandListFile", "");
  config.mCommandListFile = FilePath::Normalize(config.mCommandListFile);

  return config;
}

}
