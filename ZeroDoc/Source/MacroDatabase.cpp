#include "Precompiled.hpp"
#include "MacroDatabase.hpp"
#include "TinyXmlHelpers.hpp"
#include "DocTypeParser.hpp"

namespace Zero
{
  ////////////////////////////////////////////////////////////////////////
  // Helpers
  ////////////////////////////////////////////////////////////////////////


  // quick helper just to ensure all the ids are generated the exact same way
  String generateUniqueMacroId(StringParam folder, StringParam filename, StringParam name)
  {
    return BuildString(folder, filename, name);
  }


  ////////////////////////////////////////////////////////////////////////
  // MacroData
  ////////////////////////////////////////////////////////////////////////
  // this constructor should really only be used by tests
  MacroData::MacroData(StringRef macroString)
  {
    AppendTokensFromString(DocLangDfa::Get(), macroString, &mMacroBody);
  }

  MacroData::MacroData(TiXmlNode* startCodeline,
    TiXmlNode* endCodeline, TypeTokens& startTokens)
  {
    //mMacroBody = startTokens;

    // skip past the first two tokens as they are just '#' and 'Define'
    DocTypeParser macroCallParser(startTokens,2);

    CallNode *node = macroCallParser.Call();

    if (node != nullptr)
    {
      forRange(DocToken *param, node->mArguments.all())
      {
        mParameters.push_back(param->mText);
      }
    }

    for (TiXmlNode *codeline = startCodeline;
      codeline != endCodeline;
      codeline = codeline->NextSibling())
    {
      StringBuilder builder;

      GetTextFromAllChildrenNodesRecursively(codeline, &builder);

      if (builder.GetSize() <= 0)
        continue;

      String codeString = builder.ToString();


      AppendTokensFromString(DocLangDfa::Get(), codeString, &mMacroBody);

      if (mMacroBody[mMacroBody.size() - 1].mEnumTokenType != DocTokenType::BackwardSlash)
        break;
    }
  }

  ////////////////////////////////////////////////////////////////////////
  // MacroCall
  ////////////////////////////////////////////////////////////////////////
  bool MacroCall::LoadMacroWithName(StringParam name)
  {
    // if we have the location, go ahead and load the macro now if we do not have it already
    if (mOptions.containsKey(MacroOptionStrings[MacroOptions::Location]))
    {
      mMacro = MacroDatabase::Get()->FindMacro(name, mOptions[MacroOptionStrings[MacroOptions::Location]]);
      // if we do not find the macro after having a location, consider that an error
      if (mMacro == nullptr)
      {
        Error("Macro '%s' at location: '%s' could not be found"
          , name.c_str(), mOptions[MacroOptionStrings[MacroOptions::Location]].c_str());
      }
    }
    else
    {
      mMacro = MacroDatabase::Get()->FindMacro(name, mClass->mHeaderFile);
      // if we cannot load the macro, delete the call so we never try to expand it
      // we fail silently since most stuff will not have this anyway
      if (mMacro == nullptr)
      {
        return false;
      }
    }
    return true;
  }

  void MacroCall::ParseOptions(TypeTokens &tokens)
  {
    // loop starting from one since we know first token is MacroComment
    for (uint i = 0; i < tokens.size(); ++i)
    {
      DocToken &token = tokens[i];

      if (token.mEnumTokenType == DocTokenType::Colon)
      {
        if (i + 1 == tokens.size())
        {
          StringBuilder errMsg;
          errMsg << "Invalid MacroCall Option Syntax: ";
          forRange(DocToken &errToken, tokens.all())
          {
            errMsg << errToken.mText << " ";
          }
          
          Error(errMsg.ToString().c_str());
        }

        String option = tokens[i - 1].mText.ToLower();

        this->mOptions[option] = tokens[i + 1].mText;
      }
    }
  }

  String replaceIdentifier(DocToken &token, UnsortedMap<String, String>& argMap)
  {
    if (argMap.containsKey(token.mText))
    {
      return argMap[token.mText];
    }
    return token.mText;
  }

