#include "Precompiled.hpp"

#include "Engine/Documentation.hpp"
//#include "Serialization/Simple.hpp"
//#include "Serialization/Text.hpp"
#include "Platform/CommandLineSupport.hpp"
#include "Engine/Environment.hpp"

#include "DocConfiguration.hpp"
#include "RawDocumentation.hpp"
#include "MarkupWriter.hpp"
#include "MacroDatabase.hpp"
#include "MacroDocTests.hpp"

namespace Zero
{

// prints out the help info then exits the program
void PrintHelp(void)
{
  printf(
"\
Usage: [-flags] [-option \"argument\"]\n\
Flags:\n\
replaceTypes - if true, we will replace any typedefs in documentation with the underlying type\n\n\
loadTypedefs - if true, we will load typedefs from the output directory before doing any doc parsing\n\n\
loadTypedefsFromDoxygen - if true, we will load our typedefs info in a separate pass over doxygen namespace docs\n\n\
verbose - if true, some extra parsing information will be logged/output\n\n\
tagAllAsUnbound - if true, we tag everything we load as unbound types\n\n\
help - if true, we will print the help text then exit\n\n\
createTrimmed - if true, we will output the trimmed documentation files\n\n\
\n\n\
Options:\n\n\
doxygenPath - required if parseDoxygen flag is set\n\n\
zeroDocFile - this gets us the doc file that zilch will fill out from the engine meta\n\n\
typedefLibraryFile - defaults to file in rawDocDirectory honestly it should always be with a documentation folder\n\n\
outputDirectory - where the documentation should output\n\n\
rawDocDirectory - defaults to where it should be in the output directory\n\n\
ignoreFile - file containing list of files/directores to ignore\n\n\
ignoreSkeletonDocFile - a skeleton doc file that we ignore anything it has documented\n\n\
logFile - where to output our log of warnings/errors\n\n\
zeroEventsFile - this is the events file zero outputs\n\n\
eventsOutputLocation - this is the event list that we generate\n\n\
exceptionsFile - this is the exceptions file that we generate\n\n\
trimmedOutput - defaults to trimdoc.data in the output directory\n\n\
trimmedTypedefFile - where to load the trimmed typedef doclib location\n\n\
markupDirectory - what directory to output all the markup files to\n\n\
commandListFile - where to output the command list\n\n\
runMacroTest - what macro test to run, if -1, no tests will be run, if max(int), all tests will run\n\n\
"
  );
}

bool ValidateConfig(DocGeneratorConfig &config)
{
  if (config.mRunMacroTest > -1)
    return true;

  // we have to output something
  if (!config.mCreateTrimmed && config.mOutputDirectory.Empty() && config.mMarkupDirectory.Empty())
  {
    printf("one of the output options have to be set\n");
    return false;
  }

  bool noLoadRawOption = config.mRawDocDirectory.Empty() && config.mDoxygenPath.Empty();

  // we basically have two main cases, with and without trimmed output
  if (config.mCreateTrimmed)
  {
    if (config.mTrimmedOutput.Empty())
    {
      printf("no valid Trimmed Documentation Output directory given\n");
      return false;
    }
  }

  return true;
}

void RunDocumentationGenerator(DocGeneratorConfig &config)
{
  if (config.mVerbose)
    SetVerboseFlag();

  if (config.mLogFile.SizeInBytes() > 0)
  {
    DocLogger::Get()->StartLogger(config.mLogFile, config.mVerbose);
  }

  
  RawDocumentationLibrary *library = nullptr;
  RawTypedefLibrary *tdLibrary = RawTypedefLibrary::Get();

  if (config.mLoadTypedefs)
    tdLibrary->LoadFromFile(config.mTrimmedTypedefFile);

  // if we are going to parse doxygen
  if (config.mDoxygenPath.SizeInBytes() != 0)
  {
    library = new RawDocumentationLibrary;

    if (!config.mZilchTypesToCppFileList.Empty())
    {
      library->LoadZilchTypeCppClassList(config.mZilchTypesToCppFileList);
    }

    library->mIgnoreList.mDoxyPath = config.mDoxygenPath;

    if (!config.mIgnoreFile.Empty())
    {
      if (!Zero::LoadFromDataFile(library->mIgnoreList, config.mIgnoreFile))
      {
        Error("Unable to load doc file at: %s", config.mIgnoreFile.c_str());
      }
    }

    if (config.mZeroEventsFile.SizeInBytes())
    {
      library->LoadEventsList(config.mZeroEventsFile);
      library->mEvents.BuildMap();
    }

    // if zerodocfile then get list of classes from it that we should fill
    if (config.mZeroDocFile.SizeInBytes() > 0)
    {
      Zero::DocumentationLibrary *doc = new Zero::DocumentationLibrary();

      if (!Zero::LoadDocumentationSkeleton(*doc, config.mZeroDocFile))
      {
        Error("Unable to load doc file at: %s", config.mZeroDocFile.c_str());
      }

      doc->FinalizeDocumentation();

      library->LoadFromSkeletonFile(config.mDoxygenPath, *doc);

      library->LoadAllEnumDocumentationFromDoxygen(config.mDoxygenPath);
    }
    // otherwise get documentation from every single class file
    else
    {
      if (!library->LoadFromDoxygenDirectory(config.mDoxygenPath))
      {
        Error("Unable to load doxygen files at location: %s", config.mDoxygenPath.c_str());
      }
    }

    library->Build();
    library->FillOverloadDescriptions();

    if (!config.mShortcutsOutputFile.Empty())
    {
      library->mShortcutsLibrary.SaveToFile(config.mShortcutsOutputFile);
    }
  }
  // if we were passed in already existing raw documentation
  else if (!config.mRawDocDirectory.Empty())
  {
    library = new RawDocumentationLibrary;

    // load from documentation directory
    library->LoadFromDocumentationDirectory(config.mRawDocDirectory);
    library->Build();
  }

  if (config.mLoadTypedefsFromDoxygen)
  {
    tdLibrary->mIgnoreList.mDoxyPath = config.mDoxygenPath;

    if (!config.mIgnoreFile.Empty())
    {
      if (!Zero::LoadFromDataFile(tdLibrary->mIgnoreList, config.mIgnoreFile))
      {
        Error("Unable to load doc file at: %s", config.mIgnoreFile.c_str());
      }
    }

    tdLibrary->LoadTypedefsFromNamespaceDocumentation(config.mDoxygenPath);
  }

  tdLibrary->BuildMap();

  tdLibrary->ExpandAllTypedefs();

  if (library)
  {
    if (config.mReplaceTypes)
    {
      library->NormalizeAllTypes(tdLibrary);
    }

    if (config.mTagAllAsUnbound)
    {
      for (uint i = 0; i < library->mClasses.Size(); ++i)
      {
        library->mClasses[i]->mTags.PushBack("Non-Zilch");
      }
    }

    if (config.mOutputDirectory.SizeInBytes())
    {
      // output library and typedefs
      library->GenerateCustomDocumentationFiles(config.mOutputDirectory);
      tdLibrary->GenerateTypedefDataFile(config.mOutputDirectory);
    }

    if (config.mEventsOutputLocation.SizeInBytes())
    {
      library->SaveEventListToFile(config.mEventsOutputLocation);
    }
  }

  /////// Trimmed Documentation ///////
  if (!config.mCreateTrimmed)
    return;

  if (!library)
  {
    printf("No way to load/generate Trimmed documentation given\n");
    PrintHelp();
    return;
  }

  DocumentationLibrary trimLib;

  if (!config.mTrimmedTypedefFile.Empty())
  {
    RawTypedefLibrary trimTypedef;

    if (!trimTypedef.LoadFromFile(config.mTrimmedTypedefFile))
    {
      Error("Can't load typedef file at: %s", config.mTrimmedTypedefFile);
      return;
    }
    library->NormalizeAllTypes(&trimTypedef);
  }

  library->FillTrimmedDocumentation(trimLib);

  //trimLib.LoadFromMeta();

  trimLib.FinalizeDocumentation();

  WriteLog("saving Trimmed Documentation File as: %s\n", config.mTrimmedOutput.c_str());

  SaveTrimDocToDataFile(trimLib, config.mTrimmedOutput);
  //trimLib->La

  if (config.mVerbose || config.mWarnOnUndocumentedBoundData)
  {
    OutputListOfObjectsWithoutDesc(trimLib, &library->mIgnoreList);
  }
}

}//namespace Zero


