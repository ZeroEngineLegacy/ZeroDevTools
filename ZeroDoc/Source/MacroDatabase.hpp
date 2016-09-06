#pragma once
#include <Engine/EngineStandard.hpp>
#include "DocTypeTokens.hpp"
#include "RawDocumentation.hpp"


namespace Zero
{

  DeclareEnum3(MacroOptions, Location, Comment, StartCustomOptions);

  static const char *MacroOptionStrings[] = {
    "location",
    "comment"
  };
  struct MacroData
  {
    MacroData(StringRef macroString);
    MacroData(TiXmlNode* startCodeline, TiXmlNode* endCodeline, TypeTokens& startTokens);

    //bool mFound;
    String mName;
    //String mLocation;
    TypeTokens mMacroBody;
    Array<String> mParameters;
  };

  struct MacroCall
  {
    void ParseOptions(TypeTokens &tokens);

    void ExpandCall(void);

    void AddExpandedMacroDocToRawClass(void);

    bool LoadMacroWithName(StringParam name);

    MacroData *mMacro;

    RawClassDoc *mClass;
    //String mMacroIdentifier;

    TypeTokens mExpandedMacro;
    Array<String> mMacroArgs;
    UnsortedMap<String, String> mOptions;
  };

  class MacroDatabase
  {
  public:
  //singleton
    static MacroDatabase *Get(void);

    ///returns id for macro
    MacroData* FindMacro(StringParam name, StringParam location);

    MacroData* SaveMacroFromDoxyfile(TiXmlDocument *macroFile, StringParam name, StringParam id);

    void SaveMacroCallFromClass(RawClassDoc *classDoc, TiXmlElement* element, 
      TiXmlNode* currMethod);

    void ProcessMacroCalls(void);

    String mDoxyPath;

    Array<MacroCall> mMacroCalls;

    UnsortedMap<String, MacroData *> mUniqueMacros;

    UnsortedMap<String, MacroData *> mMacrosByName;
  };

  bool RunMacroTests(int testNo);
}

