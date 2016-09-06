#pragma once

#include "DocTypeTokens.hpp"
#include "RawDocumentation.hpp"

namespace Zero
{
  class Visitor;

  class Type;
  class Variable;
  class Function;

  class BlockNode;
  class FunctionNode;
  class CallNode;
  class ParameterNode;
  class TypeNode;
  class StatementNode;

  UniquePointer<BlockNode> ParseBlock(TypeTokens& tokens);

  class DocTypeParser
  {
  public:

    explicit DocTypeParser(TypeTokens& tokens, int index = 0) 
      : mTokens(tokens), mIndex(index) {}


    ////////////////////
    //Rules
    ////////////////////

    UniquePointer<BlockNode> Block(void);

    FunctionNode* Function(void);

    CallNode* Call(void);

    UniquePointer<ParameterNode> Parameter(void);

    UniquePointer<TypeNode> SpecifiedType(void);

    UniquePointer<TypeNode> Type(void);

    UniquePointer<TypeNode> NamedType(void);

    UniquePointer<TypeNode> FunctionType(void);

    UniquePointer<StatementNode> Statement(void);

    UniquePointer<StatementNode> DelimitedStatement(void);

    UniquePointer<StatementNode> FreeStatement(void);


  private:
    ////////////////////
    //Helpers
    ////////////////////

    bool accept(DocTokenType::Enum type);
    bool accept(DocTokenType::Enum type, DocToken*& token);

    // token type output
    template <typename T>
    T expect(T expectedValue);
    template <typename T>
    T expect(T expectedValue, String errorMsg);

    // bool output
    bool expect(DocTokenType::Enum type);
    bool expect(DocTokenType::Enum type, String errorMsg);
    bool expect(bool expected);
    bool expect(bool expected, String errorMsg);

    // token name output
    void expect(DocTokenType::Enum type, DocToken*& output);
    void expect(DocTokenType::Enum type, String errorMsg, DocToken*& output);

    ////////////////////
    //Data
    ////////////////////

    TypeTokens& mTokens;

    unsigned mIndex;
  };

  class AbstractNode
  {
  public:
    AbstractNode() {};

    // documentation that should never be written to classDoc should override this to throw
    virtual void AddToClassDoc(RawClassDoc *doc) = 0;

   };

  class BlockNode : public AbstractNode
  {
  public:
    // FunctionNode
    Array<UniquePointer<AbstractNode> > mGlobals;

    virtual void AddToClassDoc(RawClassDoc *doc) override;
  };

  class StatementNode : public AbstractNode
  {
  public:

    //void Walk(Visitor* visitor, bool visit = true) override;
    virtual void AddToClassDoc(RawClassDoc *doc) = 0;

  };

  class TypeNode : public AbstractNode
  {
  public:
    TypeNode() {};

    //void Walk(Visitor* visitor, bool visit = true) override;
    virtual void AddToClassDoc(RawClassDoc *doc) override;

    //* Semantic Analysis *//
    // for ease of extracting info we are going to maintain a list of our tokens
    Array<DocToken *> mTokens;
  };

  class VariableNode : public StatementNode
  {
  public:
    VariableNode() {};
    DocToken *mName;
    UniquePointer<TypeNode> mType;

    // Can be null (gonna ignore this for now)
    //UniquePointer<ExpressionNode> mInitialValue;
    virtual void AddToClassDoc(RawClassDoc *doc) = 0;

    //* Semantic Analysis *//
    // The variable node should create the following symbol
    Variable* mSymbol;
  };

  class ParameterNode : public VariableNode
  {
  public:
    //void Walk(Visitor* visitor, bool visit = true) override;
    virtual void AddToClassDoc(RawClassDoc *doc) override;

  };

  class FunctionNode : public AbstractNode
  {
  public:
    FunctionNode() {};
    DocToken *mName;
    Array<UniquePointer<ParameterNode> > mParameters;

    // Can be null
    UniquePointer<TypeNode> mReturnType;
    virtual void AddToClassDoc(RawClassDoc *doc) override;

    DocToken *mComment;

    //void Walk(Visitor* visitor, bool visit = true) override;

    //* Semantic Analysis *//
    // The signature type is filled out first and is used later when we create the Function symbol
    //Type* mSignatureType;
    //// The function node should create the following symbol
    //Function* mSymbol;
  };

  class CallNode : public StatementNode
  {
  public:
    Array<DocToken *> mArguments;
    virtual void AddToClassDoc(RawClassDoc *doc) override;

    DocToken *mName;
    DocToken *mComment;
  };

  // Throw this exception type when a parsing error occurrs
  class ParsingException : public std::exception
  {
  public:
    ParsingException();
    ParsingException(StringParam error);
    const char* what() const override;
    String mError;
  };

}
