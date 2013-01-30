#pragma once

#include "String\String.hpp"
#include "Engine\Documentation.hpp"
#include "Parsing.hpp"

namespace Zero
{

void LogOn(String& token, bool verbose);
void LogOff(StringParam token, bool verbose);
void GetWikiArticleIds(StringParam token, StringMap& wikiIndices, bool verbose);
bool CreateWikiPage(StringParam token, StringParam pageName, StringParam parentPageIndex, 
                    StringMap& wikiIndices, bool verbose);
void UploadClassDoc(StringParam index, ClassDoc& classDoc, Replacements& replacements, 
                    StringParam token, bool verbose);
void UploadPageContent(StringParam pageIndex, StringParam pageTitle, 
                       StringParam pageContent, StringParam token, bool verbose);

struct WikiUpdatePage
{
  WikiUpdatePage() {}
  WikiUpdatePage(StringParam page, StringParam parent)
  {
    mPageToUpdate = page;
    mParentPage = parent;
  }

  void Serialize(Serializer& stream)
  {
    SerializeName(mPageToUpdate);
    SerializeName(mParentPage);
  }

  String mPageToUpdate;
  String mParentPage;
};

struct EventEntry
{
  EventEntry() {}
  EventEntry(StringParam eventType, StringParam eventName)
  {
    mEventType = eventType;
    mEventName = eventName;
  }

  void Serialize(Serializer& stream)
  {
    SerializeName(mEventType);
    SerializeName(mEventName);
  }

  String mEventType;
  String mEventName;
};

}//namespace Zero
