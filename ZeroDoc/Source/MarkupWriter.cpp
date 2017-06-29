#include "Precompiled.hpp"
#include "Engine/EngineContainers.hpp"
#include "DocConfiguration.hpp"
#include "Platform/FileSystem.hpp"
#include "Engine/Documentation.hpp"
#include "Serialization/Simple.hpp"
#include "Support/FileSupport.hpp"
#include "Platform/FilePath.hpp"
#include "MarkupWriter.hpp"
#include "RawDocumentation.hpp"
#include "DocTypeParser.hpp"

namespace Zero
{
UnsortedMap<String, String> gLinkMap;
String gBaseLink = "zero_engine_documentation/zero_editor_documentation/code_reference/";
String gBaseClassLink = BuildString(gBaseLink, "class_reference/");
String gBaseZilchTypesLink = BuildString(gBaseLink, "zilch_base_types/");
String gBaseEnumTypesLink = BuildString(gBaseLink, "EnumList/#");
String gBaseFlagsTypesLink = BuildString(gBaseLink, "FlagsList/#");

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
    String& directory = config.mMarkupDirectory;

    CreateDirectoryAndParents(directory);

    DocumentationLibrary doc;
    LoadDocumentationSkeleton(doc, config.mTrimmedOutput);
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

void BaseMarkupWriter::WriteOutputToFile(StringParam file)
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

void BaseMarkupWriter::InsertNewSectionHeader(StringParam sectionName)
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
  forRange(String& tag, classDoc->mTags.All())
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
     << method.mName << method.mParameters << "\n\n";

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

  mOutput << ".. cpp:member:: " <<  propDoc.mType << " "  << propDoc.mName << "\n\n";

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
void RstEventListWriter::WriteEventList(StringParam eventListFilepath, StringParam outputPath)
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
  LoadEventList(eventListDoc, eventListFilepath);

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
void RstCommandRefWriter::WriteCommandRef(StringParam commandListFilepath, StringParam outputPath)
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
  LoadCommandList(cmdListDoc, commandListFilepath);

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
    forRange(String& tag, cmdDoc.mTags.All())
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
  String baseFromMarkupDirectory = "zero_engine_documentation\\zero_editor_documentation\\code_reference";
  String baseClassDirectory = FilePath::Combine(baseFromMarkupDirectory, "class_reference");
  String baseZilchTypesDirectory = FilePath::Combine(baseFromMarkupDirectory, "zilch_base_types");

  printf("Mark up: ReMarkup\n");

  // https://phab.digipen.edu/w/curriculum_development_and_documentation/rst_documentation/
  //       classname, link

