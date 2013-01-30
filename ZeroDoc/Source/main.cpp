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
  String EventsFile;
  
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
  //Load the list of events
  config.EventsFile = BuildString(config.DocumentationPath.c_str(), "EventList.data");
  //get the path to the doxygen file
  config.DoxygenPath = GetStringValue<String>(params,"doxyPath","C:\\ZeroDoxygen\\");
  config.DoxygenPath = NormalizePath(config.DoxygenPath);
  //get the path to the documentation folder (where the data files for documentation are)
  config.DocumentationRoot = BuildString(config.SourcePath.c_str(),"DevTools\\Documentation\\");
  
  return config;
}

void UpdateEventList(StringMap& params, Replacements& replacements, 
                     StringBuilder* outputString)
{
  Configurations config = LoadConfigurations(params);

  Array<EventEntry> eventsToUpdate;
  TextLoader stream;
  String eventsToUpdatePath = config.EventsFile;
  if(!FileExists(eventsToUpdatePath.c_str()))
  {
    printf("%s does not exist.", eventsToUpdatePath.c_str());
    return;
  }
  stream.Open(eventsToUpdatePath.c_str());
  SerializeName(eventsToUpdate);
  stream.Close();

  //Create the HTML for page that hosts the list of events.
  String eventHeader("");
  StringBuilder eventListHtml;
  outputString->Append("<p>");
  forRange(Array<EventEntry>::value_type& e, eventsToUpdate.all())
  {
    if(eventHeader != e.mEventType)
    {
      if(eventHeader != "") outputString->Append("</ul>");
      eventHeader = e.mEventType;
      outputString->Append("<h3>");
      Replace(*outputString, replacements, e.mEventType);
      outputString->Append("</h3>");
      outputString->Append("<ul>");
    }
    outputString->Append("<li>");
    Replace(*outputString, replacements, e.mEventName);
    outputString->Append("</li>");

  }
  eventListHtml.Append("</p>");

  
}

void PushToWiki(StringMap& params)
{
  Configurations config = LoadConfigurations(params);

  //There's a set list of classes we want to push to the wiki, load this list from a data file
  Array<WikiUpdatePage> pagesToUpdate;
  TextLoader stream;
  String pagesToUpdatePath = BuildString(config.SourcePath.c_str(), 
                                         "DevTools\\PagesToUpdate.txt");
  if(!FileExists(pagesToUpdatePath))
  {
    printf("%s does not exist.",pagesToUpdatePath.c_str());
    return;
  }
  stream.Open(pagesToUpdatePath.c_str());
  SerializeName(pagesToUpdate);
  stream.Close();

  //Now load the documentation file (the documentation for all the classes)
  if(!FileExists(config.DocumentationFile.c_str()))
  {
    printf("%s does not exist.",config.DocumentationFile.c_str());
    return;
  }
  Zero::DocumentationLibrary doc;
  Zero::LoadFromDataFile(doc, config.DocumentationFile);
  doc.Build();

  //Warn for classes that have documentation but are not parked to push to the wiki
  WarnNeedsWikiPage(pagesToUpdate, doc.Classes, config.DoxygenPath, 
                    config.DocumentationRoot, config.Verbose, config.Log);

  //Log onto the wiki and get our token to use for further operations
  Zero::String token;
  LogOn(token,config.Verbose);

  //Extract the wiki article names and their indices from the wiki
  StringMap wikiIndices;
  GetWikiArticleIds(token, wikiIndices, config.Verbose);

  //Create a map of the wiki names and ids for only what we want to push to the wiki.
  typedef StringMap WikiMap;
  WikiMap wikiIndex;
  for(uint i = 0; i < pagesToUpdate.size(); ++i)
  {
    String pageName = pagesToUpdate[i].mPageToUpdate;
    String parentPageName = pagesToUpdate[i].mParentPage;
    String index = wikiIndices.findValue(pageName,"");
    if(!index.empty())
      wikiIndex[pageName] = index;
    else
    {
      String parentIndex = wikiIndices.findValue(parentPageName,"");
      if(!parentIndex.empty())
      {
        bool success = CreateWikiPage(token,pageName,parentIndex,wikiIndex,config.Verbose);
        if(!success)
          printf("Failed to find wiki article for \"%s\" with parent \"%s\"\n",
                 pageName.c_str(), parentPageName.c_str());
        else
          printf("Creating wiki page for article \"%s\"\n",pageName.c_str());
      }
      else
        printf("Failed to find wiki parent article \"%s\" for child page \"%s\"\n",
               parentPageName.c_str(), pageName.c_str());
    }
  }

  //set up a replacement for class names to replace in the wiki, this will set
  //up links on the page to other classes when they are referenced
  Array<Replacement> classReplacements;
  WikiMap::range r = wikiIndex.all();
  forRange(WikiMap::value_type& v, r)
  {
    String name = BuildString(v.first, "");
    String link = String::Format("<a class=\"uvb\" href=\"default.asp?W%s\">%s</a>", 
                                 v.second.c_str(), v.first.c_str());
    classReplacements.push_back(Replacement(name, link));
  }
  sort(classReplacements.all());

  //Update all of the events on the event page, link relevant pages to them.
  StringBuilder eventList;
  UpdateEventList(params, classReplacements, &eventList);
  const String eventListPageTitle("Event List");
  const String eventListPage = wikiIndices[eventListPageTitle.c_str()];
  UploadPageContent(eventListPage, eventListPageTitle, eventList.ToString(), 
                    token, config.Verbose);

  //Upload the class' page to the wiki, making sure to perform the link replacements
  forRange(ClassDoc& classDoc, doc.Classes.all())
  {
    String index = wikiIndex.findValue(classDoc.Name, "");
    if(!index.empty())
    {
      //should uncomment when testing (so we don't accidentally destroy the whole wiki)
      //if(classDoc.Name == "PhysicsEffect")
        UploadClassDoc(index, classDoc, classReplacements, token, config.Verbose);
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
    ExtractMethodDocs(classDoc, doc, config.DoxygenPath, symbolReplacements);
  }

  String extraDocPath = BuildString(config.DocumentationRoot.c_str(),"ExtraDocumentation.txt");
  LoadAndReplaceDocumentation(extraDocPath,doc,symbolReplacements);

  //save the merged file back into the output file
  SaveToDataFile(doc, output);

  WarnAndLogUndocumented(doc.Classes, config.DoxygenPath, config.DocumentationRoot,
                         config.Verbose, config.Log);
}

}//namespace Zero

int main(int argc, char* argv[])
{
  Zero::StringMap params;
  Zero::ParseCommandLine(params, (Zero::cstr*)argv, argc);

  if(Zero::GetStringValue<bool>(params, "parse", false))
    Zero::ParseAndSaveDocumentation(params);
  if(Zero::GetStringValue<bool>(params, "wiki", false))
    Zero::PushToWiki(params);
}
