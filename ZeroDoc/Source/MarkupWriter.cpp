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
  UnsortedMap<String, Array<String>> gDerivedClasses;
UnsortedMap<String, String> gLinkMap;
String gBaseLink = "zero_engine_documentation/code_reference/";
String gBaseClassLink = BuildString(gBaseLink, "class_reference/");
String gBaseZilchTypesLink = BuildString(gBaseLink, "zilch_base_types/");
String gBaseEnumTypesLink = BuildString(gBaseLink, "enum_reference/#");
String gBaseFlagsTypesLink = BuildString(gBaseLink, "flags_reference/#");


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
  String baseFromMarkupDirectory = "zero_engine_documentation\\code_reference";
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
    doc.FinalizeDocumentation();


    DocToTags tagged;

    ArraySet<String> sortedTags;

    // create the base wikipage for the codeRef
    StringBuilder codeRefIndex;

    // create a sibling to the base page with categories for tags
    StringBuilder tagsCodeRefIndex;

    // create the base wikipage for zilch core types
    StringBuilder zilchCoreIndex;
  
    // Add all flags to the linkMap
    forRange(auto& flagDoc, doc.mFlags.All())
    {
      gLinkMap[flagDoc->mName] = BuildString(gBaseFlagsTypesLink, flagDoc->mName.ToLower(), "|", flagDoc->mName);
    }
    // Add all enums to the linkMap
    forRange(auto& enumDoc, doc.mEnums.All())
    {
      gLinkMap[enumDoc->mName] = BuildString(gBaseEnumTypesLink, enumDoc->mName.ToLower(), "|", enumDoc->mName);
    }
    // Add all classes to the linkMap
    forRange(ClassDoc* classDoc, doc.mClasses.All())
    {
      // check for core library types assuming those are just zilch
      if (classDoc->mLibrary == "Core")
      {
        gLinkMap[classDoc->mName] = BuildString(gBaseZilchTypesLink, classDoc->mName.ToLower(), "/");

        if (classDoc->mName.Contains("["))
        {
          String name = classDoc->mName.Replace(String('['), String('_'));
          name = name.Replace(String(']'), String(' '));
          gLinkMap[classDoc->mName] = BuildString(gBaseZilchTypesLink, name.ToLower(), "/");
        }
      }
      else
      {
        gLinkMap[classDoc->mName] = BuildString(gBaseClassLink, classDoc->mName.ToLower(), "/");

        if (classDoc->mName.Contains("["))
        {
          String name = classDoc->mName.Replace(String('['), String('_'));
          name = name.Replace(String(']'), String(' '));
          gLinkMap[classDoc->mName] = BuildString(gBaseZilchTypesLink, name.ToLower(), "/");
        }
      }
    }
    //Upload the class' page to the wiki, making sure to perform the link replacements
    forRange(ClassDoc* classDoc, doc.mClasses.All())
    {
      if (classDoc->mLibrary == "Core")
      {
		    String className = classDoc->mName;
        String filename = BuildString(className, ".txt");

        //filename = filename.Replace(" ", "_");
        //filename = filename.ToLower();

        String fullPath = FilePath::Combine(directory, baseZilchTypesDirectory);

        CreateDirectoryAndParents(fullPath);

        fullPath = FilePath::Combine(fullPath, filename);

        fullPath = FilePath::Normalize(fullPath);

        ReMarkupClassMarkupWriter::WriteClass(fullPath, classDoc, doc, tagged, sortedTags);

        // [[wiki page | name]]
        zilchCoreIndex << gBullet << "[[" << gLinkMap[classDoc->mName] << "]] \n";
      }
      else
      {
        String fullPath = FilePath::Combine(directory,baseClassDirectory);

        CreateDirectoryAndParents(fullPath);

        fullPath = FilePath::CombineWithExtension(fullPath, classDoc->mName, ".txt");

        ReMarkupClassMarkupWriter::WriteClass(fullPath, classDoc, doc, tagged, sortedTags);

        // [[wiki page | name]]
        codeRefIndex << gBullet << "[[" << gLinkMap[classDoc->mName] << "]] \n";
      }
    }

    // create the sorted file
    forRange(String& tag, sortedTags.All())
    {
      // section header for the tag
      tagsCodeRefIndex << "= " << tag;
      // divider
      tagsCodeRefIndex << "\n---  \n";

      Array<ClassDoc*> &classesToLink = tagged[tag];

      forRange(ClassDoc *classToLink, classesToLink.All())
      {
        tagsCodeRefIndex << gBullet << "[[" << gLinkMap[classToLink->mName] << "]] \n";
      }
    }

    WriteStringRangeToFile(FilePath::Combine(directory, baseFromMarkupDirectory,"class_reference.txt"), codeRefIndex.ToString());
    //gah what shuld it be named
    WriteStringRangeToFile(FilePath::Combine(directory, baseFromMarkupDirectory, "classes_by_tag_reference.txt"), tagsCodeRefIndex.ToString());
    WriteStringRangeToFile(FilePath::Combine(directory, baseFromMarkupDirectory,"zilch_base_types.txt"), zilchCoreIndex.ToString());

    WriteTagIndices(directory, tagged, doc);

    // Output separate enum list
    String enumOutput = FilePath::Combine(directory, baseFromMarkupDirectory, "enum_reference.txt");
    enumOutput = FilePath::Normalize(enumOutput);
    ReMarkupEnumReferenceWriter::WriteEnumReference(enumOutput, doc);

    // output separate flags list
    String flagsOutput = FilePath::Combine(directory, baseFromMarkupDirectory, "flags_reference.txt");
    flagsOutput = FilePath::Normalize(flagsOutput);
    ReMarkupFlagsReferenceWriter::WriteFlagsReference(flagsOutput, doc);
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

  if (!config.mAttributesOutputLocation.Empty())
  {
    String output = FilePath::Combine(config.mMarkupDirectory, baseFromMarkupDirectory);
    output = FilePath::Normalize(output);
    ReMarkupAttributeRefWriter::WriteAttributeRef(config.mAttributesOutputLocation, output);
  }
}