  // check if we are outputting class markup
  if (FileExists(config.mTrimmedOutput.c_str()))
  {
    String& directory = config.mMarkupDirectory;

    CreateDirectoryAndParents(directory);

    DocumentationLibrary doc;
    LoadDocumentationSkeleton(doc, config.mTrimmedOutput);
    //LoadFromDataFile(doc, config.mTrimmedOutput);
    doc.FinalizeDocumentation();

    DocToTags tagged;

    // create the base wikipage for the codeRef
    StringBuilder codeRefIndex;

    codeRefIndex << "\n---  \n" 
      << "Zero Engine Code Reference \n" 
      << "========================== \n"
      << "\n---  \n";

    // create the base wikipage for zilch core types
    StringBuilder zilchCoreIndex;
   
    codeRefIndex << "\n---  \n"
      << "Zilch Base Types Reference \n"
      << "========================== \n"
      << "\n---  \n";

    // Add all flags to the linkMap
    forRange(auto& flagDoc, doc.mFlags.All())
    {
      gLinkMap[flagDoc->mName] = BuildString(gBaseFlagsTypesLink, flagDoc->mName);
    }
    // Add all enums to the linkMap
    forRange(auto& enumDoc, doc.mEnums.All())
    {
      gLinkMap[enumDoc->mName] = BuildString(gBaseEnumTypesLink, enumDoc->mName);
    }
    // Add all classes to the linkMap
    forRange(ClassDoc* classDoc, doc.mClasses.All())
    {
      // check for core library types assuming those are just zilch
      if (classDoc->mLibrary == "Core")
      {
        gLinkMap[classDoc->mName] = BuildString(gBaseZilchTypesLink, classDoc->mName);
      }
      else
      {
        gLinkMap[classDoc->mName] = BuildString(gBaseClassLink, classDoc->mName);

        // add a version that does not have the extra type information
        if (classDoc->mName.Contains("["))
        {
          auto foundBracket = classDoc->mName.FindFirstOf("[");
          gLinkMap[classDoc->mName.SubString(classDoc->mName.Begin(), foundBracket.Begin())]
            = BuildString(gBaseClassLink, classDoc->mName);
        }
      }
    }
    //Upload the class' page to the wiki, making sure to perform the link replacements
    forRange(ClassDoc* classDoc, doc.mClasses.All())
    {
      if (classDoc->mLibrary == "Core")
      {
        String filename = BuildString(classDoc->mName, ".txt");

        String fullPath = FilePath::Combine(directory, baseZilchTypesDirectory);

        CreateDirectoryAndParents(fullPath);

        fullPath = FilePath::Combine(fullPath, filename);

        fullPath = FilePath::Normalize(fullPath);

        ReMarkupClassMarkupWriter::WriteClass(fullPath, classDoc, doc, tagged);

        // [[wiki page | name]]
        zilchCoreIndex << gBullet << "[[" << gLinkMap[classDoc->mName] << "]] \n";
      }
      else
      {
        String fullPath = FilePath::Combine(directory,baseClassDirectory);

        CreateDirectoryAndParents(fullPath);

        fullPath = FilePath::CombineWithExtension(fullPath, classDoc->mName, ".txt");

        ReMarkupClassMarkupWriter::WriteClass(fullPath, classDoc, doc, tagged);

        // [[wiki page | name]]
        codeRefIndex << gBullet << "[[" << gLinkMap[classDoc->mName] << "]] \n";
      }
    }

    WriteStringRangeToFile(FilePath::Combine(directory, baseFromMarkupDirectory,"class_reference.txt"), codeRefIndex.ToString());
    WriteStringRangeToFile(FilePath::Combine(directory, baseFromMarkupDirectory,"zilch_base_reference.txt"), zilchCoreIndex.ToString());

    WriteTagIndices(directory, tagged, doc);

    // Output separate enum list
    String enumOutput = FilePath::Combine(directory, baseFromMarkupDirectory, "enum_reference.txt");
    enumOutput = FilePath::Normalize(enumOutput);
    ReMarkupEnumListWriter::WriteEnumList(enumOutput, doc);

    // output separate flags list
    String flagsOutput = FilePath::Combine(directory, baseFromMarkupDirectory, "flags_reference.txt");
    flagsOutput = FilePath::Normalize(flagsOutput);
    ReMarkupFlagsListWriter::WriteFlagsList(flagsOutput, doc);
  }

  // check if we outputting commands
  if (!config.mCommandListFile.Empty())
  {
    String output = FilePath::Combine(config.mMarkupDirectory, baseFromMarkupDirectory, "command_reference.txt");
    output = FilePath::Normalize(output);
    ReMarkupCommandRefWriter::WriteCommandRef(config.mCommandListFile, output);
  }

  // check if we are outputting events
  if (!config.mEventsOutputLocation.Empty())
  {
    String output = FilePath::Combine(config.mMarkupDirectory, baseFromMarkupDirectory, "event_reference.txt");
    output = FilePath::Normalize(output);
    ReMarkupEventListWriter::WriteEventList(config.mEventsOutputLocation, output);
  }
}

void ReMarkupWriter::InsertHeaderLink(StringParam header)
{
  mOutput << "[[" << mDocURI << "#" << header << " | " << header << "]]";
}

////////////////////////////////////////////////////////////////////////
// ReMarkupWriter
////////////////////////////////////////////////////////////////////////
const String ReMarkupWriter::mEndLine("  \n\n");

ReMarkupWriter::ReMarkupWriter(StringParam name, StringParam uri)
  : BaseMarkupWriter(name), mDocURI(uri)
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

void ReMarkupWriter::InsertHeaderAtCurrentHeaderLevel(StringParam header)
{
  InsertHeaderAtCurrentHeaderLevel();

  mOutput << header << mEndLine;
}

