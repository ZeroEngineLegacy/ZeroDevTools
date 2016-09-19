///////////////////////////////////////////////////////////////////////////////
///
/// \file RawDocumentation.cpp
/// Slight reimplementations of the documentation classes for Raw doc
///
/// Authors: Joshua Shlemmer
/// Copyright 2015-2016, DigiPen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////
#include "Precompiled.hpp"

#include "RawDocumentation.hpp"
#include "Serialization/Simple.hpp"
#include "Platform\FileSystem.hpp"
#include "TinyXmlHelpers.hpp"
#include "MacroDatabase.hpp"

#include <Engine/Documentation.hpp>

namespace Zero
{

  ////////////////////////////////////////////////////////////////////////
  // Helpers
  ////////////////////////////////////////////////////////////////////////

  String GetArgumentIfString(TypeTokens &fnCall, uint argPos)
  {
    uint start = (uint)-1;
    // get first OpenParen index
    for (uint i = 0; i < fnCall.size(); ++i)
    {
      if (fnCall[i].mEnumTokenType == DocTokenType::OpenParen)
      {
        start = i;
        break;
      }
    }

    // nope out of here if we did not find a starting paren
    if (start == (uint)-1)
      return "";

    int parenCount = 0;
    uint argCount = 0;
    // start iterating from the character after that to the end
    for (uint i = start; i < fnCall.size(); ++i)
    {
      DocToken &currToken = fnCall[i];

      // if we hit an OpenParen
      if (currToken.mEnumTokenType == DocTokenType::OpenParen)
      {
        // add it to our open paren counter and continue
        ++parenCount;
        continue;
      }
      // if we hit a CloseParen
      else if (currToken.mEnumTokenType == DocTokenType::CloseParen)
      {
        // subtract from our paren counter and continue
        --parenCount;
        continue;
      }
      // if parenCount is greater then one since we count the first one
      //just skip this token by continuing
      if (parenCount > 1)
        continue;

      // if token is a comma
      if (currToken.mEnumTokenType == DocTokenType::Comma)
      {
        // add it to our arg counter
        ++argCount;
        // if arg counter is equal to argPos
        if (argCount == argPos)
        {
          // check if prev token is a string
          // if it is, return it
          // otherwise return an empty string
          if (fnCall[i - 1].mEnumTokenType == DocTokenType::StringLiteral)
            return fnCall[i - 1].mText;
          return "";
        }
      }
    }
    return "";
  }

  String GetCodeFromDocumentFile(TiXmlDocument *doc)
  {
    StringBuilder codeBuilder;

    // grab the class
    TiXmlElement* cppDef = doc->FirstChildElement(gElementTags[eDOXYGEN])
      ->FirstChildElement(gElementTags[eCOMPOUNDDEF]);

    TiXmlElement *programList = GetFirstNodeOfChildType(cppDef, "programlisting")->ToElement();

    TiXmlNode *firstCodeline = GetFirstNodeOfChildType(programList, gElementTags[eCODELINE]);
    TiXmlNode *endCodeline = GetEndNodeOfChildType(programList, gElementTags[eCODELINE]);

    for (TiXmlNode *codeline = firstCodeline;
      codeline != endCodeline;
      codeline = codeline->NextSibling())
    {
      StringBuilder lineBuilder;

      GetTextFromAllChildrenNodesRecursively(codeline, &lineBuilder);

      if (lineBuilder.GetSize() == 0)
        continue;

      String codeString = lineBuilder.ToString();

      if (codeString[0] == '#' ||
        (codeString.size() > 1 && codeString[0] == '/' && codeString[1] == '/'))
      {
        continue;
      }

      codeBuilder << codeString;
    }

    return codeBuilder.ToString();
  }

  String GetDoxyfileNameFromSourceFileName(StringParam filename)
  {
    StringBuilder builder;

    builder << '_';

    uint extensionLocation = filename.FindLastOf('.');

    String extension = filename.sub_string(extensionLocation + 1, 3);

    if (extension != "cpp" && extension != "hpp")
      return "";

    bool prevLowercase = false;

    for(uint i = 0; i < extensionLocation; ++i)
    {
      char c = filename[i];

      if (IsUpper(c) && prevLowercase)
        builder << '_';

      prevLowercase = IsLower(c);

      builder << (char)ToLower(c);
    }

    builder << "_8" << extension;

    builder << ".xml";

    return builder.ToString();
  }

  String TrimTypeTokens(const TypeTokens &tokens)
  {
    StringBuilder builder;

    for (uint i = 0; i < tokens.size(); ++i)
    {
      const DocToken &token = tokens[i];

      builder.Append(token.mText);

      if (token.mEnumTokenType > DocTokenType::QualifiersStart)
        builder.Append(" ");
    }
    return builder.ToString();
  }

  DocDfaState* DocLangDfa::Get(void)
  {
    static DocDfaState *instance;

    return instance ? instance : instance = CreateLangDfa();
  }

  // returns first child of element with value containing tag type 'type'
  TiXmlNode* GetFirstNodeOfChildType(TiXmlElement* element, StringRef type)
  {
    for (TiXmlNode* node = element->FirstChild(); node; node = node->NextSibling())
    {
      if (type == node->Value())
        return node;
    }
    return nullptr;
  }

  // returns one past the last chiled of tag type 'type', returns null if it DNE
  TiXmlNode* GetEndNodeOfChildType(TiXmlElement* element, StringRef type)
  {
    TiXmlNode* prevNode = nullptr;
    for (TiXmlNode* node = element->LastChild(); node; node = node->PreviousSibling())
    {
      if (type == node->Value())
        return prevNode;
      prevNode = node;
    }
    return nullptr;
  }

  // does as it says, removes all spaces from str and outputs it into the stringbuilder
  void CullAllSpacesFromString(StringRef str, StringBuilder* output)
  {
    forRange(char c, str.all())
    {
      if (c != ' ')
        output->Append(c);
    }
  }

  // gets Text from a Text type node or a ref to a Text type node
  //const char* GetTextFromNode(TiXmlNode* node)
  //{
  //  if (!node)
  //    return nullptr;
  //
  //  if (node->Type() == TiXmlNode::TEXT)
  //    return node->ToText()->Value();
  //  else //Check if it has a child, if it does assume ref
  //  {
  //    TiXmlNode* child = node->FirstChild();
  //
  //    return child ? node->FirstChild()->ToText()->Value() : nullptr;
  //  }
  //}

  void GetTextFromAllChildrenNodesRecursively(TiXmlNode* node, StringBuilder* output)
  {
    // just keep trodding our way down the node structure until we get every single text node
    // (I am tired of edge cases making us miss text so this is the brute force hammer)
    for (TiXmlNode *child = node->FirstChild(); child != nullptr; child = child->NextSibling())
    {
      if (child->Type() != TiXmlNode::TEXT)
      {
        GetTextFromAllChildrenNodesRecursively(child, output);
        continue;
      }

      output->Append(child->Value());
      output->Append(" ");
    }
  }

  String GetTextFromAllChildrenNodesRecursively(TiXmlNode* node)
  {
    StringBuilder output;

    GetTextFromAllChildrenNodesRecursively(node, &output);

    return output.ToString();
  }

  void GetTextFromChildrenNodes(TiXmlNode* node, StringBuilder* output)
  {
    TiXmlElement* element = node->ToElement();

    TiXmlNode* firstNode = GetFirstNodeOfChildType(element, gElementTags[eTYPE]);

    if (firstNode)
    {
      DebugBreak();
    }

    TiXmlNode* endNode = GetEndNodeOfChildType(element, gElementTags[eTYPE]);

    if (!firstNode)
    {
      TiXmlNode* firstNode = GetFirstNodeOfChildType(element, gElementTags[eHIGHLIGHT]);

      if (firstNode != nullptr)
        DebugBreak();

      TiXmlNode* endNode = GetEndNodeOfChildType(element, gElementTags[eHIGHLIGHT]);
    }

    if (firstNode == nullptr)
    {
      node = element->FirstChild();
      endNode = element->LastChild();

      for (;node != endNode->NextSibling(); node = node->NextSibling())
      {
        if (node->Type() == TiXmlNode::TEXT)
        {
          output->Append(node->Value());
          output->Append(" ");
        }
      }
    }
    else for (TiXmlNode* node = firstNode; node != endNode; node = node->NextSibling())
    {
      output->Append(node->Value());
      output->Append(" ");
    }
  }

  void getTextFromParaNodes(TiXmlNode* node, StringBuilder* output)
  {
    TiXmlElement* element = node->ToElement();

    TiXmlNode* firstPNode = GetFirstNodeOfChildType(element, gElementTags[ePARA]);
    TiXmlNode* endPNode = GetEndNodeOfChildType(element, gElementTags[ePARA]);

    for (TiXmlNode* paraNode = firstPNode; paraNode != endPNode; paraNode = paraNode->NextSibling())
    {
      for (TiXmlNode* node = paraNode->FirstChild(); node != nullptr; node = node->NextSibling())
      {
        if (node->Type() == TiXmlNode::TEXT)
        {
          output->Append(node->ToText()->Value());
          output->Append(" ");
        }
        else if (node->Type() == TiXmlNode::ELEMENT)
        {
          for (TiXmlNode* eleChild = node->FirstChild();
            eleChild != nullptr;
            eleChild = eleChild->NextSibling())
          {
            output->Append(GetTextFromAllChildrenNodesRecursively(eleChild));
            output->Append(" ");
          }
        }
      }
    }
  }

  // replaces token at location with the tokens from the typedef passed in
  uint ReplaceTypedefAtLocation(TypeTokens& tokenArray
    , DocToken* location, RawTypedefDoc& tDef)
  {
    TypeTokens newArray;

    forRange(DocToken& token, tokenArray.all())
    {
      if (&token == location)
      {
        forRange(DocToken& defToken, tDef.mDefinition.all())
        {
          newArray.push_back(defToken);
        }
      }
      else
      {
        newArray.push_back(token);
      }
    }

    tokenArray = newArray;

    return tDef.mDefinition.size() - 1;
  }

  // get type string from an element that contains Text nodes
  void BuildFullTypeString(TiXmlElement* element, StringBuilder* output)
  {
    TiXmlNode* typeNode = GetFirstNodeOfChildType(element, gElementTags[eTYPE]);

    if (!typeNode)
      return;

    GetTextFromAllChildrenNodesRecursively(typeNode, output);
  }


