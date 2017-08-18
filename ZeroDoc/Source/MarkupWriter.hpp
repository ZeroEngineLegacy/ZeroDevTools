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

    void WriteOutputToFile(StringParam outputDirectory);

  protected:
    void IndentToCurrentLevel(void);

    void InsertNewUnderline(uint length, uint headerLevel = 0);

    void InsertNewSectionHeader(StringParam sectionName);

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
    static void WriteEventList(StringParam eventListFilepath, StringParam outputPath);

    RstEventListWriter(StringParam name);

    void WriteEventEntry(StringParam eventEntry, StringParam type);
  };

  class RstCommandRefWriter : public BaseMarkupWriter
  {
  public:
    static void WriteCommandRef(StringParam commandListFilepath, StringParam outputPath);

    RstCommandRefWriter(StringParam name);

    void WriteCommandEntry(const CommandDoc &cmdDoc);
  };

  ///// ReMarkup(Phabricator) /////
  void WriteOutAllReMarkupFiles(Zero::DocGeneratorConfig& config);

  class ReMarkupWriter : public BaseMarkupWriter
  {
  public:
    ReMarkupWriter(StringParam name, StringParam uri);

  protected:
    // Markup requires spaces not tabs so need to override
    void IndentToCurrentLevel(void);

    // just prints the language specifier for a code block
    void InsertStartOfCodeBlock(StringParam name);

    void InsertDivider();

    void InsertHeaderAtCurrentHeaderLevel(void);
    void InsertHeaderAtCurrentHeaderLevel(StringParam header);

    void InsertLabel(StringParam label);

    void InsertTypeLink(StringParam className);

    void InsertHeaderLink(StringParam header);

    static const String mEndLine;

    static const String mQuoteLine;

    String mDocURI;
  };

  class ReMarkupClassMarkupWriter : public ReMarkupWriter
  {
  public:
    static void WriteClass( StringParam outputFile, ClassDoc* classDoc,
      DocumentationLibrary &lib, DocToTags& tagged);

    ReMarkupClassMarkupWriter(StringParam name, ClassDoc* classDoc, StringParam outputFile);

  protected:
    void InsertClassHeader(void);

    void InsertMethod(MethodDoc &method);

    void InsertProperty(PropertyDoc &propDoc);

    void InsertJumpTable(void);

    void InsertMethodLink(MethodDoc* methodToLink);

    void InsertPropertyLink(PropertyDoc* propToLink);

    Array<String> mBases;

    ClassDoc *mClassDoc;
  };

  class ReMarkupEnumReferenceWriter : public ReMarkupWriter
  {
  public:
    ReMarkupEnumReferenceWriter(StringParam name, StringParam uri);

    static void WriteEnumReference(StringParam outputFile, DocumentationLibrary &lib);

    void InsertEnumEntry(EnumDoc* enumDoc);

    void InsertEnumTable(const Array<EnumDoc*>& enumList);
  };

  class ReMarkupFlagsReferenceWriter : public ReMarkupWriter
  {
  public:
    ReMarkupFlagsReferenceWriter(StringParam name, StringParam uri);

    static void WriteFlagsReference(StringParam outputFile, DocumentationLibrary &lib);

    void InsertFlagsEntry(EnumDoc *flags);

    void InsertFlagTable(const Array<EnumDoc*>& enumList);
  };

  class ReMarkupEventListWriter : public ReMarkupWriter
  {
  public:
    ReMarkupEventListWriter(StringParam name, StringParam uri);

    static void WriteEventList(StringParam eventListFilepath, StringParam outputPath);

    void WriteEventEntry(EventDoc* eventDoc, StringParam type);

    void WriteEventTable(const Array<EventDoc *>& flagsList);
  };

  class ReMarkupCommandRefWriter : public ReMarkupWriter
  {
  public:
    ReMarkupCommandRefWriter(StringParam name, StringParam uri);

    static void WriteCommandRef(StringParam commandListFilepath, StringParam outputPath);

    void WriteCommandEntry(const CommandDoc &cmdDoc);
  };


}