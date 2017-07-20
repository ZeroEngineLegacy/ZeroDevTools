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
  MacroData::MacroData(String& macroString)
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
      forRange(DocToken *param, node->mArguments.All())
      {
        mParameters.PushBack(param->mText);
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

      if (mMacroBody[mMacroBody.Size() - 1].mEnumTokenType != DocTokenType::BackwardSlash)
        break;
    }
  }

  ////////////////////////////////////////////////////////////////////////
  // MacroCall
  ////////////////////////////////////////////////////////////////////////
  String MacroCall::GetOption(StringParam optionName)
  {
    String lowerName = optionName.ToLower();
    // first see if it is in our option map
    if (mOptions.ContainsKey(lowerName))
    {
      return mOptions[lowerName];
    }

    // if it is not see if it was a macro parameter
    for (uint i = 0; i < mMacro->mParameters.Size(); ++i)
    {
      String arg = mMacro->mParameters[i].ToLower();

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
    forRange(auto pair, mOptions.All())
    {
      String& commentVar = pair.second;

      // if this option does not start with a dollar sign it does not need to be replaced
      if (commentVar.c_str()[0] != '$' || commentVar.SizeInBytes() < 2)
        return;

      StringRange varName = commentVar.SubStringFromByteIndices(1, commentVar.SizeInBytes());

      String varValue = MacroDatabase::GetInstance()->SearchMacroExpandStackForOption(varName);

      if (varValue != "")
      {
        commentVar = varValue;
      }
    }
  }

  bool MacroCall::LoadMacroWithName(StringParam name)
  {
    // if we have the location, go ahead and load the macro now if we do not have it already
    if (mOptions.ContainsKey(MacroOptionStrings[MacroOptions::Location]))
    {
      mMacro = MacroDatabase::GetInstance()->FindMacro(name, mOptions[MacroOptionStrings[MacroOptions::Location]]);
      // if we do not find the macro after having a location, consider that an error
      if (mMacro == nullptr)
      {
        WriteLog("Macro '%s' at location: '%s' could not be found\n"
          , name.c_str(), mOptions[MacroOptionStrings[MacroOptions::Location]].c_str());
        return false;
      }
    }
    else
    {
      mMacro = MacroDatabase::GetInstance()->FindMacro(name, mClass->mHeaderFile);

      // if we cannot load the macro, make sure this is not just already saved by name
      if (mMacro == nullptr)
      {
        WriteLog("Macro '%s' at location: '%s' could not be found\n"
          , name.c_str(), mClass->mHeaderFile.c_str());
        return false;
      }
    }
    return true;
  }

  void MacroCall::ParseOptions(TypeTokens &tokens)
   {
    // loop starting from one since we know first token is MacroComment
    for (uint i = 0; i < tokens.Size(); ++i)
    {
      DocToken &token = tokens[i];

      if (token.mEnumTokenType == DocTokenType::Colon)
      {
        if (i + 1 == tokens.Size())
        {
          StringBuilder errMsg;
          errMsg << "Invalid MacroCall Option Syntax: ";
          forRange(DocToken &errToken, tokens.All())
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
    if (argMap.ContainsKey(token.mText))
    {
      return argMap[token.mText];
    }
    return token.mText;
  }

  void MacroCall::ExpandCall(void)
  {
    // first create a little map for our parameters
    UnsortedMap<String,String> argMap;

    if (mMacro->mParameters.Size() != mMacroArgs.Size())
      Error("Param and Arg counts do not match for call to '%s'", mMacro->mName.c_str());

    for (uint i = 0; i < mMacro->mParameters.Size(); ++i)
    {
      String& param = mMacro->mParameters[i];
      argMap[param] = mMacroArgs[i];
    }

    MacroDatabase::GetInstance()->mMacroExpandStack.PushBack(this);

    // we are going to unwrap the loop a bit to make things easier,
    // so first we are going to find the first ')' and jump to parsing after
    // that point
    TypeTokens &macroBody = mMacro->mMacroBody;
    uint start = 0;
    for (; start < macroBody.Size(); ++start)
    {
      if (macroBody[start].mEnumTokenType == DocTokenType::CloseParen)
      {
        ++start;
        break;
      }
    }

    for (uint i = start; i < macroBody.Size(); ++i)
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
        mExpandedMacro.PushBack(DocToken(replaceIdentifier(macroBody[++i], argMap), DocTokenType::StringLiteral));
      }
        break;

        // concatinate
      case DocTokenType::Concatenate:
      {
        // combine the text from it and the next token
        StringBuilder newTokenBuilder;

        // get last token
        DocToken &lastToken = mExpandedMacro.Back();

        // concatinate the two tokens into one string
        newTokenBuilder << lastToken.mText;
        // we still have to check if it is one of our arguments
        newTokenBuilder << replaceIdentifier(macroBody[++i], argMap);
        
        // delete the last token that we added (as it is now invalid)
        mExpandedMacro.PopBack();

        String newTokString = newTokenBuilder.ToString();

        mExpandedMacro.PushBack(DocToken(newTokString, DocTokenType::Identifier));
      }
        break;

        // check for arg substitution
      case DocTokenType::Identifier:
      {
        DocToken &newTok = mExpandedMacro.PushBack();

        newTok.mText = replaceIdentifier(macroBody[i], argMap);
        newTok.mEnumTokenType = DocTokenType::Identifier;
      }
      break;

      case DocTokenType::Comment:
      if (!mExpandedMacro.Empty())
      {
        DocToken& prevToken = mExpandedMacro.Back();
        // combine comment tokens
        if (prevToken.mEnumTokenType == DocTokenType::Comment)
        {
          prevToken.mText = BuildString(prevToken.mText, currToken.mText);
          break;
        }
      }
      mExpandedMacro.PushBack(currToken);
      break;
      // just copy the token into the expanded macro list
      default:
        mExpandedMacro.PushBack(currToken);
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
    // there are some macro loading scenarios where we store macros by non-unique names, check for that
    if (mMacrosByName.ContainsKey(name))
    {
      return mMacrosByName[name];
    }

    Array<String> fileList;
    String doxyName = GetDoxyfileNameFromSourceFileName(location);
    // first get the list of all possible files that could contain the macro
    GetFilesWithPartialName(mDoxyPath, doxyName, &fileList);

    // if we didn't find any matching files, we have failed in our mission, return nothing
    if (fileList.Empty())
      return nullptr;

    // if we get here, we have not already loaded the macro, go ahead and try to load it
    forRange(String& path, fileList.All())
    {
      String macroId = BuildString(path, name);

      // attempt to load macro from file

      // load the file
      TiXmlDocument macroFile;

      if (!macroFile.LoadFile(path.c_str()))
      {
        continue;
      }

      // pass it to the parser so we can actually save the macro
      MacroData* macro = SaveMacroFromDoxyfile(&macroFile, name);

      mMacrosByName[name] = macro;

      if (macro != nullptr)
        return macro;
    }

    WriteLog("Error: Unable to load macro '%s' from file: '%s'\n", name, location);
    // if we make it out of the loop, we didn't find the macro definition
    return nullptr;
  }

  /// searches codelines for the definition of the macro we are looking for, then saves it
  MacroData* MacroDatabase::SaveMacroFromDoxyfile(TiXmlDocument* macroFile, StringParam name)
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
      if (tokens.Size() < 4)
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
    //String description = DoxyToString(element, gElementTags[eBRIEFDESCRIPTION]).Trim();
    //
    //if (description.Empty())
    //  return;

    // get just the description node
    TiXmlNode* descNode = GetFirstNodeOfChildType(element, gElementTags[eBRIEFDESCRIPTION]);

    if (descNode->GetNumberOfChildren() <= 0)
      return;


    // para
    TiXmlNode* paraNode = descNode->FirstChild();

    String description = DoxyToString(element, gElementTags[eBRIEFDESCRIPTION]).Trim();

    StringRange macroStart = description.FindFirstOf("<macro");

    // if this does not contains the start of a macro tag skipit
    if (!macroStart.SizeInBytes())
    {
      return;
    }

    String macro = description.SubString(macroStart.Begin(), description.End());

    // cut the macro out of the description
    description = description.SubString(description.Begin(), macroStart.Begin());

    TiXmlDocument macroXml;

    macroXml.Parse(macro.c_str());

    TiXmlElement* macroElement = macroXml.FirstChildElement();

    MacroCall &call = mMacroCalls.PushBack();

    // unpack all attributes and save them as options
    for (TiXmlAttribute* attrib = macroElement->FirstAttribute();
      attrib != nullptr; attrib = attrib->Next())
    {
      call.AddOption(attrib->Name(), attrib->Value());
    }

    // unpack all child nodes and save them as options
    for (TiXmlElement* childOption = macroElement->FirstChildElement(); 
      childOption != nullptr; childOption = childOption->NextSiblingElement())
    {
      // save replacements separately
      if (String(childOption->Value()) == MacroOptionStrings[MacroOptions::Replacement])
      {
        // there must be an attribute as that is what we are replacing
        if (childOption->FirstAttribute() == nullptr)
        {
          WriteLog("Macro %s has comment with invalid replacement.\n", GetElementValue(element, "name"));
          Error("Invalid replacement, missing attribute");
        }
        else
        {
          call.mReplacements[childOption->FirstAttribute()->Value()] = childOption->GetText();
        }
      }
      else
      {
        call.AddOption(childOption->Value(), childOption->GetText());
      }
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

      StringIterator start = argStr.All().FindFirstNonWhitespaceRuneIt();

      argStr = argStr.SubString(start, argStr.All().FindLastNonWhitespaceRuneIt());

      call.mMacroArgs.PushBack(argStr);
    }
    // this seems to actually circumvent loading macro calls
    // LoadMacro refernenced by call, remove this call if it is not found
    if (!call.LoadMacroWithName(name))
      mMacroCalls.PopBack();
  }

  void MacroDatabase::ProcessMacroCalls(void)
  {
    forRange(MacroCall &call, this->mMacroCalls.All())
    {
      call.ExpandCall();
      call.AddExpandedMacroDocToRawClass();
    }
  }

  String MacroDatabase::SearchMacroExpandStackForOption(StringParam option)
  {
    for (int i = mMacroExpandStack.Size() - 1; i >= 0; --i)
    {
      MacroCall* call = mMacroExpandStack[i];

      String optionValue = call->GetOption(option);

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

  MacroData *SaveMacroFromString(String& name, String& macroString)
  {
    TypeTokens tokens;

    AppendTokensFromString(DocLangDfa::Get(), macroString, &tokens);

    // we know there has to be at least 3 tokens for us to care [#,define,name]
    if (tokens.Size() < 4)
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