  // replaces type tokens with corresponding mTypedefs
  // returns true if any replacements were made
  bool NormalizeTokensFromTypedefs(TypeTokens& tokens, RawTypedefLibrary* defLib, 
    RawNamespaceDoc& classNamespace)
  {
    bool madeReplacements = false;

    // loop over tokens
    for (uint i = 0; i < tokens.size(); ++i)
    {
      //should be moved into the loop
      Array<String>& names = classNamespace.mNames;

      DocToken& token = tokens[i];

      // check any namespaces this token list is inside
      
      String key = token.mText;

      // loop over possible namespace'd version
      for (int j = -1; j < (int)names.size(); ++j)
      {
        StringBuilder builder;

        for (uint k = 0; (int)k <= j; ++k)
          builder.Append(names[k]);

        builder.Append(token.mText);
        
        key = builder.ToString();

        if (defLib->mTypedefs.containsKey(key))
        {
          TypeTokens* typedefTokens = &defLib->mTypedefs[key]->mDefinition;

          // make sure they are not just the same tokens
          if (tokens.size() >= typedefTokens->size() 
            && ContainsFirstTypeInSecondType(*typedefTokens, tokens))//tokens == *typedefTokens)
          {
            break;
          }
          // this next chunk of crazy to make sure we don't redundantly expand any typedefs
          else
          {
            // first we have to make sure our token ranges are of valid size
            if ((int)i - 2 > 0 && typedefTokens->size() >= 3)
            {
              bool equal = true;

              // the magic number 3 is because we are checking for this: typedef name ns::name
              for (uint m = 0; m < 3; ++m)
              { 
                if ((*typedefTokens)[m].mText != tokens.sub_range(i - 2, 3)[m].mText)
                  equal = false;
              }
              if (equal)
              {
                break;
              }
            }
          }

          madeReplacements = true;
          i += ReplaceTypedefAtLocation(tokens, &token,*defLib->mTypedefs[key]);
          break;
        }
      }
      
    }
    return madeReplacements;
  }

  void GetFilesWithPartialName(StringParam basePath,StringParam partialName, Array<String>* output)
  {
    FileRange range(basePath);
    for (; !range.empty(); range.popFront())
    {
      FileEntry entry = range.frontEntry();

      // if this is a subdirectory, recurse down the directory
      String filePath = entry.GetFullPath();
      if (IsDirectory(filePath))
      {
        GetFilesWithPartialName(filePath, partialName, output);
      }
      // if we are not a directory, see if we are a file with partialName contained in filename
      else
      {
        if (entry.mFileName.Contains(partialName))
          output->push_back(filePath);
      }
    }
  }

  void GetFilesWithPartialName(StringParam basePath, StringParam partialName,
    IgnoreList &ignoreList, Array<String>* output)
  {
    FileRange range(basePath);
    for (; !range.empty(); range.popFront())
    {
      FileEntry entry = range.frontEntry();

      // if this is a subdirectory, recurse down the directory
      String filePath = entry.GetFullPath();

      if (ignoreList.DirectoryIsOnIgnoreList(filePath))
      {
        WriteLog("Ignoring file/directory: %s\n", filePath.c_str());
        return;
      }

      if (IsDirectory(filePath))
      {
        GetFilesWithPartialName(filePath, partialName, ignoreList, output);
      }
      // if we are not a directory, see if we are a file with partialName contained in filename
      else
      {
        if (entry.mFileName.Contains(partialName))
          output->push_back(filePath);
      }
    }
  }

  String GetFileWithExactName(StringParam basePath, StringParam exactName)
  {
    FileRange range(basePath);
    for (; !range.empty(); range.popFront())
    {
      FileEntry entry = range.frontEntry();

      // if this is a subdirectory, recurse down the directory
      String filePath = entry.GetFullPath();
      if (IsDirectory(filePath))
      {
        String retval = GetFileWithExactName(filePath, exactName);
        if (retval == "")
          continue;
        return retval;
      }
      // if we are not a directory, see if we are a file with partialName contained in filename
      else
      {
        if (entry.mFileName == exactName)
          return filePath;
      }
    }
    return "";
  }

  String CleanRedundantSpacesInDesc(StringParam description)
  {
    StringBuilder builder;

    bool prevSpace = false;

    for (uint i = 0; i < description.size(); ++i)
    {
      char currChar = description[i];

      if (currChar == ' ')
      {
        if (prevSpace)
          continue;

        prevSpace = true;
      }
      else
        prevSpace = false;

      builder.Append(currChar);
    }
    return builder.ToString();
  }

  void OutputListOfObjectsWithoutDesc(const DocumentationLibrary &trimDoc)
  {
    DocLogger *log = DocLogger::Get();

    WriteLog("Objects Missing Description:\n");

    for (uint i = 0; i < trimDoc.mClasses.size(); ++i)
    {
      ClassDoc *currClass = trimDoc.mClasses[i];
      if (currClass->mDescription == "")
        WriteLog("Class '%s' missing description\n", currClass->mName.c_str());

      for (uint j = 0; j < currClass->mMethods.size(); ++j)
      {
        MethodDoc *currMethod = currClass->mMethods[j];
        if (currMethod->mDescription == "")
        {
          WriteLog("Method '%s.%s' missing description\n"
            , currClass->mName.c_str(), currMethod->mName.c_str());
        }

      }
      for (uint j = 0; j < currClass->mProperties.size(); ++j)
      {
        PropertyDoc *currProp = currClass->mProperties[j];
        if (currProp->mDescription == "")
        {
          WriteLog("Property '%s.%s' missing description\n"
            , currClass->mName.c_str(), currProp->mName.c_str());
        }
      }
    }
  }

  bool ContainsFirstTypeInSecondType(TypeTokens &firstType, TypeTokens &secondType)
  {
    if (firstType.empty())
      return false;

    bool containedType = false;
    uint matchStart = (uint)-1;

    // we have to find the start of the match
    for (uint i = 0; i < secondType.size(); ++i)
    {
      if (secondType[i] == firstType[0])
      {
        containedType = true;
        matchStart = i;
        break;
      }
    }
    if (!containedType)
      return false;

    uint matchEnd = matchStart;
    for (uint i = 0; i < firstType.size(); ++i)
    {
      if (matchStart + i > secondType.size()
        || !(firstType[i] == secondType[i + matchStart]))
      {
        return false;
      }
    }
    return true;
  }

  // compares two document classes by alphebetical comparison of names
  template<typename T>
  bool DocCompareFn(const T& lhs, const T& rhs)
  {
    return lhs.Name < rhs.Name;
  }

  // compares two document classes by alphebetical comparison of names (for pointer types)
  template<typename T>
  bool DocComparePtrFn(const T& lhs, const T& rhs)
  {
    return lhs->mName < rhs->mName;
  }

  // specific version for typedef because it is a special snowflake
  bool TypedefDocCompareFn(const RawTypedefDoc& lhs, const RawTypedefDoc& rhs)
  {
    return lhs.mType < rhs.mType;
  }

  ////////////////////////////////////////////////////////////////////////
  // IgnoreList
  ////////////////////////////////////////////////////////////////////////
  ZeroDefineType(IgnoreList);

  bool IgnoreList::DirectoryIsOnIgnoreList(StringParam dir)
  {
    uint location = dir.FindFirstOf(mDoxyPath);

    String relativeDir;
    if (location != (uint)-1)
      relativeDir = dir.sub_string(mDoxyPath.size(), dir.size());
    else
      relativeDir = dir;

    String match;
    match = BinarySearch(mDirectories, relativeDir, match);
    if (!match.empty())
      return true;

    // now recursivly check to see if we are in a directory that was ignored
    StringRange subPath = relativeDir.sub_string(0, relativeDir.FindLastOf('\\'));

    while (subPath.size() > 2)
    {
      if (!BinarySearch(mDirectories, relativeDir, match).empty())
        return true;

      subPath.popBack();
      subPath.sub_string(0, subPath.FindLastOf('\\'));
    }

    return false;
  }

  bool IgnoreList::NameIsOnIgnoreList(StringParam name)
  {
    // strip all namespaces off of the name
    String strippedName;
    if (name.Contains(":"))
       strippedName = name.sub_string(name.FindLastOf(':') + 1, name.size());

    String match;
    match = BinarySearch(mIgnoredNames, strippedName, match);

    return !match.empty();
  }

  bool IgnoreList::empty(void)
  {
    return mDirectories.empty() || mIgnoredNames.empty();
  }

  void IgnoreList::SortList(void)
  {
    sort(mDirectories.all());
    sort(mIgnoredNames.all());
  }

  void IgnoreList::CreateIgnoreListFromDocLib(StringParam doxyPath, DocumentationLibrary &doc)
  {
    // for each class inside of the library
    for (uint i = 0; i < doc.mClasses.size(); ++i)
    {
      StringRef name = doc.mClasses[i]->mName;
      String doxName = GetDoxygenName(name);

      // add array of all of the files that pertain to that class to ignored directories
      //GetFilesWithPartialName(doxyPath, doxName, &mDirectories);

      // add the name to the ignored names list
      mIgnoredNames.push_back(name);
    }
  }

  void IgnoreList::Serialize(Serializer& stream)
  {
    SerializeName(mDirectories);
    SerializeName(mIgnoredNames);
  }

  ////////////////////////////////////////////////////////////////////////
  // DocLogger
  ////////////////////////////////////////////////////////////////////////
  DocLogger::~DocLogger()
  {
    if (mStarted)
      mLog.Close();
  }
  void DocLogger::StartLogger(StringParam path)
  {
    this->mPath = path;
    mStarted = true;

    StringRange folderPath = mPath.sub_string(0, mPath.FindLastOf('\\'));

    if (!DirectoryExists(folderPath))
    {
      CreateDirectoryAndParents(folderPath);
    }
  }

