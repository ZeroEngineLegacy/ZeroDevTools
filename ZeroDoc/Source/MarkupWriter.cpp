#include "Precompiled.hpp"
#include "Engine/EngineContainers.hpp"
#include "DocConfiguration.hpp"
#include "Platform/FileSystem.hpp"
#include "Engine/Documentation.hpp"
#include "Serialization/Simple.hpp"
#include "Support/FileSupport.hpp"
#include "Platform/FilePath.hpp"
#include "MarkupWriter.hpp"

/*
#include "Serialization/Text.hpp"
#include "Pages.hpp"
*/

namespace Zero
{
UnsortedMap<String, String> gClassLinkMap;
String gBaseLink = "Code_Reference/";

void WriteTagIndices(String outputDir, DocToTags& tagged, DocumentationLibrary &docLib)
{
  String fileName = FilePath::Combine(outputDir, "CodeIndex.rst");

  DocRange r = tagged.All();

  StringBuilder markup;
  markup << "Code Index\n";
  markup << "=================================" << "\n\n";


  markup << "Vector Math\n";
  markup << "----------------------------------\n\n";
  markup << "*  :doc:`Reference/Vector`\n";
  markup << "*  :doc:`Reference/Quat`\n\n";

  for(;!r.Empty();r.PopFront())
  {
    Array<ClassDoc*>& stuff = r.Front().second;
    Sort(stuff.All());

    String tag = r.Front().first;

    markup << tag << "\n";
    markup << "----------------------------------" << "\n\n";

    // create the reference list
    forRange(ClassDoc* doc, stuff.All())
    {
        markup << String::Format("*  :doc:`Reference/%s`\n", doc->mName.c_str(), doc->mName.c_str());
    }

    markup << "\n";
  }

  markup << "\
.. toctree::\n\
\t:hidden:\n\
\t:includehidden:\n\
\t:name: mastertoc\n\
\t:maxdepth: 1\n\
\t:titlesonly:\n\n";
  //output the doctree
  forRange(ClassDoc* doc, docLib.mClasses.All())
  {
    markup << "\tReference/" << doc->mName << "\n";
  }

  String text = markup.ToString();

  WriteStringRangeToFile(fileName, text);
}

void WriteOutAllReStructuredTextFiles(Zero::DocGeneratorConfig& config)
{
  printf("Mark up: ReStructuredText\n");

  // check if we are outputting class markup
  if (FileExists(config.mTrimmedOutput.c_str()))
  {
    StringRef directory = config.mMarkupDirectory;

    CreateDirectoryAndParents(directory);

    DocumentationLibrary doc;
    LoadFromDataFile(doc, config.mTrimmedOutput);
    doc.FinalizeDocumentation();

    DocToTags tagged;

    StringBuilder fileList;

    //Upload the class' page to the wiki, making sure to perform the link replacements
    forRange(ClassDoc* classDoc, doc.mClasses.All())
    {
      String filename = BuildString(classDoc->mName, ".rst");

      String fullPath = FilePath::Combine(directory, "Reference");

      CreateDirectoryAndParents(fullPath);

      fullPath = FilePath::Combine(fullPath, filename);

      fullPath = FilePath::Normalize(fullPath);

      RstClassMarkupWriter::WriteClass(fullPath, classDoc, doc, tagged);

      fileList << classDoc->mName << '\t' << FilePath::Combine(".", "Reference", filename) << '\n';
    }

    WriteStringRangeToFile(FilePath::Combine(directory, "classList.txt"), fileList.ToString());

    WriteTagIndices(directory, tagged, doc);
  }


  // check if we outputting commands
  if (!config.mCommandListFile.Empty())
  {
    String output = FilePath::Combine(config.mMarkupDirectory, "CommandRef.rst");
    output = FilePath::Normalize(output);
    RstCommandRefWriter::WriteCommandRef(config.mCommandListFile, output);
  }

  // check if we are outputing events
  if (!config.mEventsOutputLocation.Empty())
  {
    String output = FilePath::Combine(config.mMarkupDirectory, "EventList.rst");
    output = FilePath::Normalize(output);
    RstEventListWriter::WriteEventList(config.mEventsOutputLocation, output);
  }
}


////////////////////////////////////////////////////////////////////////
// Macro and static global Toolbox
////////////////////////////////////////////////////////////////////////
// start indent Section
#define StartIndentSection(x) do {++x.mCurrentIndentationLevel
// end indent section
#define EndIndentSection(x) --x.mCurrentIndentationLevel;}while(0)

// start header section
#define StartHeaderSection(x) if((0)){}else{++x.mCurrentSectionHeaderLevel;
// end header section
#define EndHeaderSection(x) --x.mCurrentSectionHeaderLevel;};

static const char *gBold("**");
static const char *gItalic("//");
static const char *gMonoSpaced("##");
static const char *gDeleted("~~");
static const char *gUnderlined("__");
static const char *gHighlighted("!!");
static const char *gBullet("* ");
static const char *gNumbered("# ");


////////////////////////////////////////////////////////////////////////
// BaseMarkupWriter
////////////////////////////////////////////////////////////////////////
void BaseMarkupWriter::IndentToCurrentLevel(void)
{
  mOutput << String::Repeat('\t', mCurrentIndentationLevel);
}

void BaseMarkupWriter::WriteOutputToFile(StringRef file)
{
  WriteStringRangeToFile(file, mOutput.ToString());
}

void BaseMarkupWriter::InsertNewUnderline(uint length, uint headerLevel)
{
  switch (headerLevel + mCurrentSectionHeaderLevel)
  {
  case 0:
    mOutput << String::Repeat('=', length);
    break;
  case 1:
    mOutput << String::Repeat('-', length);
    break;
  case 2:
    mOutput << String::Repeat('^', length);
    break;
  case 3:
    mOutput << String::Repeat('~', length);
    break;
    // will create purposfully invalid underline if invalid header level chosen
  default:
    mOutput << String::Repeat('?', length);
    break;
  }

  mOutput << "\n\n";
}

void BaseMarkupWriter::InsertNewSectionHeader(StringRef sectionName)
{
  mOutput << sectionName << "\n";
  InsertNewUnderline(sectionName.SizeInBytes());
}

////////////////////////////////////////////////////////////////////////
// RstClassMarkupWriter
////////////////////////////////////////////////////////////////////////
void RstClassMarkupWriter::WriteClass(
  StringParam outputFile,
  ClassDoc* classDoc,
  DocumentationLibrary &lib,
  DocToTags& tagged)
{
  // first things first, set up the tags for this class
  forRange(StringRef tag, classDoc->mTags.All())
  {
    tagged[tag].PushBack(classDoc);
  }

  // do the magic for getting directory and file
  RstClassMarkupWriter writer(classDoc->mName, classDoc);

  // top of the file
  writer.InsertClassRstHeader();
  StartHeaderSection(writer);

  // Properties
  writer.mOutput << ".. _Reference" << writer.mName << "Properties:\n\n";

  writer.InsertNewSectionHeader("Properties");

  forRange(PropertyDoc *prop, classDoc->mProperties.All())
  {
    writer.InsertProperty(*prop);
  }

  // Methods
  writer.mOutput << ".. _Reference" << writer.mName << "Methods:\n\n";

  writer.InsertNewSectionHeader("Methods");

  forRange(MethodDoc *method, classDoc->mMethods.All())
  {
    writer.InsertMethod(*method);
  }

  // bottom of the file
  EndHeaderSection(writer);
  writer.InsertClassRstFooter();

  writer.WriteOutputToFile(outputFile);
}

RstClassMarkupWriter::RstClassMarkupWriter(StringParam name, ClassDoc* classDoc) 
  : BaseMarkupWriter(name), mClassDoc(classDoc)
{
}

void RstClassMarkupWriter::InsertClassRstHeader(void)
{
  mOutput << ".. _Reference" << mName << ":\n\n" << ".. rst-class:: searchtitle\n\n";

  InsertNewSectionHeader(mName);

  mOutput << ".. rst-class:: searchdescripton\n\n"
    << mClassDoc->mDescription << "\n\n"
    << ".. include:: Description/Action.rst\n\n"
    << ".. cpp:class:: " << mName;

  if (mBases.Size() > 0)
  {
    mOutput << " : public " << mBases[0];

    for (uint i = 1; i < mBases.Size(); ++i)
    {
      mOutput << ", " << mBases[i];
    }
  }

  mOutput << "\n\n";
}

void RstClassMarkupWriter::InsertClassRstFooter(void)
{
  mOutput << ".. include:: Remarks/" << mName << ".rst\n";
}

void RstClassMarkupWriter::InsertMethod(MethodDoc &method)
{
  InsertCollapsibleSection();
  StartIndentSection((*this));

  IndentToCurrentLevel();

  mOutput << ".. cpp:function:: " << method.mReturnType << " " 
    << mName << "::" << method.mName << method.mParameters << "\n\n";

  if (!method.mDescription.Empty())
  {
    IndentToCurrentLevel();

    mOutput << "\t" << method.mDescription << "\n\n";
  }

  EndIndentSection((*this));
}

void RstClassMarkupWriter::InsertProperty(PropertyDoc &propDoc)
{
  //TODO: we need to figure out why so many things are not properly getting type
  if (propDoc.mType.Empty())
    return;

  InsertCollapsibleSection();
  StartIndentSection((*this));

  IndentToCurrentLevel();

  mOutput << ".. cpp:member:: " <<  propDoc.mType << " " << mName << "::" << propDoc.mName << "\n\n";

  if (!propDoc.mDescription.Empty())
  {
    IndentToCurrentLevel();

    mOutput << "\t" << propDoc.mDescription << "\n\n";
  }

  EndIndentSection((*this));
}


void RstClassMarkupWriter::InsertCollapsibleSection()
{
  // all functions that output anything besides headers will probably do this
  mOutput << ".. rst-class:: collapsible\n\n";
}


////////////////////////////////////////////////////////////////////////
// RstEventListWriter
////////////////////////////////////////////////////////////////////////
void RstEventListWriter::WriteEventList(StringRef eventListFilepath, StringRef outputPath)
{
  // load the file if we can

  // get outta here with that nonexistent file
  if (!FileExists(eventListFilepath))
  {
    printf("%s does not exist.", eventListFilepath.c_str());
    return;
  }

  // actually load event list now. (If this fails it probably means the file is mis-formatted)
  EventDocList eventListDoc;
  LoadFromDataFile(eventListDoc, eventListFilepath);

  Array<EventDoc *> &eventArray = eventListDoc.mEvents;

  // create the eventList
  RstEventListWriter writer("Event List");

  writer.InsertNewSectionHeader("Event List");

  // create top of file
  StartHeaderSection(writer);

  // iterate over all events and insert them
  forRange(EventDoc *eventDoc, eventArray.All())
  {
    writer.WriteEventEntry(eventDoc->mName, eventDoc->mType);
  }

  EndHeaderSection(writer);

  writer.WriteOutputToFile(outputPath);
}

RstEventListWriter::RstEventListWriter(StringParam name) : BaseMarkupWriter(name)
{
}

/*
// The tilde are there so we create a shortened link to the class DO NOT REMOVE
classMarkup << "\n^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n"
<< "Properties From: :cpp:type:`" << IterClass->mName
<< "`\n^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n";
*/

void RstEventListWriter::WriteEventEntry(StringParam eventEntry, StringParam type)
{
  InsertNewSectionHeader(eventEntry);

  // #ticks matter around links
  mOutput << ".. include:: EventListDescriptions/" << eventEntry << ".rst\n\n"
    << ":cpp:type:`" << type <<"`\n\n";
  //
}

////////////////////////////////////////////////////////////////////////
// RstCommandRefWriter
////////////////////////////////////////////////////////////////////////
void RstCommandRefWriter::WriteCommandRef(StringParam commandListFilepath, StringRef outputPath)
{
  // load the file
  // get outta here with that nonexistent file
  if (!FileExists(commandListFilepath))
  {
    printf("%s does not exist.", commandListFilepath.c_str());
    return;
  }

  // actually load event list now. (If this fails it probably means the file is mis-formatted)
  CommandDocList cmdListDoc;
  LoadFromDataFile(cmdListDoc, commandListFilepath);

  Array<CommandDoc *> &cmdArray = cmdListDoc.mCommands;

  // create the command writer
  RstCommandRefWriter writer("Zero Commands");

  // create top of file
  writer.InsertNewSectionHeader("Zero Commands");

  StartHeaderSection(writer);

  // iterate over all commands and insert them
  forRange(CommandDoc *cmdDoc, cmdArray.All())
  {
    writer.WriteCommandEntry(*cmdDoc);
  }

  EndHeaderSection(writer);

  writer.WriteOutputToFile(outputPath);
}

RstCommandRefWriter::RstCommandRefWriter(StringParam name) : BaseMarkupWriter(name)
{
}

void RstCommandRefWriter::WriteCommandEntry(const CommandDoc &cmdDoc)
{
  InsertNewSectionHeader(cmdDoc.mName);

  if (!cmdDoc.mDescription.Empty())
    mOutput << cmdDoc.mDescription << "\n\n";

  StartHeaderSection((*this));
  InsertNewSectionHeader("Tags");

  if (cmdDoc.mTags.Empty())
  {
    mOutput << "**No Tags**\n\n";
  }
  else
  {
    // actually list tags
    forRange(StringRef tag, cmdDoc.mTags.All())
    {
      mOutput << "*\t" << tag << "\n\n";
    }
  }

  InsertNewSectionHeader("Shortcut");

  if (cmdDoc.mShortcut.Empty())
  {
    mOutput << "**No Keyboard Shortcut**\n\n";
  }
  else
  {
    mOutput << "*\t" << cmdDoc.mShortcut << "\n\n";
  }

  mOutput << ".. include:: CommandPageExtensions/" << cmdDoc.mName << ".rst\n\n";

  EndHeaderSection((*this));
}


////////////////////////////////////////////////////////////////////////
// ReMarkup
////////////////////////////////////////////////////////////////////////
void WriteOutAllReMarkupFiles(Zero::DocGeneratorConfig& config)
{
  printf("Mark up: ReMarkup\n");

  // https://phab.digipen.edu/w/curriculum_development_and_documentation/rst_documentation/
  //       classname, link

  // check if we are outputting class markup
  if (FileExists(config.mTrimmedOutput.c_str()))
  {
    StringRef directory = config.mMarkupDirectory;

    CreateDirectoryAndParents(directory);

    DocumentationLibrary doc;
    LoadFromDataFile(doc, config.mTrimmedOutput);
    doc.FinalizeDocumentation();

    DocToTags tagged;

    // create the base wikipage for the codeRef
    StringBuilder codeRefIndex;

    codeRefIndex << "\n---  \n" 
      << "Zero Engine Code Reference \n" 
      << "========================== \n"
      << "\n---  \n";
    // initialize the classLinkMap so it contains all the lings
    forRange(ClassDoc* classDoc, doc.mClasses.All())
    {
      gClassLinkMap[classDoc->mName] = BuildString(gBaseLink, classDoc->mName);
    }
    //Upload the class' page to the wiki, making sure to perform the link replacements
    forRange(ClassDoc* classDoc, doc.mClasses.All())
    {
      String filename = BuildString(classDoc->mName, ".txt");

      String fullPath = FilePath::Combine(directory, "Reference");

      CreateDirectoryAndParents(fullPath);

      fullPath = FilePath::Combine(fullPath, filename);

      fullPath = FilePath::Normalize(fullPath);

      ReMarkupClassMarkupWriter::WriteClass(fullPath, classDoc, doc, tagged);

      // [[wiki page | name]]
      codeRefIndex << gBullet << "[[" << gClassLinkMap[classDoc->mName]
        << " | " << classDoc->mName << "]] \n";;

    }

    WriteStringRangeToFile(FilePath::Combine(directory, "ClassRef.txt"), codeRefIndex.ToString());

    WriteTagIndices(directory, tagged, doc);

  }

  // check if we outputting commands
  if (!config.mCommandListFile.Empty())
  {
    String output = FilePath::Combine(config.mMarkupDirectory, "CommandRef.txt");
    output = FilePath::Normalize(output);
    ReMarkupCommandRefWriter::WriteCommandRef(config.mCommandListFile, output);
  }

  // check if we are outputing events
  if (!config.mEventsOutputLocation.Empty())
  {
    String output = FilePath::Combine(config.mMarkupDirectory, "EventList.txt");
    output = FilePath::Normalize(output);
    ReMarkupEventListWriter::WriteEventList(config.mEventsOutputLocation, output);
  }

}

////////////////////////////////////////////////////////////////////////
// ReMarkupWriter
////////////////////////////////////////////////////////////////////////
const String ReMarkupWriter::mEndLine("  \n\n");

ReMarkupWriter::ReMarkupWriter(StringParam name) : BaseMarkupWriter(name)
{
}

// Markup requires spaces not tabs so need to override
void ReMarkupWriter::IndentToCurrentLevel(void)
{
  // note, requires two spaces so we just double the indentation level
  mOutput << String::Repeat(' ', mCurrentIndentationLevel * 2);
}

// just prints the language specifier for a code block
void ReMarkupWriter::InsertStartOfCodeBlock(StringParam name)
{
  IndentToCurrentLevel();
  mOutput << "lang=cpp, name=" << name << "\n";
}
void ReMarkupWriter::InsertDivider(void)
{
  mOutput << "\n---  \n";
}

void ReMarkupWriter::InsertLabel(StringParam label)
{
  mOutput << "=====" << label << mEndLine;
}

void ReMarkupWriter::InsertTypeLink(StringParam className)
{
  // THIS VERSION ONLY LINKS TYPES IN THE DOCUMENTATION LIBRARY
  /*
  if (!gClassLinkMap.ContainsKey(className))
  {
  mOutput << className;
  return;
  }

  mOutput << "[[" << gClassLinkMap[className] << " | " << className << "]]";
  */

  // THIS VERSION ATTEMPTS TO LINK EVERYTHING BUT VOID
  if (className.ToLower() == "void")
  {
    mOutput << className;
    return;
  }

  mOutput << "[[" << gBaseLink << className << " | " << className << "]]";
}

////////////////////////////////////////////////////////////////////////
// ReMarkupClassMarkupWriter
////////////////////////////////////////////////////////////////////////
void ReMarkupClassMarkupWriter::WriteClass(StringParam outputFile,
  ClassDoc* classDoc, DocumentationLibrary &lib, DocToTags& tagged)
{


  // first things first, set up the tags for this class
  forRange(StringRef tag, classDoc->mTags.All())
  {
    tagged[tag].PushBack(classDoc);
  }

  // do the magic for getting directory and file
  ReMarkupClassMarkupWriter writer(classDoc->mName, classDoc);

  // top of the file
  writer.InsertClassHeader();
  StartHeaderSection(writer);

  // Properties
  //writer.mOutput << ".. _Reference" << writer.mName << "Properties:\n\n";

  writer.InsertNewSectionHeader("Properties");
  writer.InsertDivider();

  forRange(PropertyDoc *prop, classDoc->mProperties.All())
  {
    writer.InsertProperty(*prop);
  }

  // Methods
  //writer.mOutput << ".. _Reference" << writer.mName << "Methods:\n\n";

  writer.InsertNewSectionHeader("Methods");
  writer.InsertDivider();

  forRange(MethodDoc *method, classDoc->mMethods.All())
  {
    writer.InsertMethod(*method);
  }

  // bottom of the file
  EndHeaderSection(writer);

  writer.WriteOutputToFile(outputFile);
}

ReMarkupClassMarkupWriter::ReMarkupClassMarkupWriter(StringParam name, ClassDoc* classDoc)
  : ReMarkupWriter(name), mClassDoc(classDoc)
{

}

void ReMarkupClassMarkupWriter::InsertClassHeader(void)
{
  return;
}

void ReMarkupClassMarkupWriter::InsertMethod(MethodDoc &method)
{
  StartIndentSection((*this));

  InsertStartOfCodeBlock(method.mName);
  IndentToCurrentLevel();
  mOutput << method.mReturnType << " " << mName
    << "::" << method.mName << method.mParameters << mEndLine;

  if (!method.mDescription.Empty())
  {
    mOutput << method.mDescription << mEndLine;
  }

  // put link to return type 
  mOutput << gBullet << gBold << "ReturnType: " << gBold;
  InsertTypeLink(method.mReturnType);
  mOutput << mEndLine;

  InsertLabel("Parameters:");

  if (method.mParameterList.Size() == 0)
  {
    mOutput << gBullet << gBold << "Name: " << gBold << " None" << mEndLine;
  }
  else
  {
    forRange(ParameterDoc *param, method.mParameterList.All())
    {
      //TODO: Turn these into links
      mOutput << gBullet << gBold << "Name: " << gBold << " " << param->mName << "  ";
      mOutput << gBold << "Type: " << gBold << " ";
      InsertTypeLink(param->mType);
      mOutput << mEndLine;

      if (param->mDescription != "")
      {
        mOutput << param->mDescription << mEndLine;
      }
    }
  }

  EndIndentSection((*this));
  InsertDivider();
}

void ReMarkupClassMarkupWriter::InsertProperty(PropertyDoc &propDoc)
{
  if (propDoc.mType.Empty())
    return;


  StartIndentSection((*this));
  InsertStartOfCodeBlock(propDoc.mName);
  IndentToCurrentLevel();
  mOutput << propDoc.mType << " " << mName << "::" << propDoc.mName << mEndLine;

  if (!propDoc.mDescription.Empty())
  {
    mOutput << propDoc.mDescription << mEndLine;
  }

  //Insert the type of the property
  mOutput << gBullet << gBold << "Type: " << gBold;
  InsertTypeLink(propDoc.mType);
  mOutput << mEndLine;


  EndIndentSection((*this));
  InsertDivider();
}

////////////////////////////////////////////////////////////////////////
// ReMarkupEventListWriter
////////////////////////////////////////////////////////////////////////
void ReMarkupEventListWriter::WriteEventList(StringRef eventListFilepath, StringRef outputPath)
{
  // load the file if we can

  // get outta here with that nonexistent file
  if (!FileExists(eventListFilepath))
  {
    printf("%s does not exist.", eventListFilepath.c_str());
    return;
  }

  // actually load event list now. (If this fails it probably means the file is mis-formatted)
  EventDocList eventListDoc;
  LoadFromDataFile(eventListDoc, eventListFilepath);

  Array<EventDoc *> &eventArray = eventListDoc.mEvents;

  // create the eventList
  ReMarkupEventListWriter writer("Event List");

  writer.InsertNewSectionHeader("Event List");

  writer.InsertDivider();

  // create top of file
  StartHeaderSection(writer);

  // iterate over all events and insert them
  forRange(EventDoc *eventDoc, eventArray.All())
  {
    writer.WriteEventEntry(eventDoc->mName, eventDoc->mType);
  }

  EndHeaderSection(writer);

  writer.WriteOutputToFile(outputPath);
}

ReMarkupEventListWriter::ReMarkupEventListWriter(StringParam name) : ReMarkupWriter(name)
{

}

void ReMarkupEventListWriter::WriteEventEntry(StringParam eventEntry, StringParam type)
{
  InsertNewSectionHeader(eventEntry);

  //mOutput << ".. include:: EventListDescriptions/" << eventEntry << ".rst\n\n"
  //  << ":cpp:type:`" << type << "`\n\n";

  mOutput << gBold << "Type: " << gBold;
  InsertTypeLink(type);
  mOutput << mEndLine;

  InsertDivider();
}
////////////////////////////////////////////////////////////////////////
// ReMarkupCommandRefWriter
////////////////////////////////////////////////////////////////////////
void ReMarkupCommandRefWriter::WriteCommandRef(StringParam commandListFilepath, StringRef outputPath)
{
  // load the file
  // get outta here with that nonexistent file
  if (!FileExists(commandListFilepath))
  {
    printf("%s does not exist.", commandListFilepath.c_str());
    return;
  }

  // actually load event list now. (If this fails it probably means the file is mis-formatted)
  CommandDocList cmdListDoc;
  LoadFromDataFile(cmdListDoc, commandListFilepath);

  Array<CommandDoc *> &cmdArray = cmdListDoc.mCommands;

  // create the command writer
  ReMarkupCommandRefWriter writer("Zero Commands");

  // create top of file
  writer.InsertNewSectionHeader("Zero Commands");

  writer.InsertDivider();

  StartHeaderSection(writer);

  // iterate over all commands and insert them
  forRange(CommandDoc *cmdDoc, cmdArray.All())
  {
    writer.WriteCommandEntry(*cmdDoc);
  }

  EndHeaderSection(writer);

  writer.WriteOutputToFile(outputPath);
}

ReMarkupCommandRefWriter::ReMarkupCommandRefWriter(StringParam name) : ReMarkupWriter(name)
{

}

void ReMarkupCommandRefWriter::WriteCommandEntry(const CommandDoc &cmdDoc)
{
  InsertNewSectionHeader(cmdDoc.mName);

  if (!cmdDoc.mDescription.Empty())
    mOutput << cmdDoc.mDescription << mEndLine << mEndLine;

  InsertLabel("Tags: ");

  if (cmdDoc.mTags.Empty())
  {
    mOutput << "No Tags" << mEndLine;
  }
  else
  {
    // actually list tags
    forRange(StringRef tag, cmdDoc.mTags.All())
    {
      mOutput << gBullet << tag << mEndLine;
    }
  }

  InsertLabel("Shortcut: ");

  if (cmdDoc.mShortcut.Empty())
  {
    mOutput << "No Keyboard Shortcut" << mEndLine;
  }
  else
  {
    mOutput << cmdDoc.mShortcut << mEndLine;
  }

  //mOutput << ".. include:: CommandPageExtensions/" << cmdDoc.mName << ".rst\n\n";
  InsertDivider();
}

}