  void MacroCall::ExpandCall(void)
  {
    // first create a little map for our parameters
    UnsortedMap<String,String> argMap;

    if (mMacro->mParameters.size() != mMacroArgs.size())
      Error("Param and Arg counts do not match for call to '%s'", mMacro->mName.c_str());

    for (uint i = 0; i < mMacro->mParameters.size(); ++i)
    {
      StringRef param = mMacro->mParameters[i];
      argMap[param] = mMacroArgs[i];
    }


    // we are going to unwrap the loop a bit to make things easier,
    // so first we are going to find the first ')' and jump to parsing after
    // that point
    TypeTokens &macroBody = mMacro->mMacroBody;
    uint start = 0;
    for (; start < macroBody.size(); ++start)
    {
      if (macroBody[start].mEnumTokenType == DocTokenType::CloseParen)
      {
        ++start;
        break;
      }
    }

    for (uint i = start; i < macroBody.size(); ++i)
    {
      DocToken &currToken = macroBody[i];

      switch (currToken.mEnumTokenType)
      {
        // strip out the line endings
      case DocTokenType::BackwardSlash:
        continue;

        // stringify
      case DocTokenType::Pound:
      {
        String str = BuildString("\"", replaceIdentifier(macroBody[++i], argMap), "\"");

        mExpandedMacro.push_back(DocToken(str, DocTokenType::StringLiteral));
      }
        break;

        // concatinate
      case DocTokenType::Concatenate:
      {
        // combine the text from it and the next token
        StringBuilder newTokenBuilder;

        // get last token
        DocToken &lastToken = mExpandedMacro.back();

        // concatinate the two tokens into one string
        newTokenBuilder << lastToken.mText;
        // we still have to check if it is one of our arguments
        newTokenBuilder << replaceIdentifier(macroBody[++i], argMap);
        
        // delete the last token that we added (as it is now invalid)
        mExpandedMacro.pop_back();

        String newTokString = newTokenBuilder.ToString();

        mExpandedMacro.push_back(DocToken(newTokString, DocTokenType::Identifier));
      }
        break;

        // check for arg substitution
      case DocTokenType::Identifier:
      {
        DocToken &newTok = mExpandedMacro.push_back();

        newTok.mText = replaceIdentifier(macroBody[i], argMap);
        newTok.mEnumTokenType = DocTokenType::Identifier;
      }
        break;

        // just copy the token into the expanded macro list
      default:
        mExpandedMacro.push_back(currToken);
        break;
      }
    }
  }

  void MacroCall::AddExpandedMacroDocToRawClass(void)
  {
    UniquePointer<BlockNode> parsedMacro = ParseBlock(this->mExpandedMacro);

    // all the work now happens here due to wanting to do recursive expansion
    parsedMacro->AddToClassDoc(mClass);
  }

  ////////////////////////////////////////////////////////////////////////
  // MacroDatabase
  ////////////////////////////////////////////////////////////////////////
  MacroDatabase* MacroDatabase::Get(void)
  {
    static MacroDatabase* db;

    return db ? db : db = new MacroDatabase;
  }

  MacroData *MacroDatabase::FindMacro(StringParam name, StringParam location)
  {
    // if location is empty that is an error
    if (location.empty())
    {
      // caller will need to handle the case where there is no macro data (delete the call)
      return nullptr;
    }
    
    // I hate to do this but we are going to break the directory down into tokens to get
    // each folder as we are going to see if it contains the ones we are about at all
    TypeTokens folders;
    AppendTokensFromString(DocLangDfa::Get(), location, &folders);

    String folder = "";
    forRange(DocToken &token, folders.all())
    {
      if (token.mEnumTokenType != DocTokenType::Identifier)
        continue;

      if (token.mText == "Systems")
      {
        folder = "Systems";
        break;
      }
      if (token.mText == "Extensions")
      {
        folder = "Extensions";
        break;
      }
      if (token.mText == "ExtensionLibraries")
      {
        folder = "ExtensionLibraries";
        break;
      }
    }

    // check path to pick xml folder
    StringBuilder path;

    path << mDoxyPath;

    if (!folder.empty())
    {
      path << cDirectorySeparatorChar << folder;
    }

    path << cDirectorySeparatorChar;

    // separate filename from the path
    StringRange filename
      = location.sub_string(location.FindLastOf('/') + 1, location.size());

    String uniqueId = generateUniqueMacroId(folder, filename, name);
    // if we have already loaded this macro just return that instead of parsing it again
    if (mUniqueMacros.containsKey(uniqueId))
    {
      return mUniqueMacros[uniqueId];
    }

    // generate the doxyfile name from that filename
    String fullFilename = GetDoxyfileNameFromSourceFileName(filename);

    fullFilename = GetFileWithExactName(path.ToString(), fullFilename);

    if (fullFilename.empty())
    {
      Error("file '%s' either did not have a doxy file or was not found", name.c_str());
    }

    // load the file
    TiXmlDocument macroFile;

    if (!macroFile.LoadFile(fullFilename.c_str()))
    {
      Error("unable to load file '%s", fullFilename.c_str());
    }

    // pass it to the parser so we can actually save the macro
    return SaveMacroFromDoxyfile(&macroFile, name, uniqueId);
  }