  void DocLogger::Write(const char*msgFormat...)
  {
    va_list args;
    va_start(args, msgFormat);
    //Get the number of characters needed for message
    int bufferSize;
    ZeroVSPrintfCount(msgFormat, args, 1, bufferSize);

    char* msgBuffer = (char*)alloca((bufferSize + 1) * sizeof(char));

    ZeroVSPrintf(msgBuffer, bufferSize + 1, msgFormat, args);

    va_end(args);

    printf(msgBuffer);

    if (mStarted)
    {
      if (!mLog.IsOpen())
      {
        ErrorIf(!mLog.Open(mPath, FileMode::Append, FileAccessPattern::Sequential),
          "failed to open log at: %s\n", mPath);
      }
      // we timestamp logs
      CalendarDateTime time = Time::GetLocalTime(Time::GetTime());

      StringBuilder builder;

      // month first since we are heathens
      builder << '[' << time.Hour << ':' << time.Minutes
        << " (" << time.Month << '/' << time.Day 
        << '/' << time.Year << ")] - ";

      builder << (const char *)msgBuffer;

      String timestampMsg = builder.ToString();

      mLog.Write((byte*)timestampMsg.c_str(), timestampMsg.size());

      mLog.Close();
    }
  }

  DocLogger* DocLogger::Get(void)
  {
    static DocLogger logger;

    return &logger;
  }

  ////////////////////////////////////////////////////////////////////////
  // RawNamespaceDoc
  ////////////////////////////////////////////////////////////////////////
  void RawNamespaceDoc::Serialize(Serializer& stream)
  {
    SerializeName(mNames);
  }

  void RawNamespaceDoc::GetNamesFromTokens(TypeTokens& tokens)
  {
    for (uint i = 0; i < tokens.size(); ++i)
    {
      DocToken& token = tokens[i];

      if (token.mEnumTokenType != DocTokenType::Identifier)
        continue;

      mNames.push_back(token.mText);
    }
  }

  ////////////////////////////////////////////////////////////////////////
  // RawDocumentationLibrary
  ////////////////////////////////////////////////////////////////////////
  ZeroDefineType(RawDocumentationLibrary);

  RawDocumentationLibrary::~RawDocumentationLibrary()
  {
    forRange(RawClassDoc* classDoc, mClasses.all())
    {
      delete classDoc;
    }
  }

  RawClassDoc* RawDocumentationLibrary::AddNewClass(StringParam className)
  {
    RawClassDoc* newClass = new RawClassDoc;
    mClasses.push_back(newClass);

    newClass->mName = className;

    newClass->mParentLibrary = this;

    return newClass;
  }

  void RawDocumentationLibrary::FillTrimmedDocumentation(DocumentationLibrary &trimLib)
  {
    for (uint i = 0; i < mClasses.size(); ++i)
    {
      ClassDoc* newClass = new ClassDoc;
      trimLib.mClasses.push_back(newClass);
      mClasses[i]->FillTrimmedClass(newClass);

      trimLib.mClassMap[newClass->mName] = newClass;
    }
  }

  // create path if it does not exist
  // remove file of same name if it already exists/overwrite it
  // write string out to file at location
  // output error if we fail
  void RawDocumentationLibrary::GenerateCustomDocumentationFiles(StringRef directory)
  {
    WriteLog("writing raw documentation library to directory: %s\n\n", directory.c_str());
    forRange(RawClassDoc* classDoc, mClasses.all())
    {
      String absOutputPath = BuildString(directory, classDoc->mRelativePath);

      // if we have no classpath that means the class was never loaded to begin with
      if (classDoc->mRelativePath == "")
      {
        WriteLog("empty Class found by the name of: %s\n", classDoc->mName.c_str());
        continue;
      }

      StringRange path = 
        absOutputPath.sub_string(0, absOutputPath.FindLastOf('\\'));

      if (!DirectoryExists(path))
      {
        CreateDirectoryAndParents(path);
      }

      // save class to file by the classes name, check return for fail print output
      if (!SaveToDataFile(*classDoc, absOutputPath))
      {
        Error("failed to write documentation to file at: %s\n", absOutputPath.c_str());
        continue;
      }

      mClassPaths.push_back(classDoc->mRelativePath);
    }

    String docLibFile = BuildString(directory, "\\", "Library", ".data");

    if (!SaveToDataFile(*this, docLibFile))
    {
      WriteLog("failed to write library data file at: %s\n", docLibFile.c_str());
      Error("failed to write library data file at: %s\n", docLibFile.c_str());
      return;
    }

    printf("done writing raw documentation library\n");
  }

  void RawDocumentationLibrary::FillOverloadDescriptions(void)
  {
    //String mDescription;
    // check each class and see if it has a function missing a mDescription
    forRange(RawClassDoc* classDoc, mClasses.all())
    {
      forRange(RawMethodDoc* methodDoc, classDoc->mMethods.all())
      {
        if (methodDoc->mDescription == "")
          continue;

        // see if there is a function in that same class by the same name
        if (classDoc->mMethodMap[methodDoc->mName].size() > 1)
        {
          // if there is, copy that mDescription into this method documentation
          methodDoc->mDescription = classDoc->GetDescriptionForMethod(methodDoc->mName);

          // if we found a mDescription, move on to the next method
          if (methodDoc->mDescription.size() > 0)
            continue;
        }

        // if not, check if the base class has a method by the same name
        RawClassDoc* parentClass = mClassMap.findValue(classDoc->mBaseClass, nullptr);

        while (parentClass)
        {
          methodDoc->mDescription = parentClass->GetDescriptionForMethod(methodDoc->mName);

          // if we found a mDescription, move on to the next method
          if (methodDoc->mDescription.size() > 0)
            break;

          parentClass = mClassMap.findValue(parentClass->mBaseClass, nullptr);
        }
      }
    }
  }

  void RawDocumentationLibrary::NormalizeAllTypes(RawTypedefLibrary* defLib)
  {
    forRange(RawClassDoc* classDoc, mClasses.all())
    {
      classDoc->NormalizeAllTypes(defLib);
    }
  }

  void RawDocumentationLibrary::Build(void)
  {
    forRange(RawClassDoc* classDoc, mClasses.all())
    {
      classDoc->Build();
      mClassMap.insert(classDoc->GenerateMapKey(), classDoc);
    }
  }

  void RawDocumentationLibrary::Serialize(Serializer& stream)
  {
    SerializeName(mClassPaths);
  }

  bool RawDocumentationLibrary::LoadFromDocumentationDirectory(StringParam directory)
  {
    WriteLog("loading raw documentation library from directory: %s\n\n", directory.c_str());

    String docLibFile = BuildString(directory, "/", "Library", ".data");

    // loads from the data file and checks types while loading
    if (!LoadFromDataFile(*this, docLibFile, DataFileFormat::Text, true))
      return false;

    // builds path to classes from paths stored in library file and loads them
    for (uint i = 0; i < mClassPaths.size(); ++i)
    {
      String libPath = BuildString(directory, mClassPaths[i]);

      if (mIgnoreList.DirectoryIsOnIgnoreList(libPath))
        continue;

      RawClassDoc* newClass = new RawClassDoc;
      newClass->mParentLibrary = this;
      mClasses.push_back(newClass);

      if (!LoadFromDataFile(*newClass, libPath, DataFileFormat::Text, true))
        return false;

      // we had to load the file to get the name before checking its name to ignore it
      if (mIgnoreList.NameIsOnIgnoreList(newClass->mName))
      {
        mClasses.pop_back();
        delete newClass;
      }
    }

    Build();

    printf("done loading raw documentation library");

    return true;
  }

  bool RawDocumentationLibrary::LoadFromDoxygenDirectory(StringParam doxyPath)
  {
    WriteLog("Loading Classes from Doxygen Class XML Files at: %s\n\n", doxyPath.c_str());

    MacroDatabase::GetInstance()->mDoxyPath = doxyPath;

    Array<String> classFilepaths;

    GetFilesWithPartialName(doxyPath, "class_", mIgnoreList, &classFilepaths);
    GetFilesWithPartialName(doxyPath, "struct_", mIgnoreList, &classFilepaths);

    // for each filepath
    for (uint i = 0; i < classFilepaths.size(); ++i)
    {
      StringRef filepath = classFilepaths[i];

      // open the doxy file
      TiXmlDocument doc;
      if (!doc.LoadFile(filepath.c_str()))
      {
        WriteLog("ERROR: unable to load file at: %s\n", filepath.c_str());

        continue;
      }

      // get the class name
      TiXmlElement* compoundName = doc.FirstChildElement(gElementTags[eDOXYGEN])
        ->FirstChildElement(gElementTags[eCOMPOUNDDEF])->FirstChildElement("compoundname");

      String className = GetTextFromAllChildrenNodesRecursively(compoundName);

      if (mIgnoreList.NameIsOnIgnoreList(className))
      {
        continue;
      }

      TypeTokens tokens;

      AppendTokensFromString(DocLangDfa::Get(), className, &tokens);

      className = tokens.back().mText;
      
      RawClassDoc* newClass = AddNewClass(className);

      newClass->LoadFromXmlDoc(&doc, doxyPath, classFilepaths[i]);
    }

    if (mClasses.size() != 0)
    {
      printf("\n...Done Loading Classes from Doxygen Class XML Files\n\n");
      printf("\nExpanding and parsing any macros found in Doxygen XML Files...\n\n");
      MacroDatabase::GetInstance()->ProcessMacroCalls();
      printf("\n...Done processing macros found in Doxygen XML Files\n\n");

      return true;
    }
    return false;
  }
  
  bool RawDocumentationLibrary::LoadFromSkeletonFile(StringParam doxyPath,
    const DocumentationLibrary &library)
  {
    MacroDatabase *macroDb = MacroDatabase::GetInstance();

    macroDb->mDoxyPath = doxyPath;
    // first add all the classes by name to the library
    forRange(ClassDoc *classDoc, library.mClasses.all())
    {
      // if we have already documented this, skip it
      if (mClassMap.containsKey(classDoc->mName) 
        || mIgnoreList.NameIsOnIgnoreList(classDoc->mName))
        continue;

      RawClassDoc *newClassDoc = AddNewClass(classDoc->mName);

      newClassDoc->LoadFromSkeleton(*classDoc);

      newClassDoc->LoadFromDoxygen(doxyPath);
    }


    if (mClasses.size() != 0)
    {
      printf("\n...Done Loading Classes from Doxygen Class XML Files\n\n");
      printf("\nExpanding and parsing any macros found in Doxygen XML Files...\n\n");
      MacroDatabase::GetInstance()->ProcessMacroCalls();
      printf("\n...Done processing macros found in Doxygen XML Files\n\n");

      return true;
    }
    return false;
  }

  void RawDocumentationLibrary::LoadIgnoreList(StringParam absPath)
  {
    if (!LoadFromDataFile(mIgnoreList, absPath, DataFileFormat::Text, true))
      Error("Unable to load ignore list: %s\n", absPath);
  }

