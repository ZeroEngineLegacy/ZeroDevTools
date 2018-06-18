#pragma once
#include "Support/StringMap.hpp"
#include "Platform/FilePath.hpp"
#include "Common/String/StringBuilder.hpp"

namespace Zero
{

struct DocGeneratorConfig
{
  ///// Raw Strings /////

  /// required if parseDoxygen flag is set
  String mDoxygenPath;
  /// this gets us the doc file that zilch will fill out from the engine meta
  String mZeroDocFile;
  /// defaults to file in rawDocDirectory honestly it should always be with a documentation folder
  String mTypedefLibraryFile;
  /// where the documentation should output
  String mOutputDirectory;
  /// defaults to where it should be in the output directory
  String mRawDocDirectory;
  /// file containing list of files/directores to ignore
  String mIgnoreFile;
  /// where to output our log of warnings/errors
  String mLogFile;
  /// this is the events file zero outputs
  String mZeroUserAttributesFile;
  /// this is the events file zero outputs
  String mAttributesOutputLocation;
  /// this is the events file zero outputs
  String mZeroEventsFile;
  /// this is one that we generate
  String mEventsOutputLocation;
  /// this is the exceptions file that we generate
  String mExceptionsFile;
  /// this file contains a mapping of zilch types to cpp types 
  String mZilchTypesToCppFileList;
  /// what file to output the list of all shortcuts
  String mShortcutsOutputFile;
  /// contains a list of types that should not be loaded by documentation
  String mTypeBlacklistFile;

  ///// Trimmed Strings /////
  /// defaults to trimdoc.data in the output directory
  String mTrimmedOutput;
  /// where to load the trimmed typedef doclib location
  String mTrimmedTypedefFile;

  ///// Markup Strings /////
  /// what directory to output all the markup files to
  String mMarkupDirectory;
  /// where to output the command list
  String mCommandListFile;

  /// what macro test to run, if -1, no tests will be run, if max(int), all tests will run
  int mRunMacroTest;

  ///// Raw Bools /////
  /// if true, we will replace any typedefs in documentation with the underlying type
  bool mReplaceTypes;
  /// if true, we will load typedefs from the output directory before doing any doc parsing
  bool mLoadTypedefs;
  /// if true, we will load our typedefs info in a seperate pass over doxygen namespace docs
  bool mLoadTypedefsFromDoxygen;
  /// if true, some extra parsing information will be logged/output
  bool mVerbose;
  /// if true, we tag everything we load as unbound types
  bool mTagAllAsUnbound;
  /// if true, warn for every undocumented type and function that is bound
  bool mWarnOnUndocumentedBoundData;
  /// if true, we will print the help text then exit
  bool mHelp;

  ///// Trimmed Bools /////
  
  // if true, we will output the trimmed documentation files
  bool mCreateTrimmed;
};

inline DocGeneratorConfig LoadConfigurations(StringMap& params)
{
  DocGeneratorConfig config;

  ///// Load Macro Options /////
  config.mRunMacroTest = GetStringValue<int>(params, "runMacroTest", -1);

  ///// Load Raw Options /////
  config.mReplaceTypes = GetStringValue<bool>(params, "replaceTypes", false);
  config.mLoadTypedefs = GetStringValue<bool>(params, "loadTypedefs", false);
  config.mLoadTypedefsFromDoxygen = GetStringValue<bool>(params, "loadTypedefsFromDoxygen", false);
  config.mVerbose = GetStringValue<bool>(params, "verbose", false);
  config.mHelp = GetStringValue<bool>(params, "help", false);
  config.mWarnOnUndocumentedBoundData = GetStringValue<bool>(params, "warnOnUndocumentedBoundData", false);
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

  config.mLogFile = GetStringValue<String>(params, "logFile", "");
  config.mLogFile = FilePath::Normalize(config.mLogFile);

  // location to load list of attributes users can use
  config.mZeroUserAttributesFile = GetStringValue<String>(params, "zeroUserAttributesFile", "");
  config.mZeroUserAttributesFile = FilePath::Normalize(config.mZeroUserAttributesFile);

  // location to output full documentation file for the engine to load
  config.mAttributesOutputLocation = GetStringValue<String>(params, "attributesOutputLocation", "");
  config.mAttributesOutputLocation = FilePath::Normalize(config.mAttributesOutputLocation);

  config.mZeroEventsFile = GetStringValue<String>(params, "eventsFile", "");
  config.mZeroEventsFile = FilePath::Normalize(config.mZeroEventsFile);

  config.mEventsOutputLocation = GetStringValue<String>(params, "eventsOutputLocation", "");
  config.mEventsOutputLocation = FilePath::Normalize(config.mEventsOutputLocation);

  config.mExceptionsFile = GetStringValue<String>(params, "exceptionsFile", "");
  config.mExceptionsFile = FilePath::Normalize(config.mExceptionsFile);

  ///mZilchTypesToCppFileList
  config.mZilchTypesToCppFileList = GetStringValue<String>(params, "zilchTypesToCppFileList", "");
  config.mZilchTypesToCppFileList = FilePath::Normalize(config.mZilchTypesToCppFileList);

  ///mShortcutsOutputFile
  config.mShortcutsOutputFile = GetStringValue<String>(params, "shortcutsOutputFile", "");
  config.mShortcutsOutputFile = FilePath::Normalize(config.mShortcutsOutputFile);

  ///mTypeBlacklistFile
  config.mTypeBlacklistFile = GetStringValue<String>(params, "typeBlacklistFile", "");
  config.mTypeBlacklistFile = FilePath::Normalize(config.mTypeBlacklistFile);

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
