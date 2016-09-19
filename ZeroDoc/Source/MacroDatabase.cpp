#include "Precompiled.hpp"

#include "MacroDatabase.hpp"
#include "TinyXmlHelpers.hpp"
#include "DocTypeParser.hpp"

namespace Zero
{

  ////////////////////////////////////////////////////////////////////////
  // Helpers
  ////////////////////////////////////////////////////////////////////////
  static const String gEmptyString = "";

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
  StringRef MacroCall::GetOption(StringParam optionName)
  {
    StringRef lowerName = optionName.ToLower();
    // first see if it is in our option map
    if (mOptions.containsKey(lowerName))
    {
      return mOptions[lowerName];
    }

    // if it is not see if it was a macro parameter
    for (uint i = 0; i < mMacro->mParameters.size(); ++i)
    {
      StringRef arg = mMacro->mParameters[i].ToLower();

      if (arg == lowerName)
      {
        return mMacroArgs[i];
      }
    }

    // if it is neither, just return an empty string
    return gEmptyString;
  }

  void MacroCall::AddOption(StringParam optionName, StringParam optionValue)
  {
    mOptions[optionName] = optionValue;
  }

  void MacroCall::DoOptionExpansion(void)
  {
    forRange(auto pair, mOptions.all())
    {
      String& commentVar = pair.second;

      // if this option does not start with a dollar sign it does not need to be replaced
      if (commentVar[0] != '$' || commentVar.size() < 2)
        return;

      StringRange varName = commentVar.sub_string(1, commentVar.size());

      StringRef varValue = MacroDatabase::GetInstance()->SearchMacroExpandStackForOption(varName);

      if (varValue != "")
      {
        commentVar = varValue;
      }
    }
  }

  bool MacroCall::LoadMacroWithName(StringParam name)
  {
    // if we have the location, go ahead and load the macro now if we do not have it already
    if (mOptions.containsKey(MacroOptionStrings[MacroOptions::Location]))
    {
      mMacro = MacroDatabase::GetInstance()->FindMacro(name, mOptions[MacroOptionStrings[MacroOptions::Location]]);
      // if we do not find the macro after having a location, consider that an error
      if (mMacro == nullptr)
      {
        Error("Macro '%s' at location: '%s' could not be found"
          , name.c_str(), mOptions[MacroOptionStrings[MacroOptions::Location]].c_str());
      }
    }
    else
    {
      mMacro = MacroDatabase::GetInstance()->FindMacro(name, mClass->mHeaderFile);
      // if we cannot load the macro, make sure this is not just already saved by name
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

    MacroDatabase::GetInstance()->mMacroExpandStack.push_back(this);

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
        mExpandedMacro.push_back(DocToken(replaceIdentifier(macroBody[++i], argMap), DocTokenType::StringLiteral));
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
    UniquePointer<BlockNode> parsedMacro = ParseBlock(&mExpandedMacro);
    
    // all the work now happens here due to wanting to do recursive expansion
    parsedMacro->AddToClassDoc(mClass);
  }

  ////////////////////////////////////////////////////////////////////////
  // MacroDatabase
  ////////////////////////////////////////////////////////////////////////
  MacroDatabase* MacroDatabase::GetInstance(void)
  {
    static MacroDatabase db;

    return &db;
  }

  MacroData *MacroDatabase::FindMacro(StringParam name, StringParam location)
  {
    if (mMacrosByName.containsKey(name))
    {
      return mMacrosByName[name];
    }

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

  StringRef MacroDatabase::SearchMacroExpandStackForOption(StringRef option)
  {
    for (int i = mMacroExpandStack.size() - 1; i >= 0; --i)
    {
      MacroCall* call = mMacroExpandStack[i];

      StringRef optionValue = call->GetOption(option);

      if (optionValue != "")
      {
        return optionValue;
      }
    }
    return gEmptyString;
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

}
