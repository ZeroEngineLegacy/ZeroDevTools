#include "Precompiled.hpp"

#include "DebugPrint.hpp"

#include "Engine/Documentation.hpp"
#include "Engine/EngineContainers.hpp"
#include "Serialization/Simple.hpp"
#include "Serialization/Text.hpp"
#include "Support/StringMap.hpp"
#include "Utility/FilePath.hpp"

#include "WikiOperations.hpp"
#include "TinyXmlHelpers.hpp"
#include "Platform/FileSystem.hpp"
#include "Logging.hpp"
#include "ExtraDocumentation.hpp"

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
  
  bool Verbose;
  String Log;
};

Configurations LoadConfigurations(StringMap& params)
{
  Configurations config;
  config.Verbose = GetStringValue<bool>(params,"verbose",false);
  config.Log = GetStringValue<String>(params,"log","");

  //get the path to the source
  config.SourcePath = GetStringValue<String>(params,"sourcePath","C:\\Zero\\");
  config.SourcePath = NormalizePath(config.SourcePath);
  //get the path to the documentation
  config.DocumentationPath = BuildString(config.SourcePath.all(),"\\Projects\\Editor\\");
  config.DocumentationPath = NormalizePath(config.DocumentationPath);
  //load the raw documentation (before merging with doxy) and the documentation file
  config.DocumentationRawFile = BuildString(config.DocumentationPath.c_str(),"DocumentationRaw.data");
  config.DocumentationFile  = BuildString(config.DocumentationPath.c_str(),"Documentation.data");
  //get the path to the doxygen file
  config.DoxygenPath = GetStringValue<String>(params,"doxyPath","C:\\ZeroDoxygen\\");
  config.DoxygenPath = NormalizePath(config.DoxygenPath);
  //get the path to the documentation folder (where the data files for documentation are)
  config.DocumentationRoot = BuildString(config.SourcePath.c_str(),"DevTools\\Documentation\\");
  
  return config;
}

void PushToWiki(StringMap& params)
{
  Configurations config = LoadConfigurations(params);

  //there's a set list of classes we want to push to the wiki, load this list from a data file
  Array<WikiUpdatePage> pagesToUpdate;
  TextLoader stream;
  String pagesToUpdatePath = BuildString(config.SourcePath.c_str(),"DevTools\\PagesToUpdate.txt");
  if(!FileExists(pagesToUpdatePath))
  {
    printf("%s does not exist.",pagesToUpdatePath.c_str());
    return;
  }
  stream.Open(pagesToUpdatePath.c_str());
  SerializeName(pagesToUpdate);
  stream.Close();

  //now load the documentation file (the documentation for all the classes)
  if(!FileExists(config.DocumentationFile.c_str()))
  {
    printf("%s does not exist.",config.DocumentationFile.c_str());
    return;
  }
  Zero::DocumentationLibrary doc;
  Zero::LoadFromDataFile(doc, config.DocumentationFile);
  doc.Build();
  //warn for classes that have documentation but are not parked to push to the wiki
  WarnNeedsWikiPage(pagesToUpdate,doc.Classes,config.DoxygenPath,config.DocumentationRoot,config.Verbose,config.Log);

  //log onto the wiki and get our token to use for further operations
  Zero::String token;
  LogOn(token,config.Verbose);

  //extract the wiki article names and their indices from the wiki
  StringMap wikiIndices;
  GetWikiArticleIds(token,wikiIndices,config.Verbose);

  //create a map of the wiki names and ids for only what we want to push to the wiki
  typedef StringMap WikiMap;
  WikiMap WikiIndex;
  for(uint i = 0; i < pagesToUpdate.size(); ++i)
  {
    String pageName = pagesToUpdate[i].mPageToUpdate;
    String parentPageName = pagesToUpdate[i].mParentPage;
    String index = wikiIndices.findValue(pageName,"");
    if(!index.empty())
      WikiIndex[pageName] = index;
    else
    {
      String parentIndex = wikiIndices.findValue(parentPageName,"");
      if(!parentIndex.empty())
      {
        bool success = CreateWikiPage(token,pageName,parentIndex,WikiIndex,config.Verbose);
        if(!success)
          printf("Failed to find wiki article for \"%s\" with parent \"%s\"\n",pageName.c_str(),parentPageName.c_str());
        else
          printf("Creating wiki page for article \"%s\"\n",pageName.c_str());
      }
      else
        printf("Failed to find wiki parent article \"%s\" for child page \"%s\"\n",parentPageName.c_str(),pageName.c_str());
    }
  }

  //set up a replacement for class names to replace in the wiki, this will set
  //up links on the page to other classes when they are referenced
  Array<Replacement> classReplaceMents;
  WikiMap::range r = WikiIndex.all();
  forRange(WikiMap::value_type& v, r)
  {
    String name = BuildString(v.first, "");
    String link = String::Format( "<a class=\"uvb\" href=\"default.asp?W%s\">%s</a>", v.second.c_str(), v.first.c_str());
    classReplaceMents.push_back(Replacement(name, link));
  }
  sort(classReplaceMents.all());

  ///upload the class' page to the wiki, making sure to perform the link replacements
  forRange(ClassDoc& classDoc, doc.Classes.all())
  {
    String index = WikiIndex.findValue(classDoc.Name, "");
    if(!index.empty())
    {
      //should uncomment when testing (so we don't accidentally destroy the whole wiki)
      //if(classDoc.Name == "PhysicsEffect")
      UploadClassDoc(index, classDoc, classReplaceMents, token, config.Verbose);
    }
    else
    {
      //fprintf(f,"need page %s\r\n", classDoc.Name.c_str() );
      //printf("need page %s\n", classDoc.Name.c_str());
    }
  }

  LogOff(token,config.Verbose);
}