  ///searches codelines for the definition of the macro we are looking for, then saves it
  MacroData* MacroDatabase::SaveMacroFromDoxyfile(TiXmlDocument* macroFile,
    StringParam name, StringParam id)
  {
    // start looping over codelines
    TiXmlElement* cppDef = macroFile->FirstChildElement(gElementTags[eDOXYGEN])
      ->FirstChildElement(gElementTags[eCOMPOUNDDEF]);

    TiXmlElement *programList = GetFirstNodeOfChildType(cppDef, "programlisting")->ToElement();

    TiXmlNode *firstCodeline = GetFirstNodeOfChildType(programList, gElementTags[eCODELINE]);
    TiXmlNode *endCodeline = GetEndNodeOfChildType(programList, gElementTags[eCODELINE]);

    // the majority of this loop is just getting the current function
    for (TiXmlNode *codeline = firstCodeline;
      codeline != endCodeline;
      codeline = codeline->NextSibling())
    {
      // get the tokens from the current codeline and skip empty lines
      StringBuilder builder;

      GetTextFromAllChildrenNodesRecursively(codeline, &builder);

      if (builder.GetSize() <= 0)
        continue;

      String codeString = builder.ToString();

      TypeTokens tokens;

      AppendTokensFromString(DocLangDfa::Get(), codeString, &tokens);

      // we know there has to be at least 3 tokens for us to care [#,define,name]
      if (tokens.size() < 4)
        continue;

      // if the line does not start with a '#' just skip it
      if (tokens[0].mEnumTokenType != DocTokenType::Pound)
        continue;

      // if it does start with a '#', check next word and see if it is "Define"
      if (tokens[1].mText != "define")
        continue;

      // if it was, check the next word to see if it is the name of the macro we are looking for
      if (tokens[2].mText == name)
      {
        MacroData *macro = new MacroData(codeline, endCodeline, tokens);
        // if it is, pass it to our other parsing function
        macro->mName = name;

        mUniqueMacros[id] = macro;
        mMacrosByName[name] = macro;

        return macro;
      }
    }
    return nullptr;
  }

  void MacroDatabase::SaveMacroCallFromClass(RawClassDoc *classDoc,
    TiXmlElement* element, TiXmlNode* currMethod)
  {
    // get the comment above, if we have no MacroDoc directive, we do not need to save this
    String description = DoxyToString(element, gElementTags[eBRIEFDESCRIPTION]).Trim();

    if (description.empty())
      return;

    TypeTokens descTokens;
    AppendTokensFromString(DocLangDfa::Get(), description, &descTokens);

    if (descTokens[0].mText != "MacroComment")
      return;

    MacroCall &call = mMacroCalls.push_back();

    // parse any options in the comment really quick
    if (descTokens.size() > 1)
    {
      call.ParseOptions(descTokens);
    }

    // get the name of the macro from this call (do not save it to id in case we find better)
    String name = GetElementValue(element, "name");

    // save the class
    call.mClass = classDoc;

    // get the arguments
    TiXmlNode* firstElement = GetFirstNodeOfChildType(element, gElementTags[ePARAM]);
    TiXmlNode* endNode = GetEndNodeOfChildType(element, gElementTags[ePARAM]);

    for (TiXmlNode* param = firstElement; param != endNode; param = param->NextSibling())
    {
      TiXmlElement* paramElement = param->ToElement();

      // get the typenode (which for macros is all we get per param)
      TiXmlNode* typeNode = GetFirstNodeOfChildType(paramElement, gElementTags[eTYPE]);

      if (!typeNode)
        continue;

      // if the typenode existed, get the actual type from its children
      StringBuilder argName;
      BuildFullTypeString(paramElement, &argName);

      String argStr = argName.ToString();
      uint start = argStr.FindFirstNonWhitespaceCharIndex();

      argStr = argStr.sub_string(start, argStr.FindLastNonWhitespaceCharIndex() - start + 1);

      call.mMacroArgs.push_back(argStr);
    }
    // LoadMacro refernenced by call, remove this call if it is not found
    if (!call.LoadMacroWithName(name))
      mMacroCalls.pop_back();
  }