void ReMarkupWriter::InsertHeaderAtCurrentHeaderLevel(void)
{
  for (uint i = 0; i <= this->mCurrentSectionHeaderLevel; ++i)
    mOutput << "=";
}

void ReMarkupWriter::InsertLabel(StringParam label)
{
  mOutput << "=====" << label << mEndLine;
}

void ReMarkupWriter::InsertTypeLink(StringParam className)
{
  // this version attempt to link everything but void
  if (className.ToLower() == "void")
  {
    mOutput << className;
    return;
  }

  TypeTokens tokens;

  AppendTokensFromString(DocLangDfa::Get(), className, &tokens);

  forRange(auto& token, tokens.All())
  {
    // if token is recognized as a linkable type, link it
    if (gLinkMap.ContainsKey(token.mText))
    {
      mOutput << "[[" << gLinkMap[token.mText] << "]]";
    }
    else
    {
      // otherwise, just add it to output
      mOutput << token.mText;
    }
  }
}

////////////////////////////////////////////////////////////////////////
// ReMarkupClassMarkupWriter
////////////////////////////////////////////////////////////////////////
void ReMarkupClassMarkupWriter::WriteClass(StringParam outputFile,
  ClassDoc* classDoc, DocumentationLibrary &lib, DocToTags& tagged)
{
  // first things first, set up the tags for this class
  forRange(String& tag, classDoc->mTags.All())
  {
    tagged[tag].PushBack(classDoc);
  }

  // do the magic for getting directory and file
  ReMarkupClassMarkupWriter writer(classDoc->mName, classDoc, gLinkMap[classDoc->mName]);

  // top of the file
  writer.InsertClassHeader();
  StartHeaderSection(writer);


  writer.InsertJumpTable();

  // Properties
  //writer.mOutput << ".. _Reference" << writer.mName << "Properties:\n\n";

  writer.InsertHeaderAtCurrentHeaderLevel("Properties");
  writer.InsertDivider();

  forRange(PropertyDoc *prop, classDoc->mProperties.All())
  {
    writer.InsertProperty(*prop);
  }

  // Methods
  //writer.mOutput << ".. _Reference" << writer.mName << "Methods:\n\n";

  writer.InsertHeaderAtCurrentHeaderLevel("Methods");
  writer.InsertDivider();

  forRange(MethodDoc *method, classDoc->mMethods.All())
  {
    writer.InsertMethod(*method);
  }

  // bottom of the file
  EndHeaderSection(writer);

  writer.WriteOutputToFile(outputFile);
}

ReMarkupClassMarkupWriter::ReMarkupClassMarkupWriter(StringParam name, ClassDoc* classDoc, StringParam uri)
  : ReMarkupWriter(name, uri), mClassDoc(classDoc)
{

}

void ReMarkupClassMarkupWriter::InsertClassHeader(void)
{
  //=================
  mOutput << "= " << mClassDoc->mName;
  InsertDivider();
  return;
}

void ReMarkupClassMarkupWriter::InsertMethod(MethodDoc &method)
{
  StartIndentSection((*this));
  StartHeaderSection((*this));
  // subheader for the method name
  InsertHeaderAtCurrentHeaderLevel(method.mName);
  // print the description directly under the header
  mOutput << method.mDescription << mEndLine;

  // TODO: print the cpp codeblock

  //print the zilch codeblock
  InsertStartOfCodeBlock("Zilch");
  IndentToCurrentLevel();
  mOutput << "function " << method.mName << "(";// Initialize(init : CogInitializer)

  for(uint i = 0; i < method.mParameterList.Size(); ++i)
  {
    ParameterDoc *param = method.mParameterList[i];

    // insert a comma in the start if we are not the first parameter
    if (i > 0)
    {
      mOutput << ", ";
    }

    mOutput << param->mName << " : " << param->mType;
  }

  mOutput << ")";

  if (!method.mReturnType.Empty() && method.mReturnType != "Void")
    mOutput << " : " << method.mReturnType;

  mOutput << ";" << mEndLine;

  // print parameter table
  mOutput << "|Name|Type|Description|\n|---|---|---|\n";

  forRange(ParameterDoc *param, method.mParameterList.All())
  {
    mOutput << "|" << param->mName << "|";
    InsertTypeLink(param->mType);
    mOutput << "|";
    if (param->mDescription.Empty())
    {
      mOutput << " ";
    }
    else
    {
      mOutput << param->mDescription;
    }
    mOutput << "|\n";
  }
  mOutput << mEndLine;

  // put link to return type 
  StartHeaderSection((*this));
  InsertHeaderAtCurrentHeaderLevel();
  mOutput << "ReturnType: ";
  InsertTypeLink(method.mReturnType);
  mOutput << mEndLine;

  EndHeaderSection((*this));
  EndHeaderSection((*this));
  EndIndentSection((*this));

  InsertDivider();
}