void ParseAndSaveDocumentation(StringMap& params)
{
  Configurations config = LoadConfigurations(params);

  //load the raw documentation (before merging with doxy) and load into a document
  String datafile = config.DocumentationRawFile;
  String output = config.DocumentationFile;

  if(!FileExists(datafile.c_str()) || !FileExists(output.c_str()))
  {
    printf("%s does not exist.",datafile.c_str());
    return;
  }

  Zero::DocumentationLibrary doc;
  Zero::LoadFromDataFile(doc, datafile);
  doc.Build();

  //for the wiki and for cleanliness we replace certain symbols that are used internally
  Array<Replacement> symbolReplacements;
  symbolReplacements.push_back(Replacement("Vec3Param", "Vec3"));
  symbolReplacements.push_back(Replacement("Vec4Param", "Vec4"));
  symbolReplacements.push_back(Replacement("real", "float"));
  symbolReplacements.push_back(Replacement("StringRef", "String"));
  symbolReplacements.push_back(Replacement("StringParam", "String"));
  symbolReplacements.push_back(Replacement("*", ""));
  symbolReplacements.push_back(Replacement("override", ""));
  sort(symbolReplacements.all());

  //now extract the doxygen data into the document
  forRange(ClassDoc& classDoc, doc.Classes.all())
  {
    ExtractMethodDocs( classDoc , doc, config.DoxygenPath, symbolReplacements);
  }

  String extraDocPath = BuildString(config.DocumentationRoot.c_str(),"ExtraDocumentation.txt");
  LoadAndReplaceDocumentation(extraDocPath,doc,symbolReplacements);

  //save the merged file back into the output file
  SaveToDataFile(doc, output);

  WarnAndLogUndocumented(doc.Classes,config.DoxygenPath,config.DocumentationRoot,config.Verbose,config.Log);
}

}//namespace Zero

int main(int argc, char* argv[])
{
  Zero::StringMap params;
  Zero::ParseCommandLine(params,(Zero::cstr*)argv,argc);

  if(Zero::GetStringValue<bool>(params,"parse",false))
    Zero::ParseAndSaveDocumentation(params);
  if(Zero::GetStringValue<bool>(params,"wiki",false))
    Zero::PushToWiki(params);
}
