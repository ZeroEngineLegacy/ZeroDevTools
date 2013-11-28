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

typedef HashMap<String, Array<ClassDoc*> > DocToTags;
typedef DocToTags::range DocRange;

void WriteTagIndices(String outputDir, DocToTags& tagged)
{
  DocRange r = tagged.all();

  StringBuilder markup;
  markup << "Code Index\n";
  markup << "=================================" << "\n\n";


  markup << "Vector Math\n";
  markup << "----------------------------------\n\n";
  markup << "*  :doc:`Reference/Vector`\n";
  markup << "*  :doc:`Reference/Quat`\n\n";

  for(;!r.empty();r.popFront())
  {
    Array<ClassDoc*>& stuff = r.front().second;
    sort(stuff.all());

    String tag = r.front().first;

    markup << tag << "\n";
    markup << "----------------------------------" << "\n\n";

    forRange(ClassDoc* doc, stuff.all())
    {
        markup << String::Format("*  :doc:`Reference/%s`\n", doc->Name.c_str(), doc->Name.c_str());
    }

    markup << "\n";
  }

  String fileName = BuildString(outputDir, "../CodeIndex.rst");

  String text = markup.ToString();

  WriteStringRangeToFile(fileName, text);
}


void WriteClass(String directory, ClassDoc& classDoc, DocToTags& tagged)
{
  //printf("Class %s\n", classDoc.Name.c_str());

  forRange(String tag, classDoc.Tags.all())
    tagged[tag].push_back(&classDoc);
  
  if(classDoc.Tags.empty())
    tagged["Various"].push_back(&classDoc);

  String outputDir = directory;

  StringBuilder classMarkup;

  classMarkup << classDoc.Name << "\n";
  classMarkup << "=================================" << "\n";
    
  classMarkup << ".. cpp:type:: " << classDoc.Name << "\n\n";
  
  classMarkup << classDoc.Description << "\n\n";

  if(!classDoc.BaseClass.empty())
    classMarkup << "\tBase Class: " << ":cpp:type:`" << classDoc.BaseClass << "`" << "\n\n";

  forRange(PropertyDoc& propertyDoc, classDoc.Properties.all())
  {
    if(propertyDoc.Name == "Name" && propertyDoc.Description.empty())
      continue;

    classMarkup << "\t" << ".. cpp:member:: " << propertyDoc.Type <<  " " << propertyDoc.Name << "\n\n";
    classMarkup << "\t" << propertyDoc.Description << "\n\n";
  }

  forRange(MethodDoc& methodDoc, classDoc.Methods.all())
  {
    classMarkup << "\t" <<  ".. cpp:function:: " << methodDoc.ReturnValue << " "  
      << methodDoc.Name << methodDoc.Arguments << "\n\n";

    classMarkup << "\t" << methodDoc.Description << "\n\n";
  }

  Zero::CreateDirectoryAndParents(outputDir);

  String fileName = BuildString(outputDir, classDoc.Name , ".rst");

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
  DocToTags tagged;

  //Now load the documentation file (the documentation for all the classes)
  if(!FileExists(config.DocumentationFile.c_str()))
  {
    printf("%s does not exist.",config.DocumentationFile.c_str());
    return;
  }

  //Upload the class' page to the wiki, making sure to perform the link replacements
  forRange(ClassDoc& classDoc, doc.Classes.all())
  {
    WriteClass(directory, classDoc,  tagged);
  }

  WriteTagIndices(directory, tagged);
}

}
