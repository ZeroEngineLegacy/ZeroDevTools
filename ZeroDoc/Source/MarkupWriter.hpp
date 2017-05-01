#include "Engine/EngineContainers.hpp"
namespace Zero
{
  typedef HashMap<String, Array<ClassDoc*> > DocToTags;
  typedef DocToTags::range DocRange;

  ///// BASE MARKUP ///// 
  class BaseMarkupWriter
  {
  public:
    BaseMarkupWriter(StringParam name) 
      : mName(name)
      , mCurrentIndentationLevel(0)
      , mCurrentSectionHeaderLevel(0){}

    void WriteOutputToFile(StringRef outputDirectory);

  protected:
    void IndentToCurrentLevel(void);

    void InsertNewUnderline(uint length, uint headerLevel = 0);

    void InsertNewSectionHeader(StringRef sectionName);

    StringBuilder mOutput;

    String mName;

    // can be moved up in the case of large sections that need to be indented
    uint mCurrentIndentationLevel;

    uint mCurrentSectionHeaderLevel;
  };

  ///// ReStructuredText ///// 
  void WriteOutAllReStructuredTextFiles(Zero::DocGeneratorConfig& config);

  class RstClassMarkupWriter : public BaseMarkupWriter
  {
  public:
    static void WriteClass(
      StringParam outputFile,
      ClassDoc* classDoc,
      DocumentationLibrary &lib,
      DocToTags& tagged);

    RstClassMarkupWriter(StringParam name, ClassDoc* classDoc);

  protected:
    void InsertClassRstHeader(void);

    void InsertClassRstFooter(void);

    void InsertMethod(MethodDoc &method);

    void InsertProperty(PropertyDoc &propDoc);

    void InsertCollapsibleSection(void);

    Array<String> mBases;

    ClassDoc *mClassDoc;
  };

  class RstEventListWriter : public BaseMarkupWriter
  {
  public:
    static void WriteEventList(StringRef eventListFilepath, StringRef outputPath);

    RstEventListWriter(StringParam name);

    void WriteEventEntry(StringParam eventEntry, StringParam type);
  };

  class RstCommandRefWriter : public BaseMarkupWriter
  {
  public:
    static void WriteCommandRef(StringParam commandListFilepath, StringRef outputPath);

    RstCommandRefWriter(StringParam name);

    void WriteCommandEntry(const CommandDoc &cmdDoc);
  };

  ///// ReMarkup(Phabricator) /////
  void WriteOutAllReMarkupFiles(Zero::DocGeneratorConfig& config);

  class ReMarkupWriter : public BaseMarkupWriter
  {
  public:
    ReMarkupWriter(StringParam name);

  protected:
    // Markup requires spaces not tabs so need to override
    void IndentToCurrentLevel(void);

    // just prints the language specifier for a code block
    void InsertStartOfCodeBlock(StringParam name);

    void InsertDivider();

    void InsertLabel(StringParam label);

    void InsertTypeLink(StringParam className);

    static const String mEndLine;

  };

  class ReMarkupClassMarkupWriter : public ReMarkupWriter
  {
  public:
    static void WriteClass( StringParam outputFile, ClassDoc* classDoc,
      DocumentationLibrary &lib, DocToTags& tagged);

    ReMarkupClassMarkupWriter(StringParam name, ClassDoc* classDoc);

  protected:
    void InsertClassHeader(void);

    void InsertMethod(MethodDoc &method);

    void InsertProperty(PropertyDoc &propDoc);

    Array<String> mBases;

    ClassDoc *mClassDoc;
  };

  class ReMarkupEventListWriter : public ReMarkupWriter
  {
  public:
    static void WriteEventList(StringRef eventListFilepath, StringRef outputPath);

    ReMarkupEventListWriter(StringParam name);

    void WriteEventEntry(StringParam eventEntry, StringParam type);
  };

  class ReMarkupCommandRefWriter : public ReMarkupWriter
  {
  public:
    static void WriteCommandRef(StringParam commandListFilepath, StringRef outputPath);

    ReMarkupCommandRefWriter(StringParam name);

    void WriteCommandEntry(const CommandDoc &cmdDoc);
  };


}