void ReMarkupClassMarkupWriter::InsertProperty(PropertyDoc &propDoc)
{
  if (propDoc.mType.Empty())
    return;

  StartHeaderSection((*this));
  StartIndentSection((*this));

  // subheader for the method name
  //mOutput << "====" << propDoc.mName << mEndLine;
  InsertHeaderAtCurrentHeaderLevel(propDoc.mName);
  // print the description directly under the header
  mOutput << propDoc.mDescription << mEndLine;

  // TODO: print the cpp codeblock

  //print the zilch codeblock
  InsertStartOfCodeBlock("Zilch");
  IndentToCurrentLevel();
  mOutput << "var " << propDoc.mName << " : " << propDoc.mType << ";" << mEndLine;

  StartHeaderSection((*this));
  InsertHeaderAtCurrentHeaderLevel();
  mOutput << "Type: ";
  InsertTypeLink(propDoc.mType);
  mOutput << mEndLine;
  EndHeaderSection((*this));

  EndIndentSection((*this));
  EndHeaderSection((*this));
  InsertDivider();
}

void ReMarkupClassMarkupWriter::WriteMethodTable(void)
{
  mOutput << "|Name|Description|\n|---|---|\n";

  forRange(MethodDoc *method, mClassDoc->mMethods.All())
  {
    mOutput << "|";
    InsertHeaderLink(method->mName);
    mOutput<< "|";

    if (method->mDescription.Empty())
    {
      mOutput << " ";
    }
    else
    {
      mOutput << method->mDescription;
    }

    mOutput << "|\n";
  }
  mOutput << mEndLine;
}

void ReMarkupClassMarkupWriter::WritePropertyTable(void)
{
  mOutput << "|Name|Description|" << mEndLine << "|---|---|" << mEndLine;

  forRange(PropertyDoc *prop, mClassDoc->mProperties.All())
  {
    mOutput << "|";
    InsertHeaderLink(prop->mName);
    mOutput<< "|";

    if (prop->mDescription.Empty())
    {
      mOutput << " ";
    }
    else
    {
      mOutput << prop->mDescription;
    }
    mOutput << "|" << mEndLine;
  }
}

void ReMarkupClassMarkupWriter::InsertJumpTable(void)
{
  // if we don't have stuff to jump to, don't make a jump table
  if (mClassDoc->mMethods.Empty() && mClassDoc->mProperties.Empty())
    return;


  uint dualIterator = 0;
  bool methodListLonger = false;
  uint smallestListSize;
  
  if (mClassDoc->mMethods.Size() > mClassDoc->mProperties.Size())
  {
    methodListLonger = true;
    smallestListSize = mClassDoc->mProperties.Size();
  }
  else
  {
    smallestListSize = mClassDoc->mMethods.Size();
  }

  // print the top of the table
  mOutput << "|Methods|Properties|\n|---|---|\n";

  for (; dualIterator < smallestListSize; ++dualIterator)
  {
    mOutput << "|";
    InsertHeaderLink(mClassDoc->mMethods[dualIterator]->mName);
    mOutput << "|";
    InsertHeaderLink(mClassDoc->mProperties[dualIterator]->mName);
    mOutput<< "|\n";
  }
  // print the rest of the methods if we had more methods then properties
  if (methodListLonger)
  {
    for (uint i = dualIterator; i < mClassDoc->mMethods.Size(); ++i)
    {
      mOutput << "|";
      InsertHeaderLink(mClassDoc->mMethods[i]->mName);
      mOutput << "| |\n";
    }
  }
  else
  {
    for (uint i = dualIterator; i < mClassDoc->mProperties.Size(); ++i)
    {
      mOutput << "| |";
      InsertHeaderLink(mClassDoc->mProperties[i]->mName);
      mOutput << "|\n";
    }
  }
  mOutput << mEndLine;
}

