#include "Precompiled.hpp"
#include "Engine/EngineContainers.hpp"
#include "DocConfiguration.hpp"
#include "Platform/FileSystem.hpp"
#include "Engine/Documentation.hpp"
#include "Serialization/Simple.hpp"
#include "Serialization/Text.hpp"
#include "Pages.hpp"
#include "Support/FileSupport.hpp"
#include "Platform/FilePath.hpp"

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
        markup << String::Format("*  :doc:`Reference/%s`\n", doc->mName.c_str(), doc->mName.c_str());
    }

    markup << "\n";
  }

  String fileName = FilePath::Combine(outputDir, "CodeIndex.rst");

  String text = markup.ToString();

  WriteStringRangeToFile(fileName, text);
}


void WriteClass(String directory, ClassDoc& classDoc,DocumentationLibrary &lib, DocToTags& tagged)
{

  if ( classDoc.mName.Contains("<")
    || classDoc.mName.Contains(">")
    || classDoc.mName.Contains("!")
    || classDoc.mName.Contains("\'")
    || classDoc.mName.Contains("\"")
    || classDoc.mName.empty()
    )
    return;
  //printf("Class %s\n", classDoc.Name.c_str());

  forRange(String tag, classDoc.mTags.all())
    tagged[tag].push_back(&classDoc);
  
  if(classDoc.mTags.empty())
    tagged["Various"].push_back(&classDoc);

  String outputDir = directory;

  StringBuilder classMarkup;

  classMarkup << classDoc.mName << "\n";
  classMarkup << "=================================" << "\n";
    
  classMarkup << ".. cpp:type:: " << classDoc.mName << "\n\n";
  
  classMarkup << classDoc.mDescription << "\n\n";

  if(!classDoc.mBaseClass.empty())
    classMarkup << "\tBase Class: " << ":cpp:type:`" << classDoc.mBaseClass << "`" << "\n\n";

  classMarkup << "Properties\n----------\n";

  ClassDoc *IterClass = &classDoc;

  while (IterClass)
  {
    forRange(PropertyDoc* propertyDoc, IterClass->mProperties.all())
    {
      if (propertyDoc->mName == "Name" && propertyDoc->mDescription.empty())
        continue;

      classMarkup << "\t" << ".. cpp:member:: " << propertyDoc->mType << " " << propertyDoc->mName << "\n\n";
      classMarkup << "\t" << propertyDoc->mDescription << "\n\n";
    }
    if (!IterClass->mBaseClass.empty() && lib.mClassMap.containsKey(IterClass->mBaseClass))
    {
      IterClass = lib.mClassMap[IterClass->mBaseClass];

      if (!IterClass || IterClass->mProperties.empty())
      {
        continue;
      }

      classMarkup << "\n^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n"
        << "Properties From: :cpp:type:`" << IterClass->mName
        << "`\n^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n";
    }
    else
    {
      IterClass = nullptr;
    }
  }

  IterClass = &classDoc;

  classMarkup << "Methods\n----------\n";

  while (IterClass)
  {
    forRange(MethodDoc* methodDoc, IterClass->mMethods.all())
    {
      classMarkup << "\t" << ".. cpp:function:: " << methodDoc->mReturnType << " "
        << methodDoc->mName << methodDoc->mParameters << "\n\n";

      classMarkup << "\t" << methodDoc->mDescription << "\n\n";
    }
    if (!IterClass->mBaseClass.empty() && lib.mClassMap.containsKey(IterClass->mBaseClass))
    {
      IterClass = lib.mClassMap[IterClass->mBaseClass];

      if (!IterClass || IterClass->mMethods.empty())
      {
        continue;
      }

      classMarkup << "\n^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n"
        << "Methods From: :cpp:type:`" << IterClass->mName
        << "`\n^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n";      
    }
    else
    {
      IterClass = nullptr;
    }
  }

  Zero::CreateDirectoryAndParents(outputDir);

  String filename = BuildString(classDoc.mName , ".rst");

  String fullPath = FilePath::Combine(outputDir, "Reference");

  CreateDirectoryAndParents(fullPath);

  fullPath = FilePath::Combine(fullPath, filename);

  fullPath = FilePath::Normalize(fullPath);

  String text = classMarkup.ToString();


  WriteStringRangeToFile(fullPath, text);

}

void WriteOutMarkup(Zero::DocGeneratorConfig& config)
{
  printf("Mark up\n");

  //Now load the documentation file (the documentation for all the classes)
  if (!FileExists(config.mTrimmedOutput.c_str()))
  {
    printf("%s does not exist.", config.mTrimmedOutput.c_str());
    return;
  }
  StringRef directory = config.mMarkupDirectory;

  CreateDirectoryAndParents(directory);

  DocumentationLibrary doc;
  LoadFromDataFile(doc, config.mTrimmedOutput);
  doc.FinalizeDocumentation();

  DocToTags tagged;

  //Upload the class' page to the wiki, making sure to perform the link replacements
  forRange(ClassDoc* classDoc, doc.mClasses.all())
  {
    WriteClass(directory, *classDoc, doc,  tagged);
  }

  WriteTagIndices(directory, tagged);
}

}