  void RawDocumentationLibrary::LoadEventsList(StringParam absPath)
  {
    if (!LoadFromDataFile(mEvents, absPath, DataFileFormat::Text, true))
      Error("Unable to load events list: %s\n", absPath);
  }

  void RawDocumentationLibrary::SaveEventListToFile(StringParam absPath)
  {
    CreateDirectoryAndParents(absPath.sub_string(0, absPath.FindLastOf('\\')));

    SaveToDataFile(mEvents, absPath);
  }

  RawClassDoc *RawDocumentationLibrary::GetClassByName(StringParam name,Array<String> &namespaces)
  {
    if (mClassMap.containsKey(name))
      return mClassMap[name];

    forRange(StringRef nameSpace, namespaces.all())
    {
      String newKey = BuildString(nameSpace, name);

      if (mClassMap.containsKey(newKey))
        return mClassMap[newKey];

    }
    return nullptr;
  }

  ////////////////////////////////////////////////////////////////////////
  // RawVariableDoc
  ////////////////////////////////////////////////////////////////////////
  RawVariableDoc::RawVariableDoc(void) 
  {
    mTokens = new TypeTokens; 
    mProperty = false;
  }
  RawVariableDoc::~RawVariableDoc(void) 
  {
    delete mTokens;
  }

  void RawVariableDoc::Serialize(Serializer& stream)
  {
    SerializeName(mName);
    SerializeName(mDescription);
    SerializeName(mTokens);
  }

  RawVariableDoc::RawVariableDoc(TiXmlElement* element)
  {
    mTokens = new TypeTokens;

    mName = GetElementValue(element, gElementTags[eNAME]);

    mDescription = DoxyToString(element, gElementTags[eBRIEFDESCRIPTION]).Trim();

    mDescription = CleanRedundantSpacesInDesc(mDescription);

    StringBuilder retTypeStr;
    BuildFullTypeString(element, &retTypeStr);

    AppendTokensFromString(DocLangDfa::Get(), retTypeStr.ToString(), mTokens);
  }

  ////////////////////////////////////////////////////////////////////////
  // RawTypedefDoc
  ////////////////////////////////////////////////////////////////////////


  // due to a bug in doxygen's typedef documentation,
  // we have to parse definition ourselves
  // NOTE: This should probably switch to just using the token parser
  RawTypedefDoc::RawTypedefDoc(TiXmlElement* element)
  {
    LoadFromElement(element);
  }

  void RawTypedefDoc::LoadFromElement(TiXmlElement* element)
  {
    // we are going to want the name and the type
    mType = GetElementValue(element, gElementTags[eNAME]);

    TiXmlNode* definitionNode = GetFirstNodeOfChildType(element, gElementTags[eDEFINITION]);

    String defString = definitionNode->ToElement()->GetText();

    // lets send this down the tokenizer like an adult instead of what we were doing
    AppendTokensFromString(DocLangDfa::Get(), defString, &mDefinition);

    // get rid of the first token that just says 'typedef'
    mDefinition.pop_front();

    // now find the token that has the name of the typedef in it and remove it
    for (uint i = mDefinition.size() - 1; i >= 0; --i)
    {
      if (mDefinition[i].mText == mType)
      {
        mDefinition.eraseAt(i);
        break;
      }
    }
  }

  String RawTypedefDoc::GenerateMapKey(void)
  {
    StringBuilder builder;

    for (uint i = 0; i < mNamespace.mNames.size(); ++i)
    {
      builder.Append(mNamespace.mNames[i]);
    }

    builder.Append(mType);

    return builder.ToString();
  }

  void RawTypedefDoc::Serialize(Serializer& stream)
  {
    SerializeNameDefault(mType, String(""));
    SerializeNameDefault(mDefinition, TypeTokens());
    SerializeName(mNamespace);
  }

  ////////////////////////////////////////////////////////////////////////
  // RawEnumDoc
  ////////////////////////////////////////////////////////////////////////

  RawEnumDoc::RawEnumDoc(TiXmlElement* element, TiXmlNode* enumDef)
  {
    mName = GetElementValue(element, gElementTags[eNAME]);

    // unnamed enums are automatically given a name by doxygen, remove it
    if (mName.FindFirstOf('@') != (uint)-1)
      mName = "";

    TiXmlNode* DescNode = GetFirstNodeOfChildType(element, "briefdescription");

    if (DescNode)
    {
      TiXmlElement* descEle = DescNode->FirstChildElement();
      if (descEle)
      {
        if (descEle->Type() == TiXmlNode::TEXT)
        {
          mDescription = descEle->GetText();
        }
        else
        {
          StringBuilder desc;
          GetTextFromChildrenNodes(descEle, &desc);
          mDescription = desc.ToString();
        }
      }
      mDescription = mDescription.Trim();
      mDescription = CleanRedundantSpacesInDesc(mDescription);
    }

    // we use these for iteration
    TiXmlNode* firstElement = GetFirstNodeOfChildType(element, "enumvalue");
    TiXmlNode* endNode = GetEndNodeOfChildType(element, "enumvalue");

    for (TiXmlNode* node = firstElement; node != endNode; node = node->NextSibling())
    {
      mEnumValues.push_back(node->FirstChildElement()->GetText());
    }

  }

  void RawEnumDoc::Serialize(Serializer& stream)
  {
    SerializeNameDefault(mName, String(""));

    SerializeNameDefault(mDescription, String(""));

    SerializeNameDefault(mEnumValues, Array<String>());
  }
  ////////////////////////////////////////////////////////////////////////
  // EventDoc
  ////////////////////////////////////////////////////////////////////////

  ////////////////////////////////////////////////////////////////////////
  // RawClassDoc
  ////////////////////////////////////////////////////////////////////////
  ZeroDefineType(RawClassDoc);

  RawClassDoc::RawClassDoc(void)
    : mHasBeenLoaded(false)
  {
  }

  RawClassDoc::RawClassDoc(StringParam name) 
    : mName(name)
    , mHasBeenLoaded(false)
  {
  }

  RawClassDoc::~RawClassDoc()
  {
    forRange(RawMethodDoc* methodDoc, mMethods.all())
    {
      delete methodDoc;
    }

    forRange(RawVariableDoc* variableDoc, mVariables.all())
    {
      delete variableDoc;
    }

    forRange(RawTypedefDoc* typedefDoc, mTypedefs.all())
    { 
      delete typedefDoc;
    }

    forRange(RawEnumDoc* enumDoc, mEnums.all())
    {
      delete enumDoc;
    }
  }

  void RawClassDoc::LoadFromSkeleton(const ClassDoc &skeleClass)
  {
    mTags = skeleClass.mTags;

    forRange(PropertyDoc *prop, skeleClass.mProperties.all())
    {
      RawVariableDoc *newVar = new RawVariableDoc();

      newVar->mName = prop->mName;
      newVar->mDescription = prop->mDescription;

      newVar->mTokens = new TypeTokens;
      newVar->mTokens->push_back(DocToken(prop->mType));

      newVar->mProperty = true;
      mVariables.push_back(newVar);
    }
  }

  void RawClassDoc::Serialize(Serializer& stream)
  {
    SerializeName(mName);
    SerializeName(mNamespace);
    SerializeName(mRelativePath);
    SerializeName(mHeaderFile);
    SerializeName(mBodyFile);
    SerializeName(mBaseClass);
    SerializeName(mDescription);
    SerializeName(mTypedefs);
    SerializeName(mEnums);
    SerializeName(mEvents);
    SerializeName(mVariables);
    SerializeName(mMethods);
  }

  // we could change this to take a bool wether to override or not
  void RawClassDoc::Add(RawClassDoc& classDoc)
  {
    const Array<RawMethodDoc* > empty;
    // for every method in the passed in class documentation
    forRange(RawMethodDoc* methodDoc, classDoc.mMethods.all())
    {
      // get the array of methods by the same name we have (if any)
      const Array<RawMethodDoc* >& sameNames
        = mMethodMap.findValue(methodDoc->mName, empty);
      
      // if we had any methods by the same name, check if they have the
      // same signature before we add them
      bool sameSigExists = false;
      forRange(RawMethodDoc* sameNamedMeth, sameNames.all())
      {
        if (sameNamedMeth->mReturnTokens == methodDoc->mReturnTokens)
        {
          if (sameNamedMeth->mParsedParameters == methodDoc->mParsedParameters)
          {
            // just break, once we find one, the rest of the methods don't matter
            sameSigExists = true;
            break;
          }
        }
      }
      
      if (!sameSigExists)
        mMethods.push_back(methodDoc);
    }

    // for every property in the passed in class documentation
    forRange(RawVariableDoc* propertyDoc, mVariables.all())
    {
      // if we didn't find a property by that name, add it
      if (!mVariableMap.findValue(propertyDoc->mName, nullptr))
      {
        mVariables.push_back(propertyDoc);
      }
    }
    // since we just added data (presumably) to both of these, time to clear and build
    Rebuild();
  }

  void RawClassDoc::Build(void)
  {
    // sort the method and property Arrays
    Zero::sort(mMethods.all(), DocComparePtrFn<RawMethodDoc* >);
    Zero::sort(mVariables.all(), DocComparePtrFn<RawVariableDoc* >);

    // build method map
    forRange(RawMethodDoc* methodDoc, mMethods.all())
    {
      mMethodMap[methodDoc->mName].push_back(methodDoc);
    }

    // build property map
    forRange(RawVariableDoc* propertyDoc, mVariables.all())
    {
      mVariableMap[propertyDoc->mName] = propertyDoc;
    }

  }

  // same as build but clears everything first
  void RawClassDoc::Rebuild(void)
  {
    mMethodMap.clear();
    mVariableMap.clear();

    Build();
  }