int main(int argc, char* argv[])
{
  Zero::InitializeTokens();

  // setup zilch with no doc strings since we won't need them (Sense we are making them)
  Zilch::ZilchSetup *zilchSetup = new ZilchSetup(SetupFlags::NoDocumentationStrings);

  printf("Raw Documentation Generator\n");

  Zero::Array<Zero::String> commandLine;
  Zero::CommandLineToStringArray(commandLine, (cstr*)argv, argc);

  Zero::Environment environment;
  environment.ParseCommandArgs(commandLine);


  forRange(auto& entry, environment.mParsedCommandLineArguments.All())
  {
      printf("  %s : %s\n", entry.first.c_str(), entry.second.c_str());
  }
  printf("\n");

  Zero::DocGeneratorConfig config = LoadConfigurations(environment.mParsedCommandLineArguments);

  // if the help flag is passed, ignore everything else and just print the help info
  if (config.mHelp || !Zero::ValidateConfig(config))
  {
    Zero::PrintHelp();
    return 0;
  }

  if (config.mRunMacroTest > -1)
  {
    return (int)!Zero::RunMacroTests(config.mRunMacroTest);
  }

  Zero::RunDocumentationGenerator(config);

  if (!config.mMarkupDirectory.Empty())
  {
    Zero::WriteOutAllReMarkupFiles(config);
  }


  return 0;
}