  void MacroDatabase::ProcessMacroCalls(void)
  {
    forRange(MacroCall &call, this->mMacroCalls.all())
    {
      call.ExpandCall();
      call.AddExpandedMacroDocToRawClass();
    }
  }

  ////////////////////////////////////////////////////////////////////////
  // Test Functions / Test Helpers
  ////////////////////////////////////////////////////////////////////////

  MacroData *SaveMacroFromString(StringRef name, StringRef macroString)
  {
    TypeTokens tokens;

    AppendTokensFromString(DocLangDfa::Get(), macroString, &tokens);

    // we know there has to be at least 3 tokens for us to care [#,define,name]
    if (tokens.size() < 4)
      return nullptr;

    // if the line does not start with a '#' just skip it
    if (tokens[0].mEnumTokenType != DocTokenType::Pound)
      return nullptr;

    // if it does start with a '#', check next word and see if it is "Define"
    if (tokens[1].mText != "define")
      return nullptr;

    // if it was, check the next word to see if it is the name of the macro we are looking for
    if (tokens[2].mText == name)
    {
      MacroData *macro = new MacroData(macroString);
      // if it is, pass it to our other parsing function
      macro->mName = name;

      return macro;
    }

    return nullptr;
  }

  bool doTest0(void)
  {
    // For Test0
    String simpleArgPassingTestString =
      "#define TestsSimpleArgPassing(A, B)\\\
      void ExampleFn(A B);";

    String simpleArgPassingCallTestString = "TestsSimpleArgPassing(unsigned, potato)";

    String exampleOutputString = "void ExampleFn(unsigned potato);";

    TypeTokens exampleOutputTokens;
    AppendTokensFromString(DocLangDfa::Get(), exampleOutputString, &exampleOutputTokens);


    // init the Macro
    MacroData testMacro(simpleArgPassingTestString);

    testMacro.mParameters.push_back("A");
    testMacro.mParameters.push_back("B");


    // init the MacroCall (we are cheating a bunch since parsing relies to heavy on xml
    MacroCall testCall;
    testCall.mClass = nullptr;
    testCall.mMacroArgs.push_back("unsigned");
    testCall.mMacroArgs.push_back("potato");
    testCall.mMacro = &testMacro;

    testCall.ExpandCall();

    return exampleOutputTokens == testCall.mExpandedMacro;
  }
  ///this not only is a more complex Test0, it also tests MacroComment option extraction
  bool doTest1(void)
  {
    // For Test1
    String AnchorAccessorsMacroTestString =
      "#define DeclareAnchorAccessors(ConstraintType, anchor)                                  \\\
      /* The local point of the anchor on object A.*/                                        \\\
      Vec3 GetLocalPointA() const;                                                           \\\
      void SetLocalPointA(Vec3Param localPoint);                                             \\\
      /* The local point of the anchor on object B.*/                                        \\\
      Vec3 GetLocalPointB() const;                                                           \\\
      void SetLocalPointB(Vec3Param localPoint);                                             \\\
      /* The position of the anchor on object A given a position in world space*/            \\\
      Vec3 GetWorldPointA();                                                                 \\\
      void SetWorldPointA(Vec3Param worldPoint);                                             \\\
      /* The position of the anchor on object B given a position in world space*/            \\\
      Vec3 GetWorldPointB();                                                                 \\\
      void SetWorldPointB(Vec3Param worldPoint);                                             \\\
      /* Sets the position of the anchor on object A and B given a position in world space*/ \\\
      void SetWorldPoints(Vec3Param point);                                                  \\\
      /* Virtual function for when an object link point changes*/                            \\\
      void ObjectLinkPointUpdated(size_t edgeIndex, Vec3Param localPoint) override;";

    String AnchorAccessorsMacroCallTestString = "DeclareAnchorAccessors(PositionJoint, mAnchors);";

    String AnchorAccessorsMacroCallCommentTestString
      = "///MacroComment Location : \"Systems/Physics/Joints/ConstraintAtomDefines.hpp\"";

    UnsortedMap<String, String> testOptions;
    static const String location = "location";
    testOptions[location] = "\"Systems/Physics/Joints/ConstraintAtomDefines.hpp\"";

    String exampleOutputString ="\
      /* The local point of the anchor on object A.*/                                        \\\
      Vec3 GetLocalPointA() const;                                                           \\\
      void SetLocalPointA(Vec3Param localPoint);                                             \\\
      /* The local point of the anchor on object B.*/                                        \\\
      Vec3 GetLocalPointB() const;                                                           \\\
      void SetLocalPointB(Vec3Param localPoint);                                             \\\
      /* The position of the anchor on object A given a position in world space*/            \\\
      Vec3 GetWorldPointA();                                                                 \\\
      void SetWorldPointA(Vec3Param worldPoint);                                             \\\
      /* The position of the anchor on object B given a position in world space*/            \\\
      Vec3 GetWorldPointB();                                                                 \\\
      void SetWorldPointB(Vec3Param worldPoint);                                             \\\
      /* Sets the position of the anchor on object A and B given a position in world space*/ \\\
      void SetWorldPoints(Vec3Param point);                                                  \\\
      /* Virtual function for when an object link point changes*/                            \\\
      void ObjectLinkPointUpdated(size_t edgeIndex, Vec3Param localPoint) override;";

    TypeTokens exampleOutputTokens;
    AppendTokensFromString(DocLangDfa::Get(), exampleOutputString, &exampleOutputTokens);

    // init the Macro
    MacroData testMacro(AnchorAccessorsMacroTestString);
    testMacro.mParameters.push_back("ConstraintType");
    testMacro.mParameters.push_back("anchor");


    // init the MacroCall (we are cheating a bunch since parsing relies to heavy on xml)
    MacroCall testCall;
    testCall.mClass = nullptr;
    testCall.mMacroArgs.push_back("PositionJoint");
    testCall.mMacroArgs.push_back("mAnchors");
    testCall.mMacro = &testMacro;

    // this part tests mostly just what we tested in test0
    testCall.ExpandCall();

    // now test the option extraction
    TypeTokens commentToken;
    AppendTokensFromString(DocLangDfa::Get(), AnchorAccessorsMacroCallCommentTestString, &commentToken);

    testCall.ParseOptions(commentToken);

    bool testStatus = testCall.mOptions.containsKey(location);
    testStatus &= testCall.mOptions[location] == testOptions[location];
    testStatus &= exampleOutputTokens == testCall.mExpandedMacro;

    return testStatus;
  }


