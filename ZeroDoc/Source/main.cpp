#include "Precompiled.hpp"

#include "Engine/Documentation.hpp"
#include "Serialization/Simple.hpp"
#include "Serialization/Text.hpp"

#include "DocConfiguration.hpp"
#include "RawDocumentation.hpp"
#include "MarkupWriter.hpp"

namespace Zero
{

// prints out the help info then exits the program
void PrintHelp(void)
{
  printf(
"\
Usage: [-flags] [-option \"argument\"]\n\
flags:\n\
replaceTypes - replaces types in documentation with definitions of types in the typedef library->\n\n\
loadTypedefs - if flag is set, typedefs will be loaded from typedef library data file.\n\n\
\noptions:\n\
doxyPath - if set, path passed in is used as path to load all doxygen documentation from, by default doxygen is not loaded.\n\n\
zeroDocFile - if set, zero doc file will be used to generate a list of class names to try to find and parse.\n\n\
output - location to output the raw documentation files for both classes and typedefs.\n\n\
rawDocDirectory - used to load class and typedef raw documentation, used when 'replaceTypes' is set.\n\n\
typedefsFile - used to override the path to load the raw typedef documentation file when 'replaceTypes' is set.\n\n\
trimmedOutput - if set, Trimmed Doc file will be created, if optional path set, file will output there instead of default output directory.\n\n\
trimmedTypedefFile - path to typedef file used to replace types specifically for the trimmed output file.\n\n\
"
  );
}

bool ValidateConfig(DocGeneratorConfig &config)
{
  // we have to output something
  if (!config.mCreateTrimmed && config.mOutputDirectory.empty() && config.mMarkupDirectory.empty())
  {
    printf("one of the output options have to be set\n");
    return false;
  }

  bool noLoadRawOption = config.mRawDocDirectory.empty() && config.mDoxygenPath.empty();

  // we basically have two main cases, with and without trimmed output
  if (config.mCreateTrimmed)
  {
    if (config.mTrimmedOutput.empty())
    {
      printf("no valid Trimmed Documentation Output directory given\n");
      return false;
    }
  }

  return true;
}

// wow lets see if we can somehow come up with a worse name
void RunDocumentationGenerator(DocGeneratorConfig &config)
{
  if (config.mLogFile.size() > 0)
  {
    DocLogger::Get()->StartLogger(config.mLogFile);
  }

  if (config.mVerbose)
    SetVerboseFlag();
  
  RawDocumentationLibrary *library = nullptr;
  RawTypedefLibrary tdLibrary;

  // if we are going to parse doxygen
  if (config.mDoxygenPath.size() != 0)
  {
    library = new RawDocumentationLibrary;

    library->mIgnoreList.mDoxyPath = config.mDoxygenPath;

    if (!config.mIgnoreFile.empty())
    {
      if (!Zero::LoadFromDataFile(library->mIgnoreList, config.mIgnoreFile))
      {
        Error("Unable to load doc file at: %s", config.mIgnoreFile.c_str());
      }
    }

    if (!config.mIgnoreSkeletonDocFile.empty())
    {
      DocumentationLibrary ignoreSkele;
      if (!Zero::LoadFromDataFile(ignoreSkele, config.mIgnoreSkeletonDocFile))
      {
        Error("Unable to load doc file at: %s", config.mIgnoreSkeletonDocFile.c_str());
      }

      library->mIgnoreList.CreateIgnoreListFromDocLib(config.mDoxygenPath ,ignoreSkele);
    }

    library->mIgnoreList.SortList();


    // if zerodocfile then get list of classes from it
    if (config.mZeroDocFile.size() > 0)
    {
      Zero::DocumentationLibrary doc;
      if (!Zero::LoadFromDataFile(doc, config.mZeroDocFile))
      {
        Error("Unable to load doc file at: %s", config.mZeroDocFile.c_str());
      }

      library->LoadFromSkeletonFile(config.mDoxygenPath, doc);

      doc.FinalizeDocumentation();
    }
    // otherwise get documentation from every single class file
    else
    {
      if (!library->LoadFromDoxygenDirectory(config.mDoxygenPath))
      {
        Error("Unable to load doxygen files at location: %s", config.mDoxygenPath.c_str());
      }
    }
    // if we arn't loading a typedef file, load from doxygen
    if (!config.mLoadTypedefs)
    {
      tdLibrary.LoadTypedefsFromDocLibrary(*library);
    }
    library->Build();
    library->FillOverloadDescriptions();
  }
  // if we were passed in already existing raw documentation
  else if (!config.mRawDocDirectory.empty())
  {
    library = new RawDocumentationLibrary;

    // load from documentation directory
    library->LoadFromDocumentationDirectory(config.mRawDocDirectory);
    library->Build();

    if (config.mLoadTypedefs)
      tdLibrary.LoadFromFile(config.mTypedefLibraryFile);
    else
      tdLibrary.LoadTypedefsFromDocLibrary(*library);
  }

  if (config.mLoadTypedefsFromDoxygen)
  {
    tdLibrary.mIgnoreList.mDoxyPath = config.mDoxygenPath;

    if (!config.mIgnoreFile.empty())
    {
      if (!Zero::LoadFromDataFile(tdLibrary.mIgnoreList, config.mIgnoreFile))
      {
        Error("Unable to load doc file at: %s", config.mIgnoreFile.c_str());
      }
      tdLibrary.mIgnoreList.SortList();
    }

    tdLibrary.LoadTypedefsFromNamespaceDocumentation(config.mDoxygenPath);
  }

  tdLibrary.BuildMap();

  tdLibrary.ExpandAllTypedefs();

  if (library)
  {
    if (config.mReplaceTypes)
    {
      library->NormalizeAllTypes(&tdLibrary);
    }

    if (config.mTagAllAsUnbound)
    {
      for (uint i = 0; i < library->mClasses.size(); ++i)
      {
        library->mClasses[i]->mTags.push_back("Non-Zilch");
      }
    }

    if (config.mOutputDirectory.size())
    {
      // output library and typedefs
      library->GenerateCustomDocumentationFiles(config.mOutputDirectory);
      tdLibrary.GenerateTypedefDataFile(config.mOutputDirectory);
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

  if (!config.mTrimmedTypedefFile.empty())
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

  trimLib.LoadFromMeta();

  trimLib.FinalizeDocumentation();

  WriteLog("saving Trimmed Documentation File as: %s\n", config.mTrimmedOutput.c_str());

  SaveToDataFile(trimLib, config.mTrimmedOutput);

  if (config.mVerbose)
    OutputListOfObjectsWithoutDesc(trimLib);
}

}//namespace Zero


int main(int argc, char* argv[])
{
  Zero::InitializeTokens();

  printf("Raw Documentation Generator\n");

  Zero::StringMap params;
  Zero::ParseCommandLine(params, (cstr*)argv, argc);

  forRange(auto& entry, params.all())
  {
      printf("  %s : %s\n", entry.first.c_str(), entry.second.c_str());
  }
  printf("\n");

  Zero::DocGeneratorConfig config = LoadConfigurations(params);

  // if the help flag is passed, ignore everything else and just print the help info
  if (config.mHelp || !Zero::ValidateConfig(config))
  {
    Zero::PrintHelp();
    return 0;
  }

  Zero::RunDocumentationGenerator(config);

  if (!config.mMarkupDirectory.empty())
    WriteOutMarkup(config);

  if (!config.mCommandListFile.empty())
  {
    WriteCommandReference(config);
  }

  return 0;
}