  void RawClassDoc::FillTrimmedClass(ClassDoc* trimClass)
  {
    trimClass->mName = mName;

    trimClass->mBaseClass = mBaseClass;
    trimClass->mDescription = mDescription;
    trimClass->mEventsSent = mEvents;
    trimClass->mTags = mTags;

    for (uint i = 0; i < trimClass->mEventsSent.size(); ++i)
    {
      EventDoc *eventDoc = trimClass->mEventsSent[i];
      trimClass->mEventsMap[eventDoc->mName] = eventDoc;
    }

    // pull out all variables as properites (Removing leading 'm' where exists)
    for (uint i = 0; i < mVariables.size(); ++i)
    {
      RawVariableDoc* rawVar = mVariables[i];

      if (!rawVar->mProperty)
        continue;

      PropertyDoc* trimProp = new PropertyDoc;

      // trim off the leading 'm' if it has one
      if (rawVar->mName[0] == 'm')
        trimProp->mName = rawVar->mName.sub_string(1, rawVar->mName.size());
      else
        trimProp->mName = rawVar->mName;

      trimProp->mDescription = rawVar->mDescription;

      trimProp->mType = TrimTypeTokens(*rawVar->mTokens);

      trimClass->mPropertiesMap[trimProp->mName] = trimProp;
    }

    // for each method
    for (uint i = 0; i < mMethods.size(); ++i)
    {
      if (mMethods[i]->mReturnTokens->empty())
        continue;

      // first unpack all the information
      MethodDoc* trimMethod = new MethodDoc;
      mMethods[i]->FillTrimmedMethod(trimMethod);

      String firstThreeChar = trimMethod->mName.sub_string(0, 3).ToLower();

      // is it a getter?
      if (firstThreeChar == "get")
      {
        StringRange propName = trimMethod->mName.sub_string(3, trimMethod->mName.size());

        // check if a corresponding property already exists
        if (trimClass->mPropertiesMap.containsKey(propName))
          //&& trimClass->mPropertiesMap[propName]->mType == trimMethod->mReturnType)
        {
          // if prop exists, make sure we are returning the same type
          PropertyDoc *prop = trimClass->mPropertiesMap[propName];
          
          if (prop->mDescription.empty())
          {
            prop->mDescription = trimMethod->mDescription;
          }
        }
        else
        {
          trimClass->mMethods.push_back(trimMethod);
        }
        
      }
      // 'is' functions as a getter for a boolean property
      else if (trimMethod->mName.sub_string(0, 2).ToLower() == "is")
      {
        StringRange propName = trimMethod->mName.sub_string(2, trimMethod->mName.size());

        // check if a corresponding property already exists
        if (trimClass->mPropertiesMap.containsKey(propName)
          && trimClass->mPropertiesMap[propName]->mType == trimMethod->mReturnType)
        {
          // if prop exists, make sure we are returning the same type
          // NOTE: I am going to ignore differences in const or & since it is the same
          PropertyDoc *prop = trimClass->mPropertiesMap[propName];

          if (prop->mDescription.empty())
          {
            prop->mDescription = trimMethod->mDescription;
          }
          
        }
        else
        {
          trimClass->mMethods.push_back(trimMethod);
        }
      }
      // is it a setter?
      else if (firstThreeChar == "set")
      {
        StringRange propName = trimMethod->mName.sub_string(3, trimMethod->mName.size());

        // we only care if this is already a property and has no description
        // we can do this since methods will be in alphebetical order so
        // all the 'set's with come after the 'is' and 'get's
        if (trimClass->mPropertiesMap.containsKey(propName))
          //&& trimMethod->mParameterList.size() == 1
          //&& trimMethod->mParameterList[0]->mType == trimClass->mPropertiesMap[propName]->mType)
        {
          if (trimClass->mPropertiesMap[propName]->mDescription.empty())
            trimClass->mPropertiesMap[propName]->mDescription = trimMethod->mDescription;
        }
        else
        {
          trimClass->mMethods.push_back(trimMethod);
        }
      }
      else
      {
        trimClass->mMethods.push_back(trimMethod);
      }
    }

    forRange(auto prop, trimClass->mPropertiesMap.all())
    {
      trimClass->mProperties.push_back(prop.second);
    }
  }

  bool RawClassDoc::LoadFromDoxygen(StringParam doxyPath)
  {
    TiXmlDocument doc;
    // load the doxy xml file into a tinyxml document
    if (!loadDoxyfile(doxyPath, doc))
    {
      return false;
    }

    if (!LoadFromXmlDoc(&doc))
      return false;

    LoadEvents(GetDoxygenName(mName), doxyPath);

    return true;
  }

  // param number technically starts at 1 since 0 means no param for this fn
  bool FillEventInformation(StringParam fnTokenName, uint paramNum, bool listeningFn, 
    EventDocList &libEventList, RawClassDoc *classDoc, TypeTokens &tokens)
  {
    if (tokens.contains(DocToken(fnTokenName)))
    {
      uint i = 0;
      uint commaCount = 0;
      for (; i < tokens.size() && commaCount < paramNum; ++i)
      {
        if (tokens[i].mEnumTokenType == DocTokenType::Comma)
        {
          ++commaCount;
        }
      }

      if (commaCount == paramNum)
      {
        // i - 2 because i is currently at 1 past the comma index
        StringRef eventName = tokens[i - 2].mText;

        EventDoc *eventDoc = nullptr;

        if (libEventList.mEventMap.containsKey(eventName))
        {
          eventDoc = libEventList.mEventMap[eventName];
          if (listeningFn)
          {
            eventDoc->mListeners.push_back(classDoc->mName);
            classDoc->mEventsListened.push_back(eventDoc);
          }
          else
          {
            eventDoc->mSenders.push_back(classDoc->mName);
            classDoc->mEventsSent.push_back(eventDoc);
          }
          return true;
        }
      }
    }

    return false;
  }

  String buildExceptionText(StringParam exceptionName, StringParam exceptionText)
  {
    StringBuilder builder;

    forRange(char c, exceptionName.all())
    {
      if (c != '"')
        builder.Append(c);
    }

    if (!exceptionName.empty() && !exceptionText.empty())
      builder.Append(": ");

    forRange(char c, exceptionText.all())
    {
      if (c != '"')
        builder.Append(c);
    }

    return builder.ToString();
  }

  void RawClassDoc::AddIfNewException(StringParam fnName, ExceptionDoc *errorDoc)
  {
    RawMethodDoc *currFn = mMethodMap[fnName][0];

    forRange(ExceptionDoc *doc, currFn->mPossibleExceptionThrows.all())
    {
      if (doc->mTitle == errorDoc->mTitle && doc->mMessage == errorDoc->mMessage)
      {
        delete errorDoc;
        return;
      }
    }

    mMethodMap[fnName][0]->mPossibleExceptionThrows.push_back(errorDoc);
  }

  // param number technically starts at 1 since 0 means no param for this fn
  // need to give this some newfangled way to detect what function we are in
  void RawClassDoc::FillErrorInformation(StringParam fnTokenName, StringRef fnName, TypeTokens &tokens)
  {
    if (!mMethodMap.containsKey(fnName))
    {
      return;
    }

    if (tokens.contains(DocToken(fnTokenName)))
    {
      String firstParam = GetArgumentIfString(tokens, 1);
      String secondParam = GetArgumentIfString(tokens, 2);

      if (!firstParam.empty() || !secondParam.empty())
      {
        ExceptionDoc *errorDoc = new ExceptionDoc();

        String exceptionText = buildExceptionText(firstParam, secondParam);

        errorDoc->mTitle = firstParam.empty() ? "[variable]" : firstParam;
        errorDoc->mMessage = secondParam.empty() ? "[variable]" : secondParam;

        //TODO: actually make sure we find the correct overload instead of the first one

        AddIfNewException(fnName, errorDoc);
      }
    }
  }

  void RawClassDoc::LoadEventsFromCppDoc(TiXmlDocument *doc)
  {
    ParseCodelinesInDoc(doc, [](RawClassDoc *classDoc, StringParam codeString) 
    {
      EventDocList &libEventList = classDoc->mParentLibrary->mEvents;

      TypeTokens tokens;

      AppendTokensFromString(DocLangDfa::Get(), codeString, &tokens);

      // the next block is all of the send and recieve event fns we want to parse
      // if boolean argument is false, it is a send function, otherwise, recieve

      if (FillEventInformation("DispatchEvent", 1, false, libEventList, classDoc, tokens))
      {
      }
      // only found whern we search outside of bound types
      else if (FillEventInformation("CreateCollisionEvent", 3, false, libEventList, classDoc, tokens))
      {
      }
      else if (FillEventInformation("Dispatch", 1, false, libEventList, classDoc, tokens))
      {
      }
      // SendButtonEvent(event, Events::LockStepGamepadUp, false);
      else if (FillEventInformation("SendButtonEvent", 2, false, libEventList, classDoc, tokens))
      {
      }
      // mLockStep->QueueSyncedEvent(Events::LockStepKeyUp, &syncedEvent); (sends)
      else if (FillEventInformation("QueueSyncedEvent", 1, false, libEventList, classDoc, tokens))
      {
      }
      else if (FillEventInformation("Connect", 2, true, libEventList, classDoc, tokens))
      {
      }
      else if (FillEventInformation("ConnectThisTo", 2, true, libEventList, classDoc, tokens))
      {
      }
    });
  }

  void RawClassDoc::LoadEventsFromHppDoc(TiXmlDocument *doc)
  {
    ParseCodelinesInDoc(doc, [](RawClassDoc *classDoc, StringParam codeString) {
      TypeTokens tokens;

      if (!codeString.empty())
      {
        AppendTokensFromString(DocLangDfa::Get(), codeString, &tokens);

        for (uint i = 0; i < tokens.size(); ++i)
        {
          // if we found the events namespace node, the node two up from here is event
          if (tokens[i].mText == "Events")
          {
            i += 2;
            EventDoc *eventDoc = new EventDoc;

            classDoc->mEvents.push_back(eventDoc);

            eventDoc->mName = tokens[i].mText;
            //classDoc->mEvents.push_back(tokens[i].mText);
            return;
          }
        }
      }
    });
  }

  void RawClassDoc::ParseCodelinesInDoc(TiXmlDocument *doc, void(*fn)(RawClassDoc *, StringParam))
  {
    // grab the class
    TiXmlElement* cppDef = doc->FirstChildElement(gElementTags[eDOXYGEN])
      ->FirstChildElement(gElementTags[eCOMPOUNDDEF]);

    TiXmlElement *programList = GetFirstNodeOfChildType(cppDef, "programlisting")->ToElement();

    TiXmlNode *firstCodeline = GetFirstNodeOfChildType(programList, gElementTags[eCODELINE]);
    TiXmlNode *endCodeline = GetEndNodeOfChildType(programList, gElementTags[eCODELINE]);

    for (TiXmlNode *codeline = firstCodeline;
      codeline != endCodeline;
      codeline = codeline->NextSibling())
    {
      StringBuilder builder;

      GetTextFromAllChildrenNodesRecursively(codeline, &builder);

      String codeString = builder.ToString();

      fn(this, codeString);
    }
  }

