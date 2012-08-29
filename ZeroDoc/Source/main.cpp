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

namespace Zero
{

struct WikiUpdatePage
{
  WikiUpdatePage() {}
  WikiUpdatePage(StringParam page, StringParam parent)
  {
    mPageToUpdate = page;
    mParentPage = parent;
  }

  void Serialize(Serializer& stream)
  {
    SerializeName(mPageToUpdate);
    SerializeName(mParentPage);
  }

  String mPageToUpdate;
  String mParentPage;
};

void PushToWiki(StringMap& params)
{
  String sourcePath = GetStringValue<String>(params,"sourcePath","C:\\Zero\\");
  sourcePath = NormalizePath(sourcePath);

  bool verbose = GetStringValue<bool>(params,"verbose",false);

  //there's a set list of classes we want to push to the wiki, load this list from a data file
  Array<WikiUpdatePage> pagesToUpdate;
  TextLoader stream;
  String pagesToUpdatePath = BuildString(sourcePath.c_str(),"DevTools\\PagesToUpdate.txt");
  if(!FileExists(pagesToUpdatePath))
  {
    printf("%s does not exist.",pagesToUpdatePath.c_str());
    return;
  }
  stream.Open(pagesToUpdatePath.c_str());
  SerializeName(pagesToUpdate);
  stream.Close();

  //log onto the wiki and get our token to use for further operations
  Zero::String token;
  LogOn(token,verbose);

  //extract the wiki article names and their indices from the wiki
  StringMap wikiIndices;
  GetWikiArticleIds(token,wikiIndices,verbose);

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
        bool success = CreateWikiPage(token,pageName,parentIndex,WikiIndex,verbose);
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

  
  //load the documentation file
  String documentationPath = BuildString(sourcePath.all(),"\\Projects\\Editor\\");
  documentationPath = NormalizePath(documentationPath);
  String datafile = BuildString(documentationPath.c_str(),"Documentation.data");
  Zero::DocumentationLibrary doc;
  Zero::LoadFromDataFile(doc, datafile);
  doc.Build();

  ///upload the class' page to the wiki, making sure to perform the link replacements
  forRange(ClassDoc& classDoc, doc.Classes.all())
  {
    String index = WikiIndex.findValue(classDoc.Name, "");
    if(!index.empty())
    {
      //should uncomment when testing (so we don't accidentally destroy the whole wiki)
      //if(classDoc.Name == "DynamicController")
      UploadClassDoc(index, classDoc, classReplaceMents, token, verbose);
    }
    else
    {
      //fprintf(f,"need page %s\r\n", classDoc.Name.c_str() );
      //printf("need page %s\n", classDoc.Name.c_str());
    }
  }

  LogOff(token,verbose);
}

void ParseAndSaveDocumentation(StringMap& params)
{
  //get the path to the source and to the documentation
  String sourcePath = GetStringValue<String>(params,"sourcePath","C:\\Zero\\");
  sourcePath = NormalizePath(sourcePath);
  String documentationPath = BuildString(sourcePath.all(),"\\Projects\\Editor\\");
  documentationPath = NormalizePath(documentationPath);
  //get the path to the doxygen file
  String doxygenPath = GetStringValue<String>(params,"doxyPath","C:\\ZeroDoxygen\\");
  doxygenPath = NormalizePath(doxygenPath);
  
  //load the raw documentation (before merging with doxy) and load into a document
  String datafile = BuildString(documentationPath.c_str(),"DocumentationRaw.data");
  String output = BuildString(documentationPath.c_str(),"Documentation.data");

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
    ExtractMethodDocs( classDoc , doc, doxygenPath, symbolReplacements);
  }

  //save the merged file back into the output file
  SaveToDataFile(doc, output);
}

}

int main(int argc, char* argv[])
{
  Zero::StringMap params;
  Zero::ParseCommandLine(params,(Zero::cstr*)argv,argc);

  if(Zero::GetStringValue<bool>(params,"parse",false))
    Zero::ParseAndSaveDocumentation(params);
  if(Zero::GetStringValue<bool>(params,"wiki",false))
    Zero::PushToWiki(params);
}