  bool doTest2(void)
  {
    // For Test2
    String stringifyTestString =
      "#define TestStringify(Str)\\\
      void ExampleFnWithStrDefault(String example = #Str);";

    String stringifyCallTestString = "TestStringify(defaultValue)";

    String exampleOutputString = "void ExampleFnWithStrDefault(String example = \"defaultValue\");";

    TypeTokens exampleOutputTokens;
    AppendTokensFromString(DocLangDfa::Get(), exampleOutputString, &exampleOutputTokens);

    // init the Macro
    MacroData testMacro(stringifyTestString);
    testMacro.mParameters.push_back("Str");

    // init the MacroCall (we are cheating a bunch since parsing relies to heavy on xml)
    MacroCall testCall;
    testCall.mMacroArgs.push_back("defaultValue");
    testCall.mMacro = &testMacro;

    testCall.ExpandCall();

    return testCall.mExpandedMacro == exampleOutputTokens;
  }

  bool doTest3(void)
  {
    // For Test3
    String concatTestString =
      "#define TestConcat(A,B)\\\
      void A##B##Fn(A a, B b);";

    String concatCallTestString = "TestConcat(Super, Rad);";

    String exampleOutputString = "void SuperRadFn(Super a, Rad b);";

    TypeTokens exampleOutputTokens;
    AppendTokensFromString(DocLangDfa::Get(), exampleOutputString, &exampleOutputTokens);

    // init the macro
    MacroData testMacro(concatTestString);
    testMacro.mParameters.push_back("A");
    testMacro.mParameters.push_back("B");

    // init the MacroCall (we are cheating a bunch since parsing relies to heavy on xml)
    MacroCall testCall;
    testCall.mMacroArgs.push_back("Super");
    testCall.mMacroArgs.push_back("Rad");
    testCall.mMacro = &testMacro;

    testCall.ExpandCall();

    return testCall.mExpandedMacro == exampleOutputTokens;
  }