  // this function assumes we are not contained within scope
  bool isFunctionDefinition(TypeTokens &tokens)
  {
    if (tokens.contains(DocToken(";", DocTokenType::Semicolon))
      ||tokens.contains(DocToken("#", DocTokenType::Pound)))
    {
      return false;
    }

    if (tokens.contains(DocToken("(", DocTokenType::OpenParen)))
      return true;

    return false;
  }

  String getStringFromFnTokenList(TypeTokens &tokens)
  {
    for (uint i = 0; i < tokens.size(); ++i)
    {
      DocToken &token = tokens[i];

      if (token.mEnumTokenType == DocTokenType::OpenParen)
      {
        if (i == 0)
        {
          return "";
        }
        return tokens[i - 1].mText;
      }
    }
    return "";
  }

  void RawClassDoc::ParseFnCodelinesInDoc(TiXmlDocument *doc)
  {
    // might want to maintain a static list of files we have already processed so we do
    // not end up doubling up documentation for classes that are implemented in the same
    //file.
    EventDocList &libEventList = mParentLibrary->mEvents;

    // grab the class
    TiXmlElement* cppDef = doc->FirstChildElement(gElementTags[eDOXYGEN])
      ->FirstChildElement(gElementTags[eCOMPOUNDDEF]);

    TiXmlElement *programList = GetFirstNodeOfChildType(cppDef, "programlisting")->ToElement();

    TiXmlNode *firstCodeline = GetFirstNodeOfChildType(programList, gElementTags[eCODELINE]);
    TiXmlNode *endCodeline = GetEndNodeOfChildType(programList, gElementTags[eCODELINE]);

    String currFn = "";
    RawClassDoc *currClass = this;

    Array<String> namespaces;
    namespaces.push_back("Zero");
    namespaces.push_back("Zilch");

    // the majority of this loop is just getting the current function
    for (TiXmlNode *codeline = firstCodeline;
      codeline != endCodeline;
      codeline = codeline->NextSibling())
    {
      StringBuilder builder;

      GetTextFromAllChildrenNodesRecursively(codeline, &builder);

      if (builder.GetSize() <= 0)
        continue;

      String codeString = builder.ToString();

      TypeTokens tokens;

      AppendTokensFromString(DocLangDfa::Get(), codeString, &tokens);

      if (tokens.empty())
        continue;

      // does the current line have a pound? Then just continue.
      if (tokens[0].mEnumTokenType == DocTokenType::Pound)
        continue;

      // have a semicolon?
      if (tokens.contains(DocToken(";", DocTokenType::Semicolon)))
      {
        // check if it is one of the lines we are looking for
        if (currFn.empty())
          continue;
        // otherwise, we are going to look for any exceptions, and save it if we find one
        FillErrorInformation("DoNotifyException", currFn, tokens);

        // once we are done, continue
        continue;
      }

      // does it have '::'?
      for (uint i = 0; i < tokens.size(); ++i)
      {
        DocToken &currToken = tokens[i];

        if (currToken.mEnumTokenType != DocTokenType::ScopeResolution)
          continue;

        DocToken &lhs = tokens[i - 1];
        DocToken &rhs = tokens[i + 1];


        RawClassDoc *docClass = mParentLibrary->GetClassByName(lhs.mText, namespaces);
        // check if whatever is to the left of that is a known classname
        if (docClass)
        {
          currClass = docClass;
        }

        // see if the function exists
        if (currClass->mMethodMap.containsKey(rhs.mText))
        {
          currFn = rhs.mText;
        }
        //TODO: in the event of multiple of the '::', do this same check for each one
      }
    }
  }



  void RemoveDuplicates(Array<String> &stringList)
  {
    for (uint i = 1; i < stringList.size(); ++i)
    {
      // get rid of duplicates
      if (stringList[i] == stringList[i - 1])
      {
        stringList.eraseAt(i);
        --i;
      }
    }

  }

  void RawClassDoc::SortAndPruneEventArray(void)
  {
    EventDocList &libEventList = mParentLibrary->mEvents;

    for (uint i = 0; i < libEventList.mEvents.size(); ++i)
    {
      sort(libEventList.mEvents[i]->mSenders.all());
      sort(libEventList.mEvents[i]->mListeners.all());
      RemoveDuplicates(libEventList.mEvents[i]->mSenders);
      RemoveDuplicates(libEventList.mEvents[i]->mListeners);
    }
    if (mEvents.empty())
      return;

    sort(mEvents.all(), [](EventDoc *lhs, EventDoc * rhs)
    {
      return lhs->mName < rhs->mName;
    });

    // we have to get rid of duplicates in case event was defined and bound in same class
    for (uint i = 1; i < mEvents.size(); ++i)
    {
      // get rid of duplicates
      if (mEvents[i] == mEvents[i - 1])
      {
        mEvents.eraseAt(i);
        --i;
      }
    }
  }

  bool RawClassDoc::SetRelativePath(StringRef doxyPath, StringRef filePath)
  {
    StringRange relPath = filePath.sub_string(doxyPath.size(), filePath.size() - doxyPath.size());

    relPath = relPath.sub_string(0, relPath.FindLastOf("\\xml\\"));

    if (relPath.size() == 0)
      return false;

    StringBuilder path;

    path.Append(relPath);
    path.Append('\\');

    for (int i = 0; i < (int)mNamespace.mNames.size() - 1; ++i)
    {
      path.Append(mNamespace.mNames[i]);
      path.Append('\\');
    }
    
    // there is probably a better way to do this but...   
    forRange(char c, mName.all())
    {
      if (c == ':' || c == '*' || c == '?' || c == '\"' || c == '<' || c == '>' || c == '|')
        path.Append('_');
      else
        path.Append(c);
    }

    path.Append(".data");
    // we are guarenteed to already have our name so this is safe
    mRelativePath = path.ToString();

    //RelativePath = FilePath::Normalize(RelativePath);

    return true;
  }

  bool RawClassDoc::LoadEvents(String doxName, String doxyPath)
  {
    String filename = GetDoxyfileNameFromSourceFileName(mBodyFile);

    filename = GetFileWithExactName(doxyPath, filename);

    if (filename.empty())
      return false;

    TiXmlDocument cppDoc;

    if (!cppDoc.LoadFile(filename.c_str()))
    {
      WriteLog("Failed to load file: %s\n", filename.c_str());
      // still return true since we did load the class doc
      return false;
    }

    LoadEventsFromCppDoc(&cppDoc);

    ParseFnCodelinesInDoc(&cppDoc);

    // could also load from hpp but it turns out you get everything you need from cpp

    SortAndPruneEventArray();

    return true;
  }

  bool RawClassDoc::LoadFromXmlDoc(TiXmlDocument* doc, StringRef doxyPath,
    StringRef filePath, IgnoreList *ignoreList)
  {
    if (ignoreList && ignoreList->DirectoryIsOnIgnoreList(BuildString(doxyPath,filePath)))
    {
      return false;
    }
    
    if (LoadFromXmlDoc(doc) && SetRelativePath(doxyPath, filePath))
    {
      // check if the class Initializes Meta
      if (mMethodMap.containsKey("InitializeMeta"))
      {
        // add path and name to seperatelist of events to find
        String doxName = GetDoxygenName(mName);

        LoadEvents(doxName, doxyPath);
      }

      return true;
    }

    return false;
  }

  bool RawClassDoc::LoadFromXmlDoc(TiXmlDocument* doc)
  {
    MacroDatabase *macroDb = MacroDatabase::GetInstance();

    // grab the class
    TiXmlElement* classDef = doc->FirstChildElement(gElementTags[eDOXYGEN])
      ->FirstChildElement(gElementTags[eCOMPOUNDDEF]);

    // get the namespace for the class
    TiXmlElement* compoundName = classDef->FirstChildElement("compoundname");

    String classNamespace = GetTextFromAllChildrenNodesRecursively(compoundName->FirstChild());

    TypeTokens namespaceTokens;
    AppendTokensFromString(DocLangDfa::Get(), classNamespace, &namespaceTokens);

    mNamespace.GetNamesFromTokens(namespaceTokens);

    // grab the base class if we have one
    TiXmlElement* baseClassElement = classDef->FirstChildElement(gElementTags[eBASECOMPOUNDREF]);
    if (baseClassElement != nullptr)
    {
      mBaseClass = baseClassElement->GetText();
    }

    // get the mDescription of the class
    mDescription = DoxyToString(classDef, gElementTags[eBRIEFDESCRIPTION]).Trim();

    // we are going to traverse over every section that the class contains
    for (TiXmlNode* pSection = classDef->FirstChild()
      ; pSection != 0
      ; pSection = pSection->NextSibling())
    {
      // first we need to figure out what kind of section this is

      // sometimes the first section is the mDescription, so test for that
      if (strcmp(pSection->Value(), gElementTags[eBRIEFDESCRIPTION]) == 0)
      {
        mDescription = DoxyToString(pSection->ToElement(), gElementTags[ePARA]).Trim();
      }

      // loop over all members of this section
      for (TiXmlNode* pMemberDef = pSection->FirstChild()
        ; pMemberDef != 0
        ; pMemberDef = pMemberDef->NextSibling())
      {
        TiXmlElement* memberElement = pMemberDef->ToElement();
        // check if it is actually a memberdef
        if (!memberElement
          || strcmp(memberElement->Value(), gElementTags[eMEMBERDEF]) != 0)
          continue;

        // get the kind of element we are (which happens to be the first attrib)
        TiXmlAttribute* kind = memberElement->FirstAttribute();

        // some things don't need strcmp as they have a unique first character
        switch (kind->Value()[0])
        {
        case 'v': // if we are a variable
          mVariables.push_back(new RawVariableDoc(memberElement));
          break;
        case 't': // else if we are a typedef
        {
          RawTypedefDoc* newTd = new RawTypedefDoc(memberElement);

          newTd->mNamespace.mNames = mNamespace.mNames;

          mTypedefs.push_back(newTd);
          break;
        }
        case 'e': //else if we are an enum
          mEnums.push_back(new RawEnumDoc(memberElement, pMemberDef));
          break;
        case 'f':  // function or friend

          if (strcmp(kind->Value(), "friend") == 0)
          {
            mFriends.push_back(GetElementValue(memberElement, gElementTags[eNAME]));
            break;
          }
          else // function
          {
            if (mBodyFile.empty())
            {
              TiXmlNode *locationNode = GetFirstNodeOfChildType(memberElement, "location");

              if (locationNode)
              {
                TiXmlElement *locElement = locationNode->ToElement();

                const char *attString = locElement->Attribute("file");

                if (attString)
                {
                  mHeaderFile = attString;

                  // since this path is going to be from doxygen it will have correct slashes
                  uint pos = mHeaderFile.FindLastOf(cDirectorySeparatorChar);

                  mHeaderFile = mHeaderFile.sub_string(pos + 1, mHeaderFile.size());
                }

                attString = locElement->Attribute("bodyfile");

                if (attString)
                {
                  mBodyFile = attString;

                  uint pos = mBodyFile.FindLastOf(cDirectorySeparatorChar);

                  mBodyFile = mBodyFile.sub_string(pos + 1, mBodyFile.size());
                }
              }
            }
            // if it has no return type and is not a constructor, pass it to macro paser
            if (fnIsMacroCall(memberElement, pMemberDef))
            {
              //if (this->mName == "NetPropertyConfig")
              //  DebugBreak();
              macroDb->SaveMacroCallFromClass(this, memberElement, pMemberDef);
            }
            else
            {
              mMethods.push_back(new RawMethodDoc(memberElement, pMemberDef));
            }
            break;
          }

        default: // unreconized tag
          Error("Unreconized Tag of kind: ", kind->Value(), "\n");
        }
      }
    }

    mHasBeenLoaded = true;
    Build();
    mDescription = CleanRedundantSpacesInDesc(mDescription);
    return true;
  }

