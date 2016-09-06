#pragma once

#include "Engine/EngineContainers.hpp"



namespace DocTokenType
{
  enum Enum
  {
#define TOKEN(Name, Value) Name,
#include "DocTypeTokens.inl"
#undef TOKEN
    EnumCount
  }; 
}

static const char *DocTokenTypes[] = {
#define TOKEN(Name, Value) #Name,
#include "DocTypeTokens.inl"
#undef TOKEN
  ""
};

static const char *DocTokens[] = {
#define TOKEN(Name, Value) Value,
#include "DocTypeTokens.inl"
#undef TOKEN
  ""
};

static const char *DocKeywords[DocTokenType::EnumCount - DocTokenType::KeywordStart] = {
#define TOKEN(Name, Value) Value,
#include "DocTypeKeywords.inl"
#undef TOKEN
  "INVALID_KEYWORD"
};


namespace Zero
{
  class DocToken;
  typedef Array<DocToken> TypeTokens;

  /// Builds a string from a list of tokens
  String ConvertTokenListToString(const TypeTokens& tokens);

  ///allocates new typetokens array in out
  void CopyArrayOfTokenPtrToTypeTokens(const Array<DocToken*>& in, TypeTokens*& out);

  // do no know of a more classy way to do this so for now the following fn copy strings 

  void CleanupStringToken(DocToken *String);
  void CleanupCommentToken(DocToken *Comment);


  void SetVerboseFlag(void);

  // instead of pointing to text stream directly we copy 
  // to a string since we want to be able to load these
  class DocToken
  {
  public:
    String mText;
    DocTokenType::Enum mEnumTokenType;

    ///defaults to an invalid type
    DocToken();
    ///takes text and assumes tp
    DocToken(StringParam text, DocTokenType::Enum type = DocTokenType::Identifier);

    DocToken(const DocToken &cpy);


    void Serialize(Serializer& stream);
    bool operator==(const DocToken& right);
  };

  class DocDfaState;

  struct DocDfaEdge
  {
    DocDfaState *mChild;

    char mEdgeID;

    bool operator==(const DocDfaEdge& rhs) const;
  };

  class DocDfaState
  {
  public:
    UnsortedMap<char, DocDfaEdge> mEdges;

    DocDfaState *mDefault;

    DocTokenType::Enum mTokenTypeID;
  };

  void InitializeTokens(void);

  DocDfaState* CreateLangDfa(void);
  void ReadToken(DocDfaState* startingState, const char* stream, DocToken& outToken);
  void AppendTokensFromString(DocDfaState* startingState, StringRef str, TypeTokens *output);
}