////////////////////////////////////////////////////////////////////////
// ReMarkupEnumListWriter
////////////////////////////////////////////////////////////////////////
ReMarkupEnumListWriter::ReMarkupEnumListWriter(StringParam name, StringParam uri) 
  : ReMarkupWriter(name, uri)
{

}


void ReMarkupEnumListWriter::WriteEnumList(StringParam outputFile, DocumentationLibrary &lib)
{
  // create the eventList
  ReMarkupEnumListWriter writer("Enum List", BuildString(gBaseLink, "enum_reference/"));

  writer.InsertNewSectionHeader("Enum List");

  writer.InsertDivider();

  // insert table of enums
  writer.InsertEnumTable(lib.mEnums);

  writer.InsertDivider();

  // loop over all enums and list them under headers
  forRange(EnumDoc* enumToWrite, lib.mEnums)
  {
    writer.InsertEnumEntry(enumToWrite);
  }

  writer.WriteOutputToFile(outputFile);
}

void ReMarkupEnumListWriter::InsertEnumEntry(EnumDoc* enumDoc)
{
  // name
  // description
  // values
  // value description (if any)

  StartIndentSection((*this));
  // subheader for the method name
  mOutput << "====" << enumDoc->mName << mEndLine;
  // print the description directly under the header
  mOutput << enumDoc->mDescription << mEndLine;

  mOutput << "|EnumValue|Description|\n|---|---|\n";

  forRange(auto &enumDescPair, enumDoc->mEnumValues.All())
  {
    mOutput << "|" << enumDescPair.first << "|" << enumDescPair.second << "|\n";
  }
  EndIndentSection((*this));
  InsertDivider();
  mOutput << mEndLine;
}

void ReMarkupEnumListWriter::InsertEnumTable(const Array<EnumDoc*>& enumList)
{
  // print the top of the table
  mOutput << "|Enum||\n|---|\n";

  forRange(EnumDoc* doc, enumList.All( ))
  {
    mOutput << "|";
    InsertHeaderLink(doc->mName);
    mOutput << "|\n";
  }
  mOutput << mEndLine;
}

////////////////////////////////////////////////////////////////////////
// ReMarkupFlagsListWriter
////////////////////////////////////////////////////////////////////////
ReMarkupFlagsListWriter::ReMarkupFlagsListWriter(StringParam name, StringParam uri)
  : ReMarkupWriter(name, uri)
{
}


void ReMarkupFlagsListWriter::WriteFlagsList(StringParam outputFile, DocumentationLibrary &lib)
{
  // create the eventList
  ReMarkupFlagsListWriter writer("Flags List", BuildString(gBaseLink, "flags_reference/"));

  writer.InsertNewSectionHeader("Flags List");

  writer.InsertDivider();

  // insert table of enums
  writer.InsertFlagTable(lib.mFlags);

  writer.InsertDivider();

  // loop over all enums and list them under headers
  forRange(EnumDoc* flagsToWrite, lib.mFlags)
  {
    writer.InsertFlagsEntry(flagsToWrite);
  }

  writer.WriteOutputToFile(outputFile);
}



void ReMarkupFlagsListWriter::InsertFlagsEntry(EnumDoc *flags)
{
  // name
  // description
  // values
  // value description (if any)

  StartIndentSection((*this));
  // subheader for the method name
  mOutput << "====" << flags->mName << mEndLine;
  // print the description directly under the header
  mOutput << flags->mDescription << mEndLine;

  mOutput << "|FlagName|Description|\n|---|---|\n";

  forRange(auto& enumDescPair, flags->mEnumValues.All())
  {
    mOutput << "|" << enumDescPair.first << "|" << enumDescPair.second << "|\n";
  }
  EndIndentSection((*this));
  InsertDivider();
  mOutput << mEndLine;
}