  // tests actually documenting the macro (codename: Davis Testcase)
  bool doTest4(void)
  {
    // From Test1
    String AnchorAccessorsMacroTestString =
      "#define DeclareAnchorAccessors(ConstraintType, anchor)                                  \\\
      /* The local point of the anchor on object A.*/                                        \\\
      Vec3 GetLocalPointA() const;                                                           \\\
      void SetLocalPointA(Vec3Param localPoint);                                             \\\
      /* The local point of the anchor on object B.*/                                        \\\
      Vec3 GetLocalPointB() const;                                                           \\\
      void SetLocalPointB(Vec3Param localPoint);                                             \\\
      /* The position of the anchor on object A given a position in world space*/            \\\
      Vec3 GetWorldPointA();                                                                 \\\
      void SetWorldPointA(Vec3Param worldPoint);                                             \\\
      /* The position of the anchor on object B given a position in world space*/            \\\
      Vec3 GetWorldPointB();                                                                 \\\
      void SetWorldPointB(Vec3Param worldPoint);                                             \\\
      /* Sets the position of the anchor on object A and B given a position in world space*/ \\\
      void SetWorldPoints(Vec3Param point);                                                  \\\
      /* Virtual function for when an object link point changes*/                            \\\
      void ObjectLinkPointUpdated(size_t edgeIndex, Vec3Param localPoint) override;";

    String AnchorAccessorsMacroCallTestString = "DeclareAnchorAccessors(PositionJoint, mAnchors);";

    String AnchorAccessorsMacroCallCommentTestString
      = "///MacroComment Location : \"Systems/Physics/Joints/ConstraintAtomDefines.hpp\"";

    UnsortedMap<String, String> testOptions;
    static const String location = "location";
    testOptions[location] = "\"Systems/Physics/Joints/ConstraintAtomDefines.hpp\"";

    String exampleOutputString = 
      "/* The local point of the anchor on object A.*/                                        \\\
      Vec3 GetLocalPointA() const;                                                           \\\
      void SetLocalPointA(Vec3Param localPoint);                                             \\\
      /* The local point of the anchor on object B.*/                                        \\\
      Vec3 GetLocalPointB() const;                                                           \\\
      void SetLocalPointB(Vec3Param localPoint);                                             \\\
      /* The position of the anchor on object A given a position in world space*/            \\\
      Vec3 GetWorldPointA();                                                                 \\\
      void SetWorldPointA(Vec3Param worldPoint);                                             \\\
      /* The position of the anchor on object B given a position in world space*/            \\\
      Vec3 GetWorldPointB();                                                                 \\\
      void SetWorldPointB(Vec3Param worldPoint);                                             \\\
      /* Sets the position of the anchor on object A and B given a position in world space*/ \\\
      void SetWorldPoints(Vec3Param point);                                                  \\\
      /* Virtual function for when an object link point changes*/                            \\\
      void ObjectLinkPointUpdated(size_t edgeIndex, Vec3Param localPoint) override;";

    TypeTokens exampleOutputTokens;
    AppendTokensFromString(DocLangDfa::Get(), exampleOutputString, &exampleOutputTokens);

    // init the Macro
    MacroData testMacro(AnchorAccessorsMacroTestString);
    testMacro.mParameters.push_back("ConstraintType");
    testMacro.mParameters.push_back("anchor");

    // init the MacroCall (we are cheating a bunch since parsing relies to heavy on xml)
    MacroCall testCall;
    testCall.mClass = new RawClassDoc("testClass");
    testCall.mMacroArgs.push_back("PositionJoint");
    testCall.mMacroArgs.push_back("mAnchors");
    testCall.mMacro = &testMacro;

    // this part tests mostly just what we tested in test0
    testCall.ExpandCall();

    testCall.AddExpandedMacroDocToRawClass();

    //TODO: write the rest of the test so it compares with the known correct output

    return true;
  }

