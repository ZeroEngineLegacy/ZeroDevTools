#include "Precompiled.hpp"
#include "Engine/EngineContainers.hpp"
#include "Configuration.hpp"
#include "Platform/FileSystem.hpp"
#include "Engine/Documentation.hpp"
#include "Serialization/Simple.hpp"
#include "Serialization/Text.hpp"
#include "Pages.hpp"
#include "Support/FileSupport.hpp"

namespace Zero
{

void WriteClass(String directory, ClassDoc& classDoc)
{
  //printf("Class %s\n", classDoc.Name.c_str());

  StringBuilder classMarkup;

  classMarkup << classDoc.Name << "\n";
  classMarkup << "=================================" << "\n";
    
  classMarkup << ".. cpp:type:: " << classDoc.Name << "\n\n";
  
  classMarkup << classDoc.Description << "\n\n";

  forRange(PropertyDoc& propertyDoc, classDoc.Properties.all())
  {
    classMarkup << "\t" << ".. cpp:member:: " << propertyDoc.Type <<  " " << propertyDoc.Name << "\n\n";
    classMarkup << "\t" << propertyDoc.Description << "\n\n";
  }

  forRange(MethodDoc& methodDoc, classDoc.Methods.all())
  {
    classMarkup << "\t" <<  ".. cpp:function:: " << methodDoc.ReturnValue << " "  
      << methodDoc.Name << methodDoc.Arguments << "\n\n";

    classMarkup << "\t" << methodDoc.Description << "\n\n";
  }

  String fileName = BuildString(directory, classDoc.Name , ".rst");

  String text = classMarkup.ToString();

  WriteStringRangeToFile(fileName, text);

}

void WriteOutMarkup(StringMap& params)
{
  printf("Mark up\n");

  Configurations config = LoadConfigurations(params);

  DocumentationLibrary doc;
  LoadFromDataFile(doc, config.DocumentationFile);
  doc.Build();

  String directory = GetStringValue<String>(params, "output", "Markup");

  Zero::CreateDirectoryAndParents(directory);

  //Now load the documentation file (the documentation for all the classes)
  if(!FileExists(config.DocumentationFile.c_str()))
  {
    printf("%s does not exist.",config.DocumentationFile.c_str());
    return;
  }

  //Upload the class' page to the wiki, making sure to perform the link replacements
  forRange(ClassDoc& classDoc, doc.Classes.all())
  {
    WriteClass(directory, classDoc);
  }
}

}