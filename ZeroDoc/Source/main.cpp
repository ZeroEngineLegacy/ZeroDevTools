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
#include "Configuration.hpp"
#include "MarkupWriter.hpp"
#include "Configuration.hpp"

namespace Zero
{




void ParseAndSaveDocumentation(StringMap& params)
{
  printf("ParseAndSaveDocumentation\n");

  Configurations config = LoadConfigurations(params);

  //load the raw documentation (before merging with doxy) and load into a document
  String datafile = config.DocumentationRawFile;
  String output = config.DocumentationFile;

  if(!FileExists(datafile.c_str()))
  {
    printf("%s does not exist.", datafile.c_str());
    return;
  }

  Zero::DocumentationLibrary doc;
  Zero::LoadFromDataFile(doc, datafile);
  doc.Build();

  //for the wiki and for cleanliness we replace certain symbols that are used internally
  Array<Replacement> symbolReplacements;
  symbolReplacements.push_back(Replacement("Vec2Param", "Vec2"));
  symbolReplacements.push_back(Replacement("Vec3Param", "Vec3"));
  symbolReplacements.push_back(Replacement("Vec4Param", "Vec4"));
  symbolReplacements.push_back(Replacement("real", "float"));
  symbolReplacements.push_back(Replacement("StringRef", "String"));
  symbolReplacements.push_back(Replacement("StringParam", "String"));
  symbolReplacements.push_back(Replacement("*", ""));
  symbolReplacements.push_back(Replacement("override", ""));
  sort(symbolReplacements.all());

  HashMap<String, ClassDoc> dataBase;
  //now extract the doxygen data into the document
  forRange(ClassDoc& classDoc, doc.Classes.all())
  {
    ExtractMethodDocs(classDoc, dataBase, doc, config.DoxygenPath, symbolReplacements);
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
  printf("Zero Documentation Generator\n");

  Zero::StringMap params;
  Zero::ParseCommandLine(params, (Zero::cstr*)argv, argc);

  forRange(auto& entry, params.all())
  {
      printf("  %s : %s\n", entry.first.c_str(), entry.second.c_str());
  }

  if(Zero::GetStringValue<bool>(params, "parse", false))
    Zero::ParseAndSaveDocumentation(params);

  if(Zero::GetStringValue<bool>(params, "markup", false))
    WriteOutMarkup(params);

  if(Zero::GetStringValue<bool>(params, "wiki", false))
    Zero::PushToWiki(params);
}
