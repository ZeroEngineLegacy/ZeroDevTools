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

  ClassDoc *IterClass = &classDoc;

  if (IterClass->mProperties.size())
    classMarkup << "Properties\n----------\n";

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

      // The tilde are there so we create a shortened link to the class DO NOT REMOVE
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

  if (IterClass->mMethods.size())
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

      // The tilde are there so we create a shortened link to the class DO NOT REMOVE
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


void WriteCommandReference(Zero::DocGeneratorConfig& config)
{
  //StringParam loadFilePath, StringParam outputPath

  String filePath = BuildString(config.mMarkupDirectory, "\\CommandRef.rst");

  // get outta here with that nonexistent file
  if (!FileExists(config.mCommandListFile.c_str()))
  {
    printf("%s does not exist.", config.mCommandListFile.c_str());
    return;
  }

  CreateDirectoryAndParents(config.mMarkupDirectory);

  // actually load command list now. (If this fails it probably means the file is mis-formatted)
  CommandDocList commandListDoc;
  LoadFromDataFile(commandListDoc, config.mCommandListFile);

  Array<CommandDoc *> &commandList = commandListDoc.mCommands;

  // do the fancy string building to put this markup file together
  StringBuilder markupText;

  markupText << "Zero Commands\n";
  markupText << "=================================\n";

  for (uint i = 0; i < commandList.size(); ++i)
  {
    CommandDoc *cmdDoc = commandList[i];

    markupText << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n";
    markupText << cmdDoc->mName << "\n";
    markupText << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n";

    if (!cmdDoc->mTags.empty())
    {
      markupText << "**Tags:** " << cmdDoc->mTags[0];

      for (uint j = 1; j < cmdDoc->mTags.size(); ++j)
      {
        markupText << ", " << cmdDoc->mTags[j];
      }
      markupText << "\n\n";
    }

    if (!cmdDoc->mShortcut.empty())
      markupText << "**Shortcut:** " << cmdDoc->mShortcut << "\n\n";

    markupText << cmdDoc->mDescription << "\n\n";

    markupText << "\n\n\n";
  }

  WriteStringRangeToFile(filePath, markupText.ToString());
}

// this output will not be perfect since there is no way to get ALL send and receive
void WriteEventList(Zero::DocGeneratorConfig& config)
{
  String filePath = BuildString(config.mMarkupDirectory, "\\EventList.rst");

  // get outta here with that nonexistent file
  if (!FileExists(config.mEventsOutputLocation.c_str()))
  {
    printf("%s does not exist.", config.mEventsOutputLocation.c_str());
    return;
  }

  CreateDirectoryAndParents(config.mMarkupDirectory);

  // actually load event list now. (If this fails it probably means the file is mis-formatted)
  EventDocList eventListDoc;
  LoadFromDataFile(eventListDoc, config.mEventsOutputLocation);

  Array<EventDoc *> &eventList = eventListDoc.mEvents;

  // do the fancy string building to put this markup file together
  StringBuilder markupText;

  markupText << "Zero Events\n";
  markupText << "=================================\n";

  for (uint i = 0; i < eventList.size(); ++i)
  {
    EventDoc *eventDoc = eventList[i];


    markupText << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n";
    markupText << eventDoc->mName << "\n";
    markupText << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n";
    markupText << "**Type:** " << eventDoc->mType << "\n\n";

    if (!eventDoc->mSenders.empty())
    {
      markupText << "**Senders:** \n\n";

      for (uint j = 0; j < eventDoc->mSenders.size(); ++j)
      {
        markupText << eventDoc->mSenders[j] << " \n\n";
      }
      markupText << "\n\n";
    }

    if (!eventDoc->mListeners.empty())
    {
      markupText << "**Listeners:** \n\n";

      for (uint j = 0; j < eventDoc->mListeners.size(); ++j)
      {
        markupText << eventDoc->mListeners[j] << " \n\n";
      }
      markupText << "\n\n";
    }

    markupText << "\n\n\n";
  }

  WriteStringRangeToFile(filePath, markupText.ToString());
}

// this output will not be perfect since there is no way to get ALL send and receive
void WriteExceptionList(Zero::DocGeneratorConfig& config)
{
  String filePath = BuildString(config.mMarkupDirectory, "\\ExceptionList.rst");

  // get outta here with that nonexistent file
  if (!FileExists(config.mExceptionsFile.c_str()))
  {
    printf("%s does not exist.", config.mExceptionsFile.c_str());
    return;
  }

  CreateDirectoryAndParents(config.mMarkupDirectory);

  // actually load event list now. (If this fails it probably means the file is mis-formatted)
  ExceptionDocList exceptionListDoc;
  LoadFromDataFile(exceptionListDoc, config.mExceptionsFile);

  Array<ExceptionDoc> &exceptionList = exceptionListDoc.mExceptions;


  // do the fancy string building to put this markup file together
  StringBuilder markupText;

  markupText << "Zero Exceptions\n";
  markupText << "=================================\n";

  for (uint i = 0; i < exceptionList.size(); ++i)
  {
    ExceptionDoc &exceptDoc = exceptionList[i];

    markupText << "**Class:** " << exceptDoc.mClass <<"\n\n";

    markupText << "**Function:** " << exceptDoc.mFunction << "\n\n";

    markupText << "**Message:** " << exceptDoc.mMsg << "\n\n";
    markupText << "\n\n\n";
  }

  WriteStringRangeToFile(filePath, markupText.ToString());
}

}