  bool RawClassDoc::fnIsMacroCall(TiXmlElement* element, TiXmlNode* currMethod)
  {
    String name = GetElementValue(element, gElementTags[eNAME]);
    // this means it is a constructor or deconstructor
    if (name == mName || BuildString("~", mName) == name)
    {
      return false;
    }

    StringBuilder retTypeStr;

    BuildFullTypeString(element, &retTypeStr);

    // if it is empty, that means we are a macro so return true
    return retTypeStr.ToString().empty();
  }

  bool RawClassDoc::loadDoxyfile(StringParam doxyPath, TiXmlDocument& doc)
  {
    //try to open the class file
    String fileName = FindFile(doxyPath, BuildString("class_zero_1_1"
      , GetDoxygenName(mName), ".xml"));

    bool loadOkay = doc.LoadFile(fileName.c_str());

    //if loading the class file failed, search for a struct file
    if (!loadOkay)
    {
      fileName = FindFile(doxyPath, BuildString("struct_zero_1_1"
        , GetDoxygenName(mName).c_str(), ".xml"));

      loadOkay = doc.LoadFile(fileName.c_str());

      if (!loadOkay)
      {
        fileName = FindFile(doxyPath, BuildString("struct_zero_1_1_physics_1_1"
          , GetDoxygenName(mName).c_str(), ".xml"));

        loadOkay = doc.LoadFile(fileName.c_str());
      }
    }
    if (!loadOkay)
    {
      WriteLog("Failed to load doc file for %s\n", mName.c_str());
      return false;
    }

    if (!SetRelativePath(doxyPath, fileName))
      return false;

    mHasBeenLoaded = true;
    return true;
  }

  bool RawClassDoc::loadDoxyfile(StringParam doxyPath, TiXmlDocument& doc, IgnoreList &ignoreList)
  {
    //try to open the class file
    String fileName = FindFile(doxyPath, BuildString("class_zero_1_1"
      , GetDoxygenName(mName), ".xml"), ignoreList);

    if (fileName.empty())
      return false;

    bool loadOkay = doc.LoadFile(fileName.c_str());

    //if loading the class file failed, search for a struct file
    if (!loadOkay)
    {
      fileName = FindFile(doxyPath, BuildString("struct_zero_1_1"
        , GetDoxygenName(mName).c_str(), ".xml"), ignoreList);

      if (fileName.empty())
        return false;

      loadOkay = doc.LoadFile(fileName.c_str());

      if (!loadOkay)
      {
        fileName = FindFile(doxyPath, BuildString("struct_zero_1_1_physics_1_1"
          , GetDoxygenName(mName).c_str(), ".xml"), ignoreList);

        if (fileName.empty())
          return false;

        loadOkay = doc.LoadFile(fileName.c_str());
      }
    }
    if (!loadOkay)
    {
      WriteLog("Failed to load doc file for %s\n", mName.c_str());
      return false;
    }

    if (!SetRelativePath(doxyPath, fileName))
      return false;

    mHasBeenLoaded = true;
    return true;
  }

  const StringRef RawClassDoc::GetDescriptionForMethod(StringParam methodName)
  {
    // check if we have a method with a mDescription by that name
    forRange(RawMethodDoc* method, mMethodMap[methodName].all())
    {
      if (method->mDescription.size() > 0)
        return method->mDescription;
    }

    return mMethodMap[methodName].all()[0]->mDescription;
  }

  void RawClassDoc::NormalizeAllTypes(RawTypedefLibrary* defLib)
  {
    forRange(RawVariableDoc* prop, mVariables.all())
    {
      NormalizeTokensFromTypedefs(*prop->mTokens, defLib, mNamespace);
    }
    for (uint i = 0; i < mMethods.size(); ++i)
    {
      mMethods[i]->NormalizeAllTypes(defLib, mNamespace);
    }
  }

  String RawClassDoc::GenerateMapKey(void)
  {
    StringBuilder builder;

    for (uint i = 0; i < mNamespace.mNames.size(); ++i)
    {
      builder.Append(mNamespace.mNames[i]);
    }

    builder.Append(mName);

    return builder.ToString();
  }

  ////////////////////////////////////////////////////////////////////////
  // RawMethodDoc
  ////////////////////////////////////////////////////////////////////////
  RawMethodDoc::Parameter::Parameter(void)
  {
    mTokens = new TypeTokens; 
  }

  RawMethodDoc::Parameter::~Parameter(void)
  {
    delete mTokens;
  }

  void RawMethodDoc::Parameter::Serialize(Serializer& stream)
  {
    SerializeNameDefault(mName, String(""));
    SerializeName(mDescription);
    SerializeName(mTokens);
  }

  RawMethodDoc::RawMethodDoc(void)
  { 
    mReturnTokens = new TypeTokens; 
  }

  RawMethodDoc::~RawMethodDoc(void)
  {
    delete mReturnTokens;

    forRange(Parameter* param, mParsedParameters.all())
    {
      delete param;
    }
  }

  RawMethodDoc::RawMethodDoc(TiXmlElement* element, TiXmlNode* currMethod)
  {
    mReturnTokens = new TypeTokens;

    mName = GetElementValue(element, gElementTags[eNAME]);

    mDescription = DoxyToString(element, gElementTags[eBRIEFDESCRIPTION]).Trim();

    mDescription = CleanRedundantSpacesInDesc(mDescription);

    StringBuilder retTypeStr;

    BuildFullTypeString(element, &retTypeStr);

    AppendTokensFromString(DocLangDfa::Get(), retTypeStr.ToString(), mReturnTokens);

    TiXmlNode* firstElement = GetFirstNodeOfChildType(element, gElementTags[ePARAM]);

    TiXmlNode* endNode = GetEndNodeOfChildType(element, gElementTags[ePARAM]);

    // now unpack each parameter seperate
    for (TiXmlNode* param = firstElement; param != endNode; param = param->NextSibling())
    {
      TiXmlElement* paramElement = param->ToElement();

      // create the Parameter doc
      RawMethodDoc::Parameter* parameterDoc = new RawMethodDoc::Parameter;

      // get the name of this arguement
      parameterDoc->mName = GetElementValue(paramElement, gElementTags[eDECLNAME]);

      // get the typenode
      TiXmlNode* typeNode = GetFirstNodeOfChildType(paramElement, gElementTags[eTYPE]);

      if (!typeNode)
        continue;

      // if the typenode existed, get the actual type from its children
      StringBuilder paramName;
      BuildFullTypeString(paramElement, &paramName);

      AppendTokensFromString(DocLangDfa::Get(), paramName.ToString(), parameterDoc->mTokens);

      // get brief description
      TiXmlNode* brief = GetFirstNodeOfChildType(paramElement, gElementTags[eBRIEFDESCRIPTION]);

      if (brief)
      {
        StringBuilder builder;
        getTextFromParaNodes(brief, &builder);
        parameterDoc->mDescription = builder.ToString().Trim();
      }
      else
      {
        // check for inbody
        brief = GetFirstNodeOfChildType(paramElement, "inbodydescription");

        if (brief)
        {
          StringBuilder builder;
          getTextFromParaNodes(brief, &builder);
          parameterDoc->mDescription = builder.ToString().Trim();
        }
      }
      parameterDoc->mDescription = CleanRedundantSpacesInDesc(parameterDoc->mDescription);
      mParsedParameters.push_back(parameterDoc);
    }
  }

  void RawMethodDoc::Serialize(Serializer& stream)
  {
    SerializeName(mName);
    SerializeName(mDescription);
    SerializeNameDefault(mParsedParameters, Array<Parameter* >());
    SerializeName(mReturnTokens);
  }

  void RawMethodDoc::FillTrimmedMethod(MethodDoc* trimMethod)
  {
    TypeTokens* tokens = mReturnTokens;

    trimMethod->mReturnType = TrimTypeTokens(*tokens);

    trimMethod->mName = mName;

    trimMethod->mDescription = mDescription;

    trimMethod->mPossibleExceptionThrows = mPossibleExceptionThrows;

    StringBuilder paramBuilder;
    paramBuilder.Append('(');
    for (uint i = 0; i < mParsedParameters.size(); ++i)
    {
      RawMethodDoc::Parameter* rawParam = mParsedParameters[i];

      // if we have no parameters (or we have void parameter) just leave loop
      if (rawParam->mTokens->size() == 0
        || (*rawParam->mTokens)[0].mEnumTokenType == DocTokenType::Void)
      {
        break;
      }

      ParameterDoc* trimParam = new ParameterDoc;

      trimParam->mName = rawParam->mName;

      trimParam->mDescription = rawParam->mDescription;

      tokens = rawParam->mTokens;

      trimParam->mType = TrimTypeTokens(*tokens);

      trimMethod->mParameterList.push_back(trimParam);

      if (i > 0)
        paramBuilder.Append(", ");

      paramBuilder.Append(trimParam->mType);

      if (trimParam->mName != "")
      {
        paramBuilder.Append(" ");
        paramBuilder.Append(trimParam->mName);
      }
    }
    paramBuilder.Append(')');

    trimMethod->mParameters = paramBuilder.ToString();
  }