void ReMarkupFlagsListWriter::InsertFlagTable(const Array<EnumDoc*>& flagsList)
{
  // print the top of the table
  mOutput << "|Flags||\n|---|\n";

  forRange(EnumDoc *doc, flagsList.All())
  {
    mOutput << "|";
    InsertHeaderLink(doc->mName);
    mOutput << "|\n";
  }
  mOutput << mEndLine;
}

////////////////////////////////////////////////////////////////////////
// ReMarkupEventListWriter
////////////////////////////////////////////////////////////////////////
void ReMarkupEventListWriter::WriteEventList(StringParam eventListFilepath, StringParam outputPath)
{
  // load the file if we can
  if (!FileExists(eventListFilepath))
  {
    printf("%s does not exist.", eventListFilepath.c_str());
    return;
  }

  // actually load event list now. (If this fails it probably means the file is mis-formatted)
  EventDocList eventListDoc;
  LoadEventList(eventListDoc, eventListFilepath);

  Array<EventDoc *> &eventArray = eventListDoc.mEvents;

  // create the eventList
  ReMarkupEventListWriter writer("Event Reference", BuildString(gBaseLink, "event_reference/"));

  writer.InsertNewSectionHeader("Event List");

  writer.InsertDivider();

  writer.WriteEventTable(eventArray);

  writer.InsertDivider();

  // create top of file
  StartHeaderSection(writer);

  // iterate over all events and insert them
  forRange(EventDoc *eventDoc, eventArray.All())
  {
    writer.WriteEventEntry(eventDoc, eventDoc->mType);
  }

  EndHeaderSection(writer);

  writer.WriteOutputToFile(outputPath);
}

ReMarkupEventListWriter::ReMarkupEventListWriter(StringParam name, StringParam uri) 
  : ReMarkupWriter(name, uri)
{
}

void ReMarkupEventListWriter::WriteEventEntry(EventDoc* eventDoc, StringParam type)
{
  InsertNewSectionHeader(eventDoc->mName);

  //mOutput << ".. include:: EventListDescriptions/" << eventEntry << ".rst\n\n"
  //  << ":cpp:type:`" << type << "`\n\n";

  mOutput << gBold << "Type: " << gBold;
  InsertTypeLink(type);
  mOutput << mEndLine;

  if (!eventDoc->mSenders.Empty())
  {
    mOutput << gBold << "Senders :" << gBold << mEndLine;

    StartIndentSection((*this));
    IndentToCurrentLevel();

    forRange(String& sender, eventDoc->mSenders.All())
    {
      mOutput << gBullet;
      InsertTypeLink(sender);
      mOutput <<"\n";
    }

    EndIndentSection((*this));
    mOutput << mEndLine;
  }

  InsertDivider();
}

void ReMarkupEventListWriter::WriteEventTable(const Array<EventDoc*>& eventList)
{
  // print the top of the table
  mOutput << "|Event|EventType|\n|---|---|\n";

  forRange(EventDoc* event, eventList.All())
  {
    mOutput << "|";
    InsertHeaderLink(event->mName); 
    mOutput << "|";
    InsertTypeLink(event->mType);
    mOutput << "|\n";
  }
  mOutput << mEndLine;
}

////////////////////////////////////////////////////////////////////////
// ReMarkupCommandRefWriter
////////////////////////////////////////////////////////////////////////
ReMarkupCommandRefWriter::ReMarkupCommandRefWriter(StringParam name, StringParam uri)
  : ReMarkupWriter(name, uri)
{

}

void ReMarkupCommandRefWriter::WriteCommandRef(StringParam commandListFilepath, StringParam outputPath)
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
  
  LoadCommandList(cmdListDoc, commandListFilepath);

  Array<CommandDoc *> &cmdArray = cmdListDoc.mCommands;

  // create the command writer
  ReMarkupCommandRefWriter writer("Zero Commands", BuildString(gBaseLink, "command_reference/"));

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
    forRange(String& tag, cmdDoc.mTags.All())
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
    String shortcut = cmdDoc.mShortcut.Replace("+", " ");
    mOutput << "{key " << shortcut << "}" << mEndLine;
  }

  //mOutput << ".. include:: CommandPageExtensions/" << cmdDoc.mName << ".rst\n\n";
  InsertDivider();
}

}