void ReMarkupWriter::InsertHeaderLink(StringParam header)
{
  String headerLink = CutLinkToMaxSize(header);

  mOutput << "[[" << mDocURI << "#" << headerLink.ToLower() << " | " << header << "]]";
}

String Zero::ReMarkupWriter::CutLinkToMaxSize(StringParam link)
{
  String retLink = link;
  if (retLink.SizeInBytes() > 24)
  {
    retLink = retLink.SubStringFromByteIndices(0, 24);

    if (retLink.EndsWith("-"))
    {
      retLink = retLink.SubString(retLink.Begin(), retLink.End() - 1);
    }
  }
  return retLink;
}

////////////////////////////////////////////////////////////////////////
// ReMarkupWriter
////////////////////////////////////////////////////////////////////////
const String ReMarkupWriter::mEndLine("\n\n");
const String ReMarkupWriter::mQuoteLine("> ");
const String ReMarkupWriter::mNoteLine("(NOTE) ");

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
  ClassDoc* classDoc, DocumentationLibrary &lib, DocToTags& tagged, ArraySet<String>&sortedTags)
{
  // do the magic for getting directory and file
  ReMarkupClassMarkupWriter writer(classDoc->mName, classDoc, gLinkMap[classDoc->mName]);

  // Set up the tags for this class and print them at the top
  forRange(String& tag, classDoc->mTags.All())
  {
    tagged[tag].PushBack(classDoc);

    sortedTags.FindOrInsert(tag);
    
    writer.mOutput << " {key " << tag << "}";
  }

  String& libName = classDoc->mLibrary;
  if (!libName.Empty())
  {
    StringRange libSubstring = libName.FindFirstOf("Library");

    String shortenedName = libSubstring.Empty() ?
      libName : libName.SubString(libName.Begin(), libSubstring.Begin());

    // if we do not already have this class under the tag named same as library, add it
    if (!tagged[shortenedName].Contains(classDoc))
    {
      // add our library as a tag as well
      tagged[shortenedName].PushBack(classDoc);
      sortedTags.FindOrInsert(shortenedName);
      writer.mOutput << " {key " << shortenedName << "}";
    }
  }
  else if (classDoc->mTags.Empty())
  {
    tagged["Not Tagged"].PushBack(classDoc);

    sortedTags.FindOrInsert("[Not Tagged]");
  }


  // if we added tags to the file, add a newline
  if (!classDoc->mTags.Empty())
    writer.mOutput << writer.mEndLine;

  writer.mOutput << mEndLine;

  if (!classDoc->mDescription.Empty())
  {
    writer.mOutput << mNoteLine << classDoc->mDescription << mEndLine;
  }

  writer.BuildDerivedList(lib);

  if (!classDoc->mBaseClass.Empty())
  {
    writer.mBases.PushBack(classDoc->mBaseClass);
  }

  writer.InsertJumpTable();

  writer.InsertHeaderAtCurrentHeaderLevel("Properties");
  writer.InsertDivider();

  forRange(PropertyDoc *prop, classDoc->mProperties.All())
  {
    writer.InsertProperty(*prop);
  }

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

void ReMarkupClassMarkupWriter::BuildDerivedList(DocumentationLibrary& lib)
{
  forRange(ClassDoc* derivedClass, lib.mClasses.All())
  {
    String &baseClassName = derivedClass->mBaseClass;

    if (baseClassName != mClassDoc->mName)
      continue;

    mDerivedClasses.PushBack(derivedClass->mName);
  }
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

  bool isConstructor = method.mName == mName;

  if (method.mStatic || isConstructor)
  {
    mOutput << mEndLine;
  }

  if (method.mStatic)
  {
    mOutput <<" {key static}";
  }

  if (isConstructor)
  {
    mOutput << " {key constructor}";
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

  mOutput << "\n> ``` " << mEndLine;

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

  if (propDoc.mReadOnly || propDoc.mStatic)
  {
    mOutput << mEndLine;
  }

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
  mOutput << mQuoteLine << "var " << propDoc.mName << " : " << propDoc.mType << mEndLine;

  InsertDivider();
}


void ReMarkupClassMarkupWriter::InsertJumpTable(void)
{
  Array<MethodDoc *> noOverloadedMethods;

  String prevMethodName = "";
  forRange(MethodDoc* doc, mClassDoc->mMethods.All())
  {
    if (doc->mName == prevMethodName)
      continue;

    noOverloadedMethods.PushBack(doc);
    prevMethodName = doc->mName;
  }

  // print the top of the table
  mOutput << "|Methods|Properties|Base Classes|Derived Classes|\n|---|---|---|---|\n";
  uint methodsSize = noOverloadedMethods.Size();
  uint propsSize = mClassDoc->mProperties.Size();
  uint basesSize = mBases.Size();
  uint derivedSize = mDerivedClasses.Size();

  // we are going to use one iterator for all types in table so get the largest one
  uint iterLimit = Math::Max(Math::Max(methodsSize, propsSize), Math::Max(basesSize,derivedSize));

  // if we don't have stuff to jump to, don't make a jump table
  if (iterLimit == 0)
    return;

  // print entries into table, skipping lists that our now out of range of iterator
  for (uint i = 0; i < iterLimit; ++i)
  {
    mOutput << "|";
    if (i < methodsSize)
    {
      InsertMethodLink(noOverloadedMethods[i]);
    }
    else
    {
      mOutput << " ";
    }
    mOutput << "|";

    if (i < propsSize)
    {
      InsertPropertyLink(mClassDoc->mProperties[i]);
    }
    else
    {
      mOutput << " ";
    }
    mOutput << "|";

    if (i < basesSize)
    {
      InsertTypeLink(mBases[i]);
    }
    else
    {
      mOutput << " ";
    }
    mOutput << "|";

    if (i < derivedSize)
    {
      InsertTypeLink(mDerivedClasses[i]);
    }
    else
    {
      mOutput << " ";
    }

    mOutput << "|\n";
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

  String headerLink = headerLinkBuilder.ToString();

  headerLink = headerLink.Replace("[", "-");
  headerLink = headerLink.Replace("]", "");

  headerLink = CutLinkToMaxSize(headerLink);

  String name = methodToLink->mName.CompareTo(mClassDoc->mName) == 0 ? "Constructor" : methodToLink->mName;

  mOutput << "[[" << mDocURI << "#" << headerLink.ToLower() << " | " << name << "]]";
}

void ReMarkupClassMarkupWriter::InsertPropertyLink(PropertyDoc* propToLink)
{
  StringBuilder headerLinkBuilder;

  headerLinkBuilder.Append(propToLink->mName);

  // if the type is a link, add the link fluff to the name, otherwise add the type
  if (gLinkMap.ContainsKey(propToLink->mType))
  {
     headerLinkBuilder.Append("-zero-engine-documentation");
  }
  else
  {
    headerLinkBuilder << "-" << propToLink->mType;
  }

  String headerLink = headerLinkBuilder.ToString();
  
  if (headerLink.SizeInBytes() > 24)
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
    // insert table values, making them literals to avoid creating accidental links
    mOutput << "|" << "%%%" << enumDescPair.first << "%%%" << "|" << "%%%" << enumDescPair.second << "%%%" << "|\n";
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

  // removing senders output since "sends" is used too many places to be useful atm
  /*
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
  */
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

////////////////////////////////////////////////////////////////////////
// ReMarkupAttributeRefWriter
////////////////////////////////////////////////////////////////////////
ReMarkupAttributeRefWriter::ReMarkupAttributeRefWriter(StringParam name, StringParam uri)
  : ReMarkupWriter(name, uri)
{
}
/*
I have 3 different tables, one for ObjectAttributes, one for functionAttributes, 
one for PropertyAttributes, each one then having a column for user and developer attributes
*/
void ReMarkupAttributeRefWriter::WriteAttributeRef(StringParam listFilepath, StringParam outputPath)
{
  
  // setup all the data we need before we can write the file
  AttributeLoader loader;

  loader.LoadUserAttributeFile(listFilepath);
  loader.FinalizeAttributeFile();

  AttributeDocList* docList = loader.GetAttributeList();

  // object
  Array<AttributeDoc *> mZilchObjectAttributes;
  Array<AttributeDoc *> mCppObjectAttributes;
  // property
  Array<AttributeDoc *> mZilchPropertyAttributes;
  Array<AttributeDoc *> mCppPropertyAttributes;
  // function
  Array<AttributeDoc *> mZilchFunctionAttributes;
  Array<AttributeDoc *> mCppFunctionAttributes;

  forRange(AttributeDoc* attrib, docList->mObjectAttributes.All())
  {
    if (attrib->mDeveloperAttribute)
      mCppObjectAttributes.PushBack(attrib);
    else
      mZilchObjectAttributes.PushBack(attrib);
  }
  forRange(AttributeDoc* attrib, docList->mPropertyAttributes.All())
  {
    if (attrib->mDeveloperAttribute)
      mCppPropertyAttributes.PushBack(attrib);
    else
      mZilchPropertyAttributes.PushBack(attrib);
  }
  forRange(AttributeDoc* attrib, docList->mFunctionAttributes.All())
  {
    if (attrib->mDeveloperAttribute)
      mCppFunctionAttributes.PushBack(attrib);
    else
      mZilchFunctionAttributes.PushBack(attrib);
  }

  // object
  WriteChildAttributeRef(mZilchObjectAttributes, mCppObjectAttributes, docList->mObjectAttributes,
    "Object", FilePath::Combine(outputPath, "attribute_reference","object_attribute_reference.txt"));

  // function
  WriteChildAttributeRef(mZilchFunctionAttributes, mCppFunctionAttributes, docList->mFunctionAttributes,
    "Function", FilePath::Combine(outputPath, "attribute_reference", "function_attribute_reference.txt"));

  // property
  WriteChildAttributeRef(mZilchPropertyAttributes, mCppPropertyAttributes, docList->mPropertyAttributes,
    "Property", FilePath::Combine(outputPath, "attribute_reference", "property_attribute_reference.txt"));
}

void ReMarkupAttributeRefWriter::WriteChildAttributeRef(Array<AttributeDoc *>& zilchAttrib,
  Array<AttributeDoc *>& cppAttrib, Array<AttributeDoc *>& allAttrib, StringParam attribType, StringParam outputFile)
{
  ReMarkupAttributeRefWriter writer(BuildString(attribType," Attribute Reference")
    , BuildString(gBaseLink, "attribute_reference/", attribType.ToLower(), "_attribute_reference/"));
  
  writer.WriteAttributeTable(cppAttrib, zilchAttrib);

  writer.InsertDivider();
  forRange(AttributeDoc* attrib, allAttrib.All())
  {
    writer.InsertAttributeEntry(attrib);
  }

  writer.WriteOutputToFile(outputFile);
}


void ReMarkupAttributeRefWriter::InsertAttributeEntry(AttributeDoc* attribToAdd)
{
  InsertHeaderAtCurrentHeaderLevel();

  mOutput << attribToAdd->mName << mEndLine;

  if (attribToAdd->mDeveloperAttribute)
  {
    mOutput << " {key cpp-attribute}";
  }
  else
  {
    mOutput << " {key zilch-attribute}";
  }

  if (attribToAdd->mAllowMultiple)
  {
    mOutput << " {key allow-multiple}";
  }

  if (attribToAdd->mAllowStatic)
  {
    mOutput << " {key allow-static}";
  }

  mOutput << mEndLine;

  // print the description directly under the header
  if (!attribToAdd->mDescription.Empty())
    mOutput << mQuoteLine << attribToAdd->mDescription << "\n";

  InsertDivider();
}

void ReMarkupAttributeRefWriter::WriteAttributeTable(Array<AttributeDoc*>& cppAttrib, Array<AttributeDoc*>& zilchAttrib)
{
  // print the top of the table
  mOutput << "|Zilch Attributes|C++ Attributes|\n|---|---|\n";

  uint cppSize = cppAttrib.Size();
  uint zilchSize = zilchAttrib.Size();
  

  // we are going to use one iterator for all types in table so get the largest one
  uint iterLimit = Math::Max(cppSize, zilchSize);

  // print entries into table, skipping lists that our now out of range of iterator
  for (uint i = 0; i < iterLimit; ++i)
  {
    mOutput << "|";
    if (i < zilchSize)
    {
      InsertHeaderLink(zilchAttrib[i]->mName);
    }
    else
    {
      mOutput << " ";
    }
    mOutput << "|";

    if (i < cppSize)
    {
      InsertHeaderLink(cppAttrib[i]->mName);
    }
    else
    {
      mOutput << " ";
    }

    mOutput << "|\n";
  }
  mOutput << mEndLine;
}

}