  // replaces any typedef'd types found with underlying type
  void RawMethodDoc::NormalizeAllTypes(RawTypedefLibrary* defLib, RawNamespaceDoc& classNamespace)
  {
    // normalize return tokens
    NormalizeTokensFromTypedefs(*mReturnTokens, defLib, classNamespace);

    // normalize parameters
    forRange(Parameter* arg, mParsedParameters.all())
    {
      NormalizeTokensFromTypedefs(*arg->mTokens, defLib, classNamespace);
    }
    
  }

  ////////////////////////////////////////////////////////////////////////
  // RawTypedefLibrary
  ////////////////////////////////////////////////////////////////////////
  ZeroDefineType(RawTypedefLibrary);

  void RawTypedefLibrary::LoadTypedefsFromDocLibrary(RawDocumentationLibrary& docLib)
  {
    WriteLog("\nLoading Typedefs from Intermediate Documentation Library\n\n");

    mTypedefs.clear();
    mTypedefArray.clear();
    forRange(RawClassDoc* classDoc, docLib.mClasses.all())
    {
      forRange(RawTypedefDoc* tdefDoc, classDoc->mTypedefs.all())
      {
        String mapKey = tdefDoc->GenerateMapKey();
        if (mTypedefs.containsKey(mapKey))
        {
          WriteLog("WARNING: Duplicate Typedef: %s\n%*s\nFrom Class: %s\n\n",
            tdefDoc->mType.c_str(), 10, "", classDoc->mName.c_str());
          continue;
        }

        mTypedefArray.push_back(*tdefDoc);

        mTypedefs[mapKey] = &mTypedefArray.back();
      }
    }

    Zero::sort(mTypedefArray.all(), TypedefDocCompareFn);

    BuildMap();

    printf("\n...Done Loading mTypedefs from Intermediate Documentation Library.\n");
  }

  void RawTypedefLibrary::LoadTypedefsFromNamespaceDocumentation(StringParam doxypath)
  {
    WriteLog("Loading Typedefs from Doxygen Namespace XML Files at: %s\n\n", doxypath.c_str());

    Array<String> namespaceFilepaths;

    GetFilesWithPartialName(doxypath, "namespace_", mIgnoreList, &namespaceFilepaths);

    // for each filepath
    for (uint i = 0; i < namespaceFilepaths.size(); ++i)
    {
      // open the doxy file
      TiXmlDocument doc;
      if (!doc.LoadFile(namespaceFilepaths[i].c_str()))
      {
        // print error on failure to load
        WriteLog("ERROR: unable to load file at: %s\n", namespaceFilepaths[i].c_str());
        continue;
      }

      // get doxygen node
      // get compounddef node
      TiXmlElement* namespaceDef = doc.FirstChildElement(gElementTags[eDOXYGEN])
        ->FirstChildElement(gElementTags[eCOMPOUNDDEF]);

      // we get the compound name so we can get the namespace for this typedef
      TiXmlElement* compoundName = namespaceDef->FirstChildElement("compoundname");

      // get the string out of the compoundName node
      String namespaceName = GetTextFromAllChildrenNodesRecursively(compoundName->FirstChild());

      TypeTokens namespaceTokens;

      AppendTokensFromString(DocLangDfa::Get(), namespaceName, &namespaceTokens);

      TiXmlNode* firstSectDef = GetFirstNodeOfChildType(namespaceDef, gElementTags[eSECTIONDEF]);
      TiXmlNode* endSectDef = GetEndNodeOfChildType(namespaceDef, gElementTags[eSECTIONDEF]);

      TiXmlElement* typedefSection = nullptr;

      // find the typedef section
      for (TiXmlNode* node = firstSectDef; node != endSectDef; node = node->NextSibling())
      {
        TiXmlElement* element = node->ToElement();

        // kind is always first attribute for sections
        TiXmlAttribute* attrib = element->FirstAttribute();

        if (attrib && strcmp(attrib->Value(), "typedef") == 0)
        {
          typedefSection = element;
          break;
        }
      }

      // if this is still null, this namespace doc just has no mTypedefs
      if (typedefSection == nullptr)
        continue;

      TiXmlNode* firstTypedef = GetFirstNodeOfChildType(typedefSection, gElementTags[eMEMBERDEF]);
      TiXmlNode* endTypedef = GetEndNodeOfChildType(typedefSection, gElementTags[eMEMBERDEF]);

      // all memberdefs in this section will be mTypedefs, so just parse the values of each
      for (TiXmlNode* node = firstTypedef; node != endTypedef; node = node->NextSibling())
      {
        // get the name to make sure we don't already have a typedef by this name
        TiXmlNode* nameNode = GetFirstNodeOfChildType(node->ToElement(), gElementTags[eNAME]);

        String mName = GetTextFromAllChildrenNodesRecursively(nameNode);

        RawTypedefDoc* newDoc = &mTypedefArray.push_back();

        newDoc->LoadFromElement(node->ToElement());

        newDoc->mNamespace.GetNamesFromTokens(namespaceTokens);

        String key = newDoc->GenerateMapKey();

        if (mTypedefs.containsKey(key))
        {
          mTypedefArray.pop_back();
          WriteLog("WARNING: Duplicate Typedef Key: %s\n\n", key.c_str());
          continue;
        }

        mTypedefs[key] = newDoc;
      }

    } // end namespace file loop

    Zero::sort(mTypedefArray.all(), TypedefDocCompareFn);

    BuildMap();

    printf("\n...Done Loading Typedefs from Doxygen Namespace XML Files.\n");
  }

  RawTypedefLibrary::~RawTypedefLibrary(void)
  {
  }

  void RawTypedefLibrary::GenerateTypedefDataFile(const String& directory)
  {
    String absPath = BuildString(directory, "\\Typedefs.data");

    WriteLog("writing raw typedef documentation file to location: %s\n\n", absPath.c_str());

    if (!DirectoryExists(directory))
    {
      WriteLog("directory does not exist, creating it now.\n");
      CreateDirectoryAndParents(directory);
    }

    SaveToDataFile(*this, absPath);
    printf("done writing raw typedef documentation file\n");
  }

  void RawTypedefLibrary::Serialize(Serializer& stream)
  {
    SerializeName(mTypedefArray);
  }

  bool RawTypedefLibrary::LoadFromFile(StringRef filepath)
  {    
    if (!LoadFromDataFile(*this, filepath, DataFileFormat::Text, true))
    {
      return false;
    }

    // just to be safe, sort the serialized array (even though it already should be)
    Zero::sort(mTypedefArray.all(), TypedefDocCompareFn);

    BuildMap();

    return true;
  }

  void RawTypedefLibrary::BuildMap(void)
  {
    mTypedefs.clear();

    for (uint i = 0; i < mTypedefArray.size(); ++i)
    {
      RawTypedefDoc* tdoc = &mTypedefArray[i];

      mTypedefs[tdoc->GenerateMapKey()] = tdoc;
    }
  }

  // this is literaly just the Normalize Tokens function with an extra param for typedefs
  bool NormalizeTypedefWithTypedefs(StringParam typedefKey, TypeTokens& tokens,
    RawTypedefLibrary* defLib, RawNamespaceDoc& classNamespace)
  {
    bool madeReplacements = false;

    // loop over tokens
    for (uint i = 0; i < tokens.size(); ++i)
    {
      //should be moved into the loop
      Array<String>& names = classNamespace.mNames;

      DocToken& token = tokens[i];

      // check any namespaces this token list is inside

      String key = token.mText;

      // loop over possible namespace'd version
      for (int j = -1; j < (int)names.size(); ++j)
      {
        StringBuilder builder;

        for (uint k = 0; (int)k <= j; ++k)
          builder.Append(names[k]);

        builder.Append(token.mText);

        key = builder.ToString();

        if (defLib->mTypedefs.containsKey(key))
        {
          TypeTokens* typedefTokens = &defLib->mTypedefs[key]->mDefinition;

          // make sure they are not just the same tokens
          if (typedefKey == key || (tokens.size() >= typedefTokens->size()
            && ContainsFirstTypeInSecondType(*typedefTokens, tokens)))
          {
            break;
          }


          // this next chunk of crazy to make sure we don't redundantly expand any typedefs
          // first we have to make sure our token ranges are of valid size
          if ((int)i - 2 > 0 && typedefTokens->size() >= 3)
          {
            bool equal = true;

            // the magic number 3 is because we are checking for this: typedef name ns::name
            for (uint m = 0; m < 3; ++m)
            {
              if ((*typedefTokens)[m].mText != tokens.sub_range(i - 2, 3)[m].mText)
                equal = false;
            }
            if (equal)
            {
              break;
            }
          }

          madeReplacements = true;
          i += ReplaceTypedefAtLocation(tokens, &token, *defLib->mTypedefs[key]);
          return madeReplacements;
        }
      }

    }
    return madeReplacements;
  }

  void RawTypedefLibrary::ExpandAllTypedefs(void)
  {
    typedef Pair<String, RawTypedefDoc* > mapIndex;
    BuildMap();

    // flag to see if we found a replacement
    bool replacements = false;

    Array<String> replacementList;

    // for each typedef
    for(mapIndex *iter = mTypedefs.begin(); iter != mTypedefs.end(); ++iter)
    {
      RawTypedefDoc* tDef = iter->second;

      if (NormalizeTypedefWithTypedefs(iter->first,tDef->mDefinition, this, tDef->mNamespace))
      {
        replacements = true;
        replacementList.append(iter->first);
      }
    }
    
    // while we have found a replacement
    while (replacements)
    {
      Array<String> newReplacementList;
    
      replacements = false;
      for (uint i = 0; i < replacementList.size(); ++i)
      {
        StringRef name = replacementList[i];
        RawTypedefDoc *tDef = mTypedefs[name];
    
        if (NormalizeTypedefWithTypedefs(name,tDef->mDefinition, this, tDef->mNamespace))
        {
          replacements = true;
          newReplacementList.append(name);
        }
    
      }
    
      replacementList.clear();
      replacementList = newReplacementList;
    }// end of while
    
  }

}
