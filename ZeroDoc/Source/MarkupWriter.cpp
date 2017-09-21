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
String gBaseLink = "zero_engine_documentation/code_reference/";
String gBaseClassLink = BuildString(gBaseLink, "Class_Reference/");
String gBaseZilchTypesLink = BuildString(gBaseLink, "Zilch_Base_Types/");
String gBaseEnumTypesLink = BuildString(gBaseLink, "Enum_Reference/#");
String gBaseFlagsTypesLink = BuildString(gBaseLink, "Flags_Reference/#");


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
static const char *gBullet("- ");
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
  mOutput << "= " << sectionName << "\n";
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

// helper for converting camelCase names to lowercase with underscores
String GetLinkName(StringParam name)
{
  const String openBracket = "[";
  const String closeBracket = "]";
  StringBuilder builder;
  
  StringRange nameRange = name.All();

  builder << UTF8::ToLower(nameRange.Front());

  nameRange.PopFront();

  for(; !nameRange.Empty(); nameRange.PopFront())
  {
    if (nameRange.IsCurrentRuneUpper())
    {
      builder << " " << UTF8::ToLower(nameRange.Front());
    }
    else if (nameRange.Front() == openBracket.Front())
    {
      builder << " ";
    }
    else if (nameRange.Front() == closeBracket.Front())
    {
      continue;
    }
    else
    {
      builder << UTF8::ToLower(nameRange.Front());
    }
  }

  return builder.ToString();
}

void WriteOutAllReMarkupFiles(Zero::DocGeneratorConfig& config)
{
  String baseFromMarkupDirectory = "zero_engine_documentation\\zero_editor_documentation\\code_reference";
  String baseClassDirectory = FilePath::Combine(baseFromMarkupDirectory, "Class_Reference");
  String baseZilchTypesDirectory = FilePath::Combine(baseFromMarkupDirectory, "Zilch_Base_Types");

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

    // create the base wikipage for zilch core types
    StringBuilder zilchCoreIndex;
  
    // Add all flags to the linkMap
    forRange(auto& flagDoc, doc.mFlags.All())
    {
      gLinkMap[flagDoc->mName] = BuildString(gBaseFlagsTypesLink, flagDoc->mName, "|", flagDoc->mName);
    }
    // Add all enums to the linkMap
    forRange(auto& enumDoc, doc.mEnums.All())
    {
      gLinkMap[enumDoc->mName] = BuildString(gBaseEnumTypesLink, enumDoc->mName, "|", enumDoc->mName);
    }
    // Add all classes to the linkMap
    forRange(ClassDoc* classDoc, doc.mClasses.All())
    {
      // check for core library types assuming those are just zilch
      if (classDoc->mLibrary == "Core")
      {
        gLinkMap[classDoc->mName] = BuildString(gBaseZilchTypesLink, classDoc->mName, "/");

        if (classDoc->mName.Contains("["))
        {
          String name = classDoc->mName.Replace(String('['), String('_'));
          name = name.Replace(String(']'), String(' '));
          gLinkMap[classDoc->mName] = BuildString(gBaseZilchTypesLink, name.ToLower(), "/");
        }
      }
      else
      {
        gLinkMap[classDoc->mName] = BuildString(gBaseClassLink, classDoc->mName, "/");

        if (classDoc->mName.Contains("["))
        {
          String name = classDoc->mName.Replace(String('['), String('_'));
          name = name.Replace(String(']'), String(' '));
          gLinkMap[classDoc->mName] = BuildString(gBaseZilchTypesLink, name.ToLower(), "/");
        }
      }
    }
        // add a version that does not have the extra type information
    //Upload the class' page to the wiki, making sure to perform the link replacements
    forRange(ClassDoc* classDoc, doc.mClasses.All())
    {
      if (classDoc->mLibrary == "Core")
      {
		    String className = classDoc->mName;
        String filename = BuildString(className, ".txt");

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

    WriteStringRangeToFile(FilePath::Combine(directory, baseFromMarkupDirectory,"Class Reference.txt"), codeRefIndex.ToString());
    WriteStringRangeToFile(FilePath::Combine(directory, baseFromMarkupDirectory,"Zilch Base Types.txt"), zilchCoreIndex.ToString());

    WriteTagIndices(directory, tagged, doc);

    // Output separate enum list
    String enumOutput = FilePath::Combine(directory, baseFromMarkupDirectory, "Enum Reference.txt");
    enumOutput = FilePath::Normalize(enumOutput);
    ReMarkupEnumReferenceWriter::WriteEnumReference(enumOutput, doc);

    // output separate flags list
    String flagsOutput = FilePath::Combine(directory, baseFromMarkupDirectory, "Flags Reference.txt");
    flagsOutput = FilePath::Normalize(flagsOutput);
    ReMarkupFlagsReferenceWriter::WriteFlagsReference(flagsOutput, doc);
  }

  // check if we outputting commands
  if (!config.mCommandListFile.Empty())
  {
    String output = FilePath::Combine(config.mMarkupDirectory, baseFromMarkupDirectory, "Command Reference.txt");
    output = FilePath::Normalize(output);
    ReMarkupCommandRefWriter::WriteCommandRef(config.mCommandListFile, output);
  }

  // check if we are outputting events
  if (!config.mEventsOutputLocation.Empty())
  {
    String output = FilePath::Combine(config.mMarkupDirectory, baseFromMarkupDirectory, "Event Reference.txt");
    output = FilePath::Normalize(output);
    ReMarkupEventListWriter::WriteEventList(config.mEventsOutputLocation, output);
  }
}

void ReMarkupWriter::InsertHeaderLink(StringParam header)
{
  mOutput << "[[" << mDocURI << "#" << header.ToLower() << " | " << header << "]]";
}

////////////////////////////////////////////////////////////////////////
// ReMarkupWriter
////////////////////////////////////////////////////////////////////////
const String ReMarkupWriter::mEndLine("\n\n");
const String ReMarkupWriter::mQuoteLine("> ");

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
  mOutput << mQuoteLine << "``` lang=cpp, name=" << name << "\n";
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
  mOutput << " ";
}