  // codename: Andrew Testcase
  bool doTest5(void)
  {
    String AndrewMacroTestString0 = "\
#define DeclareVariantGetSetForArithmeticTypes(property)       \\\
/* MacroComment*/                                              \\\
DeclareVariantGetSetForType(property, Integer,       int);     \\\
/* MacroComment*/                                              \\\
DeclareVariantGetSetForType(property, DoubleInteger, s64);     \\\
/* MacroComment*/                                              \\\
DeclareVariantGetSetForType(property, Integer2,      IntVec2); \\\
/* MacroComment*/                                              \\\
DeclareVariantGetSetForType(property, Integer3,      IntVec3); \\\
/* MacroComment*/                                              \\\
DeclareVariantGetSetForType(property, Integer4,      IntVec4); \\\
/* MacroComment*/                                              \\\
DeclareVariantGetSetForType(property, Real,          float);   \\\
/* MacroComment*/                                              \\\
DeclareVariantGetSetForType(property, DoubleReal,    double);  \\\
/* MacroComment*/                                              \\\
DeclareVariantGetSetForType(property, Real2,         Vec2);    \\\
/* MacroComment*/                                              \\\
DeclareVariantGetSetForType(property, Real3,         Vec3);    \\\
/* MacroComment*/                                              \\\
DeclareVariantGetSetForType(property, Real4,         Vec4);    \\\
/* MacroComment*/                                              \\\
DeclareVariantGetSetForType(property, Quaternion,    Quat)";

    String AndrewMacroTestString1 = "\
#define DeclareVariantGetSetForType(property, typeName, type)  \\\
void Set##property##typeName(type value);                      \\\
/* Andrew's getter test description*/                          \\\
type Get##property##typeName() const;";

    String AndrewMacroCallTestString = "DeclareVariantGetSetForArithmeticTypes(DeltaThreshold);";

    String AndrewMacroCallCommentTestString = "MacroComment comment : \"Controls the delta \
threshold at which a net property's primitive-components \
are considered changed during change detection\"";


    // so we have to add both macro fns to the database
    // after we do that we can go ahead and test the call
    // init the Macro


    MacroData *testMacro0 = new MacroData(AndrewMacroTestString0);
    testMacro0->mParameters.push_back("property");
    testMacro0->mName = "DeclareVariantGetSetForArithmeticTypes";

    MacroData *testMacro1 = new MacroData(AndrewMacroTestString1);
    testMacro1->mParameters.push_back("property");
    testMacro1->mParameters.push_back("typeName");
    testMacro1->mParameters.push_back("type");
    testMacro1->mName = "DeclareVariantGetSetForType";

    MacroDatabase &database = *MacroDatabase::Get();

    database.mMacrosByName[testMacro0->mName] = testMacro0;
    database.mMacrosByName[testMacro1->mName] = testMacro1;

    // init the MacroCall (we are cheating a bunch since parsing relies to heavy on xml)
    MacroCall testCall;
    testCall.mClass = new RawClassDoc("testClass");

    testCall.mMacroArgs.push_back("DeltaThreshold");
    testCall.mMacro = testMacro0;

    testCall.ExpandCall();

    testCall.AddExpandedMacroDocToRawClass();

    database.mMacrosByName.clear();
    delete testMacro0;
    delete testMacro1;
    // for now I am just going to call the test good if we have the right count of methods
    return testCall.mClass->mMethods.size() == 22;
  }