void ReMarkupWriter::InsertLabel(StringParam label)
{
  mOutput << "===== " << label << mEndLine;
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

  if (!classDoc->mDescription.Empty())
  {
    writer.InsertHeaderAtCurrentHeaderLevel("Description");
    writer.InsertDivider();
    writer.mOutput << mQuoteLine << classDoc->mDescription << mEndLine;
  }

  writer.InsertJumpTable();

  if (!classDoc->mBaseClass.Empty())
  {
    writer.mOutput << "= BaseClass: ";
    writer.InsertTypeLink(classDoc->mBaseClass);
    writer.mOutput << "\n";
    writer.InsertDivider();
  }

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
  // subheader for the method name
  InsertHeaderAtCurrentHeaderLevel();
  // put link to return type 
  mOutput << method.mName <<  " : ";
  InsertTypeLink(method.mReturnType);

  if (method.mStatic)
  {
    mOutput << " {key static}";
  }
  mOutput << mEndLine;

  //Note: every line is going to have a '>' prepended to it to make it in a quote box

  // print the description directly under the header
  mOutput << mQuoteLine << method.mDescription << "\n";

  // print parameter table
  mOutput << mQuoteLine << "|Name|Type|Description|\n" << mQuoteLine << "|---|---|---|\n";

  forRange(ParameterDoc *param, method.mParameterList.All())
  {
    mOutput << mQuoteLine << "|" << param->mName << "|";
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

  // TODO: print the cpp codeblock

  //print the zilch codeblock
  InsertStartOfCodeBlock("Zilch");
  //IndentToCurrentLevel();
  mOutput << mQuoteLine << "function " << method.mName << "(";// Initialize(init : CogInitializer)

  for (uint i = 0; i < method.mParameterList.Size(); ++i)
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

  mOutput << ";\n> ``` " << mEndLine;

  InsertDivider();
}

void ReMarkupClassMarkupWriter::InsertProperty(PropertyDoc &propDoc)
{
  if (propDoc.mType.Empty())
    return;

  // subheader for the method name
  //mOutput << "====" << propDoc.mName << mEndLine;
  InsertHeaderAtCurrentHeaderLevel();

  mOutput << propDoc.mName << " : ";
  InsertTypeLink(propDoc.mType);

  if (propDoc.mReadOnly)
  {
    mOutput << " {key read-only}";
  }
  if (propDoc.mStatic)
  {
    mOutput << " {key static}";
  }

  mOutput << mEndLine;

  // print the description directly under the header
  mOutput << mQuoteLine << propDoc.mDescription << "\n";

  // TODO: print the cpp codeblock

  //print the zilch codeblock
  InsertStartOfCodeBlock("Zilch");
  //IndentToCurrentLevel();
  mOutput << mQuoteLine << "var " << propDoc.mName << " : " << propDoc.mType << ";" << mEndLine;

  InsertDivider();
}

void ReMarkupClassMarkupWriter::InsertJumpTable(void)
{
  // if we don't have stuff to jump to, don't make a jump table
  if (mClassDoc->mMethods.Empty() && mClassDoc->mProperties.Empty())
    return;

  InsertNewSectionHeader("Member Table");

  InsertDivider();

  // print the top of the table
  mOutput << "|Methods|Properties|\n|---|---|\n";
  uint methodsSize = mClassDoc->mMethods.Size();
  uint propsSize = mClassDoc->mProperties.Size();
  uint methodIter= 0;
  uint propsIter = 0;
  String prevMethod = "";

  for (; methodIter < methodsSize && propsIter < propsSize; ++methodIter, ++propsIter)
  {
    // skip overloads (when we see duplicates, don't link to them by iterating method but not prop)
    if (mClassDoc->mMethods[methodIter]->mName == prevMethod)
    {
      --propsIter;
      continue;
    }

    mOutput << "|";
    InsertMethodLink(mClassDoc->mMethods[methodIter]);
    mOutput << "|";
    InsertPropertyLink(mClassDoc->mProperties[propsIter]);
    mOutput<< "|\n";

    prevMethod = mClassDoc->mMethods[methodIter]->mName;
  }
  // print the rest of the methods if we had more methods then properties
  if (methodIter < methodsSize)
  {
    for (uint i = methodIter; i < mClassDoc->mMethods.Size(); ++i)
    {
      if (mClassDoc->mMethods[i]->mName == prevMethod)
        continue;

      mOutput << "|";
      InsertMethodLink(mClassDoc->mMethods[i]);
      mOutput << "| |\n";

      prevMethod = mClassDoc->mMethods[i]->mName;
    }
  }
  else
  {
    for (uint i = propsIter; i < mClassDoc->mProperties.Size(); ++i)
    {
      mOutput << "| |";
      InsertPropertyLink(mClassDoc->mProperties[i]);
      mOutput << "|\n";
    }
  }
  mOutput << mEndLine;
}

void ReMarkupClassMarkupWriter::InsertMethodLink(MethodDoc* methodToLink)
{
  StringBuilder headerLinkBuilder;
  headerLinkBuilder << methodToLink->mName;
  if (methodToLink->mReturnType == "Void")
  {
    headerLinkBuilder << "-void";
  }
  else
  {
    headerLinkBuilder << "-zero-engine-documentation";
  }
  if (methodToLink->mStatic)
  {
    // we use a key in markup to display tags, so in links they become '-k'
    headerLinkBuilder << "-k";
  }


  String headerLink = headerLinkBuilder.ToString();

  if (headerLink.SizeInBytes() > 24)
  {
    headerLink = headerLink.SubStringFromByteIndices(0, 24);

    if (headerLink.EndsWith("-"))
    {
      headerLink = headerLink.SubString(headerLink.Begin(), headerLink.End() - 1);
    }
  }
  

  mOutput << "[[" << mDocURI << "#" << headerLink.ToLower() << " | " << methodToLink->mName << "]]";
}

void ReMarkupClassMarkupWriter::InsertPropertyLink(PropertyDoc* propToLink)
{
  String headerLink = BuildString(propToLink->mName, "-zero-engine-documentation");

  headerLink = headerLink.SubStringFromByteIndices(0, 24);

  if (headerLink.EndsWith("-"))
  {
    headerLink = headerLink.SubString(headerLink.Begin(), headerLink.End() - 1);
  }

  mOutput << "[[" << mDocURI << "#" << headerLink.ToLower() << " | " << propToLink->mName << "]]";
}

////////////////////////////////////////////////////////////////////////
// ReMarkupEnumReferenceWriter
////////////////////////////////////////////////////////////////////////
ReMarkupEnumReferenceWriter::ReMarkupEnumReferenceWriter(StringParam name, StringParam uri) 
  : ReMarkupWriter(name, uri)
{

}


void ReMarkupEnumReferenceWriter::WriteEnumReference(StringParam outputFile, DocumentationLibrary &lib)
{
  // create the eventList
  ReMarkupEnumReferenceWriter writer("Enum List", BuildString(gBaseLink, "enum_reference/"));

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

void ReMarkupEnumReferenceWriter::InsertEnumEntry(EnumDoc* enumDoc)
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

void ReMarkupEnumReferenceWriter::InsertEnumTable(const Array<EnumDoc*>& EnumReference)
{
  // print the top of the table
  mOutput << "|Enum||\n|---|\n";

  forRange(EnumDoc* doc, EnumReference.All( ))
  {
    mOutput << "|";
    InsertHeaderLink(doc->mName);
    mOutput << "|\n";
  }
  mOutput << mEndLine;
}

////////////////////////////////////////////////////////////////////////
// ReMarkupFlagsReferenceWriter
////////////////////////////////////////////////////////////////////////
ReMarkupFlagsReferenceWriter::ReMarkupFlagsReferenceWriter(StringParam name, StringParam uri)
  : ReMarkupWriter(name, uri)
{
}


void ReMarkupFlagsReferenceWriter::WriteFlagsReference(StringParam outputFile, DocumentationLibrary &lib)
{
  // create the eventList
  ReMarkupFlagsReferenceWriter writer("Flags List", BuildString(gBaseLink, "flags_reference/"));

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



void ReMarkupFlagsReferenceWriter::InsertFlagsEntry(EnumDoc *flags)
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

void ReMarkupFlagsReferenceWriter::InsertFlagTable(const Array<EnumDoc*>& FlagsReference)
{
  // print the top of the table
  mOutput << "|Flags||\n|---|\n";

  forRange(EnumDoc *doc, FlagsReference.All())
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

  cmdListDoc.Sort();

  Array<CommandDoc *> &cmdArray = cmdListDoc.mCommands;

  // create the command writer
  ReMarkupCommandRefWriter writer("Zero Commands", BuildString(gBaseLink, "command_reference/"));

  StartHeaderSection(writer);

  // iterate over all commands and insert them
  forRange(CommandDoc *cmdDoc, cmdArray.All())
  {
    writer.WriteCommandEntry(*cmdDoc);
  }

  EndHeaderSection(writer);

  writer.WriteOutputToFile(outputPath);
}


void ReMarkupCommandRefWriter::WriteCommandEntry(CommandDoc &cmdDoc)
{
  static const char *validMenuLocations[] =
  { "Project", "Edit", "Create", "Select", "Resources", "Windows", "Help" };
  static const unsigned int validMenuItemCount = 7;

  InsertNewSectionHeader(cmdDoc.mName);

  if (cmdDoc.mName.Contains("Create"))
  {
    cmdDoc.mTags.PushBack("Create");
  }

  bool isValidMenuItem = false;

  if (!cmdDoc.mTags.Empty())
  {
    // check if this is a valid menu item
    for (uint i = 0; i < validMenuItemCount; ++i)
    {
      if (cmdDoc.mTags[0] == validMenuLocations[i])
      {
        isValidMenuItem = true;
        break;
      }
    }
  }

  if (!cmdDoc.mDescription.Empty())
    mOutput << cmdDoc.mDescription << mEndLine;

  mOutput << "|Tags|Shortcut|Menu Selection|\n" << "|---|---|---|\n";

  mOutput << "|";

  if (cmdDoc.mTags.Empty())
  {
    mOutput << "No Tags | ";
  }
  else
  {
    // list the first tag
    mOutput << cmdDoc.mTags[0] << " | ";
  }

  if (cmdDoc.mShortcut.Empty())
  {
    mOutput << "No Keyboard Shortcut | ";
  }
  else
  {
    String shortcut = cmdDoc.mShortcut.Replace("+", " ");
    mOutput << "{key " << shortcut << "}" << " | ";
  }

  if (isValidMenuItem)
  {
    // the first tag is always the menu the option lives in
    mOutput << "{nav name=" << cmdDoc.mTags[0] << "> "
      << cmdDoc.mName << "}" << " |"<<  mEndLine;
  }
  else
  {
    mOutput << " |" << mEndLine;
  }

  //mOutput << ".. include:: CommandPageExtensions/" << cmdDoc.mName << ".rst\n\n";
  InsertDivider();
}

}