  bool doTest6(void)
  {
    // bringing back the andrew case to test variable passing

    String AndrewMacroTestString0 =
      "#define DeclareVariantGetSetForArithmeticTypes(property)       \\\
/* MacroComment verb : {verb}, comment : {comment},*/\\\
DeclareVariantGetSetForType(property, Integer,       int);     \\\
/* MacroComment verb : {verb}, comment : {comment},*/\\\
DeclareVariantGetSetForType(property, DoubleInteger, s64);     \\\
/* MacroComment verb : {verb}, comment : {comment},*/\
DeclareVariantGetSetForType(property, Integer2,      IntVec2); \\\
/* MacroComment verb : {verb}, comment : {comment},*/\\\
DeclareVariantGetSetForType(property, Integer3,      IntVec3); \\\
/* MacroComment verb : {verb}, comment : {comment},*/\\\
DeclareVariantGetSetForType(property, Integer4,      IntVec4); \\\
/* MacroComment verb : {verb}, comment : {comment},*/\\\
DeclareVariantGetSetForType(property, Real,          float);   \\\
/* MacroComment verb : {verb}, comment : {comment},*/\\\
DeclareVariantGetSetForType(property, DoubleReal,    double);  \\\
/* MacroComment verb : {verb}, comment : {comment},*/\\\
DeclareVariantGetSetForType(property, Real2,         Vec2);    \\\
/* MacroComment verb : {verb}, comment : {comment},*/\\\
DeclareVariantGetSetForType(property, Real3,         Vec3);    \\\
/* MacroComment verb : {verb}, comment : {comment},*/\\\
DeclareVariantGetSetForType(property, Real4,         Vec4);    \\\
/* MacroComment verb : {verb}, comment : {comment},*/\\\
DeclareVariantGetSetForType(property, Quaternion,    Quat)";

    String AndrewMacroTestString1 =
      "#define DeclareVariantGetSetForType(property, typeName, type)         \\\
/*Allows for {verb} of {typename} in property: {property}. {comment}.*/\\\
void Set##property##typeName(type value);                              \\\
type Get##property##typeName() const";

    String AndrewMacroCallTestString0 = "DeclareVariantGetSetForArithmeticTypes(DeltaThreshold);";
    String AndrewMacroCallCommentTestString0 = "/// MacroComment\
/// comment : \"DeltaThreshold probably does stuff\",\
/// verb : \"Tweeking\",";

    String AndrewMacroCallTestString1 = "DeclareVariantGetSetForArithmeticTypes(QuantizationRangeMin);";
    String AndrewMacroCallCommentTestString1 = "/// MacroComment\
/// comment : \"QuantizationRangeMin is gibberish to me\",\
/// verb : \"Eating\",";

    String AndrewMacroCallTestString2 = "DeclareVariantGetSetForArithmeticTypes(QuantizationRangeMax);";
    String AndrewMacroCallCommentTestString2 = "/// MacroComment\
/// comment : \"I think I am trying way to hard at writting this test case\",\
/// verb : \"Dueling\",";

    MacroData *testMacro0 = new MacroData(AndrewMacroTestString0);
    testMacro0->mParameters.push_back("property");
    testMacro0->mName = "DeclareVariantGetSetForArithmeticTypes";

    MacroData *testMacro1 = new MacroData(AndrewMacroTestString1);
    testMacro1->mParameters.push_back("property");
    testMacro1->mParameters.push_back("typeName");
    testMacro1->mParameters.push_back("type");
    testMacro1->mName = "DeclareVariantGetSetForType";

    MacroDatabase &database = *MacroDatabase::Get();

    database.mMacrosByName[testMacro0->mName] = testMacro0;
    database.mMacrosByName[testMacro1->mName] = testMacro1;

    MacroCall testCall0;
    testCall0.mClass = new RawClassDoc("testClass");
    testCall0.mMacro = testMacro0;
    testCall0.mMacroArgs.push_back("DeltaThreshold");
    testCall0.mOptions["comment"] = "DeltaThreshold probably does stuff";
    testCall0.mOptions["verb"] = "TWEEKING";

    MacroCall testCall1;
    testCall1.mClass = new RawClassDoc("testClass");
    testCall1.mMacro = testMacro0;
    testCall1.mMacroArgs.push_back("QuantizationRangeMin");
    testCall1.mOptions["comment"] = "QuantizationRangeMin is giberish to me";
    testCall1.mOptions["verb"] = "EATING";

    MacroCall testCall2;
    testCall2.mClass = new RawClassDoc("testClass");
    testCall2.mMacro = testMacro0;
    testCall2.mMacroArgs.push_back("QuantizationRangeMax");
    testCall2.mOptions["comment"] = "I think I am trying way to hard at writing this test case";
    testCall2.mOptions["verb"] = "DUELING";

    testCall0.ExpandCall();
    testCall0.AddExpandedMacroDocToRawClass();

    testCall1.ExpandCall();
    testCall1.AddExpandedMacroDocToRawClass();

    testCall2.ExpandCall();
    testCall2.AddExpandedMacroDocToRawClass();

    // TODO: on the rainy day that I stop being lazy, write the actually test check instead
    // of doing it by hand
    return true;
  }

  bool doAllTests(void)
  {
    bool retVal = doTest0();

    retVal &= doTest1();
    retVal &= doTest2();
    retVal &= doTest3();
    retVal &= doTest4();
    retVal &= doTest5();

    return retVal;
  }

  bool RunMacroTests(int test)
  {
    switch (test)
    {
    case 0:
      return doTest0();
    case 1:
      return doTest1();
    case 2:
      return doTest2();
    case 3:
      return doTest3();
    case 4:
      return doTest4();
    case 5:
      return doTest5();
    case 6:
      return doTest6();
    default:
      return doAllTests();
    }
  }
